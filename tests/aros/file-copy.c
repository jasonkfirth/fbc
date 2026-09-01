/*
    Project: FreeBASIC AROS test support
    ------------------------------------

    File: tests/aros/file-copy.c

    Purpose:

        Copy one guest-side file without depending on optional shell commands.

    Responsibilities:

        - copy one source path to one destination path in bounded blocks
        - reject incomplete reads, writes, and closes
        - report failures through the AROS kernel debug channel

    This file intentionally does NOT contain:

        - directory traversal
        - emulator control
        - package-result interpretation

    Minimal ARM AROS media does not include the ordinary Copy or Type shell
    commands.  The package test still has to move its source into RAM so GCC
    does not create temporary files on the emulated FAT system volume.
*/

#include <proto/debug.h>

#include <dos/dos.h>
#include <stdio.h>

/* ------------------------------------------------------------------------- */
/* Diagnostic output                                                         */
/* ------------------------------------------------------------------------- */

static void report_error(const char *text)
{
    /* AROS STRPTR is unsigned for historical Amiga ABI compatibility. */
    KPutStr((CONST_STRPTR)text);
}

/* ------------------------------------------------------------------------- */
/* Bounded file copy                                                         */
/* ------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    unsigned char buffer[64 * 1024];
    FILE *source;
    FILE *destination;
    size_t bytes_read;
    int result;

    if (argc != 3) {
        report_error("FBC_AROS_TEST: file-copy requires source and destination\n");
        return RETURN_ERROR;
    }

    source = fopen(argv[1], "rb");
    if (source == NULL) {
        report_error("FBC_AROS_TEST: file-copy could not open source\n");
        return RETURN_ERROR;
    }

    destination = fopen(argv[2], "wb");
    if (destination == NULL) {
        fclose(source);
        report_error("FBC_AROS_TEST: file-copy could not open destination\n");
        return RETURN_ERROR;
    }

    result = RETURN_OK;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) != 0) {
        if (fwrite(buffer, 1, bytes_read, destination) != bytes_read) {
            report_error("FBC_AROS_TEST: file-copy write failed\n");
            result = RETURN_ERROR;
            break;
        }
    }

    if (ferror(source) != 0) {
        report_error("FBC_AROS_TEST: file-copy read failed\n");
        result = RETURN_ERROR;
    }
    if (fclose(source) != 0) {
        report_error("FBC_AROS_TEST: file-copy source close failed\n");
        result = RETURN_ERROR;
    }
    if (fclose(destination) != 0) {
        report_error("FBC_AROS_TEST: file-copy destination close failed\n");
        result = RETURN_ERROR;
    }

    return result;
}

/* end of tests/aros/file-copy.c */
