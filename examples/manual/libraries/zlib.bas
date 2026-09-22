'' examples/manual/libraries/zlib.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'zlib'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibzlib
'' --------

'' Ownership:
'' The example owns src and dest after allocation. Every codec or allocation
'' failure releases acquired buffers before terminating the demonstration.

'' Zlib compress/decompress example, by yetifoot

#include Once "zlib.bi"

Dim As Integer errlev

'' This is the size of our test data in bytes.
Dim As uLong_ src_len = 100000

Print "ZLib test - Version " & *zlibVersion()
Print
Print "Test data size      : " & src_len & " bytes."

'' The size of the destination buffer for the compressed data is calculated by
'' the compressBound function.
Dim As uLong_ dest_len = compressBound(src_len)

'' Allocate our needed memory.
Dim As UByte Ptr src = Allocate(src_len)
If src = 0 Then
	Print "**** Unable to allocate source buffer ****"
	End 1
End If

Dim As UByte Ptr dest = Allocate(dest_len)
If dest = 0 Then
	Deallocate(src)
	src = 0
	Print "**** Unable to allocate compression buffer ****"
	End 1
End If

'' Fill the src buffer with random, yet still compressable data.
Randomize Timer
For i As Integer = 0 To src_len - 1
	src[i] = Rnd * 4
Next

'' Store the crc32 checksum of the input data, so we can check if the
'' uncompression has worked.
Dim As UInteger crc = crc32(0, src, src_len)

'' Perform the compression.  dest_len is passed as its address, because when
'' the function returns it will contain the size of the compressed data.
errlev = compress(dest, @dest_len, src, src_len)
If errlev <> 0 Then
	'' If the function returns a value other than 0 then an error occured.
	Print "**** Error during compress - code " & errlev & " ****"
	Deallocate(src)
	src = 0
	Deallocate(dest)
	dest = 0
	End 1
End If
Print "Compressed to       : " & dest_len & " bytes."

'' NOTE: in normal use in a program, you would store the src_len, in order to
'' be able to tell uncompress the output size.  However in this example we can
'' just leave it in src_len.  The same goes for dest_len, which is the compressed
'' datas size.

'' Wipe the src buffer before we uncompress to it, so that we can check if the
'' decompression has worked.
For i As Integer = 0 To src_len - 1
	src[i] = 0
Next

'' Perform a decompression.  This time we uncompress the data back to src.
'' src_len is passed as its address, because when
'' the function returns it will contain the size of the uncompressed data.
errlev = uncompress(src, @src_len, dest, dest_len)
If errlev <> 0 Then
	'' If the function returns a value other than 0 then an error occured.
	Print "**** Error during uncompress - code " & errlev & " ****"
	Deallocate(src)
	src = 0
	Deallocate(dest)
	dest = 0
	End 1
End If
Print "Uncompressed to     : " & src_len & " bytes."

'' Make sure the checksum of the uncompressed data matches our original data.
If crc <> crc32(0, src, src_len) Then
	Print "crc32 checksum      : FAILED"
Else
	Print "crc32 checksum      : PASSED"
End If

'' Free the buffers used in the test.
Deallocate(src)
src = 0
Deallocate(dest)
dest = 0

Print
Print "Press any key to end . . . "
Sleep

'' end of zlib.bas
