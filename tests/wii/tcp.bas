/'
    Project: FreeBASIC Wii TCP qualification
    File: tcp.bas
    Purpose: Verify that GET returns available bytes without awaiting a full buffer.
    Responsibilities: One loopback request/reply and nonce-bound test receipts.
    Ownership: This probe closes its socket and replaces only its disposable
    SD receipt files. One BASIC thread owns the connection throughout the test.
    This file does not connect to production or use account credentials.
'/
#lang "fb"
#include once "file.bi"
Private Function check_PartialRead(ByVal log_file As Long) As Long
    Const requested_bytes As UInteger = 64
    Const ready_seconds As Double = 10.0
    Const reply_seconds As Double = 1.0
    Dim connection As Long = FreeFile()
    Dim open_status As Long = Open Tcp("host=127.0.0.1,port=18078,timeout=100" As #connection)
    Print #log_file, "TCP_OPEN=" & Str(open_status)
    Dim flushed As Long = FileFlush(log_file, 1)
    If open_status <> 0 Then Return 1
    Dim started As Double = Timer
    Do While Eof(connection) <> 0 AndAlso Eoc(connection) = 0 AndAlso Timer - started < ready_seconds
        Sleep 1, 1
    Loop
    Dim received As String = Space(requested_bytes)
    Dim count As UInteger
    started = Timer
    Print #log_file, "TCP_BEFORE_GET eof=" & Str(Eof(connection)) & " eoc=" & Str(Eoc(connection))
    flushed = FileFlush(log_file, 1)
    Dim read_status As Long = Get(#connection, , received, , count)
    Dim elapsed As Double = Timer - started
    Print #log_file, "TCP_GET=" & Str(read_status) & " bytes=" & Str(count) & " seconds=" & Str(elapsed)
    Dim result As Long = 1
    If read_status = 0 AndAlso count = 4 AndAlso Left(received, count) = "PING" AndAlso elapsed < reply_seconds Then
        Dim reply As String = "OK"
        If Put(#connection, , reply) = 0 Then result = 0
    End If
    started = Timer
    Close #connection
    elapsed = Timer - started
    Print #log_file, "TCP_CLOSE_SECONDS=" & Str(elapsed)
    If elapsed >= reply_seconds Then result = 1
    Print #log_file, "TCP_PARTIAL_READ=" & Str(result = 0)
    Return result
End Function
Dim nonce As String
Dim nonce_file As Long = FreeFile()
If Open("sd:/FBQNONCE.TXT" For Input As #nonce_file) <> 0 Then End 2
Line Input #nonce_file, nonce
Close #nonce_file
If Len(nonce) <> 32 Then End 3
Dim log_file As Long = FreeFile()
If Open("sd:/TCPCHECK.TXT" For Output As #log_file) <> 0 Then End 4
Print #log_file, "RUN_NONCE=" & nonce
Dim result As Long = check_PartialRead(log_file)
Close #log_file
Dim exit_file As Long = FreeFile()
If Open("sd:/FBQEXIT.TXT" For Output As #exit_file) <> 0 Then End 5
Print #exit_file, "FBQUAL " & nonce & " " & LTrim(Str(result)) & " DONE"
Close #exit_file
End result
' end of tcp.bas
