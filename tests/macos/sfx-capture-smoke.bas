''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: sfx-capture-smoke.bas
''
'' Purpose:
''
''     Exercise the Darwin CoreAudio capture path when an input device
''     is available and permission has been granted.
''
'' Responsibilities:
''
''     - verify that the active sfxlib driver is CoreAudio
''     - start the generic capture command path
''     - wait for AudioQueue input callbacks to fill the capture ring
''     - read frames back through the generic capture API
''
'' This file intentionally does NOT contain:
''
''     - microphone permission UI handling
''     - assumptions about non-silent input
''     - platform build orchestration
''

#include once "../sfx/sfx_test_common.bi"

const SKIP_NO_COREAUDIO_CAPTURE = 77

dim as integer current = fb_sfxDeviceCurrent()
if( current < 0 ) then
	end SKIP_NO_COREAUDIO_CAPTURE
end if

dim as zstring ptr driver_name = fb_sfxDeviceName( current )
if( driver_name = 0 ) then
	end SKIP_NO_COREAUDIO_CAPTURE
end if

if( lcase( *driver_name ) <> "coreaudio" ) then
	end SKIP_NO_COREAUDIO_CAPTURE
end if

if( fb_sfxCaptureStart() <> 0 ) then
	end SKIP_NO_COREAUDIO_CAPTURE
end if

sleep 350, 1

dim as integer available = fb_sfxCaptureAvailable()
dim as single samples(0 to 511)
dim as integer got = fb_sfxCaptureReadSamples( @samples(0), 256 )
fb_sfxCaptureStop()

if( got < 0 ) then
	end 1
end if

if( available <= 0 and got <= 0 ) then
	end SKIP_NO_COREAUDIO_CAPTURE
end if

end 0

'' end of sfx-capture-smoke.bas
