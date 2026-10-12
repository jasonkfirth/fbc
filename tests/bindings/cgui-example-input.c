/*
    FreeBASIC native CGUI qualification
    File: cgui-example-input.c
    Purpose: Exercise the example through its real buttons and event queue.
    Responsibilities: Preserve AddButton calls and schedule a bounded click.
    This file contains no renderer replacement or input to other applications.
*/
#include <allegro.h>
#include <cgui.h>
#include <stdint.h>
#include <string.h>

#ifdef CGUI_EXAMPLE_NESTED
static int dialogues;
#endif
extern int __real_AddButton(int x, int y, const char *label,
                            void (*callback)(void *), void *data);

static void click_close(void *data)
{
    /* GenEvent dispatches on CGUI's event-processing thread. Click then
       invokes the original example's callback through the real widget. */
    Click((int)(intptr_t)data);
}

int __wrap_AddButton(int x, int y, const char *label,
                     void (*callback)(void *), void *data)
{
    int id = __real_AddButton(x, y, label, callback, data);
    if(label == NULL)
        return id;
#ifdef CGUI_EXAMPLE_NESTED
    if(strcmp(label, "A new_window") == 0) {
        dialogues++;
        printf("CGUI_TEST_DIALOGUE=%d\n", dialogues);
        fflush(stdout);
        if(dialogues == 1)
            GenEvent(click_close, (void *)(intptr_t)id, 1000, id);
    }
    if(strcmp(label, "Close") == 0)
        GenEvent(click_close, (void *)(intptr_t)id, dialogues == 1 ? 7000 : 2000, id);
#else
    if(strcmp(label, "Close") == 0)
        GenEvent(click_close, (void *)(intptr_t)id, 3500, id);
#endif
    return id;
}
/* end of cgui-example-input.c */
