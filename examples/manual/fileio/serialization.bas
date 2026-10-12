'' examples/manual/fileio/serialization.bas
''
'' FreeBASIC Manual: Compiler-assisted serialization
'' Purpose: Carry a serialized record through an ordinary binary file.
'' Responsibilities: File errors, bounded reads and exact byte counts.
'' Ownership: Each opened file handle is closed by this program.
'' The caller owns the output file, which is left in place for inspection.
'' This example intentionally contains no stream or socket framing.
'' --------

#include once "fbserialize.bi"

Type SavedState
    title As String
    score As Long
End Type

Dim As String filename = Command(1)
If filename = "" Then
    Print "Usage: serialization <new-or-empty-file>"
    End 1
End If

Dim As SavedState original, restored
original.title = "Saved" & Chr(0) & "game"
original.score = 42
Dim As String payload = Serialize(original)
If Err <> 0 Then End 1

Dim As Integer file_number = FreeFile()
If Open(filename For Binary Access Read Write As #file_number) <> 0 Then End 1

'' Use a new or empty file so older trailing bytes cannot survive a write.
If Lof(file_number) <> 0 Then
    Close #file_number
    Print "Choose a new or empty file."
    End 1
End If
If Put(#file_number, , payload) <> 0 Then
    Close #file_number
    End 1
End If
If Close(file_number) <> 0 Then End 1

file_number = FreeFile()
If Open(filename For Binary Access Read As #file_number) <> 0 Then End 1

'' Bound the allocation before reading. Version 1 accepts at most 16 MiB.
Dim As LongInt byte_count = Lof(file_number)
If byte_count <= 0 OrElse byte_count > 16 * 1024 * 1024 Then
    Close #file_number
    End 1
End If
Dim As String received = Space(CInt(byte_count))
Dim As UInteger bytes_read
Dim As Integer read_error = Get(#file_number, , received, , bytes_read)
Dim As Integer close_error = Close(file_number)
If read_error <> 0 OrElse close_error <> 0 Then End 1
If bytes_read <> byte_count Then End 1

If Deserialize(received, restored) = 0 Then
    Print "Deserialize failed: "; Err
    End 1
End If
Print "Title bytes: "; Len(restored.title)
Print "Score: "; restored.score

'' end of serialization.bas
