'' FreeBASIC classic AmigaOS native I/O qualification
'' -------------------------------------------------
'' File: native-io.bas
'' Purpose: Check real command streaming and native filesystem boundaries.
'' Responsibilities: Exercise binary pipes beyond their ring capacity and EOF.
'' This file intentionally does NOT contain shell or packet implementation.

#include once "fbc-int/file-info.bi"
#include once "file.bi"

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

'' END also closes streams which the BASIC program leaves open.
if open pipe("C:fb-cat <pipe-input.bin" for input as #1) <> 0 then end 10
print "Native Amiga I/O qualification passed"
end 0

'' end of native-io.bas
