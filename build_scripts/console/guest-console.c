/*
    Project: FreeBASIC console qualification
    File: guest-console.c
    Purpose: Preserve console diagnostics from unattended guest programs.
    Responsibilities: Redirect stdout after the target runtime mounts its disk.
    This file does not select tests, change their results or report guest exit.
*/

#include <stdio.h>

#if defined(FBQUAL_WII)
#define FBQUAL_CONSOLE "sd:/FBQLOG.TXT"
#define FBQUAL_ERROR "sd:/FBQERR.TXT"
#elif defined(FBQUAL_XBOX)
#define FBQUAL_CONSOLE "E:\\FBQLOG.TXT"
#define FBQUAL_ERROR "E:\\FBQERR.TXT"
#else
#error Select FBQUAL_WII or FBQUAL_XBOX for the guest console observer
#endif

extern void __real_fb_hInit(void);

/* The platform initializes video, filesystems and runtime locks in fb_hInit.
 * Redirect only after that initialization, before BASIC starts application
 * threads. Unbuffered output retains the last diagnostic if the guest stops
 * before normal CRT cleanup. Link this test helper with --wrap=fb_hInit. */
void __wrap_fb_hInit(void)
{
	__real_fb_hInit();
	if (freopen(FBQUAL_CONSOLE, "w", stdout) != NULL)
		setvbuf(stdout, NULL, _IONBF, 0);
	if (freopen(FBQUAL_ERROR, "w", stderr) != NULL)
		setvbuf(stderr, NULL, _IONBF, 0);
}

/* end of guest-console.c */
