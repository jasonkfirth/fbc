/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_env.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

FBSTRING *fb_Command(const int32 index)
{
    char *text;
    char *cursor;
    size_t len;
    size_t part_len;
    int i;

    if ((index == -1) && (fb_nuttx_argv != NULL) && (fb_nuttx_argc > 1)) {
        len = 0;

        for (i = 1; i < fb_nuttx_argc; i++) {
            if (fb_nuttx_argv[i] != NULL)
                len += strlen(fb_nuttx_argv[i]);

            if (i > 1)
                len++;
        }

        if ((len == 0) || (len > (size_t)2147483647))
            return fb_nuttx_temp_string("", 0);

        text = (char *)malloc(len + 1);

        if (text == NULL)
            return fb_nuttx_temp_string("", 0);

        cursor = text;

        for (i = 1; i < fb_nuttx_argc; i++) {
            if (i > 1) {
                *cursor = ' ';
                cursor++;
            }

            if (fb_nuttx_argv[i] != NULL) {
                part_len = strlen(fb_nuttx_argv[i]);
                memcpy(cursor, fb_nuttx_argv[i], part_len);
                cursor += part_len;
            }
        }

        *cursor = '\0';

        return fb_nuttx_temp_string(text, (int32)len);
    }

    if ((index >= 0) && (index < fb_nuttx_argc) &&
        (fb_nuttx_argv != NULL) && (fb_nuttx_argv[index] != NULL))
        return fb_StrAllocTempDescZEx(fb_nuttx_argv[index],
            fb_nuttx_string_length(fb_nuttx_argv[index], -1));

    return fb_nuttx_temp_string("", 0);
}

FBSTRING *fb_ExePath(void)
{
    if ((fb_nuttx_argv != NULL) && (fb_nuttx_argc > 0) &&
        (fb_nuttx_argv[0] != NULL))
        return fb_StrAllocTempDescZEx(fb_nuttx_argv[0],
            fb_nuttx_string_length(fb_nuttx_argv[0], -1));

    return fb_nuttx_temp_string("", 0);
}

void fb_nuttx_set_args(int argc, char **argv)
{
    fb_nuttx_argc = argc;
    fb_nuttx_argv = argv;
}

int32 fb_Shell(const FBSTRING *command)
{
    static char shell_name[] = "sh";
    static char shell_option[] = "-c";
    char *text;
    char *argv[4];
    int status;
    int pid;

    text = fb_nuttx_string_to_cstr(command);

    if (text == NULL)
        return -1;

    /*
        NuttX's normal system() wrapper is not present in the tiny QEMU
        configuration used by this port seed, but the NSH shell is registered
        as a built-in application.

        The built-in helper starts the shell as its own task.  The inline
        command path follows the NSH command line form:

            sh -c <command>

        Waiting for that task keeps FreeBASIC's SHELL behavior synchronous.
    */
    argv[0] = shell_name;
    argv[1] = shell_option;
    argv[2] = text;
    argv[3] = NULL;

    pid = exec_builtin("sh", argv, NULL);

    if (pid < 0) {
        free(text);
        return -1;
    }

    if (waitpid(pid, &status, 0) < 0) {
        free(text);
        return -1;
    }

    free(text);

    if (WIFEXITED(status))
        return (int32)WEXITSTATUS(status);

    return -1;
}

#if !defined(FB_NUTTX_USE_GENERIC_ENVIRON) || \
    (FB_NUTTX_USE_GENERIC_ENVIRON == 0)

static int fb_nuttx_set_environ_text(const char *text)
{
    const char *equals;
    char *name;
    size_t name_len;
    int result;

    if (text == NULL)
        return 0;

    equals = strchr(text, '=');

    if (equals == NULL)
        return unsetenv(text);

    name_len = (size_t)(equals - text);

    if (name_len == 0)
        return -1;

    name = (char *)malloc(name_len + 1);

    if (name == NULL)
        return -1;

    memcpy(name, text, name_len);
    name[name_len] = '\0';

    result = setenv(name, equals + 1, 1);
    free(name);

    return result;
}

FBSTRING *fb_GetEnviron(const FBSTRING *name)
{
    char *key;
    char *equals;
    char *value;
    FBSTRING *result;

    if ((name == NULL) || (name->data == NULL) || (name->len <= 0))
        return fb_nuttx_temp_string("", 0);

    key = (char *)malloc((size_t)name->len + 1);

    if (key == NULL)
        return fb_nuttx_temp_string("", 0);

    memcpy(key, name->data, (size_t)name->len);
    key[name->len] = '\0';

	equals = strchr(key, '=');

	if (equals != NULL) {
		(void)fb_nuttx_set_environ_text(key);

		free(key);

        return fb_nuttx_temp_string("", 0);
    }

    value = getenv(key);
    free(key);

    if (value == NULL)
        return fb_nuttx_temp_string("", 0);

    result = fb_nuttx_temp_copy(value, fb_nuttx_string_length(value, -1));

    return result;
}

int32 fb_SetEnviron(const FBSTRING *str)
{
    char *text;
    int result;

    if ((str == NULL) || (str->data == NULL))
        return 0;

    text = fb_nuttx_string_to_cstr(str);

    if (text == NULL)
        return -1;

    result = fb_nuttx_set_environ_text(text);
    free(text);

    return (int32)result;
}
#endif

/* end of fb_nuttx_env.c */
