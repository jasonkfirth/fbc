''
'' FreeBASIC Windows CE sfxlib link smoke test
'' --------------------------------------------
''
'' File: tests/wince/sfx_link.bas
''
'' Purpose:
''
''     Force the Windows CE WinMM driver and bundled audio decoders into one
''     target executable suitable for emulator validation.
''
'' Responsibilities:
''
''     - initialize the output path with a short generated tone
''     - make the WAV, MP3, and OGG decoder dispatch reachable
''     - stop sound activity before returning to Windows CE
''
'' This file intentionally does NOT contain:
''
''     - audible quality assertions
''     - device-selection policy
''     - emulator startup policy
''

Sound 0, 440, 0.05, 0.25

If Len( Command( 1 ) ) > 0 Then
	Sfx Load 1, Command( 1 )
End If

Sleep 100, 1
Sfx Stop

End 0

'' end of tests/wince/sfx_link.bas
