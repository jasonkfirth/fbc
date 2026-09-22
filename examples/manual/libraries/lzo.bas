'' examples/manual/libraries/lzo.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'LZO'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibLzo
'' --------

'' Ownership:
'' The example owns the compressed and decompressed buffers after allocation.
'' The compression workspace is released after compression, and every failure
'' path clears the pointers it has released before ending the program.

#include "lzo/lzo1x.bi"

Dim inbuf As ZString Ptr = @"string to compress (or not, since it's so short)"
Dim inlen As Integer = Len(*inbuf) + 1
Dim complen As lzo_uint = 100
Dim compbuf As ZString Ptr
Dim decomplen As lzo_uint = 100
Dim decompbuf As ZString Ptr
Dim workmem As Any Ptr

Print "initializing LZO: ";
If lzo_init() = 0 Then
	Print "ok"
Else
	Print "failed!"
	End 1
End If

compbuf = Allocate(complen)
If compbuf = 0 Then
	Print "unable to allocate compression buffer"
	End 1
End If

decompbuf = Allocate(decomplen)
If decompbuf = 0 Then
	Deallocate(compbuf)
	compbuf = 0
	Print "unable to allocate decompression buffer"
	End 1
End If

Print "compressing '" & *inbuf & "': ";

workmem = Allocate(LZO1X_1_15_MEM_COMPRESS)
If workmem = 0 Then
	Deallocate(compbuf)
	compbuf = 0
	Deallocate(decompbuf)
	decompbuf = 0
	Print "unable to allocate compression workspace"
	End 1
End If

If lzo1x_1_15_compress(inbuf, inlen, compbuf, @complen, workmem) = 0 Then
	Print "ok (" & inlen & " bytes in, " & complen & " bytes compressed)"
Else
	Deallocate(workmem)
	workmem = 0
	Deallocate(compbuf)
	compbuf = 0
	Deallocate(decompbuf)
	decompbuf = 0
	Print "failed!"
	End 1
End If

Deallocate(workmem)
workmem = 0

Print "decompressing: ";

If lzo1x_decompress(compbuf, complen, decompbuf, @decomplen, NULL) = 0 Then
	Print "ok: '" & *decompbuf & "' (" & complen & " bytes compressed, " & decomplen & " bytes decompressed)"
Else
	Deallocate(compbuf)
	compbuf = 0
	Deallocate(decompbuf)
	decompbuf = 0
	Print "failed!"
	End 1
End If

Deallocate(compbuf)
compbuf = 0
Deallocate(decompbuf)
decompbuf = 0

'' end of lzo.bas
