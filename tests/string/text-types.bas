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
#include "fbc-int/string.bi"
#include "fbnetwire.bi"

SUITE( fbc_tests.string_.text_types )

	TEST( truncate_in_place )
		dim as string bytes = "abc"
		dim as wstring * 32 wide = wchr(&hE9, &h1F600, 65)
		dim as ustring utf8 = uchr(&hE9, &h1F600, 65)
		dim as any ptr buffer = strptr(utf8)
		CU_ASSERT( ustring(wide) = utf8 )
		FBC.LeftSelf(bytes, 2)
		FBC.LeftSelf(wide, 2)
		FBC.LeftSelf(utf8, 2)
		CU_ASSERT( bytes = "ab" )
		CU_ASSERT( wide = wchr(&hE9, &h1F600) )
		CU_ASSERT( utf8 = uchr(&hE9, &h1F600) )
		CU_ASSERT( strptr(utf8) = buffer )
		FBC.LeftSelf(utf8, -1)
		FBC.LeftSelf(wide, -1)
		CU_ASSERT_EQUAL( len(utf8), 2 )
		CU_ASSERT( wide = wchr(&hE9, &h1F600) )
		FBC.LeftSelf(utf8, 100)
		CU_ASSERT_EQUAL( len(utf8), 2 )
		FBC.LeftSelf(utf8, 0)
		FBC.LeftSelf(wide, 0)
		CU_ASSERT_EQUAL( len(utf8), 0 )
		CU_ASSERT_EQUAL( len(wide), 0 )
	END_TEST

	TEST( unicode_wire_strings )
		const filename = "text-types-wire.tmp"
		dim as integer h = freefile()
		dim as ustring utf8 = uchr(&hE9, &h1F600, 65), received
		dim as wstring * 32 wide = wstr(utf8), wideResult = "unchanged"
		dim as string bytes = cast(string, utf8), byteResult
		CU_ASSERT_EQUAL( open(filename for binary as #h), 0 )
		CU_ASSERT( FbNetPutStringLE(h, bytes, 7) )
		CU_ASSERT( FbNetPutStringLE(h, wide, 7) )
		CU_ASSERT( FbNetPutStringLE(h, utf8, 5) )
		CU_ASSERT( FbNetPutStringLE(h, utf8, 7) )
		CU_ASSERT( FbNetPutStringLE(h, utf8, 7) )
		CU_ASSERT( FbNetPutStringLE(h, chr(&hC0), 1) )
		seek #h, 1
		CU_ASSERT( FbNetGetStringLE(h, received, 7) )
		CU_ASSERT( received = utf8 )
		CU_ASSERT( FbNetGetStringLE(h, byteResult, 7) )
		CU_ASSERT( byteResult = bytes )
		CU_ASSERT( FbNetGetStringLE(h, received, 7) )
		CU_ASSERT( received = uchr(&hE9) )
		CU_ASSERT( FbNetGetStringLE(h, wideResult, 7, 32) )
		CU_ASSERT( wideResult = wide )
		wideResult = "unchanged"
		CU_ASSERT_EQUAL( FbNetGetStringLE(h, wideResult, 7, 2), 0 )
		CU_ASSERT( wideResult = "unchanged" )
		CU_ASSERT( FbNetGetStringLE(h, received, 7) )
		CU_ASSERT( received = uchr(&hFFFD) )
		close #h
		kill filename
	END_TEST

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

	TEST( every_mixed_optional_overload )
		dim as ustring sourceU = uchr(65, &hE9, &h1F600)
		dim as ustring patternU = uchr(&hE9), replacementU = uchr(&h4E2D)
		dim as wstring * 32 sourceW = wstr(sourceU), patternW = wstr(patternU), replacementW = wstr(replacementU)
		dim as string sourceS = cast(string, sourceU), patternS = cast(string, patternU), replacementS = cast(string, replacementU)
		dim as ustring expected = uchr(65, &h4E2D, &h1F600)

		#macro check_replacements(source, pattern)
			CU_ASSERT( ustring(Replace(source, pattern, replacementS)) = expected )
			CU_ASSERT( ustring(Replace(source, pattern, replacementW)) = expected )
			CU_ASSERT( ustring(Replace(source, pattern, replacementU)) = expected )
		#endmacro
		check_replacements(sourceS, patternS)
		check_replacements(sourceS, patternW)
		check_replacements(sourceS, patternU)
		check_replacements(sourceW, patternS)
		check_replacements(sourceW, patternW)
		check_replacements(sourceW, patternU)
		check_replacements(sourceU, patternS)
		check_replacements(sourceU, patternW)
		check_replacements(sourceU, patternU)
		#undef check_replacements

		#macro check_comparisons(source)
			CU_ASSERT_EQUAL( StrComp(source, sourceS), 0 )
			CU_ASSERT_EQUAL( StrComp(source, sourceW), 0 )
			CU_ASSERT_EQUAL( StrComp(source, sourceU), 0 )
		#endmacro
		check_comparisons(sourceS)
		check_comparisons(sourceW)
		check_comparisons(sourceU)
		#undef check_comparisons
	END_TEST

	TEST( system_and_optional_arguments )
		dim as wstring * 128 wide = wstr("FBC_TEXT_TYPES_TEST=") + wchr(&hE9, &h4E2D)
		dim as ustring utf8 = "FBC_TEXT_TYPES_TEST=" + uchr(&hE9, &h4E2D)
		CU_ASSERT_EQUAL( setenviron(wide), 0 )
		CU_ASSERT( ustring(environ(wstr("FBC_TEXT_TYPES_TEST"))) = uchr(&hE9, &h4E2D) )
		CU_ASSERT_EQUAL( setenviron(utf8), 0 )
		CU_ASSERT( ustring(environ(ustring("FBC_TEXT_TYPES_TEST"))) = uchr(&hE9, &h4E2D) )
		'' Numeric date order follows the host locale. Named months also accept
		'' English names, so these conversion checks work with any date order.
		const dateText = "January 2, 2020"
		CU_ASSERT_EQUAL( DateValue(wstr(dateText)), DateSerial(2020, 1, 2) )
		CU_ASSERT_EQUAL( DateValue(ustring(dateText)), DateSerial(2020, 1, 2) )
		CU_ASSERT( IsDate(wstr(dateText)) )
		CU_ASSERT( IsDate(ustring(dateText)) )
		'' Fractions of a day may retain x87 extended precision in one call.
		'' Allow DOUBLE rounding when comparing the two text conversion paths.
		CU_ASSERT_DOUBLE_EQUAL( TimeValue(wstr("12:30:00")), TimeValue(ustring("12:30:00")), 1.e-15 )
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
		dim as long attrStatus = SetAttr(cast(string, utf8), GetAttr(utf8))
		CU_ASSERT_EQUAL( SetAttr(wide, GetAttr(utf8)), attrStatus )
		CU_ASSERT_EQUAL( SetAttr(utf8, GetAttr(wide)), attrStatus )
		CU_ASSERT_EQUAL( FileDateTime(utf8), FileDateTime(wide) )
		dim as string byteSource = cast(string, utf8), byteCopy = "copy-" + byteSource
		dim as ustring utf8Copy = "copy-" + utf8
		dim as wstring * 128 wideCopy = wstr(utf8Copy)
		#macro check_copy(source, destination)
			CU_ASSERT_EQUAL( FileCopy(source, destination), 0 )
			CU_ASSERT_EQUAL( FileLen(destination), FileLen(utf8) )
			CU_ASSERT_EQUAL( kill(utf8Copy), 0 )
		#endmacro
		check_copy(byteSource, byteCopy)
		check_copy(byteSource, wideCopy)
		check_copy(byteSource, utf8Copy)
		check_copy(wide, byteCopy)
		check_copy(wide, wideCopy)
		check_copy(wide, utf8Copy)
		check_copy(utf8, byteCopy)
		check_copy(utf8, wideCopy)
		check_copy(utf8, utf8Copy)
		#undef check_copy
		CU_ASSERT( len(dir(wide)) > 0 )
		CU_ASSERT( len(dir(utf8)) > 0 )
		dim as ustring renamed = "renamed-" + utf8
		name wide as renamed
		CU_ASSERT( FileExists(renamed) )
		CU_ASSERT( not FileExists(utf8) )
		name renamed as wide
		CU_ASSERT( FileExists(utf8) )
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

	TEST( long_unicode_formats )
		const filename = "text-types-long-using.tmp"
		dim as integer h = freefile()
		dim as ustring prefix = ustring(800, &h4E2D), received
		CU_ASSERT_EQUAL( open(filename for output encoding "utf-16" as #h), 0 )
		print #h, using prefix + "&"; ustring("x")
		print #h, using "\" + space(3000) + "\"; uchr(&h1F600)
		close #h
		CU_ASSERT_EQUAL( open(filename for input encoding "utf-16" as #h), 0 )
		line input #h, received
		CU_ASSERT( received = prefix + "x" )
		line input #h, received
		CU_ASSERT_EQUAL( len(received), 3002 )
		CU_ASSERT( left(received, 1) = uchr(&h1F600) )
		CU_ASSERT( mid(received, 2) = space(3001) )
		close #h
		kill filename
	END_TEST

END_SUITE

'' end of text-types.bas
