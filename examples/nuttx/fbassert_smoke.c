/*
    Project: FreeBASIC NuttX runtime smoke tests
    --------------------------------------------

    File: fbassert_smoke.c

    Purpose:

        Exercise the ASSERT and ASSERTWARN runtime entry points in the
        generated-C NuttX smoke harness.

    Responsibilities:

        - call the narrow ASSERTWARN helper
        - call the wide ASSERTWARN helper
        - call the hard ASSERT helper and require a non-zero program status

    This file intentionally does NOT contain:

        - FreeBASIC compiler output
        - fbctests harness logic
        - general runtime-error dispatch tests
*/

#include <stdint.h>

#ifndef FAR
#  define FAR
#endif

typedef int32_t int32;
typedef uint32_t FB_WCHAR;

void fb_Assert(char *filename, int linenum, char *funcname, char *expression);
void fb_AssertWarn(char *filename, int linenum, char *funcname,
    char *expression);
void fb_AssertWarnW(char *filename, int linenum, char *funcname,
    FB_WCHAR *expression);

int main(int argc, FAR char *argv[])
{
    static FB_WCHAR wide_warn[] =
        { 'g', 'e', 'n', 'e', 'r', 'i', 'c', ' ', 'w', 'i', 'd', 'e',
          ' ', 'a', 's', 's', 'e', 'r', 't', ' ', 'w', 'a', 'r', 'n', 0 };

    (void)argc;
    (void)argv;

    fb_AssertWarn((char *)"fbassert_smoke.c", 45, (char *)"main",
        (char *)"generic assert warn");
    fb_AssertWarnW((char *)"fbassert_smoke.c", 47, (char *)"main",
        wide_warn);
    fb_Assert((char *)"fbassert_smoke.c", 49, (char *)"main",
        (char *)"generic assert hard");

    return 0;
}

/* end of fbassert_smoke.c */
