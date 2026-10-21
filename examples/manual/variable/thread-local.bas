'' Project: FreeBASIC manual examples
'' ---------------------------------
''
'' File: thread-local.bas
''
'' Purpose:
''
''     Show the declaration form for per-thread storage.
''
'' Responsibilities:
''
''     - declare a shared variable with one value per thread
''
'' This file intentionally does NOT contain:
''
''     - thread creation or synchronization
''

Thread Local Dim Shared currentRequest As Integer = 0

Sub HandleRequest(ByVal requestNumber As Integer)
	currentRequest = requestNumber
	Print currentRequest
End Sub

HandleRequest(1)

'' end of thread-local.bas
