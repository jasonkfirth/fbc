'' examples/manual/libraries/bass.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'BASS'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibbass
'' --------

#include Once "bass.bi"

'' this mod file should be available in the fbc package:
Const SOUND_FILE = "..\..\sound\data\dne_trtn.mod"

If (BASS_GetVersion() < MAKELONG(2,2)) Then
	Print "BASS version 2.2 or above required!"
	End 1
End If

If (BASS_Init(-1, 44100, 0, 0, 0) = 0) Then
	Print "Could not initialize BASS"
	End 1
End If

Dim As HMUSIC test = BASS_MusicLoad(False, @SOUND_FILE, 0, 0, BASS_MUSIC_LOOP, 0)
If (test = 0) Then
	Print "BASS could not load '" & SOUND_FILE & "'"
	BASS_Free()
	End 1
End If

BASS_ChannelPlay(test, False)

If( Environ( "FB_EXAMPLE_AUTOPLAY" ) <> "" ) Then
	Sleep 1500, 1
	Print "BASS position seconds:"; BASS_ChannelBytes2Seconds(test, BASS_ChannelGetPosition(test, BASS_POS_BYTE))
Else
	Print "Sound playing; waiting to keypress to stop and exit..."
	Sleep
End If

BASS_ChannelStop(test)
BASS_MusicFree(test)
BASS_Stop()
BASS_Free()
