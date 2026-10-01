/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------
    File: amiga/sys_execex.c
    Purpose: Execute BASIC EXEC, CHAIN, and RUN through native DOS commands.
    Responsibilities: Quote arguments and preserve synchronous return codes.
    This file intentionally does NOT contain Unix fork or descriptor emulation.

    Amiga shell quotes use '*' as their escape character. Quote the program
    and each parsed BASIC argument so shell metacharacters remain literal.
    SystemTagList creates the child context and searches the native command path.
*/

#include "../fb.h"
#include "../unix/fb_private_console.h"
#include <proto/dos.h>
#include <dos/dostags.h>

static size_t quoted_size(const char *text)
{
    size_t result = 3; /* enclosing quotes and a separating space */
    while (*text) {
        result += (*text == '*' || *text == '"' || *text == '\n') ? 2 : 1;
        ++text;
    }
    return result;
}

static char *quote_argument(char *destination, const char *text)
{
    *destination++ = '"';
    while (*text) {
        if (*text == '*' || *text == '"' || *text == '\n') {
            *destination++ = '*';
            *destination++ = *text == '\n' ? 'N' : *text;
        } else *destination++ = *text;
        ++text;
    }
    *destination++ = '"';
    *destination++ = ' ';
    return destination;
}

FBCALL int fb_ExecEx(FBSTRING *program, FBSTRING *arguments, int do_fork)
{
    char *parsed = NULL, *command = NULL, *cursor, *destination;
    ssize_t program_length = program != NULL ? FB_STRSIZE(program) : 0;
    ssize_t argument_length = arguments != NULL ? FB_STRSIZE(arguments) : 0;
    size_t size;
    int count = 0, index, result = -1;
    struct TagItem tags[4];

    (void)do_fork;
    /* Bound command buffers instead of placing caller-sized arguments on the
       relatively small native process stack. */
    if (program == NULL || program->data == NULL || program_length <= 0 ||
        program_length >= MAX_PATH || argument_length < 0 || argument_length > 65535)
        goto done;
    parsed = malloc((size_t)argument_length + 1);
    if (parsed == NULL) goto done;
    parsed[argument_length] = '\0';
    if (argument_length != 0) {
        if (arguments->data == NULL) goto done;
        count = fb_hParseArgs(parsed, arguments->data, argument_length);
        if (count < 0) goto done;
    }
    size = quoted_size(program->data) + 1;
    cursor = parsed;
    for (index = 0; index < count; ++index) {
        size_t added = quoted_size(cursor);
        if (added > SIZE_MAX - size) goto done;
        size += added;
        cursor += strlen(cursor) + 1;
    }
    command = malloc(size);
    if (command == NULL) goto done;
    destination = quote_argument(command, program->data);
    cursor = parsed;
    for (index = 0; index < count; ++index) {
        destination = quote_argument(destination, cursor);
        cursor += strlen(cursor) + 1;
    }
    *destination = '\0';
    tags[0].ti_Tag = SYS_Input; tags[0].ti_Data = Input();
    tags[1].ti_Tag = SYS_Output; tags[1].ti_Data = Output();
    tags[2].ti_Tag = SYS_Asynch; tags[2].ti_Data = FALSE;
    tags[3].ti_Tag = TAG_DONE; tags[3].ti_Data = 0;
    FB_LOCK(); fb_hExitConsole(); FB_UNLOCK();
    result = SystemTagList(command, tags);
    FB_LOCK(); fb_hInitConsole(); FB_UNLOCK();
done:
    free(command);
    free(parsed);
    fb_hStrDelTemp(arguments);
    fb_hStrDelTemp(program);
    return result;
}

/* end of amiga/sys_execex.c */
