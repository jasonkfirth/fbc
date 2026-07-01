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
'' This file intentionally does NOT contain:
''
''     - filesystem formatting logic
''     - broad storage stress tests
''     - board initialization code
''

declare function FileCopy alias "fb_FileCopy" _
    (byval source as zstring ptr, byval destination as zstring ptr) as long

const COPY_SOURCE = "fb_filecopy_source.txt"
const COPY_DEST = "fb_filecopy_dest.txt"
const COPY_TEXT = "FreeBASIC generic FileCopy smoke"

dim as string line_text

on error goto CopyFailure

open COPY_SOURCE for output as #1
print #1, COPY_TEXT
close #1

FileCopy COPY_SOURCE, COPY_DEST

open COPY_DEST for input as #1
line input #1, line_text
close #1

if line_text <> COPY_TEXT then
    print "fbfilecopy: unexpected readback: "; line_text
    end 31
end if

kill COPY_DEST
kill COPY_SOURCE

print "FB_NUTTX_FILECOPY_SMOKE_OK"
end 0

CopyFailure:
    close
    print "fbfilecopy: failed with ERR ="; err
    end 30

'' end of fbfilecopy_smoke.bas
