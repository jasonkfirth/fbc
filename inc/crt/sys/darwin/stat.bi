''
'' FreeBASIC Darwin CRT bindings
'' -----------------------------
''
'' File: crt/sys/darwin/stat.bi
''
'' Purpose:
''
''     Describe the file-status ABI exported by the macOS system library.
''
'' Responsibilities:
''
''     Define Darwin's struct stat layout, file-mode constants, file type
''     tests, and the common file-status function declarations.
''
'' This file intentionally does NOT contain:
''
''     File descriptor creation, directory traversal, or filesystem-specific
''     metadata interfaces.
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_sys_darwin_stat_bi__
#define __crt_sys_darwin_stat_bi__

#include once "crt/time.bi"

'' -------------------------------------------------------------------------
'' File status layout
'' -------------------------------------------------------------------------

''
'' Modern macOS uses the 64-bit inode form of struct stat on both x86-64 and
'' arm64.  The four timespec members are part of Darwin's public extension to
'' the POSIX layout.  Keep the reserved fields because omitting them would
'' make the structure too small for the system calls which populate it.
''
type _stat
	st_dev as dev_t
	st_mode as mode_t
	st_nlink as nlink_t
	st_ino as ino_t
	st_uid as uid_t
	st_gid as gid_t
	st_rdev as dev_t
	st_atimespec as timespec
	st_mtimespec as timespec
	st_ctimespec as timespec
	st_birthtimespec as timespec
	st_size as off_t
	st_blocks as blkcnt_t
	st_blksize as blksize_t
	st_flags as __uint32_t
	st_gen as __uint32_t
	st_lspare as __int32_t
	st_qspare(0 to 1) as __int64_t
end type

type stat as _stat

'' The SDK supplies these source-level aliases for the seconds components.
#define st_atime st_atimespec.tv_sec
#define st_mtime st_mtimespec.tv_sec
#define st_ctime st_ctimespec.tv_sec
#define st_birthtime st_birthtimespec.tv_sec

'' -------------------------------------------------------------------------
'' File types and permission bits
'' -------------------------------------------------------------------------

#define S_IFMT &o170000
#define S_IFIFO &o010000
#define S_IFCHR &o020000
#define S_IFDIR &o040000
#define S_IFBLK &o060000
#define S_IFREG &o100000
#define S_IFLNK &o120000
#define S_IFSOCK &o140000
#define S_IFWHT &o160000

#define S_IRWXU &o000700
#define S_IRUSR &o000400
#define S_IWUSR &o000200
#define S_IXUSR &o000100
#define S_IRWXG &o000070
#define S_IRGRP &o000040
#define S_IWGRP &o000020
#define S_IXGRP &o000010
#define S_IRWXO &o000007
#define S_IROTH &o000004
#define S_IWOTH &o000002
#define S_IXOTH &o000001

#define S_ISUID &o004000
#define S_ISGID &o002000
#define S_ISVTX &o001000
#define S_ISTXT S_ISVTX
#define S_IREAD S_IRUSR
#define S_IWRITE S_IWUSR
#define S_IEXEC S_IXUSR

#define S_ISBLK(m) (((m) and S_IFMT) = S_IFBLK)
#define S_ISCHR(m) (((m) and S_IFMT) = S_IFCHR)
#define S_ISDIR(m) (((m) and S_IFMT) = S_IFDIR)
#define S_ISFIFO(m) (((m) and S_IFMT) = S_IFIFO)
#define S_ISREG(m) (((m) and S_IFMT) = S_IFREG)
#define S_ISLNK(m) (((m) and S_IFMT) = S_IFLNK)
#define S_ISSOCK(m) (((m) and S_IFMT) = S_IFSOCK)
#define S_ISWHT(m) (((m) and S_IFMT) = S_IFWHT)

#define S_TYPEISMQ(buf) 0
#define S_TYPEISSEM(buf) 0
#define S_TYPEISSHM(buf) 0
#define S_TYPEISTMO(buf) 0

#define ACCESSPERMS (S_IRWXU or S_IRWXG or S_IRWXO)
#define ALLPERMS (S_ISUID or S_ISGID or S_ISTXT or ACCESSPERMS)
#define DEFFILEMODE (S_IRUSR or S_IWUSR or S_IRGRP or S_IWGRP or S_IROTH or S_IWOTH)

'' Darwin reports allocation in 512-byte units through st_blocks.
#define S_BLKSIZE 512

'' -------------------------------------------------------------------------
'' Darwin file flags
'' -------------------------------------------------------------------------

#define UF_SETTABLE &h0000ffffUL
#define UF_NODUMP &h00000001UL
#define UF_IMMUTABLE &h00000002UL
#define UF_APPEND &h00000004UL
#define UF_OPAQUE &h00000008UL
#define UF_COMPRESSED &h00000020UL
#define UF_TRACKED &h00000040UL
#define UF_DATAVAULT &h00000080UL
#define UF_HIDDEN &h00008000UL

#define SF_SUPPORTED &h009f0000UL
#define SF_SETTABLE &h3fff0000UL
#define SF_SYNTHETIC &hc0000000UL
#define SF_ARCHIVED &h00010000UL
#define SF_IMMUTABLE &h00020000UL
#define SF_APPEND &h00040000UL
#define SF_RESTRICTED &h00080000UL
#define SF_NOUNLINK &h00100000UL
#define SF_FIRMLINK &h00800000UL
#define SF_DATALESS &h40000000UL

'' -------------------------------------------------------------------------
'' File status operations
'' -------------------------------------------------------------------------

extern "C"

''
'' Intel macOS retains both historical and 64-bit-inode entry points.  The
'' SDK redirects modern x86-64 callers to the $INODE64 symbols.  Apple silicon
'' has only the modern ABI, so its exported names have no suffix.
''
#if defined(__FB_64BIT__) and not defined(__FB_ARM__)
declare function fstat alias "fstat$INODE64" (byval __fd as long, byval __buffer as _stat ptr) as long
declare function lstat alias "lstat$INODE64" (byval __path as const zstring ptr, byval __buffer as _stat ptr) as long
declare function stat alias "stat$INODE64" (byval __path as const zstring ptr, byval __buffer as _stat ptr) as long
#else
declare function fstat (byval __fd as long, byval __buffer as _stat ptr) as long
declare function lstat (byval __path as const zstring ptr, byval __buffer as _stat ptr) as long
declare function stat (byval __path as const zstring ptr, byval __buffer as _stat ptr) as long
#endif

declare function chmod (byval __path as const zstring ptr, byval __mode as mode_t) as long
declare function fchmod (byval __fd as long, byval __mode as mode_t) as long

'' mkdir is a FreeBASIC keyword, so expose the C routine through mkdir_.
declare function mkdir_ alias "mkdir" (byval __path as const zstring ptr, byval __mode as mode_t) as long
declare function mkfifo (byval __path as const zstring ptr, byval __mode as mode_t) as long
declare function mknod (byval __path as const zstring ptr, byval __mode as mode_t, byval __device as dev_t) as long
declare function umask (byval __mask as mode_t) as mode_t

declare function chflags (byval __path as const zstring ptr, byval __flags as __uint32_t) as long
declare function fchflags (byval __fd as long, byval __flags as __uint32_t) as long
declare function lchflags (byval __path as const zstring ptr, byval __flags as __uint32_t) as long
declare function lchmod (byval __path as const zstring ptr, byval __mode as mode_t) as long

end extern

#endif

'' end of crt/sys/darwin/stat.bi
