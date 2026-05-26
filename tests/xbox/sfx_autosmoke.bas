''
'' Project: FreeBASIC Xbox validation
'' ----------------------------------
''
'' File: sfx_autosmoke.bas
''
'' Purpose:
''
''     Exercise the Xbox graphics page-flip path while repeatedly entering
''     the sfxlib SOUND path.
''
'' Responsibilities:
''
''     * follow the Behold startup shape: SCREEN 13 first, then SCREENRES
''       with two pages
''     * keep an obvious on-screen frame counter moving
''     * issue low and high pitched SOUND calls after startup
''
'' This file intentionally does NOT contain:
''
''     * controller input tests
''     * Behold gameplay logic
''     * backend-specific recovery code
''

Declare Sub DrawFrame(ByVal frameNumber As Integer, ByVal soundCount As Integer)
Declare Sub PlayTestSound(ByVal soundCount As Integer)

Dim As Integer frameNumber
Dim As Integer soundCount
Dim As Double nextFrame

Screen 13
Screenres 320, 200, 32, 2
Screenset 1, 0

nextFrame = Timer

Do
    frameNumber += 1

#If Defined(FB_AUTOSMOKE_AUDIO_PROOF)
    If frameNumber = 30 Or ((frameNumber > 30) And ((frameNumber Mod 300) = 0)) Then
        soundCount += 1
#ifndef FB_AUTOSMOKE_NO_SOUND
        Sound 0, 440, 5.0, 1.0
#endif
    End If
#Else
    If frameNumber = 90 Or ((frameNumber > 90) And ((frameNumber Mod 120) = 0)) Then
        soundCount += 1
#ifndef FB_AUTOSMOKE_NO_SOUND
        PlayTestSound soundCount
#endif
    End If
#EndIf

    DrawFrame frameNumber, soundCount
    Screencopy

    nextFrame += 1.0 / 30.0
    While Timer < nextFrame
        Sleep 1, 1
    Wend
Loop

Sub DrawFrame(ByVal frameNumber As Integer, ByVal soundCount As Integer)
    Dim As Integer x
    Dim As Integer y
    Dim As UInteger colour

    x = 20 + ((frameNumber * 3) Mod 260)
    y = 130 + ((frameNumber \ 15) Mod 28)
    colour = Rgb((frameNumber * 5) And 255, 200, 80)

    Cls
    Draw String (18, 20), "XBOX SFX AUTOSMOKE", Rgb(255, 255, 255)
    Draw String (18, 44), "Frame: " + Str(frameNumber), Rgb(200, 220, 255)
    Draw String (18, 64), "Sounds: " + Str(soundCount), Rgb(200, 220, 255)
    Draw String (18, 88), "First sound at frame 90.", Rgb(180, 180, 180)
    Draw String (18, 106), "If this freezes after a tick, sfxlib locked.", Rgb(180, 180, 180)

    Line (16, 126)-(304, 174), Rgb(50, 80, 120), B
    Line (x, y)-(x + 18, y + 18), colour, BF
End Sub

Sub PlayTestSound(ByVal soundCount As Integer)
    Select Case (soundCount Mod 4)
    Case 0
        Sound 0, 73, 0.055, 0.45
    Case 1
        Sound 1, 920, 0.05, 0.70
    Case 2
        Sound 2, 180, 0.07, 0.80
    Case Else
        Sound 3, 90, 0.10, 0.55
    End Select
End Sub

'' end of sfx_autosmoke.bas
