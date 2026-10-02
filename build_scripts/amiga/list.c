/*
    FreeBASIC AmigaOS test transport
    --------------------------------
    File: list.c
    Purpose: List a named fixture through the native DOS API on a ROM boot.
    Responsibilities: Validate the path and emit its actual file name.
    This file intentionally does NOT contain pipe or shell implementation.
*/

#include <proto/dos.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct FileInfoBlock information;
    BPTR lock;
    if (argc != 2) return 10;
    lock = Lock((CONST_STRPTR)argv[1], ACCESS_READ);
    if (lock == 0) return 10;
    if (!Examine(lock, &information)) { UnLock(lock); return 10; }
    UnLock(lock);
    if (Write(Output(), information.fib_FileName, strlen((const char *)information.fib_FileName)) < 0)
        return 10;
    return Write(Output(), "\n", 1) == 1 ? 0 : 10;
}

/* end of list.c */
