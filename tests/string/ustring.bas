'' Project: FreeBASIC tests
'' File: ustring.bas
'' Purpose: Verify the built-in UTF-8 type and its standard string operations.
'' Responsibilities: Exercise scalars, ownership, conversions, and file I/O.
'' This file intentionally does NOT contain runtime implementations.

#include "fbcunit.bi"
#include "fbc-int/symbol.bi"

const ustringWideData = wchr(&hE9, &h4E2D)
ustring_data:
data "A", ustringWideData, !"A\000B", 123

SUITE( fbc_tests.string_.ustring_ )

	type Message
		text as ustring
	end type

	private function greeting( ) as ustring
		return uchr(65, &hE9, &h4E2D, &h1F600)
	end function

	private sub appendText( byref text as ustring )
		text += uchr(&h1F642)
	end sub

	private sub changeCopy( byval text as ustring )
		text = "changed"
	end sub

	private sub writeBytes( byref text as string )
		text = chr(&hFF) + "Z"
	end sub

	dim shared as integer indexCalls

	private function nextIndex( ) as integer
		indexCalls += 1
		return 1
	end function

	private function pick overload( byref text as const string ) as integer
		return 1
	end function

	private function pick overload( byref text as const ustring ) as integer
		return 2
	end function

	TEST( construction_and_length )
		dim as ustring empty, text = greeting()
		CU_ASSERT_EQUAL( len(empty), 0 )
		CU_ASSERT_EQUAL( len(text), 4 )
		CU_ASSERT_EQUAL( sizeof(ustring), sizeof(string) )
		dim as string bytes = text
		CU_ASSERT_EQUAL( len(bytes), 10 )
		CU_ASSERT_EQUAL( pick(text), 2 )
		CU_ASSERT_EQUAL( pick(bytes), 1 )
		CU_ASSERT_EQUAL( len(ustring("A" + chr(&hC3, &hA9))), 2 )
		CU_ASSERT_EQUAL( len(ustring(1234)), 4 )
		CU_ASSERT_EQUAL( len(cast(ustring, "abc")), 3 )
		CU_ASSERT_EQUAL( len(cast(string, text)), 10 )
		CU_ASSERT( str(text) = text )
		CU_ASSERT( isTypeUstring(text) )
		const as ustring constantText = "A" + chr(&hC3, &hA9)
		CU_ASSERT_EQUAL( len(constantText), 2 )
		CU_ASSERT_EQUAL( asc(constantText, 2), &hE9 )
		dim as const ustring readOnly = text
		CU_ASSERT( readOnly = text )
	END_TEST

	TEST( asc_and_chr )
		dim as ustring text = greeting()
		CU_ASSERT_EQUAL( asc(text), 65 )
		CU_ASSERT_EQUAL( asc(text, 2), &hE9 )
		CU_ASSERT_EQUAL( asc(text, 3), &h4E2D )
		CU_ASSERT_EQUAL( asc(text, 4), &h1F600 )
		CU_ASSERT_EQUAL( asc(text, 0), 0 )
		CU_ASSERT_EQUAL( asc(text, -1), 0 )
		CU_ASSERT_EQUAL( asc(text, 5), 0 )
		CU_ASSERT_EQUAL( asc(uchr(&h10FFFF)), &h10FFFF )
		CU_ASSERT_EQUAL( asc(uchr(&hD800)), &hFFFD )
		CU_ASSERT_EQUAL( asc(uchr(&h110000)), &hFFFD )
		CU_ASSERT_EQUAL( len(uchr(0, 65, 0)), 3 )
		CU_ASSERT_EQUAL( asc(uchr(0, 65, 0), 2), 65 )
		CU_ASSERT_EQUAL( asc(uchr(-1)), &hFFFD )
#ifdef __FB_64BIT__
		CU_ASSERT_EQUAL( asc(uchr(&h100000041ull)), &hFFFD )
#endif
	END_TEST

	TEST( slicing )
		dim as ustring text = greeting()
		CU_ASSERT( left(text, 2) = uchr(65, &hE9) )
		CU_ASSERT( right(text, 2) = uchr(&h4E2D, &h1F600) )
		CU_ASSERT( mid(text, 2, 2) = uchr(&hE9, &h4E2D) )
		CU_ASSERT( mid(text, 3) = uchr(&h4E2D, &h1F600) )
		CU_ASSERT_EQUAL( len(left(text, -1)), 0 )
		CU_ASSERT_EQUAL( len(right(text, 0)), 0 )
		CU_ASSERT_EQUAL( len(mid(text, 0, 1)), 0 )
		CU_ASSERT_EQUAL( len(mid(text, 5)), 0 )
		CU_ASSERT( left(text, 999) = text )
		CU_ASSERT( right(text, 999) = text )
		CU_ASSERT( mid(text, 2, 999) = right(text, 3) )
		CU_ASSERT_EQUAL( len(left(text, 2)), 2 )
		CU_ASSERT_EQUAL( asc(right(text, 1)), &h1F600 )
	END_TEST

	TEST( indexing_and_mid_assignment )
		dim as ustring text = greeting()
		CU_ASSERT_EQUAL( text[0], 65 )
		CU_ASSERT_EQUAL( text[3], &h1F600 )
		CU_ASSERT_EQUAL( text[-1], 0 )
		CU_ASSERT_EQUAL( text[99], 0 )
		text[0] = &h1F642
		text[1] = 66
		text[1] += 1
		CU_ASSERT( text = uchr(&h1F642, 67, &h4E2D, &h1F600) )
		CU_ASSERT_EQUAL( len(text), 4 )
		mid(text, 2, 2) = uchr(&hE9, &h1F680)
		CU_ASSERT( text = uchr(&h1F642, &hE9, &h1F680, &h1F600) )
		mid(text, 3, 9) = "Z"
		CU_ASSERT( text = uchr(&h1F642, &hE9, 90, &h1F600) )
		mid(text, 2, 2) = text
		CU_ASSERT( text = uchr(&h1F642, &h1F642, &hE9, &h1F600) )
		mid(text, 0) = "bad"
		mid(text, 99) = "bad"
		CU_ASSERT_EQUAL( len(text), 4 )
		CU_ASSERT_EQUAL( greeting()[3], &h1F600 )
		for i as integer = 1 to 300
			mid(greeting(), 1, 1) = uchr(&h1F642)
		next
		indexCalls = 0
		text[nextIndex()] += 1
		CU_ASSERT_EQUAL( indexCalls, 1 )
		text[0] = -1
		CU_ASSERT_EQUAL( text[0], &hFFFD )
		dim as ustring saved = text
		text[-1] = 65
		CU_ASSERT_EQUAL( err, 1 )
		CU_ASSERT( text = saved )
		err = 0
		text[99] = 65
		CU_ASSERT_EQUAL( err, 1 )
		CU_ASSERT( text = saved )
		err = 0
	END_TEST

	TEST( search )
		dim as ustring text = uchr(&hE9, &h4E2D, &h1F600, &hE9, &h4E2D)
		CU_ASSERT_EQUAL( instr(text, uchr(&h4E2D)), 2 )
		CU_ASSERT_EQUAL( instr(3, text, uchr(&h4E2D)), 5 )
		CU_ASSERT_EQUAL( instr(text, uchr(&hE9, &h4E2D)), 1 )
		CU_ASSERT_EQUAL( instr(text, any uchr(&h1F680, &h1F600)), 3 )
		CU_ASSERT_EQUAL( instr(4, text, any uchr(&h4E2D)), 5 )
		CU_ASSERT_EQUAL( instr(text, ""), 0 )
		CU_ASSERT_EQUAL( instr(0, text, "a"), 0 )
		CU_ASSERT_EQUAL( instrrev(text, uchr(&hE9)), 4 )
		CU_ASSERT_EQUAL( instrrev(text, uchr(&hE9), 3), 1 )
		CU_ASSERT_EQUAL( instrrev(text, any uchr(&h1F600, &h4E2D)), 5 )
		CU_ASSERT_EQUAL( instrrev(text, any uchr(&h1F600, &h4E2D), 4), 3 )
		CU_ASSERT_EQUAL( instrrev(text, "", -1), 0 )
		CU_ASSERT_EQUAL( instrrev(text, "a", 99), 0 )
	END_TEST

	TEST( trimming )
		dim as ustring body = greeting(), text = "  " + body + "  "
		CU_ASSERT( trim(text) = body )
		CU_ASSERT( ltrim(text) = body + "  " )
		CU_ASSERT( rtrim(text) = "  " + body )
		dim as ustring pad = uchr(&hE9, &h4E2D)
		text = pad + pad + body + pad
		CU_ASSERT( trim(text, pad) = body )
		CU_ASSERT( ltrim(text, pad) = body + pad )
		CU_ASSERT( rtrim(text, pad) = pad + pad + body )
		text = pad + "X" + pad
		CU_ASSERT( trim(text, any pad) = "X" )
		CU_ASSERT( ltrim(text, any pad) = "X" + pad )
		CU_ASSERT( rtrim(text, any pad) = pad + "X" )
		CU_ASSERT( trim(text, "") = text )
		CU_ASSERT_EQUAL( len(trim(ustring("   "))), 0 )
		CU_ASSERT_EQUAL( len(trim(ustring(""))), 0 )
	END_TEST

	TEST( unicode_case )
		CU_ASSERT( ucase(uchr(&hDF)) = "SS" )
		CU_ASSERT( ucase(uchr(&hFB03)) = "FFI" )
		CU_ASSERT( lcase(uchr(&h130)) = uchr(105, &h307) )
		CU_ASSERT( lcase(uchr(&h39F, &h3A3)) = uchr(&h3BF, &h3C2) )
		CU_ASSERT( lcase(uchr(&h39F, &h3A3, &h391)) = uchr(&h3BF, &h3C3, &h3B1) )
		CU_ASSERT( lcase(uchr(&h39F, &h3A3, &h301)) = uchr(&h3BF, &h3C2, &h301) )
		CU_ASSERT( lcase(uchr(&h10400)) = uchr(&h10428) )
		CU_ASSERT( ucase(uchr(&hE9, 97), 1) = uchr(&hE9, 65) )
		CU_ASSERT( lcase(uchr(&hC9, 65), 1) = uchr(&hC9, 97) )
		CU_ASSERT_EQUAL( len(ucase(uchr(&hDF))), 2 )
		CU_ASSERT_EQUAL( len(lcase(uchr(&h130))), 2 )
	END_TEST

	TEST( repetition_and_alignment )
		CU_ASSERT( ustring(3, &h1F600) = uchr(&h1F600, &h1F600, &h1F600) )
		CU_ASSERT( ustring(2, uchr(&hE9, 65)) = uchr(&hE9, &hE9) )
		CU_ASSERT( string(2, uchr(&h1F600)) = uchr(&h1F600, &h1F600) )
		CU_ASSERT_EQUAL( len(string(3, uchr(&h1F600))), 3 )
		CU_ASSERT_EQUAL( len(ustring(-1, 65)), 0 )
		CU_ASSERT_EQUAL( len(ustring(3, "")), 0 )
		CU_ASSERT_EQUAL( asc(ustring(1, -1)), &hFFFD )
#ifdef __FB_64BIT__
		CU_ASSERT_EQUAL( asc(ustring(1, &h100000041ull)), &hFFFD )
#endif
		dim as ustring text = ustring(4, &h1F600)
		lset text = uchr(&hE9)
		CU_ASSERT( text = uchr(&hE9) + "   " )
		rset text = uchr(&h4E2D)
		CU_ASSERT( text = "   " + uchr(&h4E2D) )
		lset text = greeting() + "too long"
		CU_ASSERT( text = greeting() )
	END_TEST

	TEST( conversion_and_validation )
		dim as ustring text = chr(&hC0, &h80) + "A"
		CU_ASSERT( text = uchr(&hFFFD, &hFFFD, 65) )
		text = chr(&hE2, &h82) + "A"
		CU_ASSERT( text = uchr(&hFFFD, 65) )
		text = chr(&hED, &hA0, &h80)
		CU_ASSERT( text = uchr(&hFFFD, &hFFFD, &hFFFD) )
		text = chr(&hF4, &h90, &h80, &h80)
		CU_ASSERT_EQUAL( len(text), 4 )
		text = wchr(&hE9, &h4E2D)
		CU_ASSERT( text = uchr(&hE9, &h4E2D) )
		dim as wstring * 16 wide = wstr(text)
		CU_ASSERT_EQUAL( asc(wide, 1), &hE9 )
		CU_ASSERT_EQUAL( asc(wide, 2), &h4E2D )
		wide = text
		CU_ASSERT_EQUAL( asc(wide, 1), &hE9 )
		CU_ASSERT_EQUAL( asc(wide, 2), &h4E2D )
		CU_ASSERT( text = wide )
		CU_ASSERT_EQUAL( len(text + wide), 4 )
		CU_ASSERT( uchr(&hFFFD) = chr(&hFF) )
		text = "123.5"
		CU_ASSERT_EQUAL( val(text), 123.5 )
		CU_ASSERT_EQUAL( valint(text), 123 )
		CU_ASSERT_EQUAL( vallng(text), 123 )
		CU_ASSERT_EQUAL( valuint(text), 123 )
		CU_ASSERT_EQUAL( valulng(text), 123 )
	END_TEST

	TEST( ownership_arrays_and_fields )
		dim as ustring text = greeting(), copy = text
		text = text
		text += text
		CU_ASSERT_EQUAL( len(text), 8 )
		CU_ASSERT_EQUAL( len(copy), 4 )
		changeCopy(copy)
		CU_ASSERT( copy = greeting() )
		appendText(copy)
		CU_ASSERT_EQUAL( len(copy), 5 )
		writeBytes(copy)
		CU_ASSERT( copy = uchr(&hFFFD, 90) )
		dim as string bytes = "abc"
		appendText(bytes)
		CU_ASSERT_EQUAL( len(bytes), 7 )
		bytes = chr(&hFF)
		copy = "valid"
		swap copy, bytes
		CU_ASSERT( copy = uchr(&hFFFD) )
		CU_ASSERT( bytes = "valid" )
		dim as ustring items(0 to 1)
		items(0) = greeting()
		items(1) = uchr(&h1F680)
		indexCalls = 0
		writeBytes(items(nextIndex()))
		CU_ASSERT_EQUAL( indexCalls, 1 )
		CU_ASSERT( items(1) = uchr(&hFFFD, 90) )
		items(1) = uchr(&h1F680)
		swap items(0), items(1)
		CU_ASSERT_EQUAL( len(items(0)), 1 )
		CU_ASSERT_EQUAL( len(items(1)), 4 )
		dim as Message a, b
		a.text = greeting()
		b = a
		a.text = "changed"
		CU_ASSERT( b.text = greeting() )
		redim as ustring dynamicItems(0 to 1)
		dynamicItems(0) = greeting()
		redim preserve dynamicItems(0 to 4)
		CU_ASSERT( dynamicItems(0) = greeting() )
		erase dynamicItems
		dim as ustring ptr allocated = new ustring(greeting())
		CU_ASSERT_EQUAL( len(*allocated), 4 )
		delete allocated
		allocated = new ustring[2]
		allocated[1] = greeting()
		CU_ASSERT_EQUAL( len(allocated[1]), 4 )
		delete[] allocated
		CU_ASSERT_EQUAL( len(iif(len(copy) > 0, greeting(), uchr(65))), 4 )
		for i as integer = 1 to 300
			text = mid(greeting() + uchr(&h1F642), 2)
			CU_ASSERT_EQUAL( len(text), 4 )
		next
		select case greeting()
		case uchr(65, &hE9, &h4E2D, &h1F600)
			CU_ASSERT( TRUE )
		case else
			CU_ASSERT( FALSE )
		end select
	END_TEST

	TEST( file_io )
		const filename = "ustring-test.tmp"
		dim as ustring text = greeting(), readback
		dim as integer handle = freefile()
		CU_ASSERT_EQUAL( open(filename for output as #handle), 0 )
		print #handle, text
		write #handle, text
		close #handle
		CU_ASSERT_EQUAL( open(filename for input as #handle), 0 )
		line input #handle, readback
		CU_ASSERT( readback = text )
		input #handle, readback
		CU_ASSERT( readback = text )
		close #handle
		CU_ASSERT_EQUAL( open(filename for binary as #handle), 0 )
		put #handle, 1, text
		readback = ustring(4, &h1F600)
		get #handle, 1, readback
		CU_ASSERT( left(readback, 4) = text )
		dim as integer bytesRead
		readback = "          "
		CU_ASSERT_EQUAL( get(#handle, 1, readback, , bytesRead), 0 )
		CU_ASSERT_EQUAL( bytesRead, 10 )
		CU_ASSERT( readback = text )
		close #handle
		CU_ASSERT_EQUAL( open(filename for output as #handle), 0 )
		print #handle, chr(&hFF)
		write #handle, chr(&hE2, &h82) + "A"
		close #handle
		CU_ASSERT_EQUAL( open(filename for input as #handle), 0 )
		line input #handle, readback
		CU_ASSERT( readback = uchr(&hFFFD) )
		input #handle, readback
		CU_ASSERT( readback = uchr(&hFFFD, 65) )
		close #handle
		kill filename
	END_TEST

	TEST( data_read )
		dim as ustring text
		restore ustring_data
		read text
		CU_ASSERT( text = "A" )
		read text
		CU_ASSERT( text = uchr(&hE9, &h4E2D) )
		read text
		CU_ASSERT( text = uchr(65, 0, 66) )
		read text
		CU_ASSERT( text = "123" )
	END_TEST

	TEST( encoded_file_io )
		const filename = "ustring-encoded-test.tmp"
		dim as string encodings(0 to 2) = { "utf-8", "utf-16", "utf-32" }
		dim as ustring text = greeting(), readback
		dim as integer handle = freefile(), number
		for i as integer = 0 to 2
			CU_ASSERT_EQUAL( open(filename for output encoding encodings(i) as #handle), 0 )
			print #handle, text
			write #handle, text, 123
			close #handle
			CU_ASSERT_EQUAL( open(filename for input encoding encodings(i) as #handle), 0 )
			line input #handle, readback
			CU_ASSERT( readback = text )
			input #handle, readback, number
			CU_ASSERT( readback = text )
			CU_ASSERT_EQUAL( number, 123 )
			close #handle
			kill filename
		next
	END_TEST

END_SUITE

'' end of ustring.bas
