'' Project: FreeBASIC checked virtual dispatch tests
'' File: c-virtual-receiver-check.bas
'' Purpose: Check a receiver before loading its virtual table.
'' Responsibilities: Cover receiver evaluation and virtual/abstract dispatch.
'' This file intentionally does NOT change allocation or exception contracts.

#lang "fb"

type VirtualBase extends object
	declare virtual function value() as integer
end type

type Derived extends VirtualBase
	declare function value() as integer override
end type

type AbstractBase extends object
	declare abstract function value() as integer
end type

type Concrete extends AbstractBase
	declare function value() as integer override
end type

function VirtualBase.value() as integer
	return 11
end function

function Derived.value() as integer
	return 42
end function

function Concrete.value() as integer
	return 84
end function

'' The fixture owns both receiver slots and the evaluation counter.
dim shared as VirtualBase ptr selected_receiver
dim shared as AbstractBase ptr selected_abstract
dim shared as integer receiver_reads

private function getReceiver() as VirtualBase ptr
	receiver_reads += 1
	return selected_receiver
end function

private function getAbstract() as AbstractBase ptr
	receiver_reads += 1
	return selected_abstract
end function

dim as Derived instance
dim as Concrete abstract_instance
selected_receiver = @instance
selected_abstract = @abstract_instance

select case command(1)
case "null-virtual"
	selected_receiver = 0
case "null-abstract"
	selected_abstract = 0
end select

if getReceiver()->value() <> 42 then end 1
if getAbstract()->value() <> 84 then end 2
if receiver_reads <> 2 then
	print "RECEIVER_EVALUATION_COUNT"; receiver_reads
	end 3
end if
print "VIRTUAL_RECEIVER_CHECK_PASS"

'' end of c-virtual-receiver-check.bas
