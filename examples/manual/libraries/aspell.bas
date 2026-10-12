'' examples/manual/libraries/aspell.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'GNU Aspell'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibaspell
'' --------
''
'' Purpose: Check interactive UTF-8 words with the installed dictionary.
'' Responsibilities: Distinguish errors from matches and release owned objects.
'' This file contains no dictionary data or personal-dictionary writes.

'' GNU-ASspell example, converted from http://aspell.net/win32/

#include Once "aspell.bi"

Private Function valid_utf8(ByRef text As Const String) As Integer
	Dim position As Integer = 0
	Do While position < Len(text)
		Dim first As UByte = text[position]
		If first < &h80 Then
			position += 1
			Continue Do
		End If
		Dim following As Integer
		Select Case first
		Case &hc2 To &hdf: following = 1
		Case &he0 To &hef: following = 2
		Case &hf0 To &hf4: following = 3
		Case Else: Return 0
		End Select
		If following >= Len(text)-position Then Return 0
		Dim second As UByte = text[position+1]
		'' Exclude overlong encodings, UTF-16 surrogates and values above U+10FFFF.
		If first = &he0 And second < &ha0 Then Return 0
		If first = &hed And second > &h9f Then Return 0
		If first = &hf0 And second < &h90 Then Return 0
		If first = &hf4 And second > &h8f Then Return 0
		For part As Integer = 1 To following
			If text[position+part] < &h80 Or text[position+part] > &hbf Then Return 0
		Next
		position += following+1
	Loop
	Return -1
End Function

Function check_words() As Integer
Dim As AspellConfig Ptr spell_config = new_aspell_config()
If spell_config = 0 Then Return 1

'' The first argument selects a language. The optional second argument selects
'' a dictionary/data directory, useful for a separately installed word-list pack.
Dim language_code As String = Command(1)
If language_code = "" Then language_code = "en_CA"
Dim dictionary_directory As String = Command(2)
Dim configured As Long = aspell_config_replace(spell_config, "lang", language_code)
If configured Then configured = aspell_config_replace(spell_config, "encoding", "utf-8")
If configured And dictionary_directory <> "" Then
	configured = aspell_config_replace(spell_config, "dict-dir", dictionary_directory)
	If configured Then configured = aspell_config_replace(spell_config, "data-dir", dictionary_directory)
End If
If configured = 0 Then
	Dim message_text As Const ZString Ptr = aspell_config_error_message(spell_config)
	If message_text <> 0 Then Print *message_text
	delete_aspell_config(spell_config)
	Return 1
End If

'' Create speller object
Dim As AspellCanHaveError Ptr possible_err = new_aspell_speller(spell_config)
If possible_err = 0 Then
	delete_aspell_config(spell_config)
	Return 1
End If
If (aspell_error_number(possible_err) <> 0) Then
	Dim message_text As Const ZString Ptr = aspell_error_message(possible_err)
	If message_text <> 0 Then Print *message_text
	delete_aspell_can_have_error(possible_err)
	delete_aspell_config(spell_config)
	Return 1
End If
'' Conversion transfers the successful result to the speller owner. Do not
'' also delete possible_err after a successful to_aspell_speller().
Dim As AspellSpeller Ptr speller = to_aspell_speller(possible_err)
If speller = 0 Then
	delete_aspell_can_have_error(possible_err)
	delete_aspell_config(spell_config)
	Return 1
End If

Dim As String word
Dim status As Integer = 0
'' File-style console input honors redirected stdin as well as typed input.
Dim input_number As Integer = FreeFile
If Open Cons(For Input, As #input_number) <> 0 Then
	delete_aspell_speller(speller)
	delete_aspell_config(spell_config)
	Return 1
End If
Do
	Print
	Print "Enter a word (blank to quit): ";
	If Eof(input_number) Then Exit Do
	Line Input #input_number, word
	If (Len(word) = 0) Then
		Exit Do
	End If

	'' Bound suggestion work and the native C int byte count. Words are UTF-8;
	'' Some Aspell builds ignore malformed bytes. Validate before checking them.
	Const MAX_WORD_BYTES = 256
	If Len(word) > MAX_WORD_BYTES Or InStr(word, Chr(0)) <> 0 Then
		Print "Word is too long or contains a zero byte"
		status = 1
		Exit Do
	End If
	If valid_utf8(word) = 0 Then
		Print "Word contains invalid UTF-8"
		status = 1
		Exit Do
	End If
	Dim checked As Long = aspell_speller_check(speller, StrPtr(word), Len(word))
	If checked < 0 Then
		Dim message_text As Const ZString Ptr = aspell_speller_error_message(speller)
		If message_text <> 0 Then Print *message_text
		status = 1
		Exit Do
	ElseIf checked = 1 Then
		Print "Word is Correct"
	Else
		Print "Suggestions:"
		Dim suggestions As Const AspellWordList Ptr = aspell_speller_suggest(speller, StrPtr(word), Len(word))
		If suggestions = 0 Then
			status = 1
			Exit Do
		End If
		'' The list belongs to the speller; its enumeration belongs to the caller.
		Dim As AspellStringEnumeration Ptr elements = aspell_word_list_elements(suggestions)
		If elements = 0 Then
			status = 1
			Exit Do
		End If
		Do
			Dim As Const ZString Ptr w = aspell_string_enumeration_next(elements)
			If (w = 0) Then
				Exit Do
			End If
			Print "   "; *w
		Loop
		delete_aspell_string_enumeration(elements)
	End If

	' - Report the replacement
	'aspell_speller_store_repl(speller, misspelled_word, size,
	'                          correctly_spelled_word, size);

	' - Add to session or personal dictionary
	'aspell_speller_add_to_session|personal(speller, word, size)
Loop

Close #input_number
delete_aspell_speller(speller)
delete_aspell_config(spell_config)
Return status
End Function

End check_words()

'' end of aspell.bas
