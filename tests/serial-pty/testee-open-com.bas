/'
    Project: FreeBASIC runtime serial integration tests
    -------------------------------------------------

    File: testee-open-com.bas

    Purpose:

        Exercise OPEN COM against a pseudo-terminal slave supplied by the
        native test controller.

    Responsibilities:

        - request a known 1200 baud 8O1 binary configuration
        - transmit byte values that expose accidental software flow control
        - verify that received bytes retain bit 7 and embedded NUL values
        - close the COM handle so the controller can verify state restoration

    This file intentionally does NOT contain:

        - pseudo-terminal allocation
        - platform termios declarations
        - assumptions about physical serial hardware
'/

#lang "fb"

Const SERIAL_TEST_TIMEOUT_MS As Double = 5000.0


Private Function ElapsedMilliseconds(ByVal startedAt As Double) As Double
    Dim As Double currentTime = Timer

    If currentTime < startedAt Then currentTime += 86400.0
    Return (currentTime - startedAt) * 1000.0
End Function


Dim As String endpoint = Command(1)
Dim As String options
Dim As String outboundBytes
Dim As String expectedInboundBytes
Dim As String receivedBytes
Dim As Integer fileNumber
Dim As Long result
Dim As Double startedAt

If Len(endpoint) = 0 OrElse InStr(endpoint, ":") > 0 Then End 2

options = endpoint & _
    ":1200,O,8,1,CS0,DS0,CD0,RS,DT,PE,BIN,OP1000,RB4096,TB4096"
fileNumber = FreeFile
result = Open Com(options For Binary Access Read Write As #fileNumber)
If result <> 0 Then End 3

outboundBytes = Chr(&hff) & Chr(&h80) & Chr(&h11) & Chr(&h13) & _
    Chr(&h00) & Chr(&h7f) & Chr(&h02)
result = Put(#fileNumber, , outboundBytes)
If result <> 0 Then
    Close #fileNumber
    End 4
End If

expectedInboundBytes = Chr(&hfe) & Chr(&h81) & Chr(&h11) & Chr(&h13) & _
    Chr(&h00) & Chr(&h55) & Chr(&haa)
startedAt = Timer

Do
    If Loc(fileNumber) >= Len(expectedInboundBytes) Then Exit Do
    If ElapsedMilliseconds(startedAt) >= SERIAL_TEST_TIMEOUT_MS Then
        Close #fileNumber
        End 5
    End If
    Sleep 1, 1
Loop

receivedBytes = Input(Len(expectedInboundBytes), #fileNumber)
Close #fileNumber

If receivedBytes <> expectedInboundBytes Then End 6
End 0

/' end of testee-open-com.bas '/
