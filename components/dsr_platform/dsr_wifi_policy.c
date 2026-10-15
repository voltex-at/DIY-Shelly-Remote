#include "dsr_platform.h"
#include <ctype.h>
#include <string.h>

bool dsr_wifi_settings_valid(const dsr_settings_t *s)
{
    if (!s || !memchr(s->ssid, 0, sizeof(s->ssid)) || !s->ssid[0] ||
        !memchr(s->password, 0, sizeof(s->password)) || s->wifi_security >= DSR_WIFI_UNSUPPORTED ||
        s->brightness > 100 || s->idle_seconds > 3600) return false;
    size_t length = strlen(s->password);
    if (s->wifi_security == DSR_WIFI_OPEN) return length == 0;
    if (s->wifi_security == DSR_WIFI_WPA3) return length >= 1 && length <= 63;
    if (length == 64) {
        for (size_t i = 0; i < length; i++) if (!isxdigit((unsigned char)s->password[i])) return false;
        return true;
    }
    return length >= 8 && length <= 63;
}
