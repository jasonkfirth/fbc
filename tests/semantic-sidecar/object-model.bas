'' Project: FreeBASIC semantic sidecar tests
'' File: object-model.bas
'' Purpose: Exercise actual table inheritance and deferred RTTI initialization.
'' Responsibilities: Empty/plain/abstract tables, overrides, destructors and IS.
'' This file intentionally does NOT infer runtime targets from a declared type.

type PlainObjectData
	value as long
end type

union PlainObjectUnion
	value as long
	other as double
end union

type EmptyObjectTable extends object
end type

type PureObjectBase extends object
	declare abstract function ReadValue() as long
	declare abstract sub Update(byval value as long)
end type

type PartialObject extends PureObjectBase
	declare virtual function ReadValue() as long override
end type

function PartialObject.ReadValue() as long
	return 41
end function

type ConcreteObject extends PartialObject
	value as long
	declare sub Update(byval value as long) override
end type

sub ConcreteObject.Update(byval value as long)
	this.value = value
end sub

type LeafObject extends ConcreteObject
	declare function ReadValue() as long override
	declare virtual function ExtraValue() as long
end type

function LeafObject.ReadValue() as long
	return 42
end function

function LeafObject.ExtraValue() as long
	return 43
end function

type InheritedObject extends LeafObject
end type

type AbstractAgainObject extends ConcreteObject
	declare abstract function ReadValue() as long override
end type

type NewAbstractObject extends LeafObject
	declare abstract function MissingValue() as long
end type

type VirtualLifetime extends object
	declare virtual destructor()
end type

destructor VirtualLifetime()
end destructor

type VirtualLifetimeChild extends VirtualLifetime
	declare destructor() override
end type

destructor VirtualLifetimeChild()
end destructor

dim instance as InheritedObject
dim base_pointer as PureObjectBase ptr = @instance
base_pointer->Update(7)
if base_pointer->ReadValue() <> 42 or instance.value <> 7 then end 1
dim root_pointer as object ptr = @instance
if not (*root_pointer is LeafObject) then end 2
if not (*root_pointer is PureObjectBase) then end 3
dim empty_instance as EmptyObjectTable
dim empty_pointer as object ptr = @empty_instance
if *empty_pointer is LeafObject then end 4
dim lifetime_instance as VirtualLifetimeChild
print "OBJECT_MODEL_PASS"

'' end of object-model.bas
