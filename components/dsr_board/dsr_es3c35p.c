/* LCDWiki ES3C35P board port. See boards/lcdwiki-es3c35p/README.md. */
#include "dsr_board.h"
#include "dsr_ui_task.h"
#include "dsr_touch_mailbox.h"
#include "st77922_init.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define WIDTH 320
#define HEIGHT 480
/* Two 20-line DMA buffers (same 25.6 KB internal RAM as the former single
 * 40-line one): LVGL renders the next band while DMA sends the previous one,
 * instead of idling for every QSPI transfer (0.7.50). */
#define BUFFER_PIXELS (WIDTH * 20)
#define TOUCH_POINTS 5
static const char *TAG = "es3c35p";
static lv_display_t *display;
static i2c_master_dev_handle_t touch_device;
static bool touch_ready;
static uint8_t touch_max_points = 1;
static bool ui_started;
static bool sleeping;
static bool consume_until_release;
static uint8_t desired_brightness = 70;
static uint16_t idle_seconds = 60;
static esp_timer_handle_t tick_timer;
static portMUX_TYPE touch_lock = portMUX_INITIALIZER_UNLOCKED;
static dsr_touch_mailbox_t touch_mailbox;

const char *dsr_board_hardware_id(void) { return "lcdwiki-es3c35p-n16r8"; }
lv_display_t *dsr_board_display(void) { return display; }
bool dsr_board_touch_available(void) { return touch_ready; }

esp_err_t dsr_board_set_backlight(uint8_t percent)
{
    if (percent > 100) return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                                    (uint32_t)percent * 1023 / 100), TAG, "backlight duty");
    ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0), TAG, "backlight update");
    if (percent > 0) desired_brightness = percent;
    return ESP_OK;
}

esp_err_t dsr_board_apply_display(uint8_t brightness, uint16_t seconds)
{
    if(brightness>100||seconds>3600)return ESP_ERR_INVALID_ARG;
    /* Legacy records accept zero brightness. Keep wake usable, matching the
     * existing UI minimum; never persist a changed value merely on load. */
    uint8_t safe_brightness=brightness<10?10:brightness;
    if(ui_started){
        esp_err_t error=dsr_board_set_backlight(sleeping?0:safe_brightness);
        if(error!=ESP_OK)return error;
    } /* Bootstrap keeps light off until app_main starts the UI task. */
    desired_brightness=safe_brightness;idle_seconds=seconds;
    if(display)lv_display_trigger_activity(display);
    return ESP_OK;
}

esp_err_t dsr_board_restore_backlight(void)
{return dsr_board_set_backlight(desired_brightness);}

void dsr_board_sleep_display(void)
{
    if (dsr_board_set_backlight(0) == ESP_OK) sleeping = true;
}

static bool color_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *event, void *context)
{
    (void)io; (void)event; (void)context;
    /* LVGL explicitly permits flush-ready signaling from the DMA callback. */
    if (display) lv_display_flush_ready(display);
    return false;
}

static void flush(lv_display_t *disp, const lv_area_t *area, uint8_t *pixels)
{
    esp_lcd_panel_handle_t panel = lv_display_get_user_data(disp);
    lv_draw_sw_rgb565_swap(pixels, (uint32_t)lv_area_get_size(area));
    esp_err_t error = esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1,
                                              area->x2 + 1, area->y2 + 1, pixels);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "Display flush failed: %s", esp_err_to_name(error));
        lv_display_flush_ready(disp);
    }
}

static void align_area(lv_event_t *event)
{
    lv_area_t *area = lv_event_get_param(event);
    area->x1 = (area->x1 / 4) * 4;
    area->x2 = ((area->x2 / 4) * 4) + 3;
}

static esp_err_t read_touch_register(uint16_t reg, uint8_t *bytes, size_t length)
{
    uint8_t address[] = {(uint8_t)(reg >> 8), (uint8_t)reg};
    return i2c_master_transmit_receive(touch_device, address, sizeof(address), bytes, length, 10);
}

static void touch_task(void *arg)
{
    (void)arg;
    bool previous = false;
    for (;;) {
        dsr_touch_sample_t next = {0};
        uint8_t info = 0;
        uint8_t bytes[TOUCH_POINTS * 7];
        if (read_touch_register(0x0010, &info, 1) == ESP_OK && (info & 0x08) &&
            read_touch_register(0x0014, bytes, touch_max_points * 7) == ESP_OK) {
            for (unsigned i = 0; i < touch_max_points; i++) {
                const uint8_t *point = &bytes[i * 7];
                if (!(point[0] & 0x80)) continue;
                uint16_t x = ((uint16_t)(point[0] & 0x3f) << 8) | point[1];
                uint16_t y = ((uint16_t)(point[2] & 0x3f) << 8) | point[3];
                if (x < WIDTH && y < HEIGHT) next = (dsr_touch_sample_t){.pressed=true,.x=x,.y=y};
                break;
            }
        }
        portENTER_CRITICAL(&touch_lock);
        dsr_touch_push(&touch_mailbox,next,(uint32_t)(esp_timer_get_time()/1000));
        portEXIT_CRITICAL(&touch_lock);
        if (next.pressed && !previous) ESP_LOGI(TAG, "Touch x=%u y=%u", (unsigned)next.x, (unsigned)next.y);
        previous = next.pressed;
        vTaskDelay(pdMS_TO_TICKS(10)); /* 0.7.52: was 20 ms */
    }
}

static void touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    static uint16_t last_x, last_y;
    dsr_touch_sample_t sample;
    portENTER_CRITICAL(&touch_lock);
    sample = dsr_touch_pop(&touch_mailbox,(uint32_t)(esp_timer_get_time()/1000));
    portEXIT_CRITICAL(&touch_lock);
    if(sample.cancelled)lv_indev_reset(indev,NULL);
    if (sample.pressed) { last_x = sample.x; last_y = sample.y; }
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_RELEASED;
    if (sleeping && sample.pressed) {
        if (dsr_board_restore_backlight() != ESP_OK) return;
        sleeping = false;
        consume_until_release = true;
        lv_display_trigger_activity(display);
    }
    if (consume_until_release) {
        if (!sample.pressed) consume_until_release = false;
        return;
    }
    if (!sleeping && sample.pressed) data->state = LV_INDEV_STATE_PRESSED;
}

static void idle_check(lv_timer_t *timer)
{
    (void)timer;
    if (touch_ready && !sleeping && idle_seconds && lv_display_get_inactive_time(display) >= (uint32_t)idle_seconds*1000) dsr_board_sleep_display();
}

static void tick(void *arg) { (void)arg; lv_tick_inc(5); }
esp_err_t dsr_board_start_ui_task(bool (*startup_ready)(void))
{
    /* The first-format screen may already have started the task with the
     * same gate; app_main's later call is then a no-op, not a failure. */
    if (ui_started && dsr_ui_task_started_with(startup_ready)) return ESP_OK;
    if (!display || ui_started) return ESP_ERR_INVALID_STATE;
    lv_timer_t *idle = lv_timer_create(idle_check, 500, NULL);
    if (!idle) return ESP_ERR_NO_MEM;
    esp_err_t error = dsr_ui_task_start(startup_ready);
    if (error != ESP_OK) { lv_timer_delete(idle); return error; }
    ui_started = true;
    return ESP_OK;
}

void dsr_board_ui_quiesce(void) { dsr_ui_task_quiesce(); }
void dsr_board_ui_hold(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

esp_err_t dsr_board_init(void)
{
    if (display) return ESP_ERR_INVALID_STATE;
    uint32_t flash_size = 0;
    ESP_RETURN_ON_ERROR(esp_flash_get_size(esp_flash_default_chip, &flash_size), TAG, "Flash size");
    size_t psram_size = esp_psram_get_size();
    ESP_LOGI(TAG, "Flash=%lu PSRAM=%u, profile=%s", (unsigned long)flash_size,
             (unsigned)psram_size, dsr_board_hardware_id());
    if (flash_size != 16 * 1024 * 1024 || psram_size != 8 * 1024 * 1024) {
        ESP_LOGE(TAG, "Memory does not match the documented N16R8 board; stopping bring-up");
        return ESP_ERR_INVALID_SIZE;
    }
    gpio_config_t outputs = {.pin_bit_mask = (1ULL << 41) | (1ULL << 1), .mode = GPIO_MODE_OUTPUT};
    ESP_RETURN_ON_ERROR(gpio_config(&outputs), TAG, "control pins");
    gpio_set_level(41, 0); /* Keep backlight dark until UI initialization succeeds. */
    gpio_set_level(1, 1);  /* Audio amplifier is active-low; leave audio disabled. */
    ledc_timer_config_t pwm = {.speed_mode = LEDC_LOW_SPEED_MODE, .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT, .freq_hz = 5000, .clk_cfg = LEDC_AUTO_CLK};
    ESP_RETURN_ON_ERROR(ledc_timer_config(&pwm), TAG, "backlight timer");
    ledc_channel_config_t channel = {.gpio_num = 41, .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0, .timer_sel = LEDC_TIMER_0, .duty = 0};
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel), TAG, "backlight channel");

    spi_bus_config_t bus = ST77922_PANEL_BUS_QSPI_CONFIG(12, 11, 13, 14, 9, BUFFER_PIXELS * 2);
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO), TAG, "QSPI bus");
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t io_config = ST77922_PANEL_IO_QSPI_CONFIG(10, color_done, NULL);
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io), TAG, "panel IO");
    st77922_vendor_config_t vendor = {.init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]), .flags.use_qspi_interface = 1};
    esp_lcd_panel_dev_config_t panel_config = {.reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR, .bits_per_pixel = 16, .vendor_config = &vendor};
    esp_lcd_panel_handle_t panel = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st77922(io, &panel_config, &panel), TAG, "ST77922 driver");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel), TAG, "software reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel), TAG, "panel initialization");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel, false, false), TAG, "portrait orientation");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel, true), TAG, "panel on");

    lv_init();
    display = lv_display_create(WIDTH, HEIGHT);
    void *buffer = heap_caps_malloc(BUFFER_PIXELS * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void *second = heap_caps_malloc(BUFFER_PIXELS * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!display || !buffer || !second) { heap_caps_free(buffer); heap_caps_free(second); return ESP_ERR_NO_MEM; }
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, buffer, second, BUFFER_PIXELS * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(display, panel);
    lv_display_set_flush_cb(display, flush);
    lv_display_add_event_cb(display, align_area, LV_EVENT_INVALIDATE_AREA, NULL);

    i2c_master_bus_config_t i2c_config = {.i2c_port = I2C_NUM_0, .sda_io_num = 38,
        .scl_io_num = 39, .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true};
    i2c_master_bus_handle_t i2c_bus = NULL;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_config, &i2c_bus), TAG, "I2C bus");
    gpio_config_t reset = {.pin_bit_mask = 1ULL << 48, .mode = GPIO_MODE_OUTPUT};
    ESP_RETURN_ON_ERROR(gpio_config(&reset), TAG, "touch reset pin");
    gpio_set_level(48, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(48, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    i2c_device_config_t device = {.dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x55, .scl_speed_hz = 400000};
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(i2c_bus, &device, &touch_device), TAG, "touch device");
    uint8_t max_points = 0;
    touch_ready = read_touch_register(0x0009, &max_points, 1) == ESP_OK && max_points > 0 && max_points <= TOUCH_POINTS;
    if (touch_ready) {
        touch_max_points = max_points;
        if (xTaskCreate(touch_task, "dsr_touch", 3072, NULL, 6, NULL) != pdPASS) return ESP_ERR_NO_MEM;
        lv_indev_t *indev = lv_indev_create();
        if (!indev) return ESP_ERR_NO_MEM;
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_display(indev, display);
        lv_indev_set_read_cb(indev, touch_read);
        /* Match the 10 ms acquisition period; complete short taps survive a repaint. */
        lv_timer_set_period(lv_indev_get_read_timer(indev), 10);
        ESP_LOGI(TAG, "ST77922 touch responding, max points=%u", (unsigned)max_points);
    } else {
        ESP_LOGW(TAG, "ST77922 touch not identified at 0x55; display remains available for diagnostics");
    }
    esp_timer_create_args_t tick_config = {.callback = tick, .name = "dsr_lv_tick"};
    ESP_RETURN_ON_ERROR(esp_timer_create(&tick_config, &tick_timer), TAG, "LVGL tick timer");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(tick_timer, 5000), TAG, "LVGL tick start");
    return ESP_OK;
}
