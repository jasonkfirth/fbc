/*
    FreeBASIC runtime support for AROS
    ----------------------------------

    File: file_dir.c

    Purpose:

        Implement DIR and DIRNEXT with AROS filesystem attribute semantics.

    Responsibilities:

        - enumerate wildcard matches through native AROS DOS locks
        - translate AROS file metadata into FreeBASIC attributes
        - treat directories as directories even on read-only boot media
        - retain enumeration state in FreeBASIC's thread-local context

    This file intentionally does NOT contain:

        - filesystem mutation operations
        - AROS architecture-specific behavior
        - game or package layout policy

    AROS CD filesystems expose directories without a writable mode bit. The
    shared Unix implementation consequently adds fbReadOnly to fbDirectory,
    which makes the conventional DIR(path, fbDirectory) existence query reject
    valid directories. AROS directory writability is a volume property rather
    than a useful file attribute, so only non-directory entries receive the
    FreeBASIC read-only bit here.
*/

#include "../fb.h"

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------------- */
/* Enumeration state                                                         */
/* ------------------------------------------------------------------------- */

typedef struct _FB_DIRCTX
{
    int in_use;
    int attrib;
    BPTR lock;
    struct FileInfoBlock *information;
    char filespec[MAX_PATH];
    char dirname[MAX_PATH];
} FB_DIRCTX;

static void close_dir_internal(FB_DIRCTX *context)
{
    if (context->lock != BNULL)
        UnLock(context->lock);
    if (context->information != NULL)
        FreeDosObject(DOS_FIB, context->information);

    context->lock = BNULL;
    context->information = NULL;
    context->in_use = FALSE;
}

void fb_DIRCTX_Destructor(void *data)
{
    FB_DIRCTX *context;

    context = (FB_DIRCTX *)data;
    if (context->in_use)
        close_dir_internal(context);
}

static void close_dir(void)
{
    FB_DIRCTX *context;

    context = FB_TLSGETCTX(DIR);
    close_dir_internal(context);
}

/* ------------------------------------------------------------------------- */
/* Attribute and wildcard translation                                        */
/* ------------------------------------------------------------------------- */

static int get_attrib(char *name, struct stat *information)
{
    int attrib;
    int mask;

    attrib = 0;

    if (information->st_uid == geteuid())
        mask = S_IWUSR;
    else if (information->st_gid == getegid())
        mask = S_IWGRP;
    else
        mask = S_IWOTH;

    if (!S_ISDIR(information->st_mode) &&
        (information->st_mode & mask) == 0)
    {
        attrib |= 0x1;
    }

    if (name[0] == '.')
        attrib |= 0x2;

    if (S_ISCHR(information->st_mode) ||
        S_ISBLK(information->st_mode) ||
        S_ISFIFO(information->st_mode) ||
        S_ISSOCK(information->st_mode))
    {
        attrib |= 0x4;
    }

    if (S_ISDIR(information->st_mode))
        attrib |= 0x10;
    else
        attrib |= 0x20;

    return attrib;
}

static int get_native_attrib(struct FileInfoBlock *information)
{
    int attrib;

    attrib = 0;

    if (information->fib_DirEntryType > 0)
    {
        attrib |= 0x10;
    }
    else
    {
        attrib |= 0x20;
    }

    if (information->fib_FileName[0] == '.')
        attrib |= 0x2;
    if (information->fib_DirEntryType == ST_PIPEFILE)
        attrib |= 0x4;

    return attrib;
}

static int names_equal(char left, char right)
{
    if (left >= 'A' && left <= 'Z')
        left = (char)(left - 'A' + 'a');
    if (right >= 'A' && right <= 'Z')
        right = (char)(right - 'A' + 'a');

    return left == right;
}

static int match_spec(char *name)
{
    FB_DIRCTX *context;
    char *any;
    char *specification;

    context = FB_TLSGETCTX(DIR);
    any = NULL;
    specification = context->filespec;

    while (*specification || *name)
    {
        switch (*specification)
        {
            case '*':
                any = specification;
                specification++;
                while (!names_equal(*name, *specification) && *name)
                    name++;
                break;

            case '?':
                specification++;
                if (*name)
                    name++;
                break;

            default:
                if (!names_equal(*specification, *name))
                {
                    if (any && *name)
                        specification = any;
                    else
                        return FALSE;
                }
                else
                {
                    specification++;
                    name++;
                }
                break;
        }
    }

    return TRUE;
}

static char *find_next(int *attrib)
{
    FB_DIRCTX *context;
    char *name;

    context = FB_TLSGETCTX(DIR);
    name = NULL;

    for (;;)
    {
        if (ExNext(context->lock, context->information) == DOSFALSE)
        {
            close_dir();
            return NULL;
        }

        name = (char *)context->information->fib_FileName;
        *attrib = get_native_attrib(context->information);
        if (((*attrib & ~context->attrib) == 0) && match_spec(name))
            return name;
    }
}

/* ------------------------------------------------------------------------- */
/* Public DIR entry point                                                    */
/* ------------------------------------------------------------------------- */

FBCALL FBSTRING *fb_Dir(FBSTRING *filespec, int attrib, int *out_attrib)
{
    FB_DIRCTX *context;
    FBSTRING *result;
    char *name;
    char *separator;
    int temporary_attrib;
    ssize_t length;
    struct stat information;

    if (out_attrib == NULL)
        out_attrib = &temporary_attrib;

    length = FB_STRSIZE(filespec);
    name = NULL;
    context = FB_TLSGETCTX(DIR);

    if (length > 0)
    {
        if (context->in_use)
            close_dir();

        if ((strchr(filespec->data, '*') != NULL) ||
            (strchr(filespec->data, '?') != NULL))
        {
            separator = strrchr(filespec->data, '/');
            if (separator != NULL)
            {
                strncpy(context->filespec, separator + 1, MAX_PATH);
                context->filespec[MAX_PATH - 1] = '\0';
                if (separator == filespec->data)
                    length = 1;
                else
                    length = separator - filespec->data;
                if (length > MAX_PATH - 1)
                    length = MAX_PATH - 1;
                memcpy(context->dirname, filespec->data, length);
                context->dirname[length] = '\0';
            }
            else
            {
                strncpy(context->filespec, filespec->data, MAX_PATH);
                context->filespec[MAX_PATH - 1] = '\0';
                memcpy(context->dirname, ".", sizeof("."));
            }

            if ((strcmp(context->filespec, "*.*") == 0) ||
                (strcmp(context->filespec, "*.") == 0))
            {
                context->filespec[0] = '*';
                context->filespec[1] = '\0';
            }

            if ((attrib & 0x10) == 0)
                attrib |= 0x20;
            context->attrib = attrib;
            context->information = AllocDosObject(DOS_FIB, NULL);
            context->lock = Lock((CONST_STRPTR)context->dirname, SHARED_LOCK);
            if (context->information != NULL && context->lock != BNULL &&
                Examine(context->lock, context->information) != DOSFALSE)
            {
                name = find_next(out_attrib);
                if (name != NULL)
                    context->in_use = TRUE;
            }
            else
            {
                close_dir_internal(context);
            }
        }
        else if (stat(filespec->data, &information) == 0)
        {
            temporary_attrib = get_attrib(filespec->data, &information);
            if ((temporary_attrib & ~attrib) == 0)
            {
                name = strrchr(filespec->data, '/');
                if (name == NULL)
                    name = filespec->data;
                else
                    name++;
                *out_attrib = temporary_attrib;
            }
        }
    }
    else if (context->in_use)
    {
        name = find_next(out_attrib);
    }

    FB_STRLOCK();

    if (name != NULL)
    {
        length = strlen(name);
        result = fb_hStrAllocTemp_NoLock(NULL, length);
        if (result != NULL)
            fb_hStrCopy(result->data, name, length);
        else
            result = &__fb_ctx.null_desc;
    }
    else
    {
        result = &__fb_ctx.null_desc;
        *out_attrib = 0;
    }

    fb_hStrDelTemp_NoLock(filespec);

    FB_STRUNLOCK();

    return result;
}

/* end of src/rtlib/aros/file_dir.c */
