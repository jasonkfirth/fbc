'' FreeBASIC native regex qualification
'' File: regex-runtime.bas
'' Purpose: Exercise the original wrappers against the genuine engines.
'' Responsibilities: Captures, empty matches, UTF-8, replacements and allocator balance.
'' This file contains no substitute regex engine or omitted wrapper implementation.
#lang "fb"
#define REGEX_EXAMPLE_NO_MAIN
#include once "CRegex.bi"
#include once "../../examples/regex/PCRE/preg_class.bas"
#include once "../../examples/regex/TRE/replace.bas"
#include once "../../examples/regex/TRE/subexpr.bas"
#ifdef __FB_WIN32__
#include once "windows.bi"
#include once "win/psapi.bi"
#endif

Dim Shared original_malloc As Function Cdecl(ByVal amount As UInteger) As Any Ptr
Dim Shared original_free As Sub Cdecl(ByVal memory As Any Ptr)
Dim Shared live_blocks As Integer

Private Function tracked_malloc Cdecl(ByVal amount As UInteger) As Any Ptr
	Dim memory As Any Ptr = original_malloc(amount)
	If memory <> NULL Then live_blocks += 1
	Return memory
End Function

Private Sub tracked_free Cdecl(ByVal memory As Any Ptr)
	If memory <> NULL Then live_blocks -= 1
	original_free(memory)
End Sub

Sub require(ByVal condition As Boolean, ByRef description As Const String)
	If Not condition Then
		Print "REGEX_RUNTIME=FAIL "; description
		End 1
	End If
End Sub

Private Function count_matches(ByVal pattern As ZString Ptr, ByVal text As ZString Ptr) As Integer
	Dim expression As CRegex = CRegex(pattern)
	Dim count As Integer
	If expression.search(text) Then
		Do
			count += 1
			require(count <= 100, "bounded global iteration")
		Loop While expression.searchNext()
	End If
	Return count
End Function

Private Function private_bytes() As UInteger
	#ifdef __FB_WIN32__
	Dim counters As PROCESS_MEMORY_COUNTERS_EX
	counters.cb = SizeOf(counters)
	require(GetProcessMemoryInfo(GetCurrentProcess(), Cast(PROCESS_MEMORY_COUNTERS Ptr, @counters), SizeOf(counters)), "private memory query")
	Return counters.PrivateUsage
	#elseif defined(__FB_LINUX__)
	'' Linux reports resident private pages through procfs. Warm the same
	'' allocator paths before both readings so retained caches have settled.
	Dim file_number As Integer = FreeFile
	require(Open("/proc/self/smaps_rollup", For Input, As #file_number) = 0, "private memory query")
	Dim total As ULongInt, fields As Integer
	While Not Eof(file_number)
		Dim record_text As String
		Line Input #file_number, record_text
		Dim separator As Integer = InStr(record_text, ":")
		If separator = 0 Then Continue While
		Dim field_name As String = Left(record_text, separator-1)
		If field_name = "Private_Clean" Or field_name = "Private_Dirty" Or field_name = "Private_Hugetlb" Then
			require(Right(record_text, 2) = "kB", "private memory units")
			total += ValULng(Mid(record_text, separator+1))
			fields += 1
		End If
	Wend
	require(Close(#file_number) = 0 And fields >= 2, "complete private memory query")
	require(total <= CULngInt(CUInt(-1)) \ 1024, "private memory range")
	Return CUInt(total * 1024)
	#else
	#error Native private memory measurement is required for this test
	#endif
End Function

original_malloc = pcre_malloc
original_free = pcre_free
pcre_malloc = @tracked_malloc
pcre_free = @tracked_free

For cycle As Integer = 1 To 30
	Scope
		Dim expression As CRegex = CRegex("(foo)([0-9]+)?")
		require(expression.getMaxMatches() = 2, "C int capture count")
		require(expression.search("foo17"), "basic search")
		require(expression.getOfs(0) = 0 And expression.getLen(0) = 5, "whole-match offsets")
		require(*expression.getStr(1) = "foo" And *expression.getStr(2) = "17", "captures")
		require(expression.search(expression.getStr(1)), "subject may alias an old capture")
		require(*expression.getStr(0) = "foo", "capture alias copied before cleanup")
		require(expression.getOfs(2) = -1 And expression.getStr(2) = NULL, "unset capture")
		require(expression.getStr(99) = NULL And expression.getLen(99) = -1, "capture upper bound")
		require(Not expression.search("none"), "no match")
		require(expression.getStr(0) = NULL And expression.getOfs(0) = -1, "failed search cannot expose stale offsets")
		require(Not expression.search(NULL), "reject null subject")
		require(Not expression.search("foo", -2), "reject negative length")
	End Scope
	Scope
		Dim expression As CRegex = CRegex("(")
		require(expression.getMaxMatches() = -1 And expression.getErrorCode() <> 0, "invalid compile")
		require(Not expression.search("foo"), "invalid object cannot search")
	End Scope
	Scope
		Dim expression As CRegex = CRegex(".")
		Dim text As String = "a" & Chr(0) & "b"
		require(expression.search(StrPtr(text), Len(text)), "explicit binary subject")
		text = "changed by caller"
		require(expression.searchNext(), "owned subject survives caller reassignment")
		require(expression.getOfs(0) = 1 And expression.getLen(0) = 1, "embedded zero offset")
		require(expression.searchNext() AndAlso *expression.getStr(0) = "b", "binary tail")
		require(Not expression.searchNext(), "binary end")
	End Scope
	require(count_matches(".*?", "ab") = 5, "empty/nonempty alternatives at the same position")
	require(count_matches(".*?", "") = 1, "empty subject")
	require(count_matches("(*UTF8).*?", Chr(&hc3, &ha9) & "x") = 5, "UTF8 character boundaries")
	require(count_matches("(*CRLF)(?:)", !"\r\nX") = 3, "CRLF progress skips a complete newline")
	Scope
		Dim expression As preg_t
		require(expression.match_all(".*?", "ab") = 0 And expression.matches = 5, "preg global empty matches")
		require(expression.result_all(-1) = "" And expression.result(-1) = "", "negative result indices")
		require(expression.replace("(?:)", "X", "ab") = 0 And expression.result() = "XaXbX", "empty replacement preserves skipped bytes")
		require(expression.replace("(a)", "$0-$1-$$-$99-$999999999999999999999999", "ab") = 0, "bounded replacement token")
		require(expression.result() = "a-a-$--b", "numbered and literal replacement")
		require(expression.replace("a", "X", "aa", 1) = 0 And expression.result() = "aX", "replacement keeps prefix before offset")
		require(expression.match("a", "a", -1) = PCRE_ERROR_BADOFFSET, "reject negative offset")
		require(expression.match("(", "a") <> 0, "preg compile failure")
	End Scope
	Dim pattern As String = "-(.+?)-", replacement As String = "*\1*", subject As String = "Hi -you- strange -user- :D"
	require(regex_replace(pattern, replacement, subject) = "Hi *you* strange *user* :D", "TRE capture replacement")
	pattern = "" : replacement = "X" : subject = "ab"
	require(regex_replace(pattern, replacement, subject) = "XaXbX", "TRE empty replacement progression")
	pattern = "(a)?b" : replacement = "\1Z" : subject = "b"
	require(regex_replace(pattern, replacement, subject) = "Z", "TRE optional capture")
	pattern = "(a)" : replacement = "\999999999999999999999999" : subject = "ab"
	require(regex_replace(pattern, replacement, subject) = "b", "TRE large capture number")
	pattern = "(" : replacement = "x" : subject = "a"
	require(regex_replace(pattern, replacement, subject) = "", "TRE compile failure")
	Dim filename As String = "path/name.ext1.ext2"
	require(get_filepart(filename, eFPK_Basename) = "name.ext1" And get_filepart(filename, eFPK_Extension) = ".ext2", "file captures")
	filename = "name"
	require(get_filepart(filename, eFPK_Path) = "" And get_filepart(filename, eFPK_Extension) = "", "missing optional filename captures")
	require(live_blocks = 0, "PCRE pattern/study/substring allocator balance")
Next

'' Warm allocation caches first, then repeat the same TRE paths. A lost compiled
'' regex grows process-private memory each iteration; retained caches stabilize.
Dim pattern As String = "(a)", replacement As String = "\1", subject As String = "a", filename As String = "path/name.ext"
For index As Integer = 1 To 1024
	require(regex_replace(pattern, replacement, subject) = "a", "TRE warmup")
	require(get_filepart(filename, eFPK_File) = "name.ext", "TRE file capture warmup")
Next
Dim before As UInteger = private_bytes()
For index As Integer = 1 To 5000
	require(regex_replace(pattern, replacement, subject) = "a", "TRE repeated replacement")
	require(get_filepart(filename, eFPK_File) = "name.ext", "TRE repeated file capture")
Next
Dim after As UInteger = private_bytes()
require(after <= before+1024*1024, "bounded TRE private memory after 10000 regex lifetimes: before=" & Str(before) & " after=" & Str(after))
pcre_malloc = original_malloc
pcre_free = original_free
Print "REGEX_RUNTIME=PASS cycles=30 allocator_balance=0 TRE_lifetimes=10000 private_before="; before; " private_after="; after

'' end of regex-runtime.bas
