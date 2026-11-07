/'
    Project: FreeBASIC C backend regression
    File: c-constructor-reachability.bas
    Purpose: Check implicit constructor reachability in generated C.
    Responsibilities: Exercise global, local, static, array and heap lifetimes.
    This fixture intentionally does NOT use opaque inline assembly.
'/
dim shared as integer constructors, destructors

type Managed
    value as long
    declare constructor()
    declare constructor(byval initial_value as long)
    declare destructor()
end type
private constructor Managed()
    constructors += 1
    value = 11
end constructor
private constructor Managed(byval initial_value as long)
    constructors += 1
    value = initial_value
end constructor
private destructor Managed()
    destructors += 1
end destructor

type InnerOnly
    value as long
    declare constructor()
end type
private constructor InnerOnly()
    value = 37
end constructor

type OuterOnly
    inner as InnerOnly
    declare constructor()
end type
private constructor OuterOnly()
end constructor

type UnusedThing
    value as long
    declare constructor()
end type
private constructor UnusedThing()
    value = 99
end constructor
private sub unused_constructor_owner()
    dim item as UnusedThing
end sub

dim shared global_item as Managed
private sub exercise()
    if global_item.value <> 11 then end 1
    dim local_item as Managed
    if local_item.value <> 11 then end 2
    dim fixed_items(0 to 1) as Managed
    if fixed_items(0).value <> 11 orelse fixed_items(1).value <> 11 then end 3
    static stored_item as Managed
    if stored_item.value <> 11 then end 4
    dim dynamic_items() as Managed
    redim dynamic_items(0 to 1)
    if dynamic_items(0).value <> 11 orelse dynamic_items(1).value <> 11 then end 5
    dim heap_item as Managed ptr = new Managed(43)
    if heap_item = 0 then end 6
    if heap_item->value <> 43 then end 7
    delete heap_item
    dim outer as OuterOnly
    if outer.inner.value <> 37 then end 8
end sub
exercise()
if constructors <> 8 orelse destructors <> 6 then end 11
end 0
' end of c-constructor-reachability.bas
