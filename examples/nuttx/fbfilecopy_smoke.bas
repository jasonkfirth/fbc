''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbfilecopy_smoke.bas
''
'' Purpose:
''
''     Confirm that FILECOPY uses the shared C stdio rtlib helper on the
''     NuttX generated-C smoke target.
''
'' Responsibilities:
''
''     - create a source text file through ordinary BASIC file I/O
''     - copy it with FILECOPY
''     - read the destination back and verify the copied content
''     - remove temporary files before returning to NSH
''
'' Ownership:
''
''     copy_file_handle owns at most one successful Open at a time.  The two
''     fixed RAM fixtures are removed only if DIR confirms that they exist.
''
'' This file intentionally does NOT contain:
''
''     - filesystem formatting logic
''     - broad storage stress tests
''     - board initialization code
''

declare function FileCopy alias "fb_FileCopy" _
    (byval source as zstring ptr, byval destination as zstring ptr) as long

const COPY_SOURCE = "/ram/fb_filecopy_source.txt"
const COPY_DEST = "/ram/fb_filecopy_dest.txt"
const COPY_TEXT = "FreeBASIC generic FileCopy smoke"

sub CleanupCopyFiles()

    if dir(COPY_DEST) <> "" then
        kill COPY_DEST
    end if

    if dir(COPY_SOURCE) <> "" then
        kill COPY_SOURCE
    end if

end sub

dim as string line_text
dim as integer copy_file_handle = freefile
dim as long copy_result

if copy_file_handle <= 0 then
    print "fbfilecopy: FreeFile failed"
    end 30
end if

'' Fixed NuttX RAM fixture; the Open result is checked immediately below. FB-LINTER: DISABLE-NEXT-LINE FBL-IO-005
open COPY_SOURCE for output as #copy_file_handle
if err <> 0 then
    print "fbfilecopy: source open failed with ERR ="; err
    end 30
end if

print #copy_file_handle, COPY_TEXT
close #copy_file_handle

copy_result = FileCopy(COPY_SOURCE, COPY_DEST)

if copy_result <> 0 then
    CleanupCopyFiles()
    print "fbfilecopy: FileCopy failed ="; copy_result
    end 30
end if

open COPY_DEST for input as #copy_file_handle
if err <> 0 then
    CleanupCopyFiles()
    print "fbfilecopy: destination open failed with ERR ="; err
    end 30
end if

line input #copy_file_handle, line_text
close #copy_file_handle

if line_text <> COPY_TEXT then
    CleanupCopyFiles()
    print "fbfilecopy: unexpected readback: "; line_text
    end 31
end if

CleanupCopyFiles()

print "FB_NUTTX_FILECOPY_SMOKE_OK"
end 0

'' end of fbfilecopy_smoke.bas
