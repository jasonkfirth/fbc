'' Project: FreeBASIC semantic-sidecar tests
'' File: implicit-call-coordinates.bas
'' Purpose: Retain physical construction anchors beside legacy implicit calls.
'' Responsibilities: Encoded/remapped locations, construction and macro controls.
'' This file intentionally does NOT expose an editable constructor-name token.
type constructable
    value as integer
    declare constructor(byval initial as integer = 1)
end type
constructor constructable(byval initial as integer)
    value = initial
end constructor
dim first as constructable
dim second as constructable = constructable(2)
#define CONSTRUCT_ONE(name) dim name as constructable
CONSTRUCT_ONE(generated)
dim annotation as string = "💡" : dim encoded as constructable
#line 800 "virtual-construction.bas"
dim remapped as constructable
'' end of implicit-call-coordinates.bas
