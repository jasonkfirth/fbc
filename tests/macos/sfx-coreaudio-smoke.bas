''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: sfx-coreaudio-smoke.bas
''
'' Purpose:
''
''     Exercise the Darwin CoreAudio sfxlib playback driver.
''
'' Responsibilities:
''
''     - verify that the active sfxlib driver is CoreAudio
''     - enumerate CoreAudio output endpoints
''     - select the first output endpoint when one is reported
''     - play a short generated tone through the driver write path
''
'' This file intentionally does NOT contain:
''
''     - mixer signal analysis
''     - null-driver fallback tests
''     - platform build orchestration
''

#include once "../sfx/sfx_test_common.bi"

declare function fb_sfxControlDeviceList cdecl alias "fb_sfxControlDeviceList" ( ) as integer
declare function fb_sfxControlDeviceSelect cdecl alias "fb_sfxControlDeviceSelect" ( byval device as integer ) as integer

const SKIP_NO_COREAUDIO = 77

dim as integer current = fb_sfxDeviceCurrent()
if( current < 0 ) then
	end SKIP_NO_COREAUDIO
end if

dim as zstring ptr driver_name = fb_sfxDeviceName( current )
if( driver_name = 0 ) then
	end SKIP_NO_COREAUDIO
end if

if( lcase( *driver_name ) <> "coreaudio" ) then
	end SKIP_NO_COREAUDIO
end if

dim as integer outputs = fb_sfxControlDeviceList()
if( outputs > 0 ) then
	if( fb_sfxControlDeviceSelect( 0 ) <> 0 ) then
		end 1
	end if
end if

sound 440, 4
fb_sfxUpdate 8192
sleep 120, 1

end 0

'' end of sfx-coreaudio-smoke.bas
