/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_dir.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

static int fb_nuttx_dir_match_tail(const char *name, const char *tail)
{
    size_t name_len;
    size_t tail_len;

    name_len = strlen(name);
    tail_len = strlen(tail);

    if (tail_len > name_len)
        return 0;

    return strcmp(name + (name_len - tail_len), tail) == 0;
}

static int fb_nuttx_dir_match(const char *name, const char *pattern)
{
    const char *star;
    size_t prefix_len;

    if ((name == NULL) || (pattern == NULL))
        return 0;

    if ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0))
        return 0;

    if (strcmp(pattern, "*") == 0)
        return 1;

    star = strchr(pattern, '*');

    if (star == NULL)
        return strcmp(name, pattern) == 0;

    prefix_len = (size_t)(star - pattern);

    if ((prefix_len > 0) && (strncmp(name, pattern, prefix_len) != 0))
        return 0;

    return fb_nuttx_dir_match_tail(name, star + 1);
}

static DIR *fb_nuttx_dir_search;
static char *fb_nuttx_dir_mask;
static char *fb_nuttx_dir_path;

static char *fb_nuttx_dir_string_dup(const char *text)
{
    char *copy;
    size_t len;

    if (text == NULL)
        return NULL;

    len = strlen(text) + 1;
    copy = (char *)malloc(len);

    if (copy == NULL)
        return NULL;

    memcpy(copy, text, len);

    return copy;
}

static void fb_nuttx_dir_close(void)
{
    if (fb_nuttx_dir_search != NULL) {
        closedir(fb_nuttx_dir_search);
        fb_nuttx_dir_search = NULL;
    }

    free(fb_nuttx_dir_mask);
    fb_nuttx_dir_mask = NULL;

    free(fb_nuttx_dir_path);
    fb_nuttx_dir_path = NULL;
}

static int32 fb_nuttx_dir_entry_attrib(const char *name)
{
    char *path;
    size_t dir_len;
    size_t name_len;
    size_t path_len;
    int add_slash;
    struct stat st;
    int32 attrib;

    attrib = 0;

    if ((fb_nuttx_dir_path == NULL) || (name == NULL))
        return attrib;

    dir_len = strlen(fb_nuttx_dir_path);
    name_len = strlen(name);
    add_slash = 0;

    if ((dir_len > 0) && (fb_nuttx_dir_path[dir_len - 1] != '/'))
        add_slash = 1;

    if (dir_len > (SIZE_MAX - name_len - (size_t)add_slash - 1))
        return attrib;

    path_len = dir_len + (size_t)add_slash + name_len;
    path = (char *)malloc(path_len + 1);

    if (path == NULL)
        return attrib;

    memcpy(path, fb_nuttx_dir_path, dir_len);

    if (add_slash != 0)
        path[dir_len++] = '/';

    memcpy(path + dir_len, name, name_len);
    path[path_len] = '\0';

    /*
        FreeBASIC's DIR() exposes platform file attributes through the
        out parameter.  NuttX does not provide DOS-style attribute bits, but
        it does provide enough stat() information to mark directories.
    */
    if ((stat(path, &st) == 0) && S_ISDIR(st.st_mode))
        attrib |= 0x10;

    free(path);

    return attrib;
}

FBSTRING *fb_DirNext(int32 *out_attrib)
{
    struct dirent *entry;

    if (out_attrib != NULL)
        *out_attrib = 0;

    if ((fb_nuttx_dir_search == NULL) || (fb_nuttx_dir_mask == NULL))
        return fb_nuttx_temp_string("", 0);

    while ((entry = readdir(fb_nuttx_dir_search)) != NULL) {
        if (fb_nuttx_dir_match(entry->d_name, fb_nuttx_dir_mask)) {
            if (out_attrib != NULL)
                *out_attrib = fb_nuttx_dir_entry_attrib(entry->d_name);

            return fb_nuttx_temp_copy(entry->d_name,
                (int32)strlen(entry->d_name));
        }
    }

    fb_nuttx_dir_close();

    return fb_nuttx_temp_string("", 0);
}

FBSTRING *fb_Dir(const FBSTRING *pattern, const int32 attrib, int32 *out_attrib)
{
    char *path;
    char *slash;
    char cwd[256];
    const char *mask;
    const char *dir_path;
    DIR *dir;

    (void)attrib;

    if (out_attrib != NULL)
        *out_attrib = 0;

    fb_nuttx_dir_close();

    path = fb_nuttx_string_to_cstr(pattern);

    if (path == NULL)
        return fb_nuttx_temp_string("", 0);

    slash = strrchr(path, '/');
    dir_path = NULL;

    if (slash == NULL) {
        mask = path;

        /*
            Some NuttX filesystem paths behave more predictably when opened
            by their resolved name instead of ".".  Resolve mask-only DIR()
            calls through getcwd() while still falling back to "." if the
            platform cannot report the current directory.
        */
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            dir_path = cwd;
            dir = opendir(cwd);
        } else {
            dir_path = ".";
            dir = opendir(".");
        }
    } else {
        mask = slash + 1;

        if (slash == path)
            slash[1] = '\0';
        else
            slash[0] = '\0';

        dir_path = path;
        dir = opendir(path);
    }

    if (dir == NULL) {
        free(path);
        return fb_nuttx_temp_string("", 0);
    }

    fb_nuttx_dir_mask = fb_nuttx_dir_string_dup(mask);
    fb_nuttx_dir_path = fb_nuttx_dir_string_dup(dir_path);

    free(path);

    if ((fb_nuttx_dir_mask == NULL) || (fb_nuttx_dir_path == NULL)) {
        closedir(dir);
        fb_nuttx_dir_close();
        return fb_nuttx_temp_string("", 0);
    }

    fb_nuttx_dir_search = dir;

    return fb_DirNext(out_attrib);
}

FBSTRING *fb_DirNext64(int64_t *out_attrib)
{
    int32 narrowed_attrib;
    FBSTRING *result;

    narrowed_attrib = 0;
    result = fb_DirNext(&narrowed_attrib);

    if (out_attrib != NULL)
        *out_attrib = (int64_t)narrowed_attrib;

    return result;
}

FBSTRING *fb_Dir64(const FBSTRING *pattern, const int32 attrib,
    int64_t *out_attrib)
{
    int32 narrowed_attrib;
    FBSTRING *result;

    narrowed_attrib = 0;
    result = fb_Dir(pattern, attrib, &narrowed_attrib);

    if (out_attrib != NULL)
        *out_attrib = (int64_t)narrowed_attrib;

    return result;
}

/* end of fb_nuttx_dir.c */
