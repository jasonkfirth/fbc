/'
    Project: FreeBASIC C backend qualification
    File: c-destructor-reachability.bas
    Purpose: Exercise retained and unreachable private destructor bodies.
    Responsibilities: Check field, array, global, static and virtual cleanup.
    This file intentionally does NOT require graphics or external input.
'/
#lang "fb"

type LeafItem
    value as long
    declare destructor()
end type

private destructor LeafItem()
    print "LEAF="; this.value
end destructor

type PairItem
    first as LeafItem
    second as LeafItem
end type

type StaticItem
    value as long
    declare destructor()
end type

private destructor StaticItem()
    print "STATIC_ROOT="; this.value
end destructor

'' This discarded direct call must not hide the separate, late static root.
private sub unused_static_caller(byval item as StaticItem ptr)
    item->destructor()
end sub

private sub create_static()
    static item as StaticItem
    item.value = 17
end sub

type UnobservedItem
    value as long
    declare destructor()
end type

private destructor UnobservedItem()
    print "UNOBSERVED_DTOR="; this.value
end destructor

private sub unused_owner()
    dim item as UnobservedItem
end sub

type VirtualBase extends object
    declare virtual destructor()
end type

private destructor VirtualBase()
    print "VIRTUAL_BASE"
end destructor

type VirtualItem extends VirtualBase
    declare destructor()
end type

private destructor VirtualItem()
    print "VIRTUAL_ITEM"
end destructor

'' Two global field destructors must survive generation of the module wrapper.
dim shared global_pair as PairItem

private sub automatic_storage()
    dim local_pair as PairItem
    dim local_array(0 to 1) as LeafItem
    dim dynamic_array as LeafItem ptr = new LeafItem[2]
    delete[] dynamic_array
end sub

automatic_storage()
create_static()
dim polymorphic as VirtualBase ptr = new VirtualItem
delete polymorphic
print "DESTRUCTOR_ROOTS_MAIN=PASS"
end 0

' end of c-destructor-reachability.bas
