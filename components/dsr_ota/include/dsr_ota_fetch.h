#ifndef DSR_OTA_FETCH_H
#define DSR_OTA_FETCH_H
#include <stdbool.h>
#include "dsr_ota_manifest.h"
#define DSR_OTA_FETCH_URL_MAX 2048u
#define DSR_OTA_FETCH_IO_TIMEOUT_MS 5000
#define DSR_OTA_FETCH_BUDGET_US 30000000LL
typedef enum {
 DSR_FETCH_OK,DSR_FETCH_ARGUMENT,DSR_FETCH_NOMEM,DSR_FETCH_URL,
 DSR_FETCH_IO,DSR_FETCH_STATUS,DSR_FETCH_HEADERS,DSR_FETCH_SIZE,
 DSR_FETCH_INCOMPLETE,DSR_FETCH_REDIRECT,DSR_FETCH_CANCELLED,
 DSR_FETCH_TIMEOUT,DSR_FETCH_MANIFEST,DSR_FETCH_CLOCK
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
#endif
