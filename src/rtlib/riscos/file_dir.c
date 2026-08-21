/*
    FreeBASIC runtime support for RISC OS
    -------------------------------------

    File: file_dir.c

    Purpose:

        Implement DIR and DIRNEXT using RISC OS filesystem semantics.

    Responsibilities:

        - enumerate files through UnixLib directory streams
        - match wildcard specifications without case sensitivity
        - translate POSIX file metadata into FreeBASIC attributes
        - retain enumeration state in FreeBASIC's thread-local context

    This file intentionally does NOT contain:

        - UnixLib pathname or suffix-directory translation
        - filesystem mutation operations
        - platform-independent string allocation logic

    RISC OS filesystems match names without regard to letter case. UnixLib's
    reverse suffix translation can, for example, expose a file stored below a
    physical `bmp` directory as `TITLE.bmp`. A BASIC program asking DIR for
    `*.BMP` must still find that file, just as it would through native RISC OS
    filing-system calls.
*/

#include "../fb.h"

#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unixlib/local.h>

/* ------------------------------------------------------------------------- */
/* Enumeration state                                                         */
/* ------------------------------------------------------------------------- */

typedef struct _FB_DIRCTX
{
    int in_use;
    int attrib;
    DIR *dir;
    char filespec[MAX_PATH];
    char dirname[MAX_PATH];
} FB_DIRCTX;

static void close_dir_internal(FB_DIRCTX *context)
{
    closedir(context->dir);
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

    /*
        RISC OS directory access bits do not map cleanly to the DOS-style
        read-only bit returned by FreeBASIC. Marking a directory read-only
        makes the conventional DIR(path, fbDirectory) existence test reject
        it unless the caller also requests fbReadOnly.
    */

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

static int get_dirent_attrib(char *name, struct dirent *entry)
{
    int attrib;

    attrib = 0;

    if (name[0] == '.')
        attrib |= 0x2;

    if (entry->d_type == DT_CHR || entry->d_type == DT_BLK ||
        entry->d_type == DT_FIFO || entry->d_type == DT_SOCK)
    {
        attrib |= 0x4;
    }

    if (entry->d_type == DT_DIR)
        attrib |= 0x10;
    else
        attrib |= 0x20;

    return attrib;
}

static int riscos_character_equal(char left, char right)
{
    unsigned char unsigned_left;
    unsigned char unsigned_right;

    unsigned_left = (unsigned char)left;
    unsigned_right = (unsigned char)right;

    return tolower(unsigned_left) == tolower(unsigned_right);
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
                while (*name &&
                    !riscos_character_equal(*name, *specification))
                {
                    name++;
                }
                break;

            case '?':
                specification++;
                if (*name)
                    name++;
                break;

            default:
                if (!riscos_character_equal(*specification, *name))
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

static DIR *riscos_open_dir(const char *dirname)
{
    DIR *directory;
    int riscosify_control;

    /*
        UnixLib normally rejects opendir() when the final directory name is
        also a configured suffix, because that name usually identifies a
        suffix-swapped file collection rather than a logical Unix directory.
        A real RISC OS directory may legitimately have that name, however.

        Open the physical directory with that ambiguity disabled, then restore
        reverse suffix handling before readdir(). This lets readdir() expose
        files held in a suffix subdirectory while DIR still sees a real parent
        named, for example, `bmp`.
    */

    riscosify_control = __get_riscosify_control();
    __set_riscosify_control(
        riscosify_control | __RISCOSIFY_NO_REVERSE_SUFFIX);
    directory = opendir(dirname);
    __set_riscosify_control(riscosify_control);

    return directory;
}

static int get_exact_path_attrib(char *name, struct stat *information)
{
    char directory_name[MAX_PATH];
    char *leaf;
    DIR *directory;
    int attrib;
    size_t length;

    if (stat(name, information) == 0)
        return get_attrib(name, information);

    /*
        UnixLib's stat() cannot always disambiguate a real directory whose
        leaf is also in UnixEnv$sfix. A trailing separator makes the requested
        object unambiguously a directory before opendir() applies its suffix
        rules. Use that physical-directory lookup as the fallback for the
        conventional DIR(path, fbDirectory) existence test.
    */

    length = strlen(name);
    if (length >= sizeof(directory_name) - 1)
        return -1;

    memcpy(directory_name, name, length);
    if (length == 0 || directory_name[length - 1] != '/')
        directory_name[length++] = '/';
    directory_name[length] = '\0';

    directory = riscos_open_dir(directory_name);
    if (directory == NULL)
        return -1;
    closedir(directory);

    attrib = 0x10;
    leaf = strrchr(name, '/');
    if (leaf == NULL)
        leaf = name;
    else
        leaf++;

    if (leaf[0] == '.')
        attrib |= 0x2;

    return attrib;
}

/* ------------------------------------------------------------------------- */
/* Directory enumeration                                                      */
/* ------------------------------------------------------------------------- */

static char *find_next(int *attrib)
{
    FB_DIRCTX *context;
    char *name;
    struct stat information;
    struct dirent *entry;
    char buffer[MAX_PATH];

    context = FB_TLSGETCTX(DIR);
    name = NULL;

    for (;;)
    {
        entry = readdir(context->dir);
        if (entry == NULL)
        {
            close_dir();
            return NULL;
        }

        name = entry->d_name;
        strncpy(buffer, context->dirname, MAX_PATH);
        buffer[MAX_PATH - 1] = '\0';
        strncat(buffer, name, MAX_PATH - strlen(buffer) - 1);
        buffer[MAX_PATH - 1] = '\0';

        if (stat(buffer, &information) == 0)
        {
            *attrib = get_attrib(name, &information);
        }
        else
        {
            /*
                Reverse suffix enumeration synthesizes names such as
                `TITLE.bmp`. Restating that name can be ambiguous when a real
                parent directory is also named `bmp`, but UnixLib's dirent
                still contains the filing system's authoritative object type.
            */

            *attrib = get_dirent_attrib(name, entry);
        }

        if (((*attrib & ~context->attrib) == 0) && match_spec(name))
            return name;
    }
}

FBCALL FBSTRING *fb_Dir(FBSTRING *filespec, int attrib, int *out_attrib)
{
    FB_DIRCTX *context;
    FBSTRING *result;
    ssize_t length;
    int temporary_attrib;
    char *name;
    char *separator;
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

        if (strchr(filespec->data, '*') || strchr(filespec->data, '?'))
        {
            separator = strrchr(filespec->data, '/');
            if (separator)
            {
                strncpy(context->filespec, separator + 1, MAX_PATH);
                context->filespec[MAX_PATH - 1] = '\0';
                length = (separator - filespec->data) + 1;
                if (length > MAX_PATH - 1)
                    length = MAX_PATH - 1;
                memcpy(context->dirname, filespec->data, length);
                context->dirname[length] = '\0';
            }
            else
            {
                strncpy(context->filespec, filespec->data, MAX_PATH);
                context->filespec[MAX_PATH - 1] = '\0';
                memcpy(context->dirname, "./", sizeof("./"));
            }

            /* Keep historical Win32 and DOS wildcard compatibility. */

            if (!strcmp(context->filespec, "*.*") ||
                !strcmp(context->filespec, "*."))
            {
                context->filespec[0] = '*';
                context->filespec[1] = '\0';
            }

            if ((attrib & 0x10) == 0)
                attrib |= 0x20;

            context->attrib = attrib;
            context->dir = riscos_open_dir(context->dirname);
            if (context->dir)
            {
                name = find_next(out_attrib);
                if (name)
                    context->in_use = TRUE;
            }
        }
        else
        {
            temporary_attrib = get_exact_path_attrib(
                filespec->data, &information);
            if (temporary_attrib >= 0 &&
                (temporary_attrib & ~attrib) == 0)
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

    if (name)
    {
        length = strlen(name);
        result = fb_hStrAllocTemp_NoLock(NULL, length);
        if (result)
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

/* end of file_dir.c */
