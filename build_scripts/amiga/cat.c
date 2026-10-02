/*
    FreeBASIC AmigaOS test transport
    --------------------------------
    File: cat.c
    Purpose: Copy native command input to output on a ROM boot.
    Responsibilities: Preserve binary bytes and retry partial writes.
    This file intentionally does NOT contain pipe or shell implementation.
*/

#include <proto/dos.h>

int main(void)
{
    /* ROM-only shells retain their small default command stack. Keep the
       copy buffer in this command's data segment instead of consuming it. */
    static unsigned char bytes[4096];
    LONG length;
    while ((length = Read(Input(), bytes, sizeof(bytes))) > 0) {
        LONG position = 0;
        while (position < length) {
            LONG written = Write(Output(), bytes + position, length - position);
            if (written <= 0) return 10;
            position += written;
        }
    }
    return length == 0 ? 0 : 10;
}

/* end of cat.c */
