''
'' FreeBASIC Sound Library (sfxlib)
'' --------------------------------
''
'' File: sfxlib_effects.bi
''
'' Purpose:
''
''     Expose optional whole-mix audio effects provided by sfxlib.
''
'' Responsibilities:
''
''     - link the sfxlib runtime library
''     - configure the stereo ping-pong echo
''     - reset and query global effect state
''
'' This file intentionally does NOT contain:
''
''     - oscillator or envelope declarations
''     - raw sample output declarations
''     - platform audio driver details
''

#pragma once

#inclib "sfx"

namespace sfxlib

	extern "C"

	''
	'' Echo()
	''
	'' Enables a stereo ping-pong echo for the completed mixer output. Wet and
	'' feedback use the range 0.0 to 1.0; feedback is limited to 0.95. Delay is
	'' specified in seconds from 0.01 through 2.0. A wet value of zero disables
	'' the effect. Returns 0 on success and -1 for invalid settings or failure.
	''
	declare function Echo cdecl alias "fb_sfxEchoCmd" _
		( _
			byval wet as single, _
			byval delay_seconds as single, _
			byval feedback as single _
		) as long

	'' Clears the delay line and disables the effect.
	declare sub EchoReset cdecl alias "fb_sfxEchoReset" ( )

	'' Returns nonzero while the effect is enabled.
	declare function EchoEnabled cdecl alias "fb_sfxEchoEnabled" ( ) as long

	end extern

end namespace

'' end of sfxlib_effects.bi
