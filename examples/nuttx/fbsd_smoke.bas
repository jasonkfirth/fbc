''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbsd_smoke.bas
''
'' Purpose:
''
''     Confirm that a NuttX board image has mounted the RP2350-PiZero
''     MicroSD card slot where FreeBASIC file I/O can reach it.
''
'' Responsibilities:
''
''     - write a short text file to /mnt/sd0
''     - read it back through ordinary BASIC file I/O
''     - remove the test file before returning to NSH
''
'' Ownership:
''
''     sd_file_handle owns at most one successful Open at a time.  The fixed
''     SD-card fixture is removed only if DIR confirms that it exists.
''
'' This file intentionally does NOT contain:
''
''     - card formatting logic
''     - broad filesystem stress tests
''     - board initialization code
''

const SD_ROOT = "/mnt/sd0"
const SD_FILE = SD_ROOT + "/fb_sd_smoke.txt"
const SD_TEXT = "FreeBASIC SD smoke"

sub CleanupSdFile()

    if dir(SD_FILE) <> "" then
        kill SD_FILE
    end if

end sub

dim as string line_text
dim as integer sd_file_handle = freefile

if sd_file_handle <= 0 then
    print "fbsd: FreeFile failed"
    end 20
end if

print "fbsd: checking "; SD_ROOT

'' Fixed NuttX SD fixture; the Open result is checked immediately below. FB-LINTER: DISABLE-NEXT-LINE FBL-IO-005
open SD_FILE for output as #sd_file_handle
if err <> 0 then
    print "fbsd: source open failed with ERR ="; err
    end 20
end if

print #sd_file_handle, SD_TEXT
close #sd_file_handle

open SD_FILE for input as #sd_file_handle
if err <> 0 then
    CleanupSdFile()
    print "fbsd: read open failed with ERR ="; err
    end 20
end if

line input #sd_file_handle, line_text
close #sd_file_handle

if line_text <> SD_TEXT then
    CleanupSdFile()
    print "fbsd: unexpected readback: "; line_text
    end 21
end if

CleanupSdFile()

print "FB_NUTTX_SD_SMOKE_OK"
end 0

'' end of fbsd_smoke.bas
