'' Project: FreeBASIC text type tests
'' File: text-types.bas
'' Purpose: Check shared STRING, WSTRING, and USTRING language/runtime paths.
'' Responsibilities: Validate overloads, Unicode preservation, and text APIs.
'' This file intentionally does NOT exercise external C library declarations.

#include "fbcunit.bi"
#include "string.bi"
#include "datetime.bi"
#include "file.bi"
#include "dir.bi"

SUITE( fbc_tests.string_.text_types )

	TEST( optional_helpers )
		dim as string bytes = "aab"
		dim as wstring * 32 wide = wchr(&hE9, &h4E2D)
		dim as ustring utf8 = uchr(&hE9, &h4E2D, &h1F600)
		CU_ASSERT( StrReverse(bytes) = "baa" )
		CU_ASSERT( StrReverse(wide) = wchr(&h4E2D, &hE9) )
		CU_ASSERT( StrReverse(utf8) = uchr(&h1F600, &h4E2D, &hE9) )
		CU_ASSERT_EQUAL( len(StrReverse(utf8)), 3 )
		CU_ASSERT( Replace(utf8, uchr(&h4E2D), "X") = uchr(&hE9, 88, &h1F600) )
		CU_ASSERT( Replace(wide, wchr(&h4E2D), "X") = wchr(&hE9, 88) )
		CU_ASSERT( Replace(utf8, "", "X", 2) = uchr(&h4E2D, &h1F600) )
		CU_ASSERT( Replace(utf8, utf8, "") = "" )
		CU_ASSERT( Replace(ustring("Stra") + uchr(&hDF) + "e", "STRASSE", "X", 1, -1, fbTextCompare) = "X" )
		CU_ASSERT( Replace(uchr(&hDF), "s", "X", 1, -1, fbTextCompare) = uchr(&hDF) )
		CU_ASSERT_EQUAL( StrComp(uchr(&hDF), "SS", fbTextCompare), 0 )
		CU_ASSERT_EQUAL( StrComp(wchr(&hDF), ustring("SS"), fbTextCompare), 0 )
		CU_ASSERT_EQUAL( StrComp(uchr(&h130), uchr(105, &h307), fbTextCompare), 0 )
		CU_ASSERT( Format(12.5, ustring("0.0") + uchr(&hE9)) = "12.5" + uchr(&hE9) )
		CU_ASSERT( Format(12.5, wstr("0.0") + wchr(&hE9)) = wstr("12.5") + wchr(&hE9) )
		CU_ASSERT_EQUAL( len(Format(12.5, ustring("0.0") + uchr(&hE9))), 5 )
		CU_ASSERT( string(2, wide) = wchr(&hE9, &hE9) )
		swap wide, utf8
		CU_ASSERT( wide = wstr(uchr(&hE9, &h4E2D, &h1F600)) )
		CU_ASSERT( utf8 = uchr(&hE9, &h4E2D) )
	END_TEST

	TEST( system_and_optional_arguments )
		dim as wstring * 128 wide = wstr("FBC_TEXT_TYPES_TEST=") + wchr(&hE9, &h4E2D)
		dim as ustring utf8 = "FBC_TEXT_TYPES_TEST=" + uchr(&hE9, &h4E2D)
		CU_ASSERT_EQUAL( setenviron(wide), 0 )
		CU_ASSERT( ustring(environ(wstr("FBC_TEXT_TYPES_TEST"))) = uchr(&hE9, &h4E2D) )
		CU_ASSERT_EQUAL( setenviron(utf8), 0 )
		CU_ASSERT( ustring(environ(ustring("FBC_TEXT_TYPES_TEST"))) = uchr(&hE9, &h4E2D) )
		CU_ASSERT_EQUAL( DateValue(wstr("1/2/2020")), DateValue(ustring("1/2/2020")) )
		CU_ASSERT( IsDate(wstr("1/2/2020")) )
		CU_ASSERT( IsDate(ustring("1/2/2020")) )
		CU_ASSERT_EQUAL( TimeValue(wstr("12:30:00")), TimeValue(ustring("12:30:00")) )
		CU_ASSERT_EQUAL( DateAdd(wstr("d"), 1, 100), DateAdd(ustring("d"), 1, 100) )
		CU_ASSERT_EQUAL( DatePart(wstr("yyyy"), 100), DatePart(ustring("yyyy"), 100) )
		CU_ASSERT_EQUAL( DateDiff(wstr("d"), 100, 101), DateDiff(ustring("d"), 100, 101) )
	END_TEST

	TEST( paths_and_file_helpers )
		dim as wstring * 128 wide = wstr("text-types-") + wchr(&hE9, &h4E2D) + wstr(".tmp")
		dim as ustring utf8 = "text-types-" + uchr(&hE9, &h4E2D) + ".tmp"
		dim as integer handle = freefile()
		CU_ASSERT_EQUAL( open(wide for output as #handle), 0 )
		print #handle, "abc"
		close #handle
		CU_ASSERT( FileExists(utf8) )
		CU_ASSERT( FileExists(wide) )
		CU_ASSERT_EQUAL( FileLen(utf8), FileLen(wide) )
		CU_ASSERT_EQUAL( GetAttr(utf8), GetAttr(wide) )
		CU_ASSERT( len(dir(wide)) > 0 )
		CU_ASSERT( len(dir(utf8)) > 0 )
		CU_ASSERT_EQUAL( kill(utf8), 0 )
	END_TEST

	TEST( formatted_output )
		const filename = "text-types-using.tmp"
		dim as ustring utf8 = uchr(&hE9, &h4E2D, &h1F600), lineText
		dim as wstring * 32 wide = wstr(utf8)
		dim as integer handle = freefile()
		CU_ASSERT_EQUAL( open(filename for output encoding "utf-8" as #handle), 0 )
		print #handle, using "!"; utf8
		print #handle, using "!"; wide
		print #handle, using "\ \"; utf8
		print #handle, using ustring("&") + uchr(&hE9); utf8
		print #handle, using wstr("&") + wchr(&hE9); wide
		close #handle
		CU_ASSERT_EQUAL( open(filename for input encoding "utf-8" as #handle), 0 )
		line input #handle, lineText
		CU_ASSERT( lineText = uchr(&hE9) )
		line input #handle, lineText
		CU_ASSERT( lineText = uchr(&hE9) )
		line input #handle, lineText
		CU_ASSERT( lineText = utf8 )
		line input #handle, lineText
		CU_ASSERT( lineText = utf8 + uchr(&hE9) )
		line input #handle, lineText
		CU_ASSERT( lineText = utf8 + uchr(&hE9) )
		close #handle
		kill filename
	END_TEST

END_SUITE

'' end of text-types.bas
