/'
    Project: FreeBASIC Wii TCP qualification
    File: tcp_leak.bas
    Purpose: Detect leaked descriptors across repeated idle connection closure.
    Responsibilities: Open and close 32 loopback sockets and record completion.
    Ownership: One thread closes each handle. Only disposable SD receipts are
    replaced; the peer stays open until it receives our closure notification.
    This file does not use production services or account credentials.
'/
#lang "fb"
Private Function check_Descriptors(ByVal log_file As Long) As Long
    Const connection_count As Long = 32 ' More than Dolphin's 24 raw IOS sockets.
    For index As Long = 1 To connection_count
        Dim connection As Long = FreeFile()
        Dim status As Long = Open Tcp("host=127.0.0.1,port=18078,timeout=100" As #connection)
        If status <> 0 Then
            Print #log_file, "TCP_OPEN_FAILED iteration=" & Str(index) & " status=" & Str(status)
            Return 1
        End If
        If Close(#connection) <> 0 Then Return 1
    Next index
    Print #log_file, "TCP_DESCRIPTOR_CYCLES=" & Str(connection_count)
    Return 0
End Function
Dim nonce As String
Dim nonce_file As Long = FreeFile()
If Open("sd:/FBQNONCE.TXT" For Input As #nonce_file) <> 0 Then End 2
Line Input #nonce_file, nonce
Close #nonce_file
If Len(nonce) <> 32 Then End 3
Dim log_file As Long = FreeFile()
If Open("sd:/LEAKCHECK.TXT" For Output As #log_file) <> 0 Then End 4
Print #log_file, "RUN_NONCE=" & nonce
Dim result As Long = check_Descriptors(log_file)
Close #log_file
Dim exit_file As Long = FreeFile()
If Open("sd:/FBQEXIT.TXT" For Output As #exit_file) <> 0 Then End 5
Print #exit_file, "FBQUAL " & nonce & " " & LTrim(Str(result)) & " DONE"
Close #exit_file
End result
' end of tcp_leak.bas
