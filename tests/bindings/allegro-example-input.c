/*
    FreeBASIC classic Allegro example qualification
    File: allegro-example-input.c
    Purpose: Supply bounded test input inside the example's own library instance.
    Responsibilities: Real Allegro key queues and library-owned timer callbacks.
    This file contains no renderer replacement or input to other applications.
*/
#include <allegro.h>
#include <winalleg.h>
#include <allegro/internal/aintern.h>

#ifndef EXAMPLE_INPUT_COUNT
#define EXAMPLE_INPUT_COUNT 1
#endif

static int remaining = EXAMPLE_INPUT_COUNT;

static void send_escape(void)
{
    /* Windows timer mutexes are recursive. Allegro supports removing a
       callback from itself, and rechecks the queue after it returns. Stop
       this helper before the final key allows the main thread to exit. */
    remaining--;
    if(remaining == 0)
        remove_int(send_escape);
    /* Use the native driver's event entry point. Writing key[] directly
       loses the event when poll_keyboard copies its pending state over it.
       This path updates both the SDK queue and its polling state inside
       this executable; the user's physical keyboard is never modified. */
    _handle_key_press(27, KEY_ESC);
}

static DWORD WINAPI start_input(LPVOID unused)
{
    int attempt;
    (void)unused;
    /* Leave time for a real frame capture before supplying exit input. The
       bounded callback removes itself before the final input is consumed. */
    Sleep(2500);
    for(attempt = 0; attempt < 100; attempt++) {
        if(keyboard_driver != NULL) {
            /* Qualification windows may lose focus during capture. Keep
               library rendering and input processing active in that case. */
            if(gfx_driver != NULL)
                set_display_switch_mode(SWITCH_BACKGROUND);
            install_int(send_escape, 250);
            return 0;
        }
        Sleep(50);
    }
    return 1;
}

static void __attribute__((constructor)) prepare_input(void)
{
    HANDLE thread = CreateThread(NULL, 0, start_input, NULL, 0, NULL);
    if(thread != NULL)
        CloseHandle(thread);
}
/* end of allegro-example-input.c */
