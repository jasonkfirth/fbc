'' FreeBASIC classic AmigaOS native I/O qualification
'' -------------------------------------------------
'' File: native-io.bas
'' Purpose: Check real command streaming and native filesystem boundaries.
'' Responsibilities: Exercise binary pipes, EOF and relative directory traversal.
'' This file intentionally does NOT contain shell or packet implementation.

#include once "fbc-int/file-info.bi"
#include once "file.bi"
#include once "crt/stdio.bi"
#include once "vbcompat.bi"

const data_size = 200000
dim payload as string = string(data_size, 0)
for i as integer = 0 to data_size - 1
	payload[i] = i mod 251
next

if open("pipe-input.bin" for binary as #1) <> 0 then end 1
put #1, , payload
close #1
print "Source fixture written"

dim information as FB_FILE_INFO, stream_information as FB_FILE_INFO
if fb_FileQueryInfo("pipe-input.bin", 1, @information) = 0 then end 11
if information.bytes <> data_size then end 12
if (information.flags and FB_FILE_INFO_REGULAR) = 0 then end 13
if open("pipe-input.bin" for binary access read as #1) <> 0 then end 14
if fb_FileQueryStreamInfo(cptr(any ptr, fileattr(1, 2)), @stream_information) = 0 then end 15
close #1
if stream_information.bytes <> information.bytes then end 16
if stream_information.modified <> information.modified then end 17
if fb_FileQueryInfo("missing-file.bin", 0, @information) = 0 then end 18
if information.flags <> 0 then end 19

if mkdir("path-one") <> 0 then end 20
if mkdir("path-one/inner") <> 0 then end 21
dim marker as ubyte = 81
if open("path-one/inner/marker.bin" for binary as #1) <> 0 then end 22
put #1, , marker
close #1
marker = 0
if open("path-one/inner/../inner/./marker.bin" for binary access read as #1) <> 0 then end 23
get #1, , marker
close #1
if marker <> 81 then end 24
print "Native metadata and relative paths verified"

'' The shell redirects a real native command's input. Its output crosses the
'' 32 KiB handler ring several times, preserving embedded zero bytes.
if open pipe("C:fb-cat <pipe-input.bin" for binary access read as #1) <> 0 then end 2
print "Input stream opened"
dim result as string = string(data_size, 0)
get #1, , result
if result <> payload then end 3
print "Input stream read"
if not eof(1) then end 4
close #1
print "Input stream closed"

if open pipe("C:fb-cat >pipe-output.bin" for binary access write as #1) <> 0 then end 5
put #1, , payload
close #1
print "Output stream closed"
if open("pipe-output.bin" for binary access read as #1) <> 0 then end 6
get #1, , result
close #1
if result <> payload then end 7
print "Output data verified"

'' Closing an unread input must wake a producer blocked by a full ring.
if open pipe("C:fb-cat <pipe-input.bin" for input as #1) <> 0 then end 8
close #1
print "Unread stream closed"
if chdir("missing-volume:missing-directory") = 0 then end 9

'' The C library accepts native DOS paths. CHDIR must apply the same dot
'' and backslash conversion as file operations before calling that library.
dim original_directory as string = curdir
if mkdir("chdir-parent") <> 0 then end 30
if mkdir("chdir-parent/child") <> 0 then end 31
if open("chdir-parent/marker.txt" for output as #1) <> 0 then end 32
print #1, "parent directory"
close #1
if chdir("chdir-parent/child") <> 0 then end 33
if chdir("..") <> 0 then end 34
if not fileexists("marker.txt") then end 35
if chdir(".\child") <> 0 then end 36
if chdir("..\") <> 0 then end 37
if not fileexists("marker.txt") then end 38
if chdir("child/../child/.") <> 0 then end 39
if chdir("../") <> 0 then end 40
if not fileexists("marker.txt") then end 41
if chdir("..") <> 0 then end 42
if curdir <> original_directory then end 43
'' These representations of an empty string must neither crash nor move us.
dim empty_directory as string
dim empty_result as integer = chdir(empty_directory)
if curdir <> original_directory then end 44
empty_result = chdir("")
if curdir <> original_directory then end 45
kill "chdir-parent/marker.txt"
if rmdir("chdir-parent/child") <> 0 then end 46
if rmdir("chdir-parent") <> 0 then end 47
print "Native parent-directory paths passed"

'' libamiga's legacy sprintf reads DOS-sized words for %d. C callers must
'' instead retain newlib's 32-bit promoted-argument and return-value contract.
dim formatted_clock as zstring * 64
dim formatted_count as long = sprintf(@formatted_clock, "%02d-%02d-%04d", _
	clng(10), clng(16), clng(2026))
if formatted_clock <> "10-16-2026" or formatted_count <> 10 then end 48
formatted_count = sprintf(@formatted_clock, "%02d:%02d:%02d", _
	clng(13), clng(46), clng(27))
if formatted_clock <> "13:46:27" or formatted_count <> 8 then end 49
if isdate(date$) = 0 then end 50
print "Native C date/time formatting passed"

'' PC port access reports FB_RTERROR_ILLEGALFUNCTIONCALL (1) on Amiga.
'' INP returns a negative error; OUT returns the positive error number.
const ILLEGAL_FUNCTION_CALL = 1
dim input_status as long = inp(&h3da)
if input_status <> -ILLEGAL_FUNCTION_CALL or err <> ILLEGAL_FUNCTION_CALL then end 11
dim output_status as long = out(&h3da, 0)
if output_status <> ILLEGAL_FUNCTION_CALL or err <> ILLEGAL_FUNCTION_CALL then end 12
err = 0

'' END also closes streams which the BASIC program leaves open.
if open pipe("C:fb-cat <pipe-input.bin" for input as #1) <> 0 then end 10
print "Native Amiga I/O qualification passed"
end 0

'' end of native-io.bas
