#ifndef DSR_OTA_FETCH_H
#define DSR_OTA_FETCH_H
#include <stdbool.h>
#include "dsr_ota_manifest.h"
#include "dsr_ota_image.h"
#define DSR_OTA_FETCH_URL_MAX 2048u
#define DSR_OTA_FETCH_IO_TIMEOUT_MS 5000
#define DSR_OTA_FETCH_BUDGET_US 30000000LL
/* App image (several MiB + Flash writes): no-progress limit stays
 * DSR_OTA_FETCH_BUDGET_US; the whole transfer may take up to 10 min. */
#define DSR_OTA_IMAGE_BUDGET_US 600000000LL
typedef enum {
 DSR_FETCH_OK,DSR_FETCH_ARGUMENT,DSR_FETCH_NOMEM,DSR_FETCH_URL,
 DSR_FETCH_IO,DSR_FETCH_STATUS,DSR_FETCH_HEADERS,DSR_FETCH_SIZE,
 DSR_FETCH_INCOMPLETE,DSR_FETCH_REDIRECT,DSR_FETCH_CANCELLED,
 DSR_FETCH_TIMEOUT,DSR_FETCH_MANIFEST,DSR_FETCH_CLOCK,DSR_FETCH_IMAGE,
 DSR_FETCH_RELEASE
} dsr_ota_fetch_result_t;
typedef struct {
 /* Optional owner-task callback; no client handle escapes. Shared cancellation
  * state must be synchronized by the caller. Never close from another task. */
 bool (*cancelled)(void *context);
 void *context;
} dsr_ota_fetch_control_t;
/* Blocking BACKEND ONLY; never call from the UI task. No downloader caller,
 * release selection, app download, Flash, slot selection or reboot yet.
 * Initial URL is an explicit repo releases/download/tag/file <=256 B. At most
 * one manual redirect, only to exact release-assets.githubusercontent.com.
 * TLS CA bundle + hostname verification; no credentials or automatic redirects.
 * Requires plausible system UTC (>=2025-01-01) and target X.509 date checks.
 * This stage does NOT establish trusted time or add SNTP; unset clock refuses.
 * Reads raw manifest <=2048 B and returns output ONLY after genuine verifier OK.
 * Caller policy/control and their pointed data stay valid and immutable until
 * return. Production trust is deliberately empty; HTTPS is NOT publisher trust.
 * Fixed 5 s SDK I/O timeout and cooperative 30 s budget/cancellation between
 * operations/callbacks. SDK blocking connect/header reads are not a hard total
 * wall-clock deadline, and SDK header/TLS allocations exceed the body bound.
 * Every failure zeros out; manifest_result is INVALID unless verifier ran.
 * Authorize metadata ONLY on DSR_FETCH_OK, never on manifest_result alone:
 * cancellation/deadline after crypto can return failure with verifier OK.
 * No response bodies, Location query strings or secrets are logged here. */
dsr_ota_fetch_result_t dsr_ota_fetch_manifest(const char *url,
 const dsr_ota_policy_t *policy,const dsr_ota_fetch_control_t *control,
 dsr_ota_verified_t *out,dsr_manifest_result_t *manifest_result);
/* VALIDATION/DISCARD BACKEND ONLY. Fetches and verifies the manifest first;
 * ONLY DSR_FETCH_OK from that phase permits an app request. The signed URL,
 * size and digest feed the existing image validator; no caller-created verified
 * struct can enter this public chain. No app bytes escape, Flash/OTA writes,
 * slot selection, task, UI, automatic invocation or reboot. A future installer
 * must download/verify again and apply its own consent/power/slot/boot gates.
 * out is published ONLY after exact complete app size/hash/header/board checks
 * and final cancellation/timeout checks. Every failure zeros it. It is NOT a
 * full IDF segment/checksum/revision check, nor proof of an installable image.
 * Both phases separately retain TLS/clock/one-hop URL/framing policy above.
 * Each phase has a cooperative 30 s budget; app reads at most 1024 B per SDK
 * call. Positive-progress SDK reads, DNS/TLS and headers may exceed the budget
 * before control returns: NOT a hard total deadline. Never use in UI task.
 * Arguments/pointed data must remain immutable; out belongs exclusively to
 * this blocking operation until return. image_result is STATE until creation
 * is attempted, then records the actual image helper result. Neither detail
 * result authorizes anything unless this function returns DSR_FETCH_OK. */
dsr_ota_fetch_result_t dsr_ota_fetch_app(const char *manifest_url,
 const dsr_ota_policy_t *policy,const dsr_ota_fetch_control_t *control,
 dsr_ota_verified_t *out,dsr_manifest_result_t *manifest_result,
 dsr_image_result_t *image_result);
#endif
