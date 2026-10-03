/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/newlib_io.c

    Purpose:
        Adapt newlib's file descriptors to native DOS file handles.

    Responsibilities:
        - own a bounded descriptor table compatible with the pinned SDK
        - preserve logical seeks beyond EOF until a write materializes the gap
        - enforce descriptor access modes and append semantics
        - release owned DOS handles at command exit

    This file intentionally does NOT contain:
        - BASIC file-number policy, stdio buffering, or native serial requests

    AmigaDOS Seek rejects positions beyond EOF. C and BASIC permit those
    positions without extending the file, so each descriptor tracks a logical
    cursor. Before disk I/O, synchronize the physical cursor; writes explicitly
    zero any gap. Standard handles are borrowed from the current DOS process.

    The SDK's exported __fh table is used by the native file helpers. Keep its
    BPTR entries separate from descriptor metadata, and preserve those names.
    BASIC serializes file operations with FB_LOCK. C callers must synchronize
    operations on the same descriptor, including close versus an active call.
*/

#include "../fb.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <stabs.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* BASIC has 512 file numbers; the remaining slots serve C-library streams. */
#define AMIGA_FILE_DESCRIPTORS 1024

typedef struct AMIGA_DESCRIPTOR
{
    LONG position;
    int flags;
    int seekable;
} AMIGA_DESCRIPTOR;

static BPTR handles[AMIGA_FILE_DESCRIPTORS];
static AMIGA_DESCRIPTOR descriptors[AMIGA_FILE_DESCRIPTORS];
static BPTR owned_console;
BPTR *__fh = handles;
int __maxfh = AMIGA_FILE_DESCRIPTORS;

/* Force archive selection before the SDK's monolithic descriptor object. */
void fb_hAmigaInitFileDescriptors(void) {}

/* Transfer an already-open native stream into newlib. Packet-backed pipes
   use the normal stdio hooks, while their cursor remains non-seekable. */
int fb_hAmigaRegisterHandle(unsigned long handle, int flags)
{
    int descriptor;
    if (handle == 0) { errno = EBADF; return -1; }
    Forbid();
    for (descriptor = 3; descriptor < AMIGA_FILE_DESCRIPTORS; ++descriptor)
        if (handles[descriptor] == 0) break;
    if (descriptor == AMIGA_FILE_DESCRIPTORS) {
        Permit(); errno = EMFILE; return -1;
    }
    handles[descriptor] = (BPTR)handle;
    descriptors[descriptor].flags = flags;
    descriptors[descriptor].seekable = FALSE;
    descriptors[descriptor].position = 0;
    Permit();
    return descriptor;
}

static BPTR descriptor_handle(int descriptor)
{
    if (descriptor < 0 || descriptor >= AMIGA_FILE_DESCRIPTORS) {
        errno = EBADF;
        return 0;
    }
    if (descriptor < 3) {
        BPTR handle = descriptor == 0 ? Input() : Output();
        if (handle == 0) {
            if (owned_console == 0) owned_console = Open((CONST_STRPTR)"*", MODE_OLDFILE);
            handle = owned_console;
        }
        handles[descriptor] = handle;
        descriptors[descriptor].seekable = FALSE;
    }
    if (handles[descriptor] == 0) errno = EBADF;
    return handles[descriptor];
}

static int file_length(BPTR handle, LONG *length)
{
    struct FileInfoBlock information;
    if (!ExamineFH(handle, &information) || information.fib_Size < 0) {
        errno = EIO;
        return -1;
    }
    *length = information.fib_Size;
    return 0;
}

static int materialize_gap(BPTR handle, LONG length, LONG position)
{
    static const unsigned char zeros[4096];
    if (position <= length) return 0;
    if (Seek(handle, 0, OFFSET_END) == -1) return -1;
    while (length < position) {
        LONG bytes = position - length;
        LONG written;
        if ((ULONG)bytes > sizeof(zeros)) bytes = sizeof(zeros);
        written = Write(handle, (APTR)zeros, bytes);
        if (written <= 0 || written > bytes) { errno = EIO; return -1; }
        length += written;
    }
    return 0;
}

int _open(const char *name, int flags, ...)
{
    BPTR handle;
    LONG length;
    int descriptor, mode;

    if (name == NULL) { errno = EINVAL; return -1; }
    mode = (flags & O_CREAT) ? ((flags & O_TRUNC) ? MODE_NEWFILE : MODE_READWRITE)
        : MODE_OLDFILE;
    handle = Open((CONST_STRPTR)name, mode);
    if (handle == 0) { errno = ENOENT; return -1; }
    if (file_length(handle, &length) != 0) { Close(handle); return -1; }
    if ((flags & O_TRUNC) && SetFileSize(handle, 0, OFFSET_BEGINNING) == -1) {
        Close(handle); errno = EIO; return -1;
    }
    if (flags & O_TRUNC) length = 0;
    Forbid();
    for (descriptor = 3; descriptor < AMIGA_FILE_DESCRIPTORS; ++descriptor)
        if (handles[descriptor] == 0) break;
    if (descriptor == AMIGA_FILE_DESCRIPTORS) {
        Permit(); Close(handle); errno = EMFILE; return -1;
    }
    handles[descriptor] = handle;
    descriptors[descriptor].position = flags & O_APPEND ? length : 0;
    descriptors[descriptor].flags = flags;
    descriptors[descriptor].seekable = TRUE;
    Permit();
    return descriptor;
}

int _close(int descriptor)
{
    BPTR handle;
    /* Validate locally before indexing; descriptor_handle also checks its
       callers, but closing owns the descriptor table mutation. */
    if (descriptor < 0 || descriptor >= AMIGA_FILE_DESCRIPTORS) {
        errno = EBADF;
        return -1;
    }
    handle = descriptor_handle(descriptor);
    if (handle == 0) return -1;
    if (descriptor < 3) return 0;
    Forbid();
    handles[descriptor] = 0;
    memset(&descriptors[descriptor], 0, sizeof(descriptors[descriptor]));
    Permit();
    if (!Close(handle)) { errno = EIO; return -1; }
    return 0;
}

int _read(int descriptor, void *buffer, unsigned int bytes)
{
    BPTR handle = descriptor_handle(descriptor);
    AMIGA_DESCRIPTOR *file;
    LONG length, read_bytes;
    if (handle == 0) return -1;
    if (bytes > LONG_MAX || (bytes != 0 && buffer == NULL)) { errno = EINVAL; return -1; }
    file = &descriptors[descriptor];
    if (descriptor >= 3 && (file->flags & O_ACCMODE) == O_WRONLY) { errno = EBADF; return -1; }
    if (file->seekable) {
        if (file_length(handle, &length) != 0) return -1;
        if (file->position >= length) return 0;
        if (Seek(handle, file->position, OFFSET_BEGINNING) == -1) { errno = EIO; return -1; }
    }
    read_bytes = Read(handle, buffer, (LONG)bytes);
    if (read_bytes > 0 && file->seekable) file->position += read_bytes;
    if (read_bytes < 0) errno = EIO;
    return read_bytes;
}

int _write(int descriptor, const void *buffer, unsigned int bytes)
{
    BPTR handle = descriptor_handle(descriptor);
    AMIGA_DESCRIPTOR *file;
    LONG length, written;
    if (handle == 0) return -1;
    if (bytes > LONG_MAX || (bytes != 0 && buffer == NULL)) { errno = EINVAL; return -1; }
    file = &descriptors[descriptor];
    if (descriptor >= 3 && (file->flags & O_ACCMODE) == O_RDONLY) { errno = EBADF; return -1; }
    if (file->seekable) {
        if (file_length(handle, &length) != 0) return -1;
        if (file->flags & O_APPEND) file->position = length;
        if (bytes > (unsigned int)(LONG_MAX - file->position)) { errno = EOVERFLOW; return -1; }
        if (materialize_gap(handle, length, file->position) != 0 ||
            Seek(handle, file->position, OFFSET_BEGINNING) == -1) { errno = EIO; return -1; }
    }
    written = Write(handle, (APTR)buffer, (LONG)bytes);
    if (written > 0 && file->seekable) file->position += written;
    if (written < 0) errno = EIO;
    return written;
}

long _lseek(int descriptor, long offset, int origin)
{
    BPTR handle = descriptor_handle(descriptor);
    AMIGA_DESCRIPTOR *file;
    LONG length;
    long long position;
    if (handle == 0) return -1;
    file = &descriptors[descriptor];
    if (!file->seekable) { errno = ESPIPE; return -1; }
    if (origin == SEEK_SET) position = offset;
    else if (origin == SEEK_CUR) position = (long long)file->position + offset;
    else if (origin == SEEK_END) {
        if (file_length(handle, &length) != 0) return -1;
        position = (long long)length + offset;
    } else { errno = EINVAL; return -1; }
    if (position < 0 || position > LONG_MAX) { errno = EOVERFLOW; return -1; }
    file->position = (LONG)position;
    return file->position;
}

int _fstat(int descriptor, struct stat *information)
{
    BPTR handle = descriptor_handle(descriptor);
    struct FileInfoBlock native;
    if (handle == 0) return -1;
    if (information == NULL) { errno = EINVAL; return -1; }
    memset(information, 0, sizeof(*information));
    if (descriptors[descriptor].seekable) {
        if (!ExamineFH(handle, &native)) { errno = EIO; return -1; }
        return fb_hAmigaFileStat(&native, information);
    } else information->st_mode = S_IFCHR | S_IRUSR | S_IWUSR;
    return 0;
}

int _unlink(const char *name)
{
    if (name != NULL && DeleteFile((CONST_STRPTR)name)) return 0;
    errno = ENOENT;
    return -1;
}

int _isatty(int descriptor)
{
    BPTR handle = descriptor_handle(descriptor);
    if (handle == 0) return 0;
    if (IsInteractive(handle)) return 1;
    errno = ENOTTY;
    return 0;
}

int _getpid(void) { return (int)(uintptr_t)FindTask(NULL); }
int getpid(void) { return _getpid(); }
int _kill(int process, int signal) { (void)process; (void)signal; errno = ENOSYS; return -1; }
int ioctl(int descriptor, unsigned long request, ...)
{
    (void)descriptor; (void)request;
    errno = ENOTTY;
    return -1;
}

/* newlib calls underscored hooks; the SDK also exports their public aliases.
   A single implementation must satisfy both, so its older syscall objects
   cannot be selected to provide one alias with different cursor semantics. */
__asm__(".globl _open\n.set _open,__open\n"
        ".globl _close\n.set _close,__close\n"
        ".globl _read\n.set _read,__read\n"
        ".globl _write\n.set _write,__write\n"
        ".globl _lseek\n.set _lseek,__lseek\n"
        ".globl _fstat\n.set _fstat,__fstat\n");

__attribute__((used)) static void close_descriptors(void)
{
    int descriptor;
    for (descriptor = 3; descriptor < AMIGA_FILE_DESCRIPTORS; ++descriptor)
        if (handles[descriptor] != 0) _close(descriptor);
    if (owned_console != 0) Close(owned_console);
    owned_console = 0;
}

ADD2EXIT(close_descriptors, -50);

/* end of amiga/newlib_io.c */
