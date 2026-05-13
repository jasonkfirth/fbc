''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: sfx-dummy-smoke.bas
''
'' Purpose:
''
''     Exercise the shared sfxlib mixer and driver handoff path when
''     the host does not expose a usable CoreAudio output device.
''
'' Responsibilities:
''
''     - force the null output driver before sfxlib initializes
''     - verify that the requested dummy driver is active
''     - render a short generated tone through the normal sound path
''     - leave sample-level validation to the smoke runner's dump check
''
'' This file intentionally does NOT contain:
''
''     - CoreAudio device probing
''     - capture tests
''     - platform build orchestration
''

#include once "../sfx/sfx_test_common.bi"

SfxTestUseNullDriver()

device list

dim as integer current = fb_sfxDeviceCurrent()
if( current < 0 ) then
	end 2
end if

dim as zstring ptr driver_name = fb_sfxDeviceName( current )
if( driver_name = 0 ) then
	end 3
end if

if( lcase( *driver_name ) <> "null" ) then
	end 4
end if

sound 0, 440, 0.10, 0.60
fb_sfxUpdate 5000

sleep 20, 1
end 0

'' end of sfx-dummy-smoke.bas
