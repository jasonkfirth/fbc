/'
    Project: FreeBASIC DOS graphics qualification
    File: cursor_pixels.bas
    Purpose: Check real IRQ0 cursor publication against a captured framebuffer.
    Responsibilities: Verify stationary, moving, hidden, clipped and palette
        states in 8/16/32-bit modes, including bank-span driver semantics.
    This file does not draw a substitute cursor or access user documents.
    Capture helpers borrow the driver under ScreenLock; each mode restores it
    and frees its buffer before Screen 0. The ISR owns update counters.
'/
#lang "fb"
#include once "fbgfx.bi"
Extern "C"
    Declare Function captureInstall Alias "cursor_test_install"() As Long
    Declare Sub captureRestore Alias "cursor_test_restore"()
    Declare Function captureUpdates Alias "cursor_test_updates"() As ULong
    Declare Sub captureBanked Alias "cursor_test_banked"(ByVal enabled As Long)
    Declare Function capturePixel Alias "cursor_test_pixel"(ByVal x As Long, ByVal y As Long, ByVal presented As Long) As ULong
    Declare Function captureHash Alias "cursor_test_hash"(ByVal x As Long, ByVal y As Long) As ULong
    Declare Sub cursorWait Alias "fb_GfxDosIdle"(ByVal milliseconds As Long)
End Extern
Dim Shared As Integer checks, failures
Dim Shared As String report
Private Sub check(ByVal condition As Integer, ByVal labelText As String)
    checks += 1
    If condition = 0 Then
        failures += 1
        report &= "FAIL;" & labelText & Chr(10)
    End If
End Sub
Private Sub observe(ByVal depth As Integer, ByVal stage As Integer, ByVal x As Integer, ByVal y As Integer)
    report &= "CURSOR_CAPTURE_STAGE;" & Str(depth) & ";" & Str(stage) & ";" & Str(captureHash(x, y)) & Chr(10)
End Sub

For modeIndex As Integer = 0 To 2
    Dim As Integer depth = IIf(modeIndex = 0, 8, IIf(modeIndex = 1, 16, 32))
    If ScreenRes(640, 480, depth, 1) <> 0 Then End 2
    ScreenLock
    Dim As Integer captureReady = captureInstall()
    ScreenUnlock 1, 0
    If captureReady = 0 Then
        Screen 0
        End 3
    End If
    ScreenLock
    Dim As ULong backgroundColor = IIf(depth = 8, 3, RGB(20, 40, 60))
    Line (0, 0)-(639, 479), backgroundColor, BF
    Dim As ULong backgroundPixel = capturePixel(60, 60, 0)
    ScreenUnlock
    SetMouse 60, 60, 1
    cursorWait 100
    ScreenLock
    check(capturePixel(60, 60, 1) <> backgroundPixel, "initial cursor is published")
    check(capturePixel(60, 60, 0) = backgroundPixel, "framebuffer background is restored")
    observe depth, 0, 50, 50
    Dim As ULong previousUpdates = captureUpdates()
    Dim As ULong previousHash = captureHash(50, 50)
    ScreenUnlock 1, 0
    cursorWait 100
    ScreenLock
    report &= "CURSOR_STATIONARY_UPDATES;" & Str(depth) & ";" & Str(captureUpdates() - previousUpdates) & Chr(10)
#Ifdef CURSOR_EXPECT_IDLE_STABLE
    check(captureUpdates() = previousUpdates, "stationary cursor produces no display updates")
#EndIf
    check(captureHash(50, 50) = previousHash, "stationary cursor remains visible")
    ScreenUnlock 1, 0

    SetMouse 100, 100, 1
    cursorWait 80
    ScreenLock
    check(capturePixel(60, 60, 1) = backgroundPixel, "motion erases the old cursor")
    check(capturePixel(100, 100, 1) <> backgroundPixel, "motion draws the new cursor")
    observe depth, 1, 90, 90
    Dim As fb.EVENT eventValue
    Dim As Integer motionSeen
    While ScreenEvent(@eventValue)
        If eventValue.type = fb.EVENT_MOUSE_MOVE AndAlso eventValue.x = 100 AndAlso eventValue.y = 100 Then motionSeen = -1
    Wend
    check(motionSeen, "motion events continue through the presentation path")
    captureBanked 1
    Line (0, 20)-(5, 20), 1
    Line (0, 420)-(5, 420), 1
    ScreenUnlock 1, 0
    cursorWait 80
    ScreenLock
    check(capturePixel(100, 100, 1) <> backgroundPixel, "bank-span refresh preserves the cursor")
    observe depth, 2, 90, 90
    Line (100, 100)-(112, 121), 2, BF
    Dim As ULong changedBackground = capturePixel(100, 100, 0)
    ScreenUnlock 1, 0
    cursorWait 80
    ScreenLock
    check(capturePixel(100, 100, 0) = changedBackground, "drawing beneath the cursor survives restoration")
    observe depth, 3, 90, 90
    ScreenUnlock 1, 0
    SetMouse , , 0
    cursorWait 80
    ScreenLock
    check(capturePixel(100, 100, 1) = changedBackground, "hiding publishes the edited background")
    observe depth, 4, 90, 90
    ScreenUnlock 1, 0
    SetMouse , , 1
    cursorWait 80
    ScreenLock
    ' The new background quantizes to black in RGB565. Inspect the white
    ' interior so an identical black outline is not mistaken for a hidden cursor.
    check(capturePixel(102, 103, 1) <> changedBackground, "showing redraws without other damage")
    observe depth, 5, 90, 90
    If depth = 8 Then
        Dim As ULong oldWhite = capturePixel(102, 103, 1)
        Palette 15, RGB(255, 0, 0)
        ScreenUnlock 1, 0
        cursorWait 80
        ScreenLock
        check(capturePixel(102, 103, 1) <> oldWhite, "palette updates refresh the cursor color")
        observe depth, 6, 90, 90
    End If
    ScreenUnlock 1, 0
    SetMouse 639, 479, 1
    cursorWait 80
    ScreenLock
    check(capturePixel(100, 100, 1) = changedBackground, "clipped motion erases its old location")
    check(capturePixel(639, 479, 1) <> backgroundPixel, "cursor clips at the final pixel")
    observe depth, 7, 630, 470
    ScreenUnlock 1, 0
    SetMouse , , 0
    cursorWait 80
    ScreenLock
    check(capturePixel(639, 479, 1) = backgroundPixel, "clipped cursor hiding restores the final pixel")
    observe depth, 8, 630, 470
    captureRestore()
    ScreenUnlock
    Screen 0
Next modeIndex
Print report;
Print "CURSOR_PIXEL_CHECKS;"; checks; ";"; failures
If failures <> 0 Then End 1
Print "CURSOR_PIXELS_PASS"
' end of cursor_pixels.bas
