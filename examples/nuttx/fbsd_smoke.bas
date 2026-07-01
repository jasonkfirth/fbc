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
'' This file intentionally does NOT contain:
''
''     - card formatting logic
''     - broad filesystem stress tests
''     - board initialization code
''

const SD_ROOT = "/mnt/sd0"
const SD_FILE = SD_ROOT + "/fb_sd_smoke.txt"

dim as string line_text

on error goto SdFailure

print "fbsd: checking "; SD_ROOT

open SD_FILE for output as #1
print #1, "FreeBASIC SD smoke"
close #1

open SD_FILE for input as #1
line input #1, line_text
close #1

if line_text <> "FreeBASIC SD smoke" then
    print "fbsd: unexpected readback: "; line_text
    end 21
end if

kill SD_FILE

print "FB_NUTTX_SD_SMOKE_OK"
end 0

SdFailure:
    print "fbsd: failed with ERR ="; err
    end 20

'' end of fbsd_smoke.bas
