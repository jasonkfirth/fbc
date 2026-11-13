'' Project: FreeBASIC DOS compiler regressions
'' File: tls-basic-consumer.bas
'' Purpose: Verify initialized BASIC TLS across separately compiled modules.
'' Responsibilities: Check initial values, shared keys and worker isolation.
'' This file does not declare an external C TLS ABI.

Thread Local Extern dosTlsValue As Long
Thread Local Extern dosTlsWide As ULongInt
Declare Function dosTlsRead() As Long
Declare Function dosTlsPrivateRead() As Long
Private Thread Local Dim Shared moduleTlsValue As Long = 73

Dim Shared workerFailure As Long

Private Sub worker(ByVal ignored As Any Ptr)
	'' Identically named private objects in different modules are distinct.
	If moduleTlsValue <> 73 OrElse dosTlsPrivateRead() <> 41 Then
		workerFailure = 5
		Exit Sub
	End If
	moduleTlsValue = 74
	'' Read through the extern first, before the provider has accessed its
	'' variable in this thread. The definition still supplies its initializer.
	If dosTlsValue <> 17 OrElse dosTlsRead() <> 17 Then
		workerFailure = 1
		Exit Sub
	End If
	If dosTlsWide <> &h8877665544332211ULL Then
		workerFailure = 2
		Exit Sub
	End If
	dosTlsValue = 33
	dosTlsWide = &h0807060504030201ULL
	If dosTlsRead() <> 33 OrElse dosTlsWide <> &h0807060504030201ULL Then workerFailure = 3
End Sub

If dosTlsValue <> 17 OrElse dosTlsRead() <> 17 Then End 1
If dosTlsWide <> &h8877665544332211ULL Then End 2
If moduleTlsValue <> 73 OrElse dosTlsPrivateRead() <> 41 Then End 7

Dim As Any Ptr handle = ThreadCreate(@worker)
If handle = 0 Then End 3
ThreadWait(handle)
If workerFailure <> 0 Then End 4
If dosTlsValue <> 17 OrElse dosTlsRead() <> 17 Then End 5
If dosTlsWide <> &h8877665544332211ULL Then End 6
If moduleTlsValue <> 73 OrElse dosTlsPrivateRead() <> 41 Then End 8
Print "DOS_TLS_MODULES=PASS"

'' end of tls-basic-consumer.bas
