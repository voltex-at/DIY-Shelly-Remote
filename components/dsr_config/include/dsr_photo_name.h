#pragma once
#include <stdbool.h>
/* Workspace IDs remain 32 lowercase hex digits (full 128-bit content ID).
 * New basenames are 27 bytes; including '/' and NUL fits SPIFFS NAME_LEN32.
 * Reader accepts the previous 37-byte basename too; no geometry migration. */
bool dsr_photo_filename(const char *id,char out[38]);
bool dsr_photo_filename_id(const char *name,char id[33]);
