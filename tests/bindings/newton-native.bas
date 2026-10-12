'' FreeBASIC Newton 3.13 binding qualification
'' File: newton-native.bas
'' Purpose: Compare native C layouts and exercise the real physics callbacks.
'' Responsibilities: SDK checks, bounded simulation and owned world cleanup.
'' This file contains no replacement physics or asynchronous application loop.

#lang "fb"
#include once "Newton.bi"

' One solver thread and synchronous updates protect these callback counters.
Dim Shared As Long force_calls, invalid_callback, destroyed_worlds

Sub apply_gravity Cdecl(ByVal body As Const NewtonBody Const Ptr, ByVal timestep As dFloat, ByVal thread_index As Long)
	' A mass of 2 kg needs a force of -19.62 N for terrestrial gravity.
	Dim As dFloat force(0 To 3) = {0, -19.62, 0, 0}
	If body = 0 OrElse timestep <= 0 OrElse thread_index < 0 Then
		invalid_callback = 1
		Exit Sub
	End If
	force_calls += 1
	NewtonBodySetForce(body, @force(0))
End Sub

Sub world_destroyed Cdecl(ByVal world As Const NewtonWorld Const Ptr)
	If world = 0 Then invalid_callback = 1
	destroyed_worlds += 1
End Sub

Function check_world() As Integer
	If NewtonWorldGetVersion() <> 313 OrElse NewtonWorldFloatSize() <> SizeOf(dFloat) Then Return 2
	Dim As Long baseline = NewtonGetMemoryUsed()
	Dim As NewtonWorld Ptr world = NewtonCreate()
	If world = 0 Then Return 3
	NewtonSetThreadsCount(world, 1)
	NewtonWorldSetDestructorCallback(world, @world_destroyed)
	Dim As Integer status = 1
	Dim As NewtonCollision Ptr collision = NewtonCreateBox(world, 1, 2, 3, 17, 0)
	If collision <> 0 Then
		Dim As NewtonCollisionInfoRecord info
		NewtonCollisionGetInfo(collision, @info)
		If info.m_collisionType = SERIALIZE_ID_BOX AndAlso info.m_collisionUserID = 17 AndAlso _
			Abs(info.m_box.m_x - 1) < 0.0001 AndAlso Abs(info.m_box.m_y - 2) < 0.0001 AndAlso Abs(info.m_box.m_z - 3) < 0.0001 Then
			Dim As dFloat matrix(0 To 15) = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}
			Dim As NewtonBody Ptr body = NewtonCreateDynamicBody(world, collision, @matrix(0))
			' The body keeps its own reference; release our collision reference.
			NewtonDestroyCollision(collision)
			collision = 0
			If body <> 0 Then
				NewtonBodySetMassProperties(body, 2, NewtonBodyGetCollision(body))
				NewtonBodySetLinearDamping(body, 0)
				NewtonBodySetForceAndTorqueCallback(body, @apply_gravity)
				' Sixty steps of 1/120 second cover half a second of free fall.
				For step_index As Integer = 1 To 60
					NewtonUpdate(world, 1.0 / 120.0)
				Next
				NewtonBodyGetMatrix(body, @matrix(0))
				If NewtonWorldGetBodyCount(world) = 1 AndAlso force_calls >= 60 AndAlso invalid_callback = 0 AndAlso _
					matrix(13) < -1.1 AndAlso matrix(13) > -1.4 AndAlso Abs(matrix(12)) < 0.0001 AndAlso Abs(matrix(14)) < 0.0001 Then
					Print "FALL "; matrix(13); " CALLBACKS "; force_calls
					status = 0
				End If
			End If
		End If
		If collision <> 0 Then NewtonDestroyCollision(collision)
	End If
	' The world owns the body, and destruction joins the engine's workers.
	NewtonDestroy(world)
	If destroyed_worlds <> 1 OrElse NewtonGetMemoryUsed() <> baseline Then status = 4
	If status = 0 Then Print "NEWTON_NATIVE=PASS version=313 float=4 cleanup=1"
	Return status
End Function

Print "ABI "; SizeOf(Long); " "; SizeOf(dFloat); " "; SizeOf(dFloat64); " "; SizeOf(dLong); " "; SizeOf(Any Ptr); " "; _
	SizeOf(NewtonCollisionInfoRecord); " "; SizeOf(NewtonJointRecord); " "; SizeOf(NewtonWorldConvexCastReturnInfo); " "; _
	SizeOf(NewtonUserMeshCollisionCollideDesc); " "; OffsetOf(NewtonCollisionInfoRecord, m_paramArray(0)); " "; _
	OffsetOf(NewtonWorldConvexCastReturnInfo, m_contactID); " "; OffsetOf(NewtonWorldConvexCastReturnInfo, m_hitBody)
End check_world()
'' end of newton-native.bas
