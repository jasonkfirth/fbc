/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_minrt.c

    Purpose:

        Provide the temporary runtime bridge used by the generated-C
        RISC-V NuttX smoke tests.

    Responsibilities:

        - expose enough runtime symbols to keep generated-C smoke tests useful
        - map those symbols onto simple C and NuttX behavior
        - own compact file, TCP, serial, and process-wide runtime state
        - provide a staging point while the real NuttX target is moved toward
          the normal rtlib, gfxlib2, and sfxlib build paths

    This file intentionally does NOT contain:

        - a permanent replacement for the normal FreeBASIC runtime
        - graphics or audio command implementations
        - board-specific hardware drivers

    Maintenance note:

        Do not keep adding command implementations here if the generic rtlib
        code can be made to build for NuttX.  This shim exists to keep the
        QEMU smoke path alive while the target is wired into the normal
        runtime libraries.
*/

#include "fb.h"

#if defined(__has_include)
# if __has_include(<nuttx/config.h>)
#  include <nuttx/config.h>
# endif
#endif

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#if defined(CONFIG_SERIAL_TERMIOS) && (CONFIG_SERIAL_TERMIOS != 0)
#include <termios.h>
#define FB_NUTTX_HAVE_SERIAL_TERMIOS 1
#else
#define FB_NUTTX_HAVE_SERIAL_TERMIOS 0
#endif

#if defined(__has_include)
# if __has_include(<nuttx/ioexpander/gpio.h>)
#  include <nuttx/ioexpander/gpio.h>
#  define FB_NUTTX_HAVE_GPIO 1
# endif
#endif

#ifndef FB_NUTTX_HAVE_GPIO
#define FB_NUTTX_HAVE_GPIO 0
#endif

#include "builtin/builtin.h"

/*
    Several shared rtlib helpers read the normal runtime context even in
    single-threaded generated-C smoke programs.  The gfxlib and sfxlib bridge
    objects also provide weak copies for their own link paths; NuttX flat
    builds coalesce the weak definitions until the target has one permanent
    runtime initialization object.
*/
#define FB_NUTTX_WEAK __attribute__((weak))

#if !defined(FB_NUTTX_HAS_FULL_RT_CONTEXT) || \
    (FB_NUTTX_HAS_FULL_RT_CONTEXT == 0)
FB_RTLIB_CTX __fb_ctx FB_NUTTX_WEAK;
char __fb_errmsg[FB_ERRMSG_SIZE] FB_NUTTX_WEAK;
#endif

/* ------------------------------------------------------------------------- */
/* Temporary string descriptors                                              */
/* ------------------------------------------------------------------------- */

#define FB_NUTTX_TEMP_DESC_COUNT 8
#define FB_NUTTX_HEX_BUFFER_COUNT 8
#define FB_NUTTX_HEX_BUFFER_SIZE 9
#define FB_NUTTX_MAX_FILES 16
#define FB_NUTTX_LINE_BUFFER_SIZE 256
#define FB_NUTTX_INPUT_BUFFER_SIZE 256
#define FB_NUTTX_FIXEDLEN_MASK 0x7fffffffu
#define FB_NUTTX_FILE_KIND_NONE 0
#define FB_NUTTX_FILE_KIND_FILE 1
#define FB_NUTTX_FILE_KIND_TCP 2
#define FB_NUTTX_FILE_KIND_TCP_SERVER 3
#define FB_NUTTX_FILE_KIND_COM 4
#define FB_NUTTX_DATA_END -1
#define FB_NUTTX_DATA_TEXT 2
#define FB_NUTTX_DATA_DOUBLE 4
#define FB_NUTTX_DATA_STRING 11
#define FB_NUTTX_DATA_LONGINT 13

#if defined(CONFIG_NET) && defined(CONFIG_NET_TCP)
#define FB_NUTTX_HAVE_TCP 1
#else
#define FB_NUTTX_HAVE_TCP 0
#endif

typedef struct FB_NUTTX_DATA_DESC {
    int16_t type __attribute__((packed, aligned(1)));
    void *node __attribute__((packed, aligned(1)));
} FB_NUTTX_DATA_DESC;

/*
    The generated C backend uses temporary FBSTRING descriptors for string
    literals and conversion results. The hosted runtime allocates and releases
    those descriptors through its normal string system.

    For the NuttX smoke target we keep a small static ring instead. That is
    enough for simple PRINT statements and avoids introducing heap ownership
    rules before the target has a real runtime library.
*/
static FBSTRING fb_nuttx_temp_desc[FB_NUTTX_TEMP_DESC_COUNT];
static unsigned fb_nuttx_next_temp_desc;

/*
    HEX() returns a temporary string. A small buffer ring lets more than one
    converted value remain valid through a simple expression or print call.
*/
static char fb_nuttx_hex_buffer[FB_NUTTX_HEX_BUFFER_COUNT][FB_NUTTX_HEX_BUFFER_SIZE];
static unsigned fb_nuttx_next_hex_buffer;
static FILE *fb_nuttx_files[FB_NUTTX_MAX_FILES];
static int fb_nuttx_file_kind[FB_NUTTX_MAX_FILES];
static int fb_nuttx_tcp_timeout_ms[FB_NUTTX_MAX_FILES];
static int32 fb_nuttx_file_record_len[FB_NUTTX_MAX_FILES];
static unsigned int fb_nuttx_serial_output_lines[FB_NUTTX_MAX_FILES];
#if FB_NUTTX_HAVE_SERIAL_TERMIOS
static struct termios fb_nuttx_serial_old_termios[FB_NUTTX_MAX_FILES];
static unsigned char fb_nuttx_serial_old_termios_valid[FB_NUTTX_MAX_FILES];
#endif
static int fb_nuttx_input_file_num;
static char fb_nuttx_input_line[FB_NUTTX_INPUT_BUFFER_SIZE];
static size_t fb_nuttx_input_pos;
static int fb_nuttx_input_has_line;
static uint32_t fb_nuttx_random_state = 327680;
static int32 fb_nuttx_random_is_qb = 1;
static int32 fb_nuttx_error_num;
static const FB_NUTTX_DATA_DESC *fb_nuttx_data_cursor;
static const FBSTRING *fb_nuttx_print_using_format;
static int fb_nuttx_argc;
static char **fb_nuttx_argv;
static int32 fb_nuttx_cursor_row = 1;
static int32 fb_nuttx_cursor_column = 1;

FBSTRING *fb_StrAssign(void *dst_void, const int32 dst_len, const void *src,
    const int32 src_len, const int32 fill_rem);
void fb_StrDelete(const FBSTRING *s);

static int32 fb_nuttx_source_length(const void *src, const int32 src_len);
static const char *fb_nuttx_source_data(const void *src, const int32 src_len);

static int32 fb_nuttx_string_length(const char *text, const int32 requested_len)
{
    size_t measured_len;

    if (text == NULL)
        return 0;

    if (requested_len >= 0)
        return requested_len;

    measured_len = strlen(text);

    if (measured_len > (size_t)2147483647)
        return (int32)2147483647;

    return (int32)measured_len;
}

static int fb_nuttx_is_fixed_length(const int32 len)
{
    return len < -1;
}

static int32 fb_nuttx_decode_fixed_length(const int32 len)
{
    return (int32)((uint32_t)len & FB_NUTTX_FIXEDLEN_MASK);
}

static FBSTRING *fb_nuttx_temp_string(const char *text, const int32 len)
{
    FBSTRING *desc;

    desc = &fb_nuttx_temp_desc[fb_nuttx_next_temp_desc];

    fb_nuttx_next_temp_desc++;

    if (fb_nuttx_next_temp_desc >= FB_NUTTX_TEMP_DESC_COUNT)
        fb_nuttx_next_temp_desc = 0;

    desc->data = (char *)text;
    desc->len = len;
    /*
        size == 0 marks a borrowed temporary. fb_StrDelete() only frees
        descriptors with owned storage.
    */
    desc->size = 0;

    return desc;
}

FBSTRING *fb_StrAllocTempDescZEx(const char *text, const int32 len)
{
    return fb_nuttx_temp_string(text, fb_nuttx_string_length(text, len));
}

FBSTRING *fb_StrAllocTempDescF(const void *text, const int32 len)
{
    if ((text == NULL) || !fb_nuttx_is_fixed_length(len))
        return fb_nuttx_temp_string("", 0);

    return fb_nuttx_temp_string(text, fb_nuttx_decode_fixed_length(len));
}

/* ------------------------------------------------------------------------- */
/* File number support                                                       */
/* ------------------------------------------------------------------------- */

static FILE *fb_nuttx_stream_for_file(const int32 file_num)
{
    if (file_num == 0)
        return stdout;

    if ((file_num < 0) || (file_num >= FB_NUTTX_MAX_FILES))
        return NULL;

    return fb_nuttx_files[file_num];
}

#if FB_NUTTX_HAVE_TCP
static int32 fb_nuttx_tcp_file_eof(const int32 file_num);
static void fb_nuttx_tcp_mark_file_closed(const int32 file_num);
#endif

static int32 fb_nuttx_file_seek_position(FILE *stream, const int32 file_num,
    const int32 position)
{
    long offset;

    if (position <= 0)
        return 0;

    offset = (long)position - 1;

    if ((file_num > 0) && (file_num < FB_NUTTX_MAX_FILES) &&
        (fb_nuttx_file_record_len[file_num] > 0)) {
        offset *= (long)fb_nuttx_file_record_len[file_num];
    }

    if (fseek(stream, offset, SEEK_SET) != 0)
        return -1;

    return 0;
}

static char *fb_nuttx_string_to_cstr(const FBSTRING *s)
{
    char *text;

    if ((s == NULL) || (s->data == NULL) || (s->len <= 0))
        return NULL;

    text = (char *)malloc((size_t)s->len + 1);

    if (text == NULL)
        return NULL;

    memcpy(text, s->data, (size_t)s->len);
    text[s->len] = '\0';

    return text;
}

/*
    Program exit handling

    The hosted runtime terminates the process directly from END or from an
    unhandled runtime error. The NuttX smoke harness still needs a stable
    marker so silent fbctests-style programs can be checked from the QEMU log.

    fb_End() reports the marker before exiting. Programs that fall through
    normally report status zero from the small generated app main.
*/
void fb_nuttx_report_status(const int32 status)
{
    printf("fb-nuttx-status=%" PRId32 "\n", status);
    fflush(stdout);
}

/* ------------------------------------------------------------------------- */
/* Runtime command implementations                                           */
/* ------------------------------------------------------------------------- */

#include "gpiopins.c"
#include "fb_nuttx_data.c"
#include "fb_nuttx_error.c"
#include "fb_nuttx_gosub.c"
#include "fb_nuttx_file.c"
#include "fb_nuttx_device_protocols.c"
#include "fb_nuttx_pipe.c"
#include "fb_nuttx_file_array.c"
#if FB_NUTTX_HAVE_TCP
#include "fb_nuttx_tcp.c"
#else
int32 fb_FileOpenTcpServer(const FBSTRING *text, const uint32 mode,
    const uint32 access, const uint32 lock, const int32 file_num,
    const int32 len, const char *encoding)
{
    (void)text;
    (void)mode;
    (void)access;
    (void)lock;
    (void)file_num;
    (void)len;
    (void)encoding;

    return -1;
}

int32 fb_FileOpenTcp(const FBSTRING *text, const uint32 mode,
    const uint32 access, const uint32 lock, const int32 file_num,
    const int32 len, const char *encoding)
{
    (void)text;
    (void)mode;
    (void)access;
    (void)lock;
    (void)file_num;
    (void)len;
    (void)encoding;

    return -1;
}

int32 fb_TcpAccept(const int32 file_num)
{
    (void)file_num;

    return 0;
}

int32 fb_Eoc(const int32 file_num)
{
    (void)file_num;

    return -1;
}
#endif
#include "fb_nuttx_array.c"
#include "fb_nuttx_object.c"
#include "fb_nuttx_system.c"
#include "fb_nuttx_thread.c"
#include "fb_nuttx_input.c"
#include "fb_nuttx_memory.c"
#include "fb_nuttx_string.c"
#include "fb_nuttx_wstring.c"
#include "fb_nuttx_math.c"
#include "fb_nuttx_datetime.c"
#include "fb_nuttx_dir.c"
#include "fb_nuttx_string_extra.c"
#include "fb_nuttx_env.c"
#include "fb_nuttx_console.c"
#include "fb_nuttx_write.c"
#include "fb_nuttx_print_using.c"
#include "fb_nuttx_convert.c"

/* end of fb_nuttx_minrt.c */
