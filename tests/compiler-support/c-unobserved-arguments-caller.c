/*
    Project: FreeBASIC C backend regression controls
    File: c-unobserved-arguments-caller.c
    Purpose: Call original BASIC entries with their complete pointer ABI.
    Responsibilities: Check the default and CDecl contracts from native C.
    This file intentionally does NOT contain: Program startup or platform APIs.
*/
#if defined(__i386__)
#define FB_CALL __attribute__((stdcall))
#else
#define FB_CALL
#endif
extern int FB_CALL IGNOREDNARROW(char *text);
extern int r91_native_entry(char *text, int value);
int r91_native_probe(void);

int r91_native_probe(void)
{
    char text[] = "native";
    return IGNOREDNARROW(text) + r91_native_entry(text, 27);
}

/* end of c-unobserved-arguments-caller.c */
