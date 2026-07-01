/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_system.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

#include <malloc.h>

#if !defined(FB_NUTTX_USE_GENERIC_CLOCK) || \
    (FB_NUTTX_USE_GENERIC_CLOCK == 0)
double fb_Timer(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return 0.0;

    return (double)now.tv_sec + ((double)now.tv_nsec / 1000000000.0);
}

static int fb_nuttx_clamp_date_field(int value, int max_value)
{
    if (value < 0)
        return 0;

    if (value > max_value)
        return max_value;

    return value;
}

static void fb_nuttx_store_2_digits(char *text, int value)
{
    value = fb_nuttx_clamp_date_field(value, 99);

    text[0] = (char)('0' + (value / 10));
    text[1] = (char)('0' + (value % 10));
}

static void fb_nuttx_store_4_digits(char *text, int value)
{
    value = fb_nuttx_clamp_date_field(value, 9999);

    text[0] = (char)('0' + (value / 1000));
    text[1] = (char)('0' + ((value / 100) % 10));
    text[2] = (char)('0' + ((value / 10) % 10));
    text[3] = (char)('0' + (value % 10));
}

FBSTRING *fb_Date(void)
{
    time_t now;
    struct tm *local_now;
    char text[11];

    /*
        FreeBASIC's DATE function returns a fixed-width mm-dd-yyyy string.
        NuttX provides the normal C time interfaces when the board
        configuration enables them, which is enough for this minimal runtime.
    */
    now = time(NULL);
    local_now = localtime(&now);

    if (local_now == NULL)
        return fb_StrAllocTempDescZEx("00-00-0000", 10);

    fb_nuttx_store_2_digits(&text[0], local_now->tm_mon + 1);
    text[2] = '-';
    fb_nuttx_store_2_digits(&text[3], local_now->tm_mday);
    text[5] = '-';
    fb_nuttx_store_4_digits(&text[6], local_now->tm_year + 1900);
    text[10] = '\0';

    return fb_StrAllocTempDescZEx(text, 10);
}

FBSTRING *fb_Time(void)
{
    time_t now;
    struct tm *local_now;
    char text[9];

    /*
        FreeBASIC's TIME function returns hh:mm:ss.  The value is only as
        accurate as the board clock configured under NuttX, but the shape of
        the result should remain compatible.
    */
    now = time(NULL);
    local_now = localtime(&now);

    if (local_now == NULL)
        return fb_StrAllocTempDescZEx("00:00:00", 8);

    fb_nuttx_store_2_digits(&text[0], local_now->tm_hour);
    text[2] = ':';
    fb_nuttx_store_2_digits(&text[3], local_now->tm_min);
    text[5] = ':';
    fb_nuttx_store_2_digits(&text[6], local_now->tm_sec);
    text[8] = '\0';

    return fb_StrAllocTempDescZEx(text, 8);
}
#endif

#if defined(FB_NUTTX_WITH_GFXLIB)
extern void fb_nuttx_gfx_compat_exit(void);
#else
#define FB_NUTTX_GFX_FALLBACK __attribute__((visibility("default"), used, weak))

void fb_nuttx_gfx_compat_exit(void) FB_NUTTX_GFX_FALLBACK;
void fb_nuttx_gfx_compat_exit(void)
{
}

#undef FB_NUTTX_GFX_FALLBACK
#endif

void fb_nuttx_set_args(int argc, char **argv);

static void *fb_nuttx_error_handler;
static void *fb_nuttx_error_resume_label;
static void *fb_nuttx_error_resume_next_label;

void fb_Init(const int32 argc, char **argv, const int32 lang)
{
    /*
        The normal FreeBASIC runtime initializes a broad process context here.
        This temporary NuttX runtime is still intentionally smaller: command()
        and the generated __FB_ARGC__/__FB_ARGV__ path only need the program
        arguments to be captured before the BASIC main body runs.
    */
    (void)lang;

    fb_nuttx_set_args((int)argc, argv);
}

void fb_End(const int32 status)
{
    if (__fb_ctx.errmsg != NULL) {
        fputs(__fb_ctx.errmsg, stdout);
        fflush(stdout);
        __fb_ctx.errmsg = NULL;
    }

    fb_nuttx_gfx_compat_exit();

    fb_nuttx_report_status(status);
    exit((int)status);
}

void *fb_ErrorThrowEx(const int32 err_num, const int32 line_num,
    const char *filename, const void *res_label, const void *res_next)
{
    (void)line_num;
    (void)filename;
    (void)res_label;
    (void)res_next;

    fb_nuttx_error_num = err_num;

    fb_nuttx_error_resume_label = (void *)res_label;
    fb_nuttx_error_resume_next_label = (void *)res_next;

    if (fb_nuttx_error_handler != NULL)
        return fb_nuttx_error_handler;

    if (res_label != NULL)
        return (void *)res_label;

    if (res_next != NULL)
        return (void *)res_next;

    fb_nuttx_report_status((err_num != 0) ? err_num : 1);
    exit((err_num != 0) ? (int)err_num : 1);

    return NULL;
}

int32 fb_ErrorGetNum(void)
{
    return fb_nuttx_error_num;
}

#if !defined(FB_NUTTX_WITH_GFXLIB)
int fb_ErrorSetNum(int err_num)
{
    fb_nuttx_error_num = err_num;

    return err_num;
}
#endif

void *fb_ErrorSetHandler(void *new_handler)
{
    void *old_handler;

    old_handler = fb_nuttx_error_handler;
    fb_nuttx_error_handler = new_handler;

    return old_handler;
}

void *fb_ErrorResume(void)
{
    void *resume_label;

    resume_label = fb_nuttx_error_resume_label;

    fb_nuttx_error_resume_label = NULL;
    fb_nuttx_error_resume_next_label = NULL;

    if (resume_label != NULL)
        return resume_label;

    fb_nuttx_report_status(1);
    exit(1);

    return NULL;
}

void *fb_ErrorResumeNext(void)
{
    void *resume_label;

    resume_label = fb_nuttx_error_resume_next_label;

    fb_nuttx_error_resume_label = NULL;
    fb_nuttx_error_resume_next_label = NULL;

    if (resume_label != NULL)
        return resume_label;

    fb_nuttx_report_status(1);
    exit(1);

    return NULL;
}

int32 fb_Chain(const FBSTRING *program)
{
    /*
        NuttX can spawn tasks on some boards, but the QEMU smoke harness runs
        each generated example as a single built-in app.  Report CHAIN as
        unsupported for now instead of pretending that a process image was
        replaced.
    */
    (void)program;

    fb_nuttx_error_num = 5;

    return -1;
}

int32 fb_Exec(const FBSTRING *program, const FBSTRING *args)
{
    /*
        Keep EXEC linked but unsupported in the built-in-app smoke runtime.
        A fuller NuttX port can later map this to task_spawn() where the board
        configuration actually provides executable files or named apps.
    */
    (void)program;
    (void)args;

    fb_nuttx_error_num = 5;

    return -1;
}

int32 fb_Run(const FBSTRING *program, const FBSTRING *args)
{
    (void)program;
    (void)args;

    fb_nuttx_error_num = 5;

    return -1;
}

uint32 fb_GetMemAvail(const int32 mode)
{
    struct mallinfo info;

    /*
        FRE() is advisory even on hosted targets.  NuttX exposes the active
        user heap through mallinfo(), and fordblks is the byte count for free
        chunks that malloc can still use.

        The BASIC-facing return type is 32-bit on this target. Clamp instead
        of wrapping if a larger emulated or future board heap is reported.
    */
    (void)mode;

    info = mallinfo();

    if (info.fordblks <= 0)
        return 0;

    if ((uint64_t)info.fordblks > UINT32_MAX)
        return UINT32_MAX;

    return (uint32)info.fordblks;
}

int32 fb_SleepEx(const int32 msec, const int32 allow_wake)
{
    useconds_t usec;

    (void)allow_wake;

    if (msec <= 0)
        return 0;

    usec = (useconds_t)msec * 1000u;
    usleep(usec);

    return 0;
}

void fb_Sleep(const int32 msec)
{
    (void)fb_SleepEx(msec, 0);
}

/* ------------------------------------------------------------------------- */
/* Console input support                                                     */
/* ------------------------------------------------------------------------- */
