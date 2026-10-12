'' FreeBASIC XML examples
'' File: expat.bas
'' Purpose: Display an XML file using Expat's streaming callbacks.
'' Responsibilities: Preserve text chunks, match C callback types, and release parser/file state.
'' This file contains no tree construction, external entity loading or background parsing.

#include once "expat.bi"
#include once "crt/string.bi"

'' Narrow mode uses libexpat. Define XML_UNICODE to use the compatible
'' wide-character libexpatw SDK instead; its XML_Char layout must match.
Const BUFFER_SIZE = 1024
Const MAX_INDENT = 64  '' Bound display indentation for deeply nested input.

Type Context
	parser As XML_Parser
	nesting As Integer
	line_open As Boolean
End Type

Private Sub finish_text(ByRef ctx As Context)
	If ctx.line_open Then Print
	ctx.line_open = False
End Sub

Private Function indentation(ByRef ctx As Context) As Integer
	If ctx.nesting > MAX_INDENT Then Return MAX_INDENT
	Return ctx.nesting
End Function

Sub elementBegin cdecl(ByVal userdata As Any Ptr, ByVal element As Const XML_Char Ptr, _
                       ByVal attributes As Const XML_Char Ptr Ptr)
	Dim ctx As Context Ptr = userdata
	If ctx = NULL Or element = NULL Then Exit Sub
	finish_text(*ctx)
	Print Space(indentation(*ctx)); *element;
	If Cast(Any Ptr, attributes) <> NULL Then
		While attributes[0] <> NULL
			If attributes[1] = NULL Then Exit While
			Print " "; *attributes[0]; "='"; *attributes[1]; "'";
			attributes += 2
		Wend
	End If
	Print
	ctx->nesting += 1
End Sub

Sub elementEnd cdecl(ByVal userdata As Any Ptr, ByVal element As Const XML_Char Ptr)
	Dim ctx As Context Ptr = userdata
	If ctx = NULL Then Exit Sub
	finish_text(*ctx)
	If ctx->nesting > 0 Then ctx->nesting -= 1
End Sub

Sub charData cdecl(ByVal userdata As Any Ptr, ByVal chars As Const XML_Char Ptr, ByVal length As Long)
	Dim ctx As Context Ptr = userdata
	If ctx = NULL Or chars = NULL Or length <= 0 Then Exit Sub
	If Not ctx->line_open Then Print Space(indentation(*ctx));
	ctx->line_open = True

	'' Callback data is not terminated. Print bounded copies continuously,
	'' retaining all character data rather than silently dropping long text.
	'' XML forbids embedded zero characters in successfully parsed text.
	Dim text As XML_Char * (BUFFER_SIZE+1)
	Dim position As Long = 0
	Do While position < length
		Dim count As Long = length-position
		If count > BUFFER_SIZE Then count = BUFFER_SIZE
		#ifdef XML_UNICODE
			'' Keep a UTF-16 surrogate pair in the same output conversion.
			If SizeOf(XML_Char) = 2 And count < length-position Then
				Dim last_unit As UInteger = chars[position+count-1]
				If last_unit >= &hd800 And last_unit <= &hdbff Then count -= 1
			End If
		#endif
		memcpy(@text[0], @chars[position], count * SizeOf(XML_Char))
		text[count] = 0
		Print text;
		position += count
	Loop
End Sub

Function main() As Integer
	Dim filename As String = Command(1)
	If Len(filename) = 0 Then
		Print "Usage: expat <xmlfilename>"
		Return 1
	End If
	Dim file_number As Integer = FreeFile()
	If Open(filename, For Binary, Access Read, As #file_number) <> 0 Then
		Print "Could not open file: '"; filename; "'"
		Return 1
	End If
	Dim ctx As Context
	ctx.parser = XML_ParserCreate(NULL)
	If ctx.parser = NULL Then
		Close #file_number
		Print "XML_ParserCreate failed"
		Return 1
	End If
	'' The local context outlives every synchronous XML_Parse callback.
	XML_SetUserData(ctx.parser, @ctx)
	XML_SetElementHandler(ctx.parser, @elementBegin, @elementEnd)
	XML_SetCharacterDataHandler(ctx.parser, @charData)

	Dim buffer(0 To BUFFER_SIZE-1) As UByte
	Dim exit_status As Integer = 0
	Do
		Dim count As Integer
		Dim input_status As Integer = Get(#file_number, , buffer(0), BUFFER_SIZE, count)
		If input_status <> 0 Then
			Print "File input error"
			exit_status = 1
			Exit Do
		End If
		Dim finished As Boolean = EOF(file_number) <> 0
		'' Finalize even an empty input, so Expat reports its syntax error.
		If XML_Parse(ctx.parser, @buffer(0), count, finished) = XML_STATUS_ERROR Then
			finish_text(ctx)
			Print filename & "(" & XML_GetCurrentLineNumber(ctx.parser) & "): Error from XML parser: "
			Print *XML_ErrorString(XML_GetErrorCode(ctx.parser))
			exit_status = 1
			Exit Do
		End If
		If finished Then Exit Do
	Loop
	finish_text(ctx)
	If Close(#file_number) <> 0 Then exit_status = 1
	XML_ParserFree(ctx.parser)
	Return exit_status
End Function

End main()

'' end of expat.bas
