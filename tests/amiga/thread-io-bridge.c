/*
    FreeBASIC classic AmigaOS thread qualification
    ---------------------------------------------
    File: thread-io-bridge.c
    Purpose: Observe native process streams across FreeBASIC thread creation.
    Responsibilities: Own one temporary redirection and compare borrowed handles.
    This file intentionally does NOT contain: thread or console implementations.
*/

#include <dos/dos.h>
#include <proto/dos.h>

/* The main thread initializes these before creation and restores them only
   after joining every child. Children only read the expected handles. */
static BPTR original_output;
static BPTR expected_input;
static BPTR expected_output;

int amiga_test_redirect_output(void)
{
    expected_output = Open("thread-console.txt", MODE_NEWFILE);
    if (expected_output == (BPTR)0) return -1;
    expected_input = Input();
    original_output = SelectOutput(expected_output);
    return 0;
}

int amiga_test_inherited_streams(void)
{
    return Input() == expected_input && Output() == expected_output ? 0 : -1;
}

void amiga_test_restore_output(void)
{
    SelectOutput(original_output);
    Close(expected_output);
    expected_output = (BPTR)0;
}

/* end of thread-io-bridge.c */
