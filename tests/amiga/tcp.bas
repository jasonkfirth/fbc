' Project: FreeBASIC classic AmigaOS runtime tests
' File: tests/amiga/tcp.bas
' Purpose: Qualify TCP streams and socket lifetime across native task exit.
' Responsibilities: Run the shared TCP suite and use a listener after its
' creator has exited, then close it from the controlling task.
' This file intentionally does NOT contain a socket provider or external peer.

#ifndef __FB_DEBUG__
#error Build this qualification with -g so the shared suite's assertions run.
#endif

If Command(1) = "no-library" Then
	Dim handle As Integer = FreeFile()
	If Open TCP Server( "host=127.0.0.1,port=19094" As #handle ) = 0 Then
		Close #handle
		End 1
	End If
	Print "PASS missing native socket library"
	End 0
End If

#include once "../file/tcp.bas"

Dim Shared As Integer inheritedListener
Dim Shared As Integer inheritedListenerError

Sub CreateListener( ByVal parameter As Any Ptr )
	inheritedListener = FreeFile()
	inheritedListenerError = Open TCP Server( _
		"host=localhost,port=19094,backlog=1,timeout=1000" As #inheritedListener )
End Sub

Scope
	Dim creator As Any Ptr = ThreadCreate( @CreateListener )
	Assert( creator <> 0 )
	ThreadWait( creator )
	Assert( inheritedListenerError = 0 )

	' An Amiga task owns its SocketBase and descriptor table. The BASIC
	' handle must still work after that task and its TLS have been released.
	Dim client As Integer = FreeFile()
	Assert( Open TCP( "host=127.0.0.1,port=19094" As #client ) = 0 )
	Dim peer As Integer = TCP Accept( #inheritedListener )
	Assert( peer > 0 )
	Dim sent As UByte = 173
	Dim received As UByte
	Assert( Put( #client, , sent ) = 0 )
	Assert( wait_for_tcp_data( peer ) )
	Assert( Get( #peer, , received ) = 0 )
	Assert( received = sent )
	Close #client, #peer, #inheritedListener
End Scope

Dim Shared As Integer fragmentListener

Sub SendFragmentedRecord( ByVal parameter As Any Ptr )
	Dim peer As Integer = TCP Accept( #fragmentListener )
	Assert( peer > 0 )
	' INPUT must keep waiting when a numeric token, quoted string or record
	' delimiter arrives in separate packets. Each delay lets the reader run.
	Print #peer, "12";
	Sleep 40, 1
	Print #peer, "3,";
	Sleep 40, 1
	Print #peer, Chr(34) & "frag";
	Sleep 40, 1
	Print #peer, "ment" & Chr(34)
	Close #peer
End Sub

Scope
	fragmentListener = FreeFile()
	Assert( Open TCP Server( "host=127.0.0.1,port=19095,timeout=2000" As #fragmentListener ) = 0 )
	Dim writer As Any Ptr = ThreadCreate( @SendFragmentedRecord )
	Assert( writer <> 0 )
	Dim client As Integer = FreeFile()
	Assert( Open TCP( "host=127.0.0.1,port=19095,timeout=2000" As #client ) = 0 )
	Dim number As Integer
	Dim text As String
	Input #client, number, text
	Assert( number = 123 )
	Assert( text = "fragment" )
	Close #client
	ThreadWait( writer )
	Close #fragmentListener
End Scope

Print "PASS native TCP streams, burst, wildcard, task exit and fragmented INPUT"

' end of tests/amiga/tcp.bas
