''
'' FreeBASIC Linux sound backend
'' -----------------------------
''
'' File: shutdown-linux-pulse-smoke.bas
''
'' Purpose:
''
''     Verify that a live PulseAudio worker releases its driver lock during
''     explicit sfxlib shutdown.
''
'' Responsibilities:
''
''     - require the PulseAudio driver instead of accepting a fallback
''     - start background audio output
''     - request shutdown while the worker is active
''
'' This file intentionally does NOT contain:
''
''     - subjective audio checks
''     - graphics initialization
''     - a fallback for machines without a PulseAudio server
''

declare function fb_sfxInit cdecl alias "fb_sfxInit" ( ) as long
declare sub fb_sfxExit cdecl alias "fb_sfxExit" ( )
declare function fb_sfxDeviceCurrent cdecl alias "fb_sfxDeviceCurrent" ( ) as long
declare function fb_sfxDeviceName cdecl alias "fb_sfxDeviceName" _
	( byval device_id as long ) as zstring ptr

setenviron "SFXLIB_DRIVER=PulseAudio"

if( fb_sfxInit() <> 0 ) then
	end 1
end if

dim as long current_device = fb_sfxDeviceCurrent()
dim as zstring ptr driver_name = fb_sfxDeviceName( current_device )

if( current_device < 0 or driver_name = 0 ) then
	fb_sfxExit()
	end 2
end if

if( lcase( *driver_name ) <> "pulseaudio" ) then
	fb_sfxExit()
	end 3
end if

sound 0, 440, 0.10, 0.20
sleep 50, 1

fb_sfxExit()
end 0

'' end of shutdown-linux-pulse-smoke.bas
