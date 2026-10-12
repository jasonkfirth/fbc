'' FreeBASIC libzip examples
'' File: zip2.bas
'' Purpose: Add text strings to a ZIP archive.
'' Responsibilities: Check commit errors and transfer ownership of each zip source.
'' This file contains no binary-buffer input or archive extraction.

#include once "zip.bi"

Function zipAddFileFromString(ByVal filename As ZString Ptr, ByVal entry As ZString Ptr, _
                              ByVal text As ZString Ptr) As Boolean
	If filename = NULL Or entry = NULL Or text = NULL Then Return False
	Dim archive As zip_t Ptr = zip_open(filename, ZIP_CREATE, NULL)
	If archive = NULL Then
		Print "could not open output archive"
		Return False
	End If
	'' freep=0 borrows this FreeBASIC-owned text until zip_close completes.
	'' libzip must not release it with its own allocator.
	Dim source As zip_source_t Ptr = zip_source_buffer(archive, text, Len(*text), 0)
	If source = NULL Then
		zip_discard(archive)
		Return False
	End If
	If zip_file_add(archive, entry, source, ZIP_FL_OVERWRITE) < 0 Then
		'' Failed insertion retains caller ownership; successful insertion transfers it.
		zip_source_free(source)
		zip_discard(archive)
		Return False
	End If
	If zip_close(archive) <> 0 Then
		Print "could not commit output archive"
		zip_discard(archive)
		Return False
	End If
	Print "OK"
	Return True
End Function

Dim text As String = "<My text           >......6...3..2"
Dim archive_name As String = Command(1)
If archive_name = "" Then archive_name = "26.zip"
If Not zipAddFileFromString(archive_name, "file3.txt", text) Then End 1
If Not zipAddFileFromString(archive_name, "BIMBO/file3.txt", text) Then End 1
Sleep

'' end of zip2.bas
