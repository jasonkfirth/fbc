' Project: FreeBASIC semantic observation tests
' File: parsed-string-intrinsics.bas
' Purpose: Preserve intrinsic inputs through literal folding and runtime lowering.
' Responsibilities: CHR families, trim selectors, macros and inactive source.
' This file intentionally does NOT execute string operations or read input.
#lang "fb"

#define BAD_CHARACTER Chr(300)
Const NamedCharacter As Integer = 300
Sub StringInputs(ByVal codePoint As Integer, ByRef textValue As String)
    Dim As String literalCharacter = Chr(&H3A9)
    Dim As String runtimeCharacters = Chr(0, codePoint, 256)
    Dim As String namedCharacterResult = Chr(NamedCharacter)
    Dim As String computedCharacter = Chr(300 - 1)
    Dim As WString * 8 wideCharacter = WChr(&H3A9)
    Dim As UString unicodeCharacter = UChr(&H3A9)
    Dim As String macroCharacter = BAD_CHARACTER
    Dim As String ordinaryTrim = Trim(textValue)
    Dim As String exactTrim = Trim(textValue, ".txt")
    Dim As String leftSet = LTrim(textValue, Any !"f\&h66x")
    Dim As String rightSet = RTrim(textValue, Any "aaba")
    Dim As WString * 8 wideSet = Trim(wideCharacter, Any ".tt")
#if 0
    Dim As String inactiveCharacter = Chr(400)
#endif
End Sub

' end of parsed-string-intrinsics.bas
