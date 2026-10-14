#include "dsr_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
#define CHECK(expr) do { checks++; assert(expr); } while (0)

static void test_addresses(void)
{
    const char *valid[] = {"192.168.1.12", "10.0.0.1", "172.16.2.254", "169.254.1.1"};
    const char *invalid[] = {"", "localhost", "127.0.0.1", "0.0.0.0", "255.255.255.255", "224.0.0.1",
        "192.168.1.999", "192.168.1", "192.168.1.1:80", "192.168.01.1", "1.2.3.4/relay/0?turn=on",
        "1.2.3.4\r\nAuthorization: secret", "http://1.2.3.4", "1.2.3.4@evil.invalid", "1.2.3.4.", "1..2.3"};
    for (size_t i = 0; i < sizeof(valid) / sizeof(*valid); i++) CHECK(dsr_ipv4_valid(valid[i]));
    for (size_t i = 0; i < sizeof(invalid) / sizeof(*invalid); i++) CHECK(!dsr_ipv4_valid(invalid[i]));
    CHECK(!dsr_ipv4_valid(NULL));
}

static void test_protocol(void)
{
    dsr_request_t request;
    CHECK(dsr_request_build(DSR_GEN1, 0, DSR_GET_STATUS, false, &request) == DSR_OK);
    CHECK(!request.post && strcmp(request.path, "/relay/0") == 0 && !request.body[0]);
    CHECK(dsr_request_build(DSR_GEN1, 1, DSR_SET_OUTPUT, true, &request) == DSR_OK);
    CHECK(strcmp(request.path, "/relay/1?turn=on") == 0 && !request.post);
    CHECK(dsr_request_build(DSR_GEN1, 1, DSR_SET_OUTPUT, false, &request) == DSR_OK);
    CHECK(strcmp(request.path, "/relay/1?turn=off") == 0);
    CHECK(dsr_request_build(DSR_GEN2_PLUS, 3, DSR_GET_STATUS, false, &request) == DSR_OK);
    CHECK(strcmp(request.path, "/rpc/Switch.GetStatus?id=3") == 0 && !request.post);
    CHECK(dsr_request_build(DSR_GEN2_PLUS, 3, DSR_SET_OUTPUT, true, &request) == DSR_OK);
    CHECK(request.post && strcmp(request.path, "/rpc/Switch.Set") == 0);
    CHECK(strcmp(request.body, "{\"id\":3,\"on\":true}") == 0);
    CHECK(dsr_request_build(DSR_GEN2_PLUS, 0, DSR_SET_OUTPUT, false, &request) == DSR_OK);
    CHECK(strcmp(request.body, "{\"id\":0,\"on\":false}") == 0);
    CHECK(dsr_request_build(99, 0, DSR_GET_STATUS, false, &request) == DSR_UNSUPPORTED);
    CHECK(dsr_request_build(DSR_GEN1, 16, DSR_GET_STATUS, false, &request) == DSR_INVALID);
    CHECK(dsr_request_build(DSR_GEN1, 0, 99, false, &request) == DSR_INVALID);
    CHECK(dsr_request_build(DSR_GEN1, 0, DSR_GET_STATUS, false, NULL) == DSR_INVALID);
}

static void test_registry(void)
{
    dsr_registry_t registry = {0};
    dsr_device_t device = {.host = "192.168.1.10", .name = "Desk", .generation = DSR_GEN2_PLUS, .has_switch = true};
    CHECK(dsr_registry_upsert(&registry, &device) == DSR_OK && registry.count == 1);
    device.output = true;
    CHECK(dsr_registry_upsert(&registry, &device) == DSR_OK && registry.count == 1 && registry.items[0].output);
    device.channel = 1;
    CHECK(dsr_registry_upsert(&registry, &device) == DSR_OK && registry.count == 2);
    for (unsigned i = 2; i < DSR_MAX_DEVICES; i++) {
        snprintf(device.host, sizeof(device.host), "192.168.1.%u", 10 + i);
        CHECK(dsr_registry_upsert(&registry, &device) == DSR_OK);
    }
    CHECK(registry.count == DSR_MAX_DEVICES);
    snprintf(device.host, sizeof(device.host), "192.168.1.250");
    CHECK(dsr_registry_upsert(&registry, &device) == DSR_FULL && registry.count == DSR_MAX_DEVICES);
    device = registry.items[0];
    device.online = false;
    CHECK(dsr_registry_upsert(&registry, &device) == DSR_OK && registry.count == DSR_MAX_DEVICES);
    memset(device.host, 'x', sizeof(device.host));
    CHECK(dsr_registry_upsert(&registry, &device) == DSR_INVALID);
    CHECK(dsr_registry_upsert(NULL, &device) == DSR_INVALID);
}

static void test_versions(void)
{
    uint32_t parts[3];
    CHECK(dsr_version_parse("v1.2.3", parts) && parts[0] == 1 && parts[1] == 2 && parts[2] == 3);
    CHECK(dsr_version_parse("0.0.0", parts));
    CHECK(dsr_version_parse("4294967295.0.0", parts));
    const char *bad[] = {"", "1", "1.2", "1.2.3.4", "v", "1.2.3-rc1", "1.2.3+build", "1.2.3 ",
        "01.2.3", "-1.2.3", "4294967296.0.0", "99999999999999999999999.0.0", "1..3"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(*bad); i++) CHECK(!dsr_version_parse(bad[i], parts));
    CHECK(!dsr_version_parse(NULL, parts));
    CHECK(dsr_version_is_newer("0.9.9", "0.10.0"));
    CHECK(dsr_version_is_newer("1.99.99", "2.0.0"));
    CHECK(!dsr_version_is_newer("2.0.0", "1.99.99"));
    CHECK(!dsr_version_is_newer("1.2.3", "v1.2.3"));
    CHECK(!dsr_version_is_newer("1.2.3", "2.0.0-rc1"));
    CHECK(!dsr_version_is_newer("0.0.1-dev", "1.0.0"));
}

static void test_update_policy(void)
{
    dsr_update_metadata_t meta = {.hardware_id = "test-board-r1", .version = "0.2.0",
        .url = "https://example.invalid/firmware.bin", .sha256 = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
        .size = 4096};
    CHECK(dsr_update_metadata_check(&meta, "test-board-r1", "0.1.0", 4096) == DSR_OK);
    CHECK(dsr_update_metadata_check(&meta, "test-board-r2", "0.1.0", 4096) == DSR_INVALID);
    CHECK(dsr_update_metadata_check(&meta, "unconfigured", "0.1.0", 4096) == DSR_INVALID);
    CHECK(dsr_update_metadata_check(&meta, "test-board-r1", "0.1.0", 4095) == DSR_INVALID);
    CHECK(dsr_update_metadata_check(&meta, "test-board-r1", "0.2.0", 4096) == DSR_INVALID);
    meta.version = "0.3.0-rc1";
    CHECK(dsr_update_metadata_check(&meta, "test-board-r1", "0.1.0", 4096) == DSR_INVALID);
    meta.version = "0.3.0";
    meta.url = "http://example.invalid/firmware.bin";
    CHECK(dsr_update_metadata_check(&meta, "test-board-r1", "0.1.0", 4096) == DSR_INVALID);
    meta.url = "https://";
    CHECK(dsr_update_metadata_check(&meta, "test-board-r1", "0.1.0", 4096) == DSR_INVALID);
    meta.url = "https://example.invalid/firmware.bin";
    meta.sha256 = "short";
    CHECK(dsr_update_metadata_check(&meta, "test-board-r1", "0.1.0", 4096) == DSR_INVALID);
    meta.sha256 = "g123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    CHECK(dsr_update_metadata_check(&meta, "test-board-r1", "0.1.0", 4096) == DSR_INVALID);
    CHECK(dsr_update_metadata_check(NULL, "test-board-r1", "0.1.0", 4096) == DSR_INVALID);
}

int main(void)
{
    test_addresses(); test_protocol(); test_registry(); test_versions(); test_update_policy();
    printf("PASS: %u checks (IPv4, Gen1/Gen2+ requests, registry, versions, OTA metadata policy)\n", checks);
    return 0;
}
