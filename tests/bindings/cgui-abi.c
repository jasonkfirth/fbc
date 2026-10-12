/*
    FreeBASIC native CGUI qualification
    File: cgui-abi.c
    Purpose: Establish native ABI facts and exercise callback and output pointers.
    Responsibilities: Guarded C int outputs, event data identity and cleanup.
    This file contains no FreeBASIC implementation or simulated CGUI API.
*/
#include <allegro.h>
#include <cgui.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct guarded_int { uint32_t before; int value; uint32_t after; };
static uint32_t cookie = 0x71234567;
static int callbacks;

static void finish(void *data)
{
    if(data != &cookie || cookie != 0x71234567)
        exit(10);
    callbacks++;
    cookie = 0x12345678;
    StopProcessEvents();
    CloseWin(NULL);
}

int main(int argc, char **argv)
{
    struct guarded_int x = {0xA1B2C3D4, -1, 0xE5F60718};
    struct guarded_int y = x, w = x, h = x;
    int dialogue;
    printf("ABI %u %u %u %u %u %u %u %u\n", (unsigned)sizeof(int),
           (unsigned)sizeof(unsigned), (unsigned)sizeof(void *),
           (unsigned)sizeof(t_cgui_colors), (unsigned)sizeof(cgui_ver),
           (unsigned)sizeof(event_message_buffer_size), (unsigned)sizeof(long),
           (unsigned)sizeof(struct guarded_int));
    printf("CONSTANTS %u %u %u %u\n", AUTOINDICATOR, ALIGNCENTRE, CGUI_INIT_WINDOWED, W_FLOATING);
    printf("VERSION %d %d %d %d\n", cgui_ver, cgui_rev, cgui_minor_rev, cgui_release_date);
    if(argc == 2 && strcmp(argv[1], "--abi") == 0)
        return 0;
    if(!InitCguiWindowedMode())
        return 1;
    dialogue = MkDialogue(ADAPTIVE, "Native CGUI callback", 0);
    AddTextBox(TOPLEFT, "Native callback and integer pointer checks", 220, 0, 0);
    DisplayWin();
    GetWinInfo(dialogue, &x.value, &y.value, &w.value, &h.value);
    if(x.before != 0xA1B2C3D4 || x.after != 0xE5F60718 ||
       y.before != x.before || y.after != x.after ||
       w.before != x.before || w.after != x.after ||
       h.before != x.before || h.after != x.after || w.value <= 0 || h.value <= 0)
        return 2;
    GenEvent(finish, &cookie, 100, dialogue);
    ProcessEvents();
    DeInitCgui();
    if(callbacks != 1 || cookie != 0x12345678)
        return 3;
    puts("CGUI_C_RUNTIME=PASS");
    return 0;
}
END_OF_MAIN()
/* end of cgui-abi.c */
