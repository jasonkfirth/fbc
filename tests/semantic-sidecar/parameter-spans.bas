'' Project: FreeBASIC semantic sidecar tests
'' File: parameter-spans.bas
'' Purpose: Exercise complete written formal spans without changing name ranges.
'' Responsibilities: List boundaries, defaults, descriptors and generated forms.
'' This file intentionally does NOT implement a second declaration parser.

declare sub SeparateLines( _
    byval FirstValue as long, _
    byval SecondValue as long)

sub SeparateLines(byval BodyFirst as long, byval BodySecond as long)
    print BodyFirst + BodySecond
end sub

declare sub SplitType(byval PrototypeValue _
    as long)

sub SplitType(byval BodyValue _
    as long)
    print BodyValue
end sub

sub DefaultSpan(byval DefaultValue as long = _
    iif(-1, len("a,b"), 7))
    print DefaultValue
end sub

declare sub Unnamed(byval as long, byref _
    as long)

declare sub LogValues cdecl(byval FormatText as zstring ptr, _
    . _
    . _
    .)

declare sub ArraySpan(Values( _
    any, any) _
    as long = any)

type SpanOwner
    Value as long
    declare sub Method(byval PrototypeAmount _
        as long)
end type

sub SpanOwner.Method(byval BodyAmount _
    as long)
    print BodyAmount
end sub

type Callback as function(byval InputValue _
    as long) as long

declare sub CallbackSpan(byval Handler as function( _
    byval NestedValue as long) as long)

#define FIELD_TYPE long
sub ExpandedType(byval Item as FIELD_TYPE)
    print Item
end sub

#define PARAM_NAME ExpandedValue
sub ExpandedName(byval PARAM_NAME as long)
    print PARAM_NAME
end sub

#define WHOLE_FORMAL byval FromMacro as long
sub ExpandedAll(WHOLE_FORMAL)
    print FromMacro
end sub

SeparateLines(2, 3)
SplitType(7)
DefaultSpan()
dim OwnerValue as SpanOwner
OwnerValue.Method(11)
ExpandedType(13)
ExpandedName(17)
ExpandedAll(19)

'' end of parameter-spans.bas
