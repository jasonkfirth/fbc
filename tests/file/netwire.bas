''
'' FreeBASIC portable network wire helper tests
'' --------------------------------------------
''
'' File: netwire.bas
''
'' Purpose:
''
''     Verify that fbnetwire.bi produces stable little-endian byte streams.
''
'' Responsibilities:
''
''     - check fixed-width integer byte order
''     - check Single and Double byte order
''     - check file-handle helpers for primitive values and strings
''
'' This file intentionally does NOT contain:
''
''     - TCP socket tests
''     - packet protocol tests for any application
''     - platform-specific networking setup
''

#include "fbcunit.bi"
#include "fbnetwire.bi"

SUITE( fbc_tests.file_.netwire_ )

	TEST( integer_encoding )
		dim as ubyte bytes( 0 to FB_NETWIRE_INT64_BYTES - 1 )
		dim as ulong u32
		dim as long s32
		dim as ulongint u64
		dim as longint s64

		FbNetEncodeUInt16LE( &habcd, @bytes(0) )
		CU_ASSERT( bytes(0) = &hcd )
		CU_ASSERT( bytes(1) = &hab )
		CU_ASSERT( FbNetDecodeUInt16LE( @bytes(0) ) = &habcd )

		FbNetEncodeInt16LE( -2, @bytes(0) )
		CU_ASSERT( bytes(0) = &hfe )
		CU_ASSERT( bytes(1) = &hff )
		CU_ASSERT( FbNetDecodeInt16LE( @bytes(0) ) = -2 )

		u32 = 2309737967UL
		FbNetEncodeUInt32LE( u32, @bytes(0) )
		CU_ASSERT( bytes(0) = &hef )
		CU_ASSERT( bytes(1) = &hcd )
		CU_ASSERT( bytes(2) = &hab )
		CU_ASSERT( bytes(3) = &h89 )
		CU_ASSERT( FbNetDecodeUInt32LE( @bytes(0) ) = u32 )

		s32 = -1234567
		FbNetEncodeInt32LE( s32, @bytes(0) )
		CU_ASSERT( FbNetDecodeInt32LE( @bytes(0) ) = s32 )

		u64 = &h0123456789abcdefULL
		FbNetEncodeUInt64LE( u64, @bytes(0) )
		CU_ASSERT( bytes(0) = &hef )
		CU_ASSERT( bytes(1) = &hcd )
		CU_ASSERT( bytes(2) = &hab )
		CU_ASSERT( bytes(3) = &h89 )
		CU_ASSERT( bytes(4) = &h67 )
		CU_ASSERT( bytes(5) = &h45 )
		CU_ASSERT( bytes(6) = &h23 )
		CU_ASSERT( bytes(7) = &h01 )
		CU_ASSERT( FbNetDecodeUInt64LE( @bytes(0) ) = u64 )

		s64 = -123456789012345LL
		FbNetEncodeInt64LE( s64, @bytes(0) )
		CU_ASSERT( FbNetDecodeInt64LE( @bytes(0) ) = s64 )
	END_TEST

	TEST( float_encoding )
		dim as ubyte bytes( 0 to FB_NETWIRE_DOUBLE_BYTES - 1 )

		FbNetEncodeSingleLE( 1.0, @bytes(0) )
		CU_ASSERT( bytes(0) = &h00 )
		CU_ASSERT( bytes(1) = &h00 )
		CU_ASSERT( bytes(2) = &h80 )
		CU_ASSERT( bytes(3) = &h3f )
		CU_ASSERT( FbNetDecodeSingleLE( @bytes(0) ) = 1.0 )

		FbNetEncodeDoubleLE( 1.0, @bytes(0) )
		CU_ASSERT( bytes(0) = &h00 )
		CU_ASSERT( bytes(1) = &h00 )
		CU_ASSERT( bytes(2) = &h00 )
		CU_ASSERT( bytes(3) = &h00 )
		CU_ASSERT( bytes(4) = &h00 )
		CU_ASSERT( bytes(5) = &h00 )
		CU_ASSERT( bytes(6) = &hf0 )
		CU_ASSERT( bytes(7) = &h3f )
		CU_ASSERT( FbNetDecodeDoubleLE( @bytes(0) ) = 1.0 )
	END_TEST

	TEST( file_roundtrip )
		const filename = "./file/netwire.tmp"

		dim as integer fileno
		dim as long s32
		dim as longint s64
		dim as single sng
		dim as double dbl
		dim as string text

		fileno = freefile()
		if( open( filename for binary access write as #fileno ) <> 0 ) then
			CU_FAIL( "could not create netwire test file" )
			exit sub
		end if

		CU_ASSERT( FbNetPutInt32LE( fileno, -42 ) )
		CU_ASSERT( FbNetPutInt64LE( fileno, -9876543210LL ) )
		CU_ASSERT( FbNetPutSingleLE( fileno, 2.5 ) )
		CU_ASSERT( FbNetPutDoubleLE( fileno, 1.25 ) )
		text = "hello"
		CU_ASSERT( FbNetPutStringLE( fileno, text, 64 ) )
		close #fileno

		fileno = freefile()
		if( open( filename for binary access read as #fileno ) <> 0 ) then
			CU_FAIL( "could not reopen netwire test file" )
			exit sub
		end if

		CU_ASSERT( FbNetGetInt32LE( fileno, s32 ) )
		CU_ASSERT( s32 = -42 )
		CU_ASSERT( FbNetGetInt64LE( fileno, s64 ) )
		CU_ASSERT( s64 = -9876543210LL )
		CU_ASSERT( FbNetGetSingleLE( fileno, sng ) )
		CU_ASSERT( sng = 2.5 )
		CU_ASSERT( FbNetGetDoubleLE( fileno, dbl ) )
		CU_ASSERT( dbl = 1.25 )
		CU_ASSERT( FbNetGetStringLE( fileno, text, 64 ) )
		CU_ASSERT( text = "hello" )
		close #fileno

		kill filename
	END_TEST

END_SUITE

'' end of netwire.bas
