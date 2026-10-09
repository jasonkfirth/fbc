' TEST_MODE : COMPILE_AND_RUN_OK

/'
    Project: FreeBASIC compiler regression tests
    ---------------------------------------------

    File: open-standard-error.bas

    Purpose:

        Verify that OPEN ERR links and opens the process standard-error
        stream with the target C runtime.

    Responsibilities:

        - exercise the standard-error stream binding used by OPEN ERR
        - detect obsolete Windows CRT stream accessor references at link time
        - verify that the opened runtime handle can be closed normally

    This file intentionally does NOT contain:

        - console redirection setup
        - output-format checks
        - target-specific stream workarounds
'/

Dim As Integer ErrorFileNumber = FreeFile

Open Err For Output As #ErrorFileNumber
If Err <> 0 Then End 1

Close #ErrorFileNumber
If Err <> 0 Then End 1

/' end of open-standard-error.bas '/
