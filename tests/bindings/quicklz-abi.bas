'' Project: FreeBASIC native library regression checks
'' File: quicklz-abi.bas
'' Purpose: Check state layouts and execute the actual QuickLZ 1.5.0 codec.
'' Responsibilities: Typed state buffers, binary round trips and checked lengths.
'' This file contains no malformed-data decoder calls or replacement codec.

#lang "fb"
#include once "quicklz.bi"

Print "ABI "; SizeOf(ui32); " "; SizeOf(ui16); " "; SizeOf(qlz_hash_compress); " "; _
	OffsetOf(qlz_hash_compress, cache); " "; OffsetOf(qlz_hash_compress, offset); " "; _
	SizeOf(qlz_hash_decompress); " "; SizeOf(qlz_state_compress); " "; _
	OffsetOf(qlz_state_compress, hash(0)); " "; OffsetOf(qlz_state_compress, hash_counter(0)); " "; _
	SizeOf(qlz_state_decompress); " "; OffsetOf(qlz_state_decompress, hash_counter(0)); " "; _
	OffsetOf(qlz_state_decompress, stream_counter)

'' QuickLZ must be compiled with the same level and streaming settings as
'' this binding. Query the actual library before lending it typed storage.
If qlz_get_setting(0) <> QLZ_COMPRESSION_LEVEL Then End 1
If qlz_get_setting(3) <> QLZ_STREAMING_BUFFER Then End 1
If qlz_get_setting(1) <> SizeOf(qlz_state_compress) Then End 1
If qlz_get_setting(2) <> SizeOf(qlz_state_decompress) Then End 1
If qlz_get_setting(-1) <> -1 Then End 1
If qlz_get_setting(7) <> 1 OrElse qlz_get_setting(8) <> 5 OrElse qlz_get_setting(9) <> 0 Then End 1

Dim As qlz_state_compress encoder
Dim As qlz_state_decompress decoder
Dim As String original = Space(32768)
For index As Integer = 0 To Len(original) - 1
	original[index] = CUByte(index And 255)
Next
'' The C API requires input size plus 400 bytes of output capacity. This
'' fixed test size also stays within QuickLZ's unsigned 32-bit format limit.
Dim As String compressed = Space(Len(original) + 400)
Dim As size_t compressed_size = qlz_compress(StrPtr(original), StrPtr(compressed), Len(original), @encoder)
If compressed_size = 0 OrElse compressed_size > Len(compressed) Then End 1
If qlz_size_compressed(StrPtr(compressed)) <> compressed_size Then End 1
If qlz_size_decompressed(StrPtr(compressed)) <> Len(original) Then End 1
Dim As String restored = Space(Len(original))
If qlz_decompress(StrPtr(compressed), StrPtr(restored), @decoder) <> Len(original) Then End 1
If restored <> original Then End 1
Print "QUICKLZ_NATIVE_PASS"

'' end of quicklz-abi.bas
