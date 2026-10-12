/*
 * FreeBASIC external-library qualification
 * File: wxc-native.c
 * Purpose: Check the publisher's wx-c DLL through its native C export ABI.
 * Responsibilities: Primitive widths, Unicode strings and colour ownership.
 * This file contains no replacement widgets or compiled wxWidgets classes.
 */
#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* The publisher's wxnet_globals.h uses __stdcall for its MSVC Windows build.
 * Opaque pointers and these signatures come from wxstring.cxx and colour.cxx.
 * This test owns all objects it constructs and runs on a single thread.
 */
static void *(__stdcall *string_create)(const char *);
static void (__stdcall *string_delete)(void *);
static size_t (__stdcall *string_length)(const void *);
static wchar_t (__stdcall *string_character)(const void *, size_t);
static void *(__stdcall *colour_create)(unsigned char, unsigned char, unsigned char);
static void (__stdcall *colour_delete)(void *);
static unsigned char (__stdcall *colour_red)(const void *);
static unsigned char (__stdcall *colour_green)(const void *);
static unsigned char (__stdcall *colour_blue)(const void *);
static void *(__stdcall *app_create)(void);
static void (__stdcall *app_register)(void *, unsigned char (__stdcall *)(void),
                                    int (__stdcall *)(void), void *, void *);
static void (__stdcall *app_run)(int, char **);
static unsigned char (__stdcall *app_init)(void *);
static int (__stdcall *app_exit)(void *);
static void *(__stdcall *frame_create)(void);
static unsigned char (__stdcall *frame_initialize)(void *, void *, int, void *,
                                                   int, int, int, int, unsigned int, void *);
static void (__stdcall *window_colour)(void *, void *);
static unsigned char (__stdcall *window_show)(void *, unsigned char);
static void (__stdcall *window_refresh)(void *, unsigned char, void *);
static void (__stdcall *window_update)(void *);

/* The GUI callbacks run on the framework's main thread. wxApp_Run owns the
 * widget graph until WM_CLOSE ends the loop; these globals are borrowed.
 */
static void *app, *frame;
static HWND frame_hwnd;
static int init_calls, exit_calls, checked_pixels;

static BOOL CALLBACK find_frame(HWND hwnd, LPARAM unused)
{
    char title[128];
    DWORD owner;
    (void)unused;
    GetWindowThreadProcessId(hwnd, &owner);
    GetWindowTextA(hwnd, title, sizeof(title));
    if (owner == GetCurrentProcessId() && !strcmp(title, "wx-c native qualification"))
        frame_hwnd = hwnd;
    return TRUE;
}

static unsigned char __stdcall on_init(void)
{
    void *title = string_create("wx-c native qualification");
    void *colour = colour_create(17, 129, 250);
    HDC dc;
    int x, y;
    frame = frame_create();
    if (!frame || !title || !colour) exit(10);
    /* wxCAPTION, wxSYSTEM_MENU and wxCLOSE_BOX from wx-c's defs.bi. */
    if (!frame_initialize(frame, NULL, -1, title, -1, -1, 320, 240,
                          0x20000000u | 0x00000800u | 0x00001000u, NULL)) exit(11);
    string_delete(title);
    window_colour(frame, colour);
    colour_delete(colour);
    window_show(frame, 1);
    window_refresh(frame, 1, NULL);
    window_update(frame);
    EnumThreadWindows(GetCurrentThreadId(), find_frame, 0);
    if (!frame_hwnd) exit(12);
    dc = GetDC(frame_hwnd);
    if (!dc) exit(13);
    for (y = 20; y <= 80; y += 10) {
        for (x = 20; x <= 80; x += 10) {
            if (GetPixel(dc, x, y) != RGB(17, 129, 250)) exit(14);
            ++checked_pixels;
        }
    }
    ReleaseDC(frame_hwnd, dc);
    ++init_calls;
    puts("WXC_GUI_READY pixels=49");
    fflush(stdout);
    return app_init(app);
}

static int __stdcall on_exit(void)
{
    ++exit_calls;
    return app_exit(app);
}

static void resolve(HMODULE module, const char *name, void *destination, size_t bytes)
{
    FARPROC address = GetProcAddress(module, name);
    if (!address || bytes != sizeof(address)) {
        fprintf(stderr, "Missing native export: %s\n", name);
        exit(2);
    }
    /* Windows represents all DLL function addresses in the FARPROC slot.
     * Copying it avoids pretending that an object pointer is a function.
     */
    memcpy(destination, &address, bytes);
}

int main(int argc, char **argv)
{
    HMODULE module;
    void *text, *empty, *colour;
    int index;
    const char *library = argc == 2 ? argv[1] : getenv("WXC_NATIVE_DLL");
    static const wchar_t expected[] = {65, 233, 937};
    if ((argc != 1 && argc != 2) || !library) return 1;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    module = LoadLibraryA(library);
    if (!module) {
        fprintf(stderr, "WXC_LOAD_ERROR=%lu\n", (unsigned long)GetLastError());
        return 3;
    }
    resolve(module, "_wxString_ctorUTF8@4", &string_create, sizeof(string_create));
    resolve(module, "_wxString_dtor@4", &string_delete, sizeof(string_delete));
    resolve(module, "_wxString_GetLength@4", &string_length, sizeof(string_length));
    resolve(module, "_wxString_CharAtUTF16@8", &string_character, sizeof(string_character));
    resolve(module, "_wxColour_ctorByParts@12", &colour_create, sizeof(colour_create));
    resolve(module, "_wxColour_dtor@4", &colour_delete, sizeof(colour_delete));
    resolve(module, "_wxColour_Red@4", &colour_red, sizeof(colour_red));
    resolve(module, "_wxColour_Green@4", &colour_green, sizeof(colour_green));
    resolve(module, "_wxColour_Blue@4", &colour_blue, sizeof(colour_blue));
    printf("WXC_ABI=%zu,%zu,%zu,%zu,%zu\n", sizeof(void *), sizeof(int),
           sizeof(long), sizeof(size_t), sizeof(wchar_t));
    text = string_create("A\xc3\xa9\xce\xa9");
    empty = string_create("");
    if (!text || !empty || string_length(text) != 3 || string_length(empty) != 0) return 4;
    for (index = 0; index < 3; ++index) {
        if (string_character(text, (size_t)index) != expected[index]) return 5;
    }
    colour = colour_create(17, 129, 250);
    if (!colour || colour_red(colour) != 17 || colour_green(colour) != 129 || colour_blue(colour) != 250) return 6;
    colour_delete(colour);
    string_delete(empty);
    string_delete(text);
    if (argc == 1) {
        resolve(module, "_wxApp_ctor@0", &app_create, sizeof(app_create));
        resolve(module, "_wxApp_RegisterVirtual@20", &app_register, sizeof(app_register));
        resolve(module, "_wxApp_Run@8", &app_run, sizeof(app_run));
        resolve(module, "_wxApp_OnInit@4", &app_init, sizeof(app_init));
        resolve(module, "_wxApp_OnExit@4", &app_exit, sizeof(app_exit));
        resolve(module, "_wxFrame_ctor@0", &frame_create, sizeof(frame_create));
        resolve(module, "_wxFrame_Create@40", &frame_initialize, sizeof(frame_initialize));
        resolve(module, "_wxWindow_SetBackgroundColour@8", &window_colour, sizeof(window_colour));
        resolve(module, "_wxWindow_Show@8", &window_show, sizeof(window_show));
        resolve(module, "_wxWindow_Refresh@12", &window_refresh, sizeof(window_refresh));
        resolve(module, "_wxWindow_Update@4", &window_update, sizeof(window_update));
        app = app_create();
        if (!app) return 8;
        app_register(app, on_init, on_exit, NULL, NULL);
        app_run(0, NULL);
        if (init_calls != 1 || exit_calls != 1 || checked_pixels != 49 || IsWindow(frame_hwnd)) return 9;
        puts("WXC_GUI=PASS init=1 exit=1 pixels=49 frame_destroyed=1");
    }
    if (!FreeLibrary(module)) return 7;
    puts("WXC_NATIVE=PASS unicode=3 empty=0 rgb=17,129,250 owned_objects_released=3");
    return 0;
}
/* end of wxc-native.c */
