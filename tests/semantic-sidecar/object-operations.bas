'' Project: FreeBASIC semantic sidecar tests
'' File: object-operations.bas
'' Purpose: Exercise selected base/member construction, copying and destruction.
'' Responsibilities: Explicit/default/chained constructors, generated members and exclusions.
'' This file intentionally does NOT assume compiler build order is runtime order.
#lang "fb"

type OperationLeaf
    value as long
    declare constructor()
    declare constructor(byref rhs as const OperationLeaf)
    declare destructor()
end type

constructor OperationLeaf()
    value = 1
end constructor
constructor OperationLeaf(byref rhs as const OperationLeaf)
    value = rhs.value
end constructor
destructor OperationLeaf()
end destructor

type OperationBase extends object
    value as long
    declare constructor()
    declare constructor(byval initial as long)
    declare virtual destructor()
end type
constructor OperationBase()
    value = 10
end constructor
constructor OperationBase(byval initial as long)
    value = initial
end constructor
destructor OperationBase()
end destructor

type OperationDerived extends OperationBase
    first as OperationLeaf
    second(0 to 1) as OperationLeaf
    initialized as long = 7
    flags : 3 as uinteger
    fixed_text as string * 3
    uninitialized as long = any
    redim dynamic_values(0 to 1) as long
    union
        union_integer as long
        union_double as double
    end union
    declare constructor()
    declare constructor(byval ignored as long)
    declare virtual destructor()
end type
constructor OperationDerived()
    base(5)
end constructor
constructor OperationDerived(byval ignored as long)
    constructor()
end constructor
destructor OperationDerived()
end destructor

type OperationAuto
    text as string
    items(0 to 1) as OperationLeaf
    empty_values(any) as long
    untouched as long = any
end type

type OperationPlain
    value as long
end type

function OperationAssignedResult() as OperationAuto
    dim local_value as OperationAuto
    local_value.text = "assigned"
    local_value.untouched = 18
    function = local_value
end function
function OperationReturnedResult() as OperationAuto
    dim local_value as OperationAuto
    local_value.text = "returned"
    local_value.untouched = 19
    return local_value
end function

sub OperationConsume(byval value as OperationLeaf)
    if value.value <> 1 then end 2
end sub

private sub OperationUnused()
    dim value as OperationDerived
end sub

scope
    dim value as OperationDerived = OperationDerived(1)
    dim automatic as OperationAuto
    automatic.untouched = 17
    dim copied as OperationAuto = automatic
    copied = OperationReturnedResult()
    automatic = OperationAssignedResult()
    OperationConsume(value.first)
    dim plain_value as OperationPlain
    plain_value.value = 9
    print value.value; ":"; value.initialized; ":"; copied.text; ":"; automatic.text; ":"; plain_value.value
end scope

'' end of object-operations.bas
