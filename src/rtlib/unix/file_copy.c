/*
    FreeBASIC rtlib: unix/file_copy.c

    Copy regular files while protecting source/destination aliases. Opened
    descriptors establish identity before truncation, including hard links
    and symbolic links. This file does not implement atomic replacement,
    directory copies, metadata cloning or source snapshot isolation.
*/

#include "../fb.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

FBCALL int fb_FileCopy( const char *source, const char *destination )
{
	int src = -1, dst = -1;
	int result = FB_RTERROR_ILLEGALFUNCTIONCALL;
	struct stat source_stat, destination_stat;
	unsigned char buffer[16384];
	ssize_t count, written, offset;

	if (!source || !*source || !destination || !*destination)
		goto done;
	/* O_NONBLOCK prevents a mistaken FIFO pathname from hanging the caller.
	   Regular files ignore this flag; other file types are rejected below. */
	src = open(source, O_RDONLY | O_NONBLOCK);
	if (src < 0 || fstat(src, &source_stat) != 0 ||
	    !S_ISREG(source_stat.st_mode))
		goto done;
	dst = open(destination, O_WRONLY | O_CREAT | O_NONBLOCK, 0666);
	if (dst < 0 || fstat(dst, &destination_stat) != 0 ||
	    !S_ISREG(destination_stat.st_mode) ||
	    (source_stat.st_dev == destination_stat.st_dev &&
	     source_stat.st_ino == destination_stat.st_ino))
		goto done;
	/* Pathnames may be renamed concurrently; these descriptors keep referring
	   to the objects whose identities were checked. Contents need caller-level
	   synchronization if another process is writing the source or destination. */
	if (ftruncate(dst, 0) != 0)
		goto done;
	for (;;) {
		count = read(src, buffer, sizeof(buffer));
		if (count < 0 && errno == EINTR)
			continue;
		if (count < 0)
			goto done;
		if (count == 0)
			break;
		for (offset = 0; offset < count; offset += written) {
			written = write(dst, buffer + offset, count - offset);
			if (written < 0 && errno == EINTR) {
				written = 0;
				continue;
			}
			if (written <= 0)
				goto done;
		}
	}
	result = FB_RTERROR_OK;
done:
	/* Do not retry close after EINTR: some kernels have already released the
	   descriptor. A delayed output error must still reach the BASIC caller. */
	if (dst >= 0 && close(dst) != 0)
		result = FB_RTERROR_ILLEGALFUNCTIONCALL;
	if (src >= 0 && close(src) != 0)
		result = FB_RTERROR_ILLEGALFUNCTIONCALL;
	return fb_ErrorSetNum(result);
}

/* end of file_copy.c */
