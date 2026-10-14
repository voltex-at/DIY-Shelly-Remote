#include "dsr_core.h"
#include <stdio.h>
#include <string.h>

static bool ascii_digit(char c) { return c >= '0' && c <= '9'; }

bool dsr_ipv4_valid(const char *host)
{
    if (!host) return false;
    const char *p = host;
    unsigned first = 0;
    for (unsigned part = 0; part < 4; part++) {
        unsigned value = 0, digits = 0;
        const char *start = p;
        while (ascii_digit(*p)) {
            if (++digits > 3) return false;
            value = value * 10 + (unsigned)(*p++ - '0');
        }
        if (!digits || value > 255 || (digits > 1 && *start == '0')) return false;
        if (part == 0) first = value;
        if (part < 3) { if (*p++ != '.') return false; }
        else if (*p != '\0') return false;
    }
    /* Local device endpoint: refuse unspecified, multicast and broadcast ranges. */
    return first != 0 && first < 224 && first != 127;
}

dsr_result_t dsr_request_build(dsr_generation_t gen, uint8_t channel,
                             dsr_command_t cmd, bool on, dsr_request_t *out)
{
    if (!out || channel > DSR_MAX_CHANNEL) return DSR_INVALID;
    memset(out, 0, sizeof(*out));
    if (cmd != DSR_GET_STATUS && cmd != DSR_SET_OUTPUT) return DSR_INVALID;
    if (gen == DSR_GEN1) {
        snprintf(out->path, sizeof(out->path), "/relay/%u%s%s", (unsigned)channel,
                 cmd == DSR_SET_OUTPUT ? "?turn=" : "",
                 cmd == DSR_SET_OUTPUT ? (on ? "on" : "off") : "");
    } else if (gen == DSR_GEN2_PLUS) {
        if (cmd == DSR_GET_STATUS) {
            snprintf(out->path, sizeof(out->path), "/rpc/Switch.GetStatus?id=%u", (unsigned)channel);
        } else {
            out->post = true;
            snprintf(out->path, sizeof(out->path), "/rpc/Switch.Set");
            snprintf(out->body, sizeof(out->body), "{\"id\":%u,\"on\":%s}",
                     (unsigned)channel, on ? "true" : "false");
        }
    } else return DSR_UNSUPPORTED;
    return DSR_OK;
}

dsr_result_t dsr_registry_upsert(dsr_registry_t *registry, const dsr_device_t *device)
{
    if (!registry || !device || registry->count > DSR_MAX_DEVICES ||
        !memchr(device->host, '\0', sizeof(device->host)) ||
        !memchr(device->name, '\0', sizeof(device->name)) ||
        !dsr_ipv4_valid(device->host) || device->channel > DSR_MAX_CHANNEL ||
        (device->generation != DSR_GEN1 && device->generation != DSR_GEN2_PLUS)) return DSR_INVALID;
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->items[i].host, device->host) == 0 &&
            registry->items[i].channel == device->channel) {
            registry->items[i] = *device;
            return DSR_OK;
        }
    }
    if (registry->count == DSR_MAX_DEVICES) return DSR_FULL;
    registry->items[registry->count++] = *device;
    return DSR_OK;
}

bool dsr_version_parse(const char *text, uint32_t parts[3])
{
    if (!text || !parts) return false;
    if (*text == 'v') text++;
    uint32_t parsed[3];
    for (unsigned part = 0; part < 3; part++) {
        const char *start = text;
        uint32_t value = 0;
        while (ascii_digit(*text)) {
            uint32_t digit = (uint32_t)(*text++ - '0');
            if (value > (UINT32_MAX - digit) / 10) return false;
            value = value * 10 + digit;
        }
        if (text == start || (text - start > 1 && *start == '0')) return false;
        parsed[part] = value;
        if (part < 2) { if (*text++ != '.') return false; }
        else if (*text != '\0') return false;
    }
    memcpy(parts, parsed, sizeof(parsed));
    return true;
}

bool dsr_version_is_newer(const char *current, const char *candidate)
{
    uint32_t a[3], b[3];
    if (!dsr_version_parse(current, a) || !dsr_version_parse(candidate, b)) return false;
    for (unsigned i = 0; i < 3; i++) {
        if (b[i] != a[i]) return b[i] > a[i];
    }
    return false;
}

dsr_result_t dsr_update_metadata_check(const dsr_update_metadata_t *meta,
                                     const char *hardware_id, const char *current,
                                     size_t inactive_slot_size)
{
    if (!meta || !hardware_id || !*hardware_id || strcmp(hardware_id, "unconfigured") == 0 ||
        !meta->hardware_id || strcmp(meta->hardware_id, hardware_id) != 0 ||
        !dsr_version_is_newer(current, meta->version) || !meta->url ||
        strncmp(meta->url, "https://", 8) != 0 || !meta->url[8] ||
        meta->url[8] == '/' || !meta->sha256 || strlen(meta->sha256) != 64 ||
        !meta->size || meta->size > inactive_slot_size) return DSR_INVALID;
    for (const char *p = meta->url; *p; p++) {
        if ((unsigned char)*p <= 32 || *p == '@' || *p == '\\' || *p == '#') return DSR_INVALID;
    }
    for (const char *p = meta->sha256; *p; p++) {
        if (!ascii_digit(*p) && !(*p >= 'a' && *p <= 'f')) return DSR_INVALID;
    }
    return DSR_OK;
}
