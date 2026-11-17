/'
    Project: FreeBASIC console qualification
    File: emulator-exit.bas
    Purpose: Exercise guest file I/O and a controller-selected exit status.
    Responsibilities: Read the requested status and leave a BASIC-written file.
    This file does not launch emulators or generate its own acceptance receipt.
'/
#lang "fb"

dim as integer handle = freefile
dim as integer requested_status

if open("ENDCODE.TXT" for input as #handle) <> 0 then end 17
input #handle, requested_status
close #handle
if requested_status <> 0 andalso requested_status <> 7 then end 18

#ifdef __FB_XBOX__
    const output_name = "E:\BASIO.TXT"
#else
    const output_name = "BASIO.TXT"
#endif

handle = freefile
if open(output_name for output as #handle) <> 0 then end 19
print #handle, "FREEBASIC_CONSOLE_FILE_IO"
close #handle
end requested_status

' end of emulator-exit.bas
