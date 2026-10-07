'' Project: FreeBASIC semantic sidecar tests
'' File: string-declarations.bas
'' Purpose: Preserve scalar dynamic String declarations before lowering.
'' Responsibilities: Field defaults, selected RHS and original storage grammar.
'' This file intentionally does NOT implement lint policy or backend handling.
#lang "fb"

#define EMPTY_VALUE ""
#define EMPTY_LOCAL(identifier) dim identifier as string = EMPTY_VALUE
#define EMPTY_FIELD(identifier) identifier as string = EMPTY_VALUE

type TextAlias as string
type StringOwner
    EmptyField as string = ""
    DefaultField as string
    NonemptyField as string = "x"
    FixedField as string * 4 = ""
    ArrayField(0 to 1) as string = {"", ""}
    EMPTY_FIELD(MacroField)
    type Nested
        EmptyNested as TextAlias = ("")
    end type
end type

function EmptyResult() as string
    return ""
end function

sub CheckDefaults()
    const EmptyConstant = ""
    dim LocalText as string = ""
    dim ConstantText as TextAlias = EmptyConstant
    dim FoldedText as string = "" + ""
    static StaticText as string
    dim as string FirstText = "", SecondText = ""
    EMPTY_LOCAL(MacroText)
    dim FromMacro as string = EMPTY_VALUE
    dim ContinuedText _
        as string _
        => (EMPTY_VALUE)
    if len(LocalText) = 0 then dim InlineText as string = ""
    #if 1
        dim ActiveText as string = ""
    #else
        dim InactiveText as string = ""
    #endif
    dim DefaultText as string
    dim FixedText as string * 4 = ""
    dim ZeroText as zstring * 4 = ""
    dim WideText as wstring * 4 = ""
    dim ArrayText(0 to 1) as string = {"", ""}
    dim ConstText as const string = ""
    var InferredText = ""
    dim byref ReferenceText as string = LocalText
    dim NonemptyText as string = "x"
    dim FromCall as string = EmptyResult()
    dim OwnerValue as StringOwner
    LocalText = ""
end sub

CheckDefaults()
'' end of string-declarations.bas
