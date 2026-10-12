'' FreeBASIC Newton example
'' File: test.bas
'' Purpose: Create and release a world using the Newton 3.13 SDK.
'' Responsibilities: Check the loaded SDK and world allocation before cleanup.
'' This example contains no bodies, simulation loop or rendering.

#include once "Newton.bi"

' The float precision is a native SDK build choice, not a runtime conversion.
If NewtonWorldGetVersion() <> NEWTON_MAJOR_VERSION * 100 + NEWTON_MINOR_VERSION OrElse _
	NewtonWorldFloatSize() <> SizeOf(dFloat) Then
	Print "Newton SDK version or float precision does not match Newton.bi"
	End 1
End If

Dim As NewtonWorld Ptr world = NewtonCreate()
If world = 0 Then
	Print "Could not create the Newton world"
	End 1
End If

NewtonDestroy(world)
'' end of test.bas
