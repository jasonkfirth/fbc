'' .zip unpacking using libzip
#include once "zip.bi"

Sub create_parent_dirs(ByVal file As ZString Ptr)
    '' Given a path like this:
    ''    foo/bar/baz/file.ext
    '' Do these mkdir()'s:
    ''    foo
    ''    foo/bar
    ''    foo/bar/baz
    If file = NULL Then Exit Sub

    Dim As UByte Ptr p = file
    If p = NULL Then Exit Sub
    Do
        '' p advances within file's checked NUL-terminated ZSTRING storage.
        '' FB-LINTER: DISABLE-NEXT-LINE FBL-PTR-001
        Select Case (*p)
        Case Asc("/")
            '' FB-LINTER: DISABLE-NEXT-LINE FBL-PTR-001
            *p = 0
            MkDir(*file)
            '' FB-LINTER: DISABLE-NEXT-LINE FBL-PTR-001
            *p = Asc("/")
        Case 0
            Exit Do
        End Select
        p += 1
    Loop
End Sub

'' Asks libzip for information on file number 'i' in the .zip file,
'' and then extracts it, while creating directories as needed.
Private Sub unpack_zip_file(ByVal zip As zip_t Ptr, ByVal i As Integer)
    #define BUFFER_SIZE (1024 * 512)
    Static As UByte chunk(0 To (BUFFER_SIZE - 1))
    #define buffer (@chunk(0))

    '' Retrieve the filename. libzip may have no usable name for a malformed entry.
    Dim As Const ZString Ptr archive_name = zip_get_name(zip, i, 0)
    If archive_name = NULL Then
        Print "could not retrieve the archive entry name"
        Return
    End If

    Dim As String filename = *archive_name
    Print "file: " & filename & ", ";

    '' Retrieve the file size via a zip_stat().
    Dim As zip_stat stat
    If (zip_stat_index(zip, i, 0, @stat)) Then
        Print "zip_stat() failed"
        Return
    End If

    If ((stat.valid And ZIP_STAT_SIZE) = 0) Then
        Print "could not retrieve file size from zip_stat()"
        Return
    End If

    Print stat.size & " bytes"

    '' Create directories if needed
    create_parent_dirs(filename)

    '' Write out the file
    Dim As Integer fo = FreeFile()
    If (Open(filename, For Binary, Access Write, As #fo)) Then
        Print "could not open output file"
        Return
    End If

    '' Input for the file comes from libzip
    Dim As zip_file_t Ptr fi = zip_fopen_index(zip, i, 0)
    If fi = NULL Then
        Print "could not open the archive entry"
        Close #fo
        Return
    End If

    Do
        '' Write out the file content as returned by zip_fread(), which
        '' also does the decoding and everything.
        '' zip_fread() fills our buffer
        Dim As Integer bytes = _
            zip_fread(fi, buffer, BUFFER_SIZE)
        If (bytes < 0) Then
            Print "zip_fread() failed"
            Exit Do
        End If

        '' EOF?
        If (bytes = 0) Then
            Exit Do
        End If

        '' Write <bytes> amount of bytes of the file
        If (Put(#fo, , *buffer, bytes)) Then
            Print "file output failed"
            Exit Do
        End If
    Loop

    '' Done
    zip_fclose(fi)
    Close #fo
End Sub

Sub unpack_zip(ByRef archive As String)
    Dim As zip_t Ptr zip = zip_open(archive, ZIP_CHECKCONS, NULL)
    If (zip = NULL) Then
        Print "could not open input file " & archive
        Return
    End If

    '' For each file in the .zip... (really nice API, thanks libzip)
    For i As Integer = 0 To (zip_get_num_entries(zip, 0) - 1)
        unpack_zip_file(zip, i)
    Next

    zip_close(zip)
End Sub


    unpack_zip("test.zip")
