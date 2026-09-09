/*
    FreeBASIC runtime tests: copy-alias-posix.c
    Verify descriptor identity, complete copies and non-regular-file rejection
    using an exclusively created fixture. No pre-existing files are changed.
*/

#include "../../src/rtlib/fb.h"
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

FBCALL int fb_ErrorSetNum(int error) { return error; }

static unsigned char contents[40001];

static void verify(const char *path)
{
	unsigned char actual[sizeof(contents)];
	FILE *file = fopen(path, "rb");
	assert(file != NULL);
	assert(fread(actual, 1, sizeof(actual), file) == sizeof(actual));
	assert(fgetc(file) == EOF && !ferror(file));
	assert(fclose(file) == 0);
	assert(memcmp(actual, contents, sizeof(actual)) == 0);
}

int main(void)
{
	char directory[] = "/tmp/fb-copy-alias-XXXXXX";
	char source[128], destination[128], hardlink[128], symlink_path[128], fifo[128];
	struct stat info;
	FILE *file;
	size_t i;

	assert(mkdtemp(directory));
	assert(snprintf(source, sizeof(source), "%s/source", directory) < (int)sizeof(source));
	assert(snprintf(destination, sizeof(destination), "%s/destination", directory) < (int)sizeof(destination));
	assert(snprintf(hardlink, sizeof(hardlink), "%s/hardlink", directory) < (int)sizeof(hardlink));
	assert(snprintf(symlink_path, sizeof(symlink_path), "%s/symlink", directory) < (int)sizeof(symlink_path));
	assert(snprintf(fifo, sizeof(fifo), "%s/fifo", directory) < (int)sizeof(fifo));
	for (i = 0; i < sizeof(contents); i++) contents[i] = (unsigned char)(i * 37);
	file = fopen(source, "wb");
	assert(file);
	assert(fwrite(contents, 1, sizeof(contents), file) == sizeof(contents));
	assert(fclose(file) == 0);
	assert(link(source, hardlink) == 0);
	assert(symlink(source, symlink_path) == 0);
	assert(lstat(symlink_path, &info) == 0 && S_ISLNK(info.st_mode));
	assert(fb_FileCopy(source, source) != 0);
	verify(source);
	assert(fb_FileCopy(source, hardlink) != 0);
	verify(source);
	assert(fb_FileCopy(source, symlink_path) != 0);
	verify(source);
	assert(fb_FileCopy(symlink_path, source) != 0);
	verify(source);
	assert(fb_FileCopy(source, destination) == 0);
	verify(destination);
	file = fopen(destination, "ab");
	assert(file && fputs("extra tail", file) >= 0 && fclose(file) == 0);
	assert(fb_FileCopy(source, destination) == 0);
	verify(destination);
	assert(fb_FileCopy(NULL, destination) != 0);
	assert(fb_FileCopy(directory, destination) != 0);
	assert(fb_FileCopy(source, directory) != 0);
	assert(mkfifo(fifo, 0600) == 0);
	assert(fb_FileCopy(source, fifo) != 0);
	assert(fb_FileCopy(fifo, destination) != 0);
	verify(source);
	verify(destination);
	assert(unlink(fifo) == 0);
	assert(unlink(symlink_path) == 0);
	assert(unlink(hardlink) == 0);
	assert(unlink(destination) == 0);
	assert(unlink(source) == 0);
	assert(rmdir(directory) == 0);
	puts("copy-alias-posix: passed");
	return 0;
}

/* end of copy-alias-posix.c */
