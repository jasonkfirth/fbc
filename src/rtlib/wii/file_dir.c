/*
    FreeBASIC runtime Wii directory enumeration
    -------------------------------------------

    File: file_dir.c

    Purpose:

        Implement the DIR runtime function on top of libogc/newlib directory
        APIs.

    Responsibilities:

        - enumerate FAT-backed Wii directories for DIR and DIR()
        - apply FreeBASIC's simple attribute filtering rules
        - convert DOS-style path separators before touching the filesystem

    This file intentionally does NOT contain:

        - file opening
        - current-directory management
        - executable path discovery
*/

#include "../fb.h"
#include <dirent.h>
#include <sys/stat.h>

typedef struct _FB_DIRCTX {
	int in_use;
	int attrib;
	DIR *dir;
	char filespec[MAX_PATH];
	char dirname[MAX_PATH];
} FB_DIRCTX;

static void close_dir_internal(FB_DIRCTX *ctx)
{
	if (ctx->dir != NULL)
		closedir(ctx->dir);

	ctx->dir = NULL;
	ctx->in_use = FALSE;
}

void fb_DIRCTX_Destructor(void *data)
{
	FB_DIRCTX *ctx = (FB_DIRCTX *)data;

	if (ctx->in_use)
		close_dir_internal(ctx);
}

static void close_dir(void)
{
	FB_DIRCTX *ctx = FB_TLSGETCTX(DIR);

	close_dir_internal(ctx);
}

static int get_attrib(const char *name, const struct stat *info)
{
	int attrib = 0;

	if ((info->st_mode & S_IWUSR) == 0)
		attrib |= 0x1;	/* read only */

	if (name[0] == '.')
		attrib |= 0x2;	/* hidden */

	if (S_ISDIR(info->st_mode))
		attrib |= 0x10;	/* directory */
	else
		attrib |= 0x20;	/* archive */

	return attrib;
}

static int match_spec(const char *name)
{
	FB_DIRCTX *ctx = FB_TLSGETCTX(DIR);
	const char *spec = ctx->filespec;
	const char *any = NULL;

	while ((*spec) || (*name)) {
		switch (*spec) {
		case '*':
			any = spec;
			spec++;
			while ((*name != *spec) && (*name))
				name++;
			break;

		case '?':
			spec++;
			if (*name)
				name++;
			break;

		default:
			if (*spec != *name) {
				if ((any) && (*name))
					spec = any;
				else
					return FALSE;
			} else {
				spec++;
				name++;
			}
			break;
		}
	}

	return TRUE;
}

static char *find_next(int *attrib)
{
	FB_DIRCTX *ctx = FB_TLSGETCTX(DIR);
	struct dirent *entry;
	struct stat info;
	char buffer[MAX_PATH];
	char *name;

	for (;;) {
		entry = readdir(ctx->dir);
		if (entry == NULL) {
			close_dir();
			return NULL;
		}

		name = entry->d_name;
		strncpy(buffer, ctx->dirname, MAX_PATH);
		buffer[MAX_PATH - 1] = '\0';
		strncat(buffer, name, MAX_PATH - strlen(buffer) - 1);
		buffer[MAX_PATH - 1] = '\0';

		if (stat(buffer, &info) != 0)
			continue;

		*attrib = get_attrib(name, &info);
		if (((*attrib & ~ctx->attrib) == 0) && match_spec(name))
			return name;
	}
}

static int copy_filespec(char *dst, FBSTRING *filespec)
{
	ssize_t len = FB_STRSIZE(filespec);

	if (len <= 0) {
		dst[0] = '\0';
		return TRUE;
	}

	if (len >= MAX_PATH)
		return FALSE;

	memcpy(dst, filespec->data, (size_t)len);
	dst[len] = '\0';
	fb_hConvertPath(dst);

	return TRUE;
}

FBCALL FBSTRING *fb_Dir(FBSTRING *filespec, int attrib, int *out_attrib)
{
	FB_DIRCTX *ctx;
	FBSTRING *res;
	ssize_t len;
	int tmp_attrib;
	char path[MAX_PATH];
	char *name = NULL;
	char *p;
	struct stat info;

	if (out_attrib == NULL)
		out_attrib = &tmp_attrib;
	*out_attrib = 0;

	if (filespec == NULL)
		return &__fb_ctx.null_desc;

	ctx = FB_TLSGETCTX(DIR);

	if (!copy_filespec(path, filespec)) {
		fb_hStrDelTemp(filespec);
		return &__fb_ctx.null_desc;
	}

	len = strlen(path);

	if (len > 0) {
		/* findfirst */
		if (ctx->in_use)
			close_dir();

		if ((strchr(path, '*') != NULL) || (strchr(path, '?') != NULL)) {
			p = strrchr(path, '/');
			if (p != NULL) {
				strncpy(ctx->filespec, p + 1, MAX_PATH);
				ctx->filespec[MAX_PATH - 1] = '\0';
				len = (p - path) + 1;
				if (len > MAX_PATH - 1)
					len = MAX_PATH - 1;
				memcpy(ctx->dirname, path, (size_t)len);
				ctx->dirname[len] = '\0';
			} else {
				strncpy(ctx->filespec, path, MAX_PATH);
				ctx->filespec[MAX_PATH - 1] = '\0';
				memcpy(ctx->dirname, "./", sizeof("./"));
			}

			if ((!strcmp(ctx->filespec, "*.*")) || (!strcmp(ctx->filespec, "*."))) {
				ctx->filespec[0] = '*';
				ctx->filespec[1] = '\0';
			}

			if ((attrib & 0x10) == 0)
				attrib |= 0x20;

			ctx->attrib = attrib;
			ctx->dir = opendir(ctx->dirname);
			if (ctx->dir != NULL) {
				name = find_next(out_attrib);
				if (name != NULL)
					ctx->in_use = TRUE;
			}
		} else {
			if (stat(path, &info) == 0) {
				int allowed_attrib = attrib;

				if ((allowed_attrib & 0x10) == 0)
					allowed_attrib |= 0x20;

				tmp_attrib = get_attrib(path, &info);
				if ((tmp_attrib & ~allowed_attrib) == 0) {
					name = strrchr(path, '/');
					if (name == NULL)
						name = path;
					else
						name++;
					*out_attrib = tmp_attrib;
				}
			}
		}
	} else {
		/* findnext */
		if (ctx->in_use)
			name = find_next(out_attrib);
	}

	FB_STRLOCK();

	if (name != NULL) {
		len = strlen(name);
		res = fb_hStrAllocTemp_NoLock(NULL, len);
		if (res != NULL)
			fb_hStrCopy(res->data, name, len);
		else
			res = &__fb_ctx.null_desc;
	} else {
		res = &__fb_ctx.null_desc;
		*out_attrib = 0;
	}

	fb_hStrDelTemp_NoLock(filespec);

	FB_STRUNLOCK();

	return res;
}

/* end of file_dir.c */
