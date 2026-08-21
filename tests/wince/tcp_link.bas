''
'' FreeBASIC Windows CE TCP link smoke test
'' ----------------------------------------
''
'' File: tests/wince/tcp_link.bas
''
'' Purpose:
''
''     Force the Windows CE TCP device and classic WinSock imports into one
''     target executable without requiring a server during link validation.
''
'' Responsibilities:
''
''     - exercise OPEN TCP client parsing and socket creation
''     - close a connection if the emulator happens to provide an echo service
''
'' This file intentionally does NOT test:
''
''     - loopback data transfer
''     - TCP ACCEPT
''     - timeout behavior
''

Dim As Integer result

result = Open( "TCP:host=127.0.0.1,port=7,timeout=25" _
	For Binary Access Read Write As #1 )
If result = 0 Then
	Close #1
End If

End 0

'' end of tcp_link.bas
