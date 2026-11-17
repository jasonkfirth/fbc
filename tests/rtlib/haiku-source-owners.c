/*
    Project: FreeBASIC Haiku runtime qualification
    File: haiku-source-owners.c
    Purpose: Execute the current native path, memory and input helper owners.
    Responsibilities: Check buffer bounds, archive ownership and helper state.
    Physical keyboard events and graphical focus retain application testing.
*/
#include "fb.h"
#include "drivers/haiku/haiku_runtime.h"
#include <image.h>
#include <OS.h>
#include <limits.h>

#ifndef HOST_HAIKU
#error This regression requires the native Haiku ABI and runtime APIs.
#endif

static unsigned long checks;

static void check_at(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "HAIKU_OWNER_FAIL line=%d check=%lu\n", line, checks);
        exit(1);
    }
}
#define CHECK(condition) check_at((condition), __LINE__)

static void check_paths(const char *application)
{
    unsigned char guarded[258];
    char reference[257];
    char *destination = (char *)&guarded[1];
#ifndef AUDIT_LEGACY_NAME
    const char *base = strrchr(application, '/');
    base = base ? base + 1 : application;
#endif
    CHECK(fb_hGetExeName(NULL, 1) == NULL && fb_hGetExePath(NULL, 1) == NULL);
    CHECK(fb_hGetExeName(destination, 0) == NULL && fb_hGetExePath(destination, 0) == NULL);
    CHECK(fb_hGetExeName(destination, -1) == NULL && fb_hGetExePath(destination, -1) == NULL);
    for (size_t capacity = 1; capacity <= 256; capacity++) {
        memset(guarded, 0xA5, sizeof(guarded));
#ifdef AUDIT_LEGACY_NAME
        const char *source = application;
#else
        const char *source = base;
#endif
        size_t copied = strlen(source);
        if (copied >= capacity) copied = capacity - 1;
        memcpy(reference, source, copied);
        reference[copied] = 0;
        char *result = fb_hGetExeName(destination, (ssize_t)capacity);
        CHECK(result != NULL && strcmp(destination, reference) == 0);
#ifdef AUDIT_LEGACY_NAME
        char *slash = strrchr(reference, '/');
        CHECK(strcmp(result, slash ? slash + 1 : reference) == 0);
#else
        CHECK(result == destination);
#endif
        CHECK(guarded[0] == 0xA5 && guarded[capacity + 1] == 0xA5);
        memset(guarded, 0xA5, sizeof(guarded));
        copied = strlen(application);
        if (copied >= capacity) copied = capacity - 1;
        memcpy(reference, application, copied);
        reference[copied] = 0;
        char *last_slash = strrchr(reference, '/');
        if (last_slash) *last_slash = 0;
        CHECK(fb_hGetExePath(destination, (ssize_t)capacity) == destination);
        CHECK(strcmp(destination, reference) == 0);
        CHECK(guarded[0] == 0xA5 && guarded[capacity + 1] == 0xA5);
    }
}

int main(void)
{
    image_info image;
    int32 cookie = 0;
    int found = 0;
    while (get_next_image_info(0, &cookie, &image) == B_OK) {
        if (image.type == B_APP_IMAGE) {
            found = 1;
            break;
        }
    }
    CHECK(found);
    check_paths(image.name);
    CHECK(fb_GetMemAvail(0) > 0 && fb_GetMemAvail(1) > 0);
    CHECK(fb_GetMemAvail(INT_MIN) > 0 && fb_GetMemAvail(INT_MAX) > 0);
    CHECK(fb_hConsoleHasFocus() == 1);
    CHECK(fb_hInitScancodes() == 0);
    unsigned char first[256];
    for (unsigned int code = 0; code < 256; code++)
        first[code] = fb_hHaikuTranslateScancode((unsigned char)code);
    CHECK(fb_hInitScancodes() == 0);
    for (unsigned int code = 0; code < 256; code++)
        CHECK(first[code] == fb_hHaikuTranslateScancode((unsigned char)code));
    printf("HAIKU_OWNER_PASS checks=%lu\n", checks);
    return 0;
}

/* end of haiku-source-owners.c */
