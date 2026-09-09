/*
    FreeBASIC runtime tests: copy-alias-posix.c
    Verify descriptor identity, complete copies and non-regular-file rejection
    using an exclusively created fixture. No pre-existing files are changed.
*/

#include "../../src/rtlib/fb.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

FBCALL int fb_ErrorSetNum(int error) { return error; }

static unsigned char contents[40001];

static void require(int condition)
{
	if (!condition)
		abort();
}

static void verify(const char *path)
{
	unsigned char actual[sizeof(contents)];
	FILE *file = fopen(path, "rb");
	int close_result;

	require(file != NULL);
	require(fread(actual, 1, sizeof(actual), file) == sizeof(actual));
	require(fgetc(file) == EOF && !ferror(file));
	close_result = fclose(file);
	require(close_result == 0);
	require(memcmp(actual, contents, sizeof(actual)) == 0);
}

int main(void)
{
	char directory[] = "/tmp/fb-copy-alias-XXXXXX";
	char source[128], destination[128], hardlink[128], symlink_path[128], fifo[128];
	struct stat info;
	FILE *file;
	size_t i;
	int close_result;

	require(mkdtemp(directory) != NULL);
	require(snprintf(source, sizeof(source), "%s/source", directory) < (int)sizeof(source));
	require(snprintf(destination, sizeof(destination), "%s/destination", directory) < (int)sizeof(destination));
	require(snprintf(hardlink, sizeof(hardlink), "%s/hardlink", directory) < (int)sizeof(hardlink));
	require(snprintf(symlink_path, sizeof(symlink_path), "%s/symlink", directory) < (int)sizeof(symlink_path));
	require(snprintf(fifo, sizeof(fifo), "%s/fifo", directory) < (int)sizeof(fifo));
	for (i = 0; i < sizeof(contents); i++) contents[i] = (unsigned char)(i * 37);
	file = fopen(source, "wb");
	require(file != NULL);
	require(fwrite(contents, 1, sizeof(contents), file) == sizeof(contents));
	close_result = fclose(file);
	require(close_result == 0);
	require(link(source, hardlink) == 0);
	require(symlink(source, symlink_path) == 0);
	require(lstat(symlink_path, &info) == 0 && S_ISLNK(info.st_mode));
	require(fb_FileCopy(source, source) != 0);
	verify(source);
	require(fb_FileCopy(source, hardlink) != 0);
	verify(source);
	require(fb_FileCopy(source, symlink_path) != 0);
	verify(source);
	require(fb_FileCopy(symlink_path, source) != 0);
	verify(source);
	require(fb_FileCopy(source, destination) == 0);
	verify(destination);
	file = fopen(destination, "ab");
	require(file != NULL);
	require(fputs("extra tail", file) >= 0);
	close_result = fclose(file);
	require(close_result == 0);
	require(fb_FileCopy(source, destination) == 0);
	verify(destination);
	require(fb_FileCopy(NULL, destination) != 0);
	require(fb_FileCopy(directory, destination) != 0);
	require(fb_FileCopy(source, directory) != 0);
	require(mkfifo(fifo, 0600) == 0);
	require(fb_FileCopy(source, fifo) != 0);
	require(fb_FileCopy(fifo, destination) != 0);
	verify(source);
	verify(destination);
	require(unlink(fifo) == 0);
	require(unlink(symlink_path) == 0);
	require(unlink(hardlink) == 0);
	require(unlink(destination) == 0);
	require(unlink(source) == 0);
	require(rmdir(directory) == 0);
	puts("copy-alias-posix: passed");
	return 0;
}

/* end of copy-alias-posix.c */
