''
'' FreeBASIC portable network wire helpers
'' ---------------------------------------
''
'' File: fbnetwire.bi
''
'' Purpose:
''
''     Provide small source-level helpers for writing fixed-size values to
''     files or OPEN TCP handles without depending on the target CPU's byte
''     order or native Integer size.
''
'' Responsibilities:
''
''     - encode and decode fixed-width integer values in little-endian order
''     - encode and decode Single and Double values in little-endian order
''     - provide simple byte, value, and length-prefixed string file helpers
''
'' This file intentionally does NOT contain:
''
''     - socket creation or connection management
''     - EOF/EOC readiness policy
''     - packet framing beyond a basic Int32 length-prefixed string helper
''

#ifndef __FB_NETWIRE_BI__
#define __FB_NETWIRE_BI__

const FB_NETWIRE_INT16_BYTES = 2
const FB_NETWIRE_INT32_BYTES = 4
const FB_NETWIRE_INT64_BYTES = 8
const FB_NETWIRE_SINGLE_BYTES = 4
const FB_NETWIRE_DOUBLE_BYTES = 8

private function FbNetWireIsLittleEndian() as integer
	dim as ushort probe = &h0102
	dim as ubyte ptr bytes = cptr( ubyte ptr, @probe )

	return ( bytes[0] = &h02 )
end function

private sub FbNetWireCopyBytes( byval dst as ubyte ptr, byval src as ubyte ptr, byval bytes as integer )
	dim as integer i

	if( dst = 0 orelse src = 0 orelse bytes <= 0 ) then
		exit sub
	end if

	for i = 0 to bytes - 1
		dst[i] = src[i]
	next i
end sub

private sub FbNetWireCopyBytesReversed( byval dst as ubyte ptr, byval src as ubyte ptr, byval bytes as integer )
	dim as integer i

	if( dst = 0 orelse src = 0 orelse bytes <= 0 ) then
		exit sub
	end if

	for i = 0 to bytes - 1
		dst[i] = src[bytes - 1 - i]
	next i
end sub

private sub FbNetEncodeUInt16LE( byval value as ushort, byval bytes as ubyte ptr )
	if( bytes = 0 ) then
		exit sub
	end if

	bytes[0] = cubyte( value and 255 )
	bytes[1] = cubyte( ( value shr 8 ) and 255 )
end sub

private function FbNetDecodeUInt16LE( byval bytes as ubyte ptr ) as ushort
	if( bytes = 0 ) then
		return 0
	end if

	return cushort( bytes[0] ) or ( cushort( bytes[1] ) shl 8 )
end function

private sub FbNetEncodeInt16LE( byval value as short, byval bytes as ubyte ptr )
	FbNetEncodeUInt16LE( cushort( value ), bytes )
end sub

private function FbNetDecodeInt16LE( byval bytes as ubyte ptr ) as short
	return cshort( FbNetDecodeUInt16LE( bytes ) )
end function

private sub FbNetEncodeUInt32LE( byval value as ulong, byval bytes as ubyte ptr )
	if( bytes = 0 ) then
		exit sub
	end if

	bytes[0] = cubyte( value and 255 )
	bytes[1] = cubyte( ( value shr 8 ) and 255 )
	bytes[2] = cubyte( ( value shr 16 ) and 255 )
	bytes[3] = cubyte( ( value shr 24 ) and 255 )
end sub

private function FbNetDecodeUInt32LE( byval bytes as ubyte ptr ) as ulong
	if( bytes = 0 ) then
		return 0
	end if

	return culng( bytes[0] ) or _
		( culng( bytes[1] ) shl 8 ) or _
		( culng( bytes[2] ) shl 16 ) or _
		( culng( bytes[3] ) shl 24 )
end function

private sub FbNetEncodeInt32LE( byval value as long, byval bytes as ubyte ptr )
	FbNetEncodeUInt32LE( culng( value ), bytes )
end sub

private function FbNetDecodeInt32LE( byval bytes as ubyte ptr ) as long
	return clng( FbNetDecodeUInt32LE( bytes ) )
end function

private sub FbNetEncodeUInt64LE( byval value as ulongint, byval bytes as ubyte ptr )
	dim as integer i

	if( bytes = 0 ) then
		exit sub
	end if

	for i = 0 to FB_NETWIRE_INT64_BYTES - 1
		bytes[i] = cubyte( value and 255 )
		value = value shr 8
	next i
end sub

private function FbNetDecodeUInt64LE( byval bytes as ubyte ptr ) as ulongint
	dim as integer i
	dim as ulongint value = 0

	if( bytes = 0 ) then
		return 0
	end if

	for i = FB_NETWIRE_INT64_BYTES - 1 to 0 step -1
		value = ( value shl 8 ) or culngint( bytes[i] )
	next i

	return value
end function

private sub FbNetEncodeInt64LE( byval value as longint, byval bytes as ubyte ptr )
	FbNetEncodeUInt64LE( culngint( value ), bytes )
end sub

private function FbNetDecodeInt64LE( byval bytes as ubyte ptr ) as longint
	return clngint( FbNetDecodeUInt64LE( bytes ) )
end function

private sub FbNetEncodeSingleLE( byval value as single, byval bytes as ubyte ptr )
	dim as single tmp = value
	dim as ubyte ptr src = cptr( ubyte ptr, @tmp )

	if( bytes = 0 ) then
		exit sub
	end if

	if( FbNetWireIsLittleEndian() ) then
		FbNetWireCopyBytes( bytes, src, FB_NETWIRE_SINGLE_BYTES )
	else
		FbNetWireCopyBytesReversed( bytes, src, FB_NETWIRE_SINGLE_BYTES )
	end if
end sub

private function FbNetDecodeSingleLE( byval bytes as ubyte ptr ) as single
	dim as single value
	dim as ubyte ptr dst = cptr( ubyte ptr, @value )

	if( bytes = 0 ) then
		return 0.0
	end if

	if( FbNetWireIsLittleEndian() ) then
		FbNetWireCopyBytes( dst, bytes, FB_NETWIRE_SINGLE_BYTES )
	else
		FbNetWireCopyBytesReversed( dst, bytes, FB_NETWIRE_SINGLE_BYTES )
	end if

	return value
end function

private sub FbNetEncodeDoubleLE( byval value as double, byval bytes as ubyte ptr )
	dim as double tmp = value
	dim as ubyte ptr src = cptr( ubyte ptr, @tmp )

	if( bytes = 0 ) then
		exit sub
	end if

	if( FbNetWireIsLittleEndian() ) then
		FbNetWireCopyBytes( bytes, src, FB_NETWIRE_DOUBLE_BYTES )
	else
		FbNetWireCopyBytesReversed( bytes, src, FB_NETWIRE_DOUBLE_BYTES )
	end if
end sub

private function FbNetDecodeDoubleLE( byval bytes as ubyte ptr ) as double
	dim as double value
	dim as ubyte ptr dst = cptr( ubyte ptr, @value )

	if( bytes = 0 ) then
		return 0.0
	end if

	if( FbNetWireIsLittleEndian() ) then
		FbNetWireCopyBytes( dst, bytes, FB_NETWIRE_DOUBLE_BYTES )
	else
		FbNetWireCopyBytesReversed( dst, bytes, FB_NETWIRE_DOUBLE_BYTES )
	end if

	return value
end function

private function FbNetPutBytes( byval fileno as integer, byval bytes as ubyte ptr, byval byte_count as integer ) as integer
	dim as integer i

	if( fileno <= 0 orelse bytes = 0 orelse byte_count < 0 ) then
		return 0
	end if

	for i = 0 to byte_count - 1
		if( put( #fileno, , bytes[i] ) <> 0 ) then
			return 0
		end if
	next i

	return 1
end function

private function FbNetGetBytes( byval fileno as integer, byval bytes as ubyte ptr, byval byte_count as integer ) as integer
	dim as integer i

	if( fileno <= 0 orelse bytes = 0 orelse byte_count < 0 ) then
		return 0
	end if

	for i = 0 to byte_count - 1
		if( get( #fileno, , bytes[i] ) <> 0 ) then
			return 0
		end if
	next i

	return 1
end function

private function FbNetPutInt16LE( byval fileno as integer, byval value as short ) as integer
	dim as ubyte bytes( 0 to FB_NETWIRE_INT16_BYTES - 1 )

	FbNetEncodeInt16LE( value, @bytes(0) )
	return FbNetPutBytes( fileno, @bytes(0), FB_NETWIRE_INT16_BYTES )
end function

private function FbNetGetInt16LE( byval fileno as integer, byref value as short ) as integer
	dim as ubyte bytes( 0 to FB_NETWIRE_INT16_BYTES - 1 )

	if( FbNetGetBytes( fileno, @bytes(0), FB_NETWIRE_INT16_BYTES ) = 0 ) then
		return 0
	end if

	value = FbNetDecodeInt16LE( @bytes(0) )
	return 1
end function

private function FbNetPutInt32LE( byval fileno as integer, byval value as long ) as integer
	dim as ubyte bytes( 0 to FB_NETWIRE_INT32_BYTES - 1 )

	FbNetEncodeInt32LE( value, @bytes(0) )
	return FbNetPutBytes( fileno, @bytes(0), FB_NETWIRE_INT32_BYTES )
end function

private function FbNetGetInt32LE( byval fileno as integer, byref value as long ) as integer
	dim as ubyte bytes( 0 to FB_NETWIRE_INT32_BYTES - 1 )

	if( FbNetGetBytes( fileno, @bytes(0), FB_NETWIRE_INT32_BYTES ) = 0 ) then
		return 0
	end if

	value = FbNetDecodeInt32LE( @bytes(0) )
	return 1
end function

private function FbNetPutInt64LE( byval fileno as integer, byval value as longint ) as integer
	dim as ubyte bytes( 0 to FB_NETWIRE_INT64_BYTES - 1 )

	FbNetEncodeInt64LE( value, @bytes(0) )
	return FbNetPutBytes( fileno, @bytes(0), FB_NETWIRE_INT64_BYTES )
end function

private function FbNetGetInt64LE( byval fileno as integer, byref value as longint ) as integer
	dim as ubyte bytes( 0 to FB_NETWIRE_INT64_BYTES - 1 )

	if( FbNetGetBytes( fileno, @bytes(0), FB_NETWIRE_INT64_BYTES ) = 0 ) then
		return 0
	end if

	value = FbNetDecodeInt64LE( @bytes(0) )
	return 1
end function

private function FbNetPutSingleLE( byval fileno as integer, byval value as single ) as integer
	dim as ubyte bytes( 0 to FB_NETWIRE_SINGLE_BYTES - 1 )

	FbNetEncodeSingleLE( value, @bytes(0) )
	return FbNetPutBytes( fileno, @bytes(0), FB_NETWIRE_SINGLE_BYTES )
end function

private function FbNetGetSingleLE( byval fileno as integer, byref value as single ) as integer
	dim as ubyte bytes( 0 to FB_NETWIRE_SINGLE_BYTES - 1 )

	if( FbNetGetBytes( fileno, @bytes(0), FB_NETWIRE_SINGLE_BYTES ) = 0 ) then
		return 0
	end if

	value = FbNetDecodeSingleLE( @bytes(0) )
	return 1
end function

private function FbNetPutDoubleLE( byval fileno as integer, byval value as double ) as integer
	dim as ubyte bytes( 0 to FB_NETWIRE_DOUBLE_BYTES - 1 )

	FbNetEncodeDoubleLE( value, @bytes(0) )
	return FbNetPutBytes( fileno, @bytes(0), FB_NETWIRE_DOUBLE_BYTES )
end function

private function FbNetGetDoubleLE( byval fileno as integer, byref value as double ) as integer
	dim as ubyte bytes( 0 to FB_NETWIRE_DOUBLE_BYTES - 1 )

	if( FbNetGetBytes( fileno, @bytes(0), FB_NETWIRE_DOUBLE_BYTES ) = 0 ) then
		return 0
	end if

	value = FbNetDecodeDoubleLE( @bytes(0) )
	return 1
end function

private function FbNetPutStringLE( byval fileno as integer, byref text as const string, byval max_bytes as long ) as integer
	dim as string payload = text
	dim as long byte_count

	if( max_bytes < 0 ) then
		return 0
	end if

	if( len( payload ) > max_bytes ) then
		payload = left$( payload, max_bytes )
	end if

	byte_count = clng( len( payload ) )
	if( FbNetPutInt32LE( fileno, byte_count ) = 0 ) then
		return 0
	end if

	if( byte_count = 0 ) then
		return 1
	end if

	return FbNetPutBytes( fileno, cptr( ubyte ptr, strptr( payload ) ), byte_count )
end function

private function FbNetGetStringLE( byval fileno as integer, byref text as string, byval max_bytes as long ) as integer
	dim as long byte_count

	if( max_bytes < 0 ) then
		return 0
	end if

	if( FbNetGetInt32LE( fileno, byte_count ) = 0 ) then
		return 0
	end if

	if( byte_count < 0 orelse byte_count > max_bytes ) then
		return 0
	end if

	if( byte_count = 0 ) then
		text = ""
		return 1
	end if

	text = space$( byte_count )
	return FbNetGetBytes( fileno, cptr( ubyte ptr, strptr( text ) ), byte_count )
end function

#endif

'' end of fbnetwire.bi
