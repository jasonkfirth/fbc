/'
    Project: FreeBASIC compiler regression tests
    ---------------------------------------------

    File: crt-time-issue356.bas

    Purpose:

        Verify that the Win64 CRT timezone data imports retain the exact
        MinGW symbol names required by the linker.

    Responsibilities:

        • cover the undefined _timezone import reported by GitHub issue #356
        • exercise the CRT daylight and timezone data declarations

    This file intentionally does NOT contain:

        • platform linker implementation
        • runtime timezone conversion logic
 '/

#include once "crt/time.bi"

dim as long daylight_value = _daylight
dim as long timezone_value = _timezone

if daylight_value = -1 then end 1
if timezone_value = -1 then end 1

/' end of crt-time-issue356.bas '/
