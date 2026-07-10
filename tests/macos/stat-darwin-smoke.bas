''
'' FreeBASIC macOS CRT tests
'' ------------------------
''
'' File: stat-darwin-smoke.bas
''
'' Purpose:
''
''     Verify Darwin's file-status ABI and the stat-family functions exported
''     by the macOS system library.
''
'' Responsibilities:
''
''     - check SDK-derived type sizes, structure offsets, and mode constants
''     - inspect a temporary regular file by descriptor and by path
''     - verify permissions, symbolic-link status, and directory status
''     - remove every temporary object before returning
''
'' This file intentionally does NOT contain:
''
''     - filesystem-specific flag mutation
''     - tests whose results depend on the user's current working directory
''     - privileged file operations
''

#include once "crt/sys/stat.bi"
#include once "crt/stdlib.bi"
#include once "crt/unistd.bi"

const SMOKE_OK = 0
const SMOKE_CREATE_FAILED = 1
const SMOKE_WRITE_FAILED = 2
const SMOKE_FSTAT_FAILED = 3
const SMOKE_STAT_FAILED = 4
const SMOKE_CHMOD_FAILED = 5
const SMOKE_SYMLINK_FAILED = 6
const SMOKE_LSTAT_FAILED = 7
const SMOKE_MKDIR_FAILED = 8
const SMOKE_DIRECTORY_STAT_FAILED = 9
const SMOKE_CLEANUP_FAILED = 10

type StatAlignmentProbe
	marker as ubyte
	value as _stat
end type

'' These values come from the public macOS SDK definitions in sys/stat.h.
#assert sizeof(dev_t) = 4
#assert sizeof(mode_t) = 2
#assert sizeof(nlink_t) = 2
#assert sizeof(ino_t) = 8
#assert sizeof(off_t) = 8
#assert sizeof(blkcnt_t) = 8
#assert sizeof(blksize_t) = 4
#assert sizeof(_stat) = 144
#assert offsetof(StatAlignmentProbe, value) = 8
#assert offsetof(_stat, st_dev) = 0
#assert offsetof(_stat, st_mode) = 4
#assert offsetof(_stat, st_nlink) = 6
#assert offsetof(_stat, st_ino) = 8
#assert offsetof(_stat, st_uid) = 16
#assert offsetof(_stat, st_gid) = 20
#assert offsetof(_stat, st_rdev) = 24
#assert offsetof(_stat, st_atimespec) = 32
#assert offsetof(_stat, st_mtimespec) = 48
#assert offsetof(_stat, st_ctimespec) = 64
#assert offsetof(_stat, st_birthtimespec) = 80
#assert offsetof(_stat, st_size) = 96
#assert offsetof(_stat, st_blocks) = 104
#assert offsetof(_stat, st_blksize) = 112
#assert offsetof(_stat, st_flags) = 116
#assert offsetof(_stat, st_gen) = 120
#assert offsetof(_stat, st_lspare) = 124
#assert offsetof(_stat, st_qspare(0)) = 128

#assert S_IFMT = &o170000
#assert S_IFREG = &o100000
#assert S_IFLNK = &o120000
#assert S_IFDIR = &o040000
#assert ACCESSPERMS = &o0777
#assert ALLPERMS = &o7777
#assert DEFFILEMODE = &o0666
#assert S_BLKSIZE = 512

dim result as long = SMOKE_OK
dim file_descriptor as long = -1
dim file_exists as long = 0
dim link_exists as long = 0
dim directory_exists as long = 0
dim file_path as zstring * 64 = "/tmp/fbc-stat-smoke.XXXXXX"
dim link_path as string
dim directory_path as string
dim file_status as _stat
dim link_status as _stat
dim directory_status as _stat
dim payload as zstring * 20 = "FreeBASIC stat test"

file_descriptor = mkstemp(@file_path)
if( file_descriptor < 0 ) then
	result = SMOKE_CREATE_FAILED
else
	file_exists = -1
	link_path = file_path + ".link"
	directory_path = file_path + ".dir"

	do
		if( write_(file_descriptor, @payload, len(payload)) <> len(payload) ) then
			result = SMOKE_WRITE_FAILED
			exit do
		end if

		if( fstat(file_descriptor, @file_status) <> 0 ) then
			result = SMOKE_FSTAT_FAILED
			exit do
		end if

		if( not S_ISREG(file_status.st_mode) or file_status.st_size <> len(payload) ) then
			result = SMOKE_FSTAT_FAILED
			exit do
		end if

		if( chmod(@file_path, cast(mode_t, &o0640)) <> 0 ) then
			result = SMOKE_CHMOD_FAILED
			exit do
		end if

		if( stat(@file_path, @file_status) <> 0 ) then
			result = SMOKE_STAT_FAILED
			exit do
		end if

		if( (file_status.st_mode and ACCESSPERMS) <> &o0640 ) then
			result = SMOKE_CHMOD_FAILED
			exit do
		end if

		if( symlink(@file_path, strptr(link_path)) <> 0 ) then
			result = SMOKE_SYMLINK_FAILED
			exit do
		end if
		link_exists = -1

		if( lstat(strptr(link_path), @link_status) <> 0 ) then
			result = SMOKE_LSTAT_FAILED
			exit do
		end if

		if( not S_ISLNK(link_status.st_mode) ) then
			result = SMOKE_LSTAT_FAILED
			exit do
		end if

		if( mkdir_(strptr(directory_path), cast(mode_t, &o0700)) <> 0 ) then
			result = SMOKE_MKDIR_FAILED
			exit do
		end if
		directory_exists = -1

		if( stat(strptr(directory_path), @directory_status) <> 0 ) then
			result = SMOKE_DIRECTORY_STAT_FAILED
			exit do
		end if

		if( not S_ISDIR(directory_status.st_mode) ) then
			result = SMOKE_DIRECTORY_STAT_FAILED
			exit do
		end if

		exit do
	loop
end if

'' Close the descriptor before removing the file.  Cleanup failures are only
'' reported when an earlier, more useful failure has not already been found.
if( file_descriptor >= 0 ) then
	if( close_(file_descriptor) <> 0 and result = SMOKE_OK ) then
		result = SMOKE_CLEANUP_FAILED
	end if
end if

if( link_exists ) then
	if( unlink(strptr(link_path)) <> 0 and result = SMOKE_OK ) then
		result = SMOKE_CLEANUP_FAILED
	end if
end if

if( directory_exists ) then
	if( rmdir_(strptr(directory_path)) <> 0 and result = SMOKE_OK ) then
		result = SMOKE_CLEANUP_FAILED
	end if
end if

if( file_exists ) then
	if( unlink(@file_path) <> 0 and result = SMOKE_OK ) then
		result = SMOKE_CLEANUP_FAILED
	end if
end if

end result

'' end of stat-darwin-smoke.bas
