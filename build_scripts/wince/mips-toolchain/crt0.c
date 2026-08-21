/*
    FreeBASIC Windows CE MIPS startup
    ---------------------------------

    File: wince/mips-toolchain/crt0.c

    Purpose:

        Start and stop Clang-generated MIPS PE programs without depending on
        a proprietary Microsoft MIPS C runtime.

    Responsibilities:

        - expose the Windows CE executable and DLL entry points
        - run GNU-style and PE .CRT constructor tables in a stable order
        - adapt the Unicode WinMain command line to conventional argc/argv
        - invoke an executable's main function and preserve its exit status
        - provide the small atexit/exit surface required by the FB runtime
        - finish the process through the Windows CE COREDLL API

    This file intentionally does NOT contain:

        - C standard-library implementations supplied by COREDLL
        - compiler builtins or PE import-library construction
        - device, emulator, graphics, or sound policy

    Constructor ordering:

        FreeBASIC's fbrt0 object deliberately uses GNU .ctors sections so
        runtime initialization precedes user constructors.  Clang emits
        ordinary C constructors in sorted .CRT$XC sections.  Shutdown walks
        those sets in the opposite ownership order, leaving fbrt0 cleanup
        until every program destructor has completed.
*/

typedef void (*FB_CRT_FUNCTION)(void);

typedef void *FB_CRT_HANDLE;
typedef unsigned long FB_CRT_DWORD;
typedef int FB_CRT_BOOL;

#define FB_CRT_PROCESS_DETACH 0UL
#define FB_CRT_PROCESS_ATTACH 1UL
#define FB_CRT_ATEXIT_CAPACITY 128U
#define FB_CRT_MAX_ARGUMENTS 64U
#define FB_CRT_ARGUMENT_BUFFER_CAPACITY 4096U
#define FB_CRT_WIDE_TOKEN_CAPACITY 1024U
#define FB_CRT_MODULE_PATH_CAPACITY 260U
#define FB_CRT_CODE_PAGE_ACP 0U
#define FB_CRT_INVALID_PARAMETER 87

/* ------------------------------------------------------------------------- */
/* Windows CE imports                                                        */
/* ------------------------------------------------------------------------- */

extern FB_CRT_BOOL TerminateProcess(FB_CRT_HANDLE process,
                                    unsigned int exit_code);

#if !defined(FB_CRT_BUILD_DLL)

extern FB_CRT_DWORD GetModuleFileNameW(FB_CRT_HANDLE module,
                                       unsigned short *filename,
                                       FB_CRT_DWORD capacity);
extern int WideCharToMultiByte(unsigned int code_page,
                               FB_CRT_DWORD flags,
                               const unsigned short *wide_text,
                               int wide_length,
                               char *text,
                               int text_capacity,
                               const char *default_character,
                               FB_CRT_BOOL *used_default_character);

#endif

#if defined(FB_CRT_TRACE_STARTUP)

#define FB_CRT_GENERIC_WRITE 0x40000000UL
#define FB_CRT_CREATE_ALWAYS 2UL
#define FB_CRT_FILE_ATTRIBUTE_NORMAL 0x80UL
#define FB_CRT_TRACE_MAGIC 0x54524346UL
#define FB_CRT_TRACE_LEGACY_CONSTRUCTOR 1UL
#define FB_CRT_TRACE_PE_CONSTRUCTOR 2UL
#define FB_CRT_TRACE_STARTUP_COMPLETE 3UL

typedef struct FB_CRT_TRACE_RECORD {
    FB_CRT_DWORD magic;
    FB_CRT_DWORD stage;
    FB_CRT_DWORD function_address;
} FB_CRT_TRACE_RECORD;

extern FB_CRT_HANDLE CreateFileW(const unsigned short *filename,
                                 FB_CRT_DWORD desired_access,
                                 FB_CRT_DWORD share_mode,
                                 void *security_attributes,
                                 FB_CRT_DWORD creation_disposition,
                                 FB_CRT_DWORD flags_and_attributes,
                                 FB_CRT_HANDLE template_file);
extern FB_CRT_BOOL WriteFile(FB_CRT_HANDLE file,
                             const void *buffer,
                             FB_CRT_DWORD bytes_to_write,
                             FB_CRT_DWORD *bytes_written,
                             void *overlapped);
extern FB_CRT_BOOL CloseHandle(FB_CRT_HANDLE object);

static const unsigned short fb_crt_trace_path[] = {
    '\\', 'S', 't', 'o', 'r', 'a', 'g', 'e', ' ',
    'C', 'a', 'r', 'd', '\\',
    'f', 'b', 'c', '-', 'c', 'r', 't', '-', 't', 'r', 'a', 'c', 'e',
    '.', 'b', 'i', 'n', 0
};

static void fb_crt_trace_constructor(FB_CRT_DWORD stage,
                                     FB_CRT_FUNCTION function)
{
    FB_CRT_TRACE_RECORD record;
    FB_CRT_HANDLE file;
    FB_CRT_DWORD bytes_written;

    record.magic = FB_CRT_TRACE_MAGIC;
    record.stage = stage;
    record.function_address = (FB_CRT_DWORD)function;

    file = CreateFileW(fb_crt_trace_path,
                       FB_CRT_GENERIC_WRITE,
                       0,
                       0,
                       FB_CRT_CREATE_ALWAYS,
                       FB_CRT_FILE_ATTRIBUTE_NORMAL,
                       0);
    if (file == 0 || file == (FB_CRT_HANDLE)-1)
        return;

    bytes_written = 0;
    WriteFile(file, &record, sizeof(record), &bytes_written, 0);
    CloseHandle(file);
}

#endif

static void fb_crt_exit_process(FB_CRT_DWORD exit_code)
{
    /* Windows CE encodes the current process as kernel pseudo-handle 66. */
    TerminateProcess((FB_CRT_HANDLE)66, (unsigned int)exit_code);

    for (;;)
        ;
}

/* ------------------------------------------------------------------------- */
/* Linker-owned constructor tables                                           */
/* ------------------------------------------------------------------------- */

extern FB_CRT_FUNCTION fb_crt_legacy_ctors[]
    __asm__("__CTOR_LIST__");
extern FB_CRT_FUNCTION fb_crt_legacy_dtors[]
    __asm__("__DTOR_LIST__");

static FB_CRT_FUNCTION fb_crt_xc_start
    __attribute__((section(".CRT$XCA"), used)) = 0;
static FB_CRT_FUNCTION fb_crt_xc_end
    __attribute__((section(".CRT$XCZ"), used)) = 0;
static FB_CRT_FUNCTION fb_crt_xt_start
    __attribute__((section(".CRT$XTA"), used)) = 0;
static FB_CRT_FUNCTION fb_crt_xt_end
    __attribute__((section(".CRT$XTZ"), used)) = 0;

/* ------------------------------------------------------------------------- */
/* Process-local termination state                                           */
/* ------------------------------------------------------------------------- */

static FB_CRT_FUNCTION fb_crt_atexit_functions[FB_CRT_ATEXIT_CAPACITY];
static unsigned int fb_crt_atexit_count;
static int fb_crt_shutdown_started;

/* ------------------------------------------------------------------------- */
/* Constructor and destructor walkers                                        */
/* ------------------------------------------------------------------------- */

static void fb_crt_run_forward(FB_CRT_FUNCTION *first,
                               FB_CRT_FUNCTION *last)
{
    FB_CRT_FUNCTION *entry;

    for (entry = first; entry < last; ++entry) {
        if (*entry != 0) {
#if defined(FB_CRT_TRACE_STARTUP)
            fb_crt_trace_constructor(FB_CRT_TRACE_PE_CONSTRUCTOR, *entry);
#endif
            (*entry)();
        }
    }
}

static void fb_crt_run_reverse(FB_CRT_FUNCTION *first,
                               FB_CRT_FUNCTION *last)
{
    FB_CRT_FUNCTION *entry;

    entry = last;
    while (entry > first) {
        --entry;
        if (*entry != 0)
            (*entry)();
    }
}

static void fb_crt_run_legacy_ctors(void)
{
    unsigned int count;

    count = 0;
    while (fb_crt_legacy_ctors[count + 1] != 0)
        ++count;

    while (count > 0) {
#if defined(FB_CRT_TRACE_STARTUP)
        fb_crt_trace_constructor(FB_CRT_TRACE_LEGACY_CONSTRUCTOR,
                                 fb_crt_legacy_ctors[count]);
#endif
        fb_crt_legacy_ctors[count]();
        --count;
    }
}

static void fb_crt_run_legacy_dtors(void)
{
    unsigned int index;

    index = 1;
    while (fb_crt_legacy_dtors[index] != 0) {
        fb_crt_legacy_dtors[index]();
        ++index;
    }
}

static void fb_crt_startup(void)
{
    fb_crt_run_legacy_ctors();
    fb_crt_run_forward(&fb_crt_xc_start + 1, &fb_crt_xc_end);
#if defined(FB_CRT_TRACE_STARTUP)
    fb_crt_trace_constructor(FB_CRT_TRACE_STARTUP_COMPLETE, 0);
#endif
}

static void fb_crt_shutdown(void)
{
    if (fb_crt_shutdown_started)
        return;

    fb_crt_shutdown_started = 1;
    fb_crt_run_reverse(&fb_crt_xt_start + 1, &fb_crt_xt_end);

    while (fb_crt_atexit_count > 0) {
        --fb_crt_atexit_count;
        fb_crt_atexit_functions[fb_crt_atexit_count]();
    }

    fb_crt_run_legacy_dtors();
}

/* ------------------------------------------------------------------------- */
/* Minimal termination API                                                   */
/* ------------------------------------------------------------------------- */

int atexit(FB_CRT_FUNCTION function)
{
    if (function == 0 || fb_crt_atexit_count >= FB_CRT_ATEXIT_CAPACITY)
        return -1;

    fb_crt_atexit_functions[fb_crt_atexit_count] = function;
    ++fb_crt_atexit_count;
    return 0;
}

void _cexit(void)
{
    fb_crt_shutdown();
}

void exit(int exit_code)
{
    fb_crt_shutdown();
    fb_crt_exit_process((FB_CRT_DWORD)exit_code);
}

void _exit(int exit_code)
{
    fb_crt_exit_process((FB_CRT_DWORD)exit_code);
}

#if !defined(FB_CRT_BUILD_DLL)

/* ------------------------------------------------------------------------- */
/* Executable command-line adapter                                           */
/* ------------------------------------------------------------------------- */

extern int main(int argument_count, char **arguments);

static int fb_crt_is_argument_space(unsigned short character)
{
    return character == (unsigned short)' ' ||
           character == (unsigned short)'\t';
}

static int fb_crt_store_argument(const unsigned short *wide_argument,
                                 char *argument_buffer,
                                 unsigned int *buffer_used,
                                 char **argument)
{
    unsigned int remaining;
    int converted;

    if (wide_argument == 0 || argument_buffer == 0 || buffer_used == 0 ||
        argument == 0 || *buffer_used >= FB_CRT_ARGUMENT_BUFFER_CAPACITY)
        return 0;

    remaining = FB_CRT_ARGUMENT_BUFFER_CAPACITY - *buffer_used;
    converted = WideCharToMultiByte(FB_CRT_CODE_PAGE_ACP,
                                    0,
                                    wide_argument,
                                    -1,
                                    argument_buffer + *buffer_used,
                                    (int)remaining,
                                    0,
                                    0);
    if (converted <= 0 || (unsigned int)converted > remaining)
        return 0;

    *argument = argument_buffer + *buffer_used;
    *buffer_used += (unsigned int)converted;
    return 1;
}

static int fb_crt_append_wide_character(unsigned short *token,
                                        unsigned int *token_length,
                                        unsigned short character)
{
    if (token == 0 || token_length == 0 ||
        *token_length + 1U >= FB_CRT_WIDE_TOKEN_CAPACITY)
        return 0;

    token[*token_length] = character;
    ++*token_length;
    return 1;
}

static int fb_crt_build_arguments(unsigned short *command_line,
                                  int *argument_count,
                                  char **arguments,
                                  char *argument_buffer)
{
    unsigned short module_path[FB_CRT_MODULE_PATH_CAPACITY];
    unsigned short token[FB_CRT_WIDE_TOKEN_CAPACITY];
    unsigned short *cursor;
    unsigned int buffer_used;
    unsigned int index;
    unsigned int module_length;
    unsigned int slash_count;
    unsigned int token_length;
    int count;
    int in_quotes;

    if (argument_count == 0 || arguments == 0 || argument_buffer == 0)
        return 0;

    module_length = GetModuleFileNameW(0,
                                       module_path,
                                       FB_CRT_MODULE_PATH_CAPACITY);
    if (module_length == 0 || module_length >= FB_CRT_MODULE_PATH_CAPACITY)
        return 0;

    module_path[module_length] = 0;
    buffer_used = 0;
    count = 0;

    if (!fb_crt_store_argument(module_path,
                               argument_buffer,
                               &buffer_used,
                               &arguments[count]))
        return 0;
    ++count;

    cursor = command_line;
    while (cursor != 0 && *cursor != 0) {
        while (fb_crt_is_argument_space(*cursor))
            ++cursor;

        if (*cursor == 0)
            break;
        if ((unsigned int)count >= FB_CRT_MAX_ARGUMENTS)
            return 0;

        token_length = 0;
        in_quotes = 0;

        while (*cursor != 0) {
            if (!in_quotes && fb_crt_is_argument_space(*cursor))
                break;

            if (*cursor == (unsigned short)'\\') {
                slash_count = 0;
                while (cursor[slash_count] == (unsigned short)'\\')
                    ++slash_count;

                if (cursor[slash_count] == (unsigned short)'"') {
                    for (index = 0; index < slash_count / 2U; ++index) {
                        if (!fb_crt_append_wide_character(token,
                                                          &token_length,
                                                          (unsigned short)'\\'))
                            return 0;
                    }

                    cursor += slash_count;
                    if ((slash_count & 1U) != 0) {
                        if (!fb_crt_append_wide_character(token,
                                                          &token_length,
                                                          (unsigned short)'"'))
                            return 0;
                        ++cursor;
                    } else {
                        in_quotes = !in_quotes;
                        ++cursor;
                    }
                    continue;
                }

                for (index = 0; index < slash_count; ++index) {
                    if (!fb_crt_append_wide_character(token,
                                                      &token_length,
                                                      (unsigned short)'\\'))
                        return 0;
                }
                cursor += slash_count;
                continue;
            }

            if (*cursor == (unsigned short)'"') {
                if (in_quotes && cursor[1] == (unsigned short)'"') {
                    if (!fb_crt_append_wide_character(token,
                                                      &token_length,
                                                      (unsigned short)'"'))
                        return 0;
                    cursor += 2;
                } else {
                    in_quotes = !in_quotes;
                    ++cursor;
                }
                continue;
            }

            if (!fb_crt_append_wide_character(token,
                                              &token_length,
                                              *cursor))
                return 0;
            ++cursor;
        }

        token[token_length] = 0;
        if (!fb_crt_store_argument(token,
                                   argument_buffer,
                                   &buffer_used,
                                   &arguments[count]))
            return 0;
        ++count;
    }

    arguments[count] = 0;
    *argument_count = count;
    return 1;
}

#endif

/* ------------------------------------------------------------------------- */
/* Windows CE loader entry points                                            */
/* ------------------------------------------------------------------------- */

#if !defined(FB_CRT_BUILD_DLL)

void WinMainCRTStartup(FB_CRT_HANDLE instance,
                       FB_CRT_HANDLE previous_instance,
                       unsigned short *command_line,
                       int show_command)
{
    char argument_buffer[FB_CRT_ARGUMENT_BUFFER_CAPACITY];
    char *arguments[FB_CRT_MAX_ARGUMENTS + 1U];
    int argument_count;
    int exit_code;

    (void)instance;
    (void)previous_instance;
    (void)show_command;

    fb_crt_startup();
    if (!fb_crt_build_arguments(command_line,
                                &argument_count,
                                arguments,
                                argument_buffer))
        exit(FB_CRT_INVALID_PARAMETER);

    exit_code = main(argument_count, arguments);
    exit(exit_code);
}

#else

__attribute__((weak))
FB_CRT_BOOL DllMain(FB_CRT_HANDLE module,
                    FB_CRT_DWORD reason,
                    void *reserved)
{
    (void)module;
    (void)reason;
    (void)reserved;
    return 1;
}

FB_CRT_BOOL DllMainCRTStartup(FB_CRT_HANDLE module,
                              FB_CRT_DWORD reason,
                              void *reserved)
{
    FB_CRT_BOOL result;

    if (reason == FB_CRT_PROCESS_ATTACH)
        fb_crt_startup();

    result = DllMain(module, reason, reserved);

    if (reason == FB_CRT_PROCESS_DETACH)
        fb_crt_shutdown();

    return result;
}

#endif

/* end of wince/mips-toolchain/crt0.c */
