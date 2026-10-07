''
'' FreeBASIC sfxlib tests
'' File: raw-header-gosub.bas
'' Purpose: Keep the raw audio header usable by legacy GOSUB callers.
'' Responsibilities: Compile its wide-filename wrapper with OPTION GOSUB.
'' This test contains no playback or file output.
''
' TEST_MODE : COMPILE_AND_RUN

#lang "fblite"
option gosub ' FB-LINTER: DISABLE-LINE FBL-OPT-007 REASON: This fixture checks the header in legacy GOSUB mode.
#include once "sfxlib_raw.bi"

dim as wstring * 2 empty_filename
if sfxlib.OutputCaptureSave(@empty_filename) <> -1 then end 1
print "PASS raw SFX wide filename under OPTION GOSUB"
end 0

'' end of raw-header-gosub.bas
