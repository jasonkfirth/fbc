'' FreeBASIC libzip examples
'' File: zip.bas
'' Purpose: Extract regular ZIP entries into a fresh destination.
'' Responsibilities: Validate relative paths and sizes, check reads/writes, release handles.
'' This file contains no link creation, file attributes or concurrent extraction.

#include once "zip.bi"
#include once "file.bi"
#include once "dir.bi"

'' The example bounds disk use before extracting any data. These are demo
'' limits, not libzip limits: 10,000 entries and one GiB of decoded bytes.
Const MAX_ENTRIES As zip_int64_t = 10000
Const MAX_TOTAL_SIZE As zip_uint64_t = 1024ull * 1024 * 1024
Const BUFFER_SIZE = 1024 * 512

'' Archive separators are normalized before component validation. A fresh output
'' directory prevents following preexisting links or overwriting unrelated files.
'' The caller must not concurrently modify that directory during extraction.
Private Function checked_name(ByRef filename As String) As Boolean
	If Len(filename) = 0 Then Return False
	For position As Integer = 0 To Len(filename)-1
		If filename[position] = Asc("\") Then filename[position] = Asc("/")
		If filename[position] < 32 Or filename[position] = Asc(":") Or _
		   filename[position] = Asc("*") Or filename[position] = Asc("?") Then Return False
	Next
	If Left(filename, 1) = "/" Then Return False
	Dim first As Integer = 1
	Do While first <= Len(filename)
		Dim separator As Integer = InStr(first, filename, "/")
		Dim component As String
		If separator = 0 Then
			component = Mid(filename, first)
		Else
			component = Mid(filename, first, separator-first)
		End If
		If component = "" Or component = "." Or component = ".." Then Return False
		#ifdef __FB_WIN32__
			'' Win32 normalizes trailing dots/spaces and interprets device_name names.
			If Right(component, 1) = "." Or Right(component, 1) = " " Then Return False
			Dim device_name As String = UCase(component)
			Dim dot As Integer = InStr(device_name, ".")
			If dot <> 0 Then device_name = Left(device_name, dot-1)
			device_name = RTrim(device_name)
			If device_name = "CON" Or device_name = "PRN" Or device_name = "AUX" Or device_name = "NUL" Or _
			   device_name = "CONIN$" Or device_name = "CONOUT$" Then Return False
			If Left(device_name, 3) = "COM" Or Left(device_name, 3) = "LPT" Then
				Dim suffix As String = Mid(device_name, 4)
				If Len(suffix) = 1 AndAlso suffix[0] >= Asc("1") And suffix[0] <= Asc("9") Then Return False
				'' Win32 also recognizes the superscript 1, 2 and 3 aliases.
				If suffix = Chr(&hc2, &hb9) Or suffix = Chr(&hc2, &hb2) Or suffix = Chr(&hc2, &hb3) Then Return False
			End If
		#endif
		If separator = 0 Then Exit Do
		first = separator+1
	Loop
	Return True
End Function

Private Function create_directory(ByRef directory As String) As Boolean
	If MkDir(directory) = 0 Then Return True
	'' DIR's output mask distinguishes a directory from an existing file.
	'' Use the built-in API supported by the installed 32-bit runtime too.
	Dim attributes As Integer
	Dim item As String = Dir(directory, fbDirectory Or fbArchive Or fbReadOnly Or fbHidden Or fbSystem, attributes)
	Return Len(item) <> 0 AndAlso (attributes And fbDirectory) <> 0
End Function

Private Function create_parent_dirs(ByRef filename As String) As Boolean
	Dim separator As Integer = InStr(filename, "/")
	Do While separator <> 0
		Dim directory As String = Left(filename, separator-1)
		'' A drive prefix or root separator is part of the caller's destination.
		If directory <> "" And Right(directory, 1) <> ":" Then
			If Not create_directory(directory) Then Return False
		End If
		separator = InStr(separator+1, filename, "/")
	Loop
	Return True
End Function

Private Function unpack_zip_file(ByVal archive As zip_t Ptr, ByVal index As zip_uint64_t, _
                                 ByRef destination As String) As Boolean
	Dim archive_name As Const ZString Ptr = zip_get_name(archive, index, 0)
	If archive_name = NULL Then Return False
	Dim filename As String = *archive_name
	If Not checked_name(filename) Then Return False
	Dim directory_entry As Boolean = Right(filename, 1) = "/"
	Dim output_name As String = destination & "/" & filename
	If directory_entry Then
		output_name = Left(output_name, Len(output_name)-1)
		Return create_parent_dirs(output_name) AndAlso create_directory(output_name)
	End If
	If Not create_parent_dirs(output_name) Then Return False
	If FileExists(output_name) Then
		Print "refusing existing output: "; output_name
		Return False
	End If

	Dim stat As zip_stat_t
	zip_stat_init(@stat)
	If zip_stat_index(archive, index, 0, @stat) <> 0 Then Return False
	If (stat.valid And ZIP_STAT_SIZE) = 0 Then Return False
	Dim input_file As zip_file_t Ptr = zip_fopen_index(archive, index, 0)
	If input_file = NULL Then Return False
	Dim output_file As Integer = FreeFile()
	If Open(output_name, For Binary, Access Write, As #output_file) <> 0 Then
		zip_fclose(input_file)
		Return False
	End If

	'' One extraction call owns this buffer and both open files.
	Static chunk(0 To BUFFER_SIZE-1) As UByte
	Dim total As zip_uint64_t = 0
	Dim succeeded As Boolean = True
	Do
		Dim bytes As zip_int64_t = zip_fread(input_file, @chunk(0), BUFFER_SIZE)
		If bytes < 0 Or bytes > BUFFER_SIZE Then
			succeeded = False
			Exit Do
		End If
		If bytes = 0 Then Exit Do
		If total > stat.size OrElse bytes > stat.size-total Then
			succeeded = False
			Exit Do
		End If
		If Put(#output_file, , chunk(0), CInt(bytes)) <> 0 Then
			succeeded = False
			Exit Do
		End If
		total += bytes
	Loop
	If zip_fclose(input_file) <> 0 Then succeeded = False
	If Close(#output_file) <> 0 Then succeeded = False
	If total <> stat.size Then succeeded = False
	If Not succeeded Then
		'' This name was created by this call in the fresh destination only.
		Kill output_name
		Return False
	End If
	Print "file: "; filename; ", "; total; " bytes"
	Return True
End Function

Function unpack_zip(ByRef archive_name As String, ByRef destination As String) As Integer
	Dim archive As zip_t Ptr = zip_open(archive_name, ZIP_CHECKCONS, NULL)
	If archive = NULL Then
		Print "could not open input file "; archive_name
		Return 1
	End If
	Dim entries As zip_int64_t = zip_get_num_entries(archive, 0)
	Dim total_size As zip_uint64_t = 0
	Dim succeeded As Boolean = entries >= 0 And entries <= MAX_ENTRIES
	'' Validate the complete name/size list before creating output directories.
	If succeeded Then
		For index As zip_int64_t = 0 To entries-1
			Dim entry_name As Const ZString Ptr = zip_get_name(archive, index, 0)
			If entry_name = NULL Then
				succeeded = False
				Exit For
			End If
			Dim filename As String = *entry_name
			If Not checked_name(filename) Then
				Print "invalid archive path: "; filename
				succeeded = False
				Exit For
			End If
			Dim stat As zip_stat_t
			zip_stat_init(@stat)
			If zip_stat_index(archive, index, 0, @stat) <> 0 Then
				succeeded = False
				Exit For
			End If
			If (stat.valid And ZIP_STAT_SIZE) = 0 OrElse stat.size > MAX_TOTAL_SIZE-total_size Then
				Print "extraction size limit exceeded"
				succeeded = False
				Exit For
			End If
			If Right(filename, 1) = "/" And stat.size <> 0 Then
				Print "directory entry contains file data"
				succeeded = False
				Exit For
			End If
			total_size += stat.size
		Next
	End If
	If Not succeeded Then
		Print "archive validation failed"
		zip_discard(archive)
		Return 1
	End If

	'' Never reuse an existing destination, including a directory link.
	If Len(destination) = 0 OrElse MkDir(destination) <> 0 Then
		Print "destination must be a new directory: "; destination
		zip_discard(archive)
		Return 1
	End If
	For index As zip_int64_t = 0 To entries-1
		If Not unpack_zip_file(archive, index, destination) Then
			Print "entry extraction failed"
			succeeded = False
			Exit For
		End If
	Next
	If zip_close(archive) <> 0 Then
		'' A failed close retains the archive; discard releases it.
		zip_discard(archive)
		succeeded = False
	End If
	If Not succeeded Then Return 1
	Return 0
End Function

Dim archive_name As String = Command(1)
Dim destination As String = Command(2)
If archive_name = "" Then archive_name = "test.zip"
If destination = "" Then destination = "test-unpacked"
End unpack_zip(archive_name, destination)

'' end of zip.bas
