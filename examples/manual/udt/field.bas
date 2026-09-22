'' examples/manual/udt/field.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'FIELD'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgField
'' --------

'' Layout: bytes 0-13 are the BMP file header; bytes 14-53 are its DIB header.
Type bitmap_header Field = 1
	bfType          As UShort
	bfsize          As ULong
	bfReserved1     As UShort
	bfReserved2     As UShort
	bfOffBits       As ULong
	biSize          As ULong
	biWidth         As ULong
	biHeight        As ULong
	biPlanes        As UShort
	biBitCount      As UShort
	biCompression   As ULong
	biSizeImage     As ULong
	biXPelsPerMeter As ULong
	biYPelsPerMeter As ULong
	biClrUsed       As ULong
	biClrImportant  As ULong
End Type

Dim bmp_header As bitmap_header
Dim file_number As Integer

'Open up bmp.bmp and get its header data:
'Note: Will not work without a bmp.bmp to load . . .
file_number = FreeFile

If Open("bmp.bmp" For Binary As #file_number) <> 0 Then
	Print "Could not open bmp.bmp"
Else
	' The bitmap_header layout above is the BMP file-format contract.
	' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
	If Get(#file_number, , bmp_header) <> 0 Then
		Print "Could not read the BMP header"
	Else
		Print bmp_header.biWidth, bmp_header.biHeight
	End If

	Close #file_number
End If

Sleep
