/*
    Project: FreeBASIC AROS test support
    ------------------------------------

    File: tests/aros/log-relay.c

    Purpose:

        Copy a guest-side text report to the AROS kernel debug channel.

    Responsibilities:

        - open one DOS path supplied by the test startup script
        - relay that file in bounded chunks through the central debug channel
        - optionally require a completion marker in the report
        - return a conventional process status for shell automation

    This file intentionally does NOT contain:

        - emulator control
        - test-result interpretation
        - architecture-specific compiler flags

    AROS test media is read-only, and its DOS shell has no portable serial
    stream name.  KPutStr is the platform-supported path to the same debug
    channel selected by the boot loader's debug=serial option.
*/

#include <proto/debug.h>

#include <dos/dos.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Test report relay                                                         */
/* ------------------------------------------------------------------------- */

static void relay_text(const char *text)
{
    /* AROS STRPTR is unsigned for historical Amiga ABI compatibility. */
    KPutStr((CONST_STRPTR)text);
}

int main(int argc, char **argv)
{
    char buffer[1024];
    const char *required_text;
    int found_required_text;
    int read_any_text;
    FILE *report;

    if ((argc < 2) || (argc > 3)) {
        relay_text(
            "FBC_AROS_TEST: log-relay requires a path and optional marker\n"
        );
        return RETURN_ERROR;
    }

    required_text = (argc == 3) ? argv[2] : NULL;
    found_required_text = (required_text == NULL);
    read_any_text = 0;
    report = fopen(argv[1], "r");
    if (report == NULL) {
        relay_text("FBC_AROS_TEST: log-relay could not open report\n");
        return RETURN_ERROR;
    }

    while (fgets(buffer, sizeof(buffer), report) != NULL) {
        read_any_text = 1;
        if ((required_text != NULL) &&
            (strstr(buffer, required_text) != NULL)) {
            found_required_text = 1;
        }
        relay_text(buffer);
    }

    if (ferror(report) != 0) {
        fclose(report);
        relay_text("FBC_AROS_TEST: log-relay could not read report\n");
        return RETURN_ERROR;
    }

    fclose(report);

    if (read_any_text == 0) {
        relay_text("FBC_AROS_TEST: test report was empty\n");
        return RETURN_ERROR;
    }

    if (found_required_text == 0) {
        relay_text("FBC_AROS_TEST: completion marker was absent\n");
        return RETURN_ERROR;
    }

    return RETURN_OK;
}

/* end of tests/aros/log-relay.c */
