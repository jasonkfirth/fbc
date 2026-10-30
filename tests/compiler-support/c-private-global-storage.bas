/'
	Project: FreeBASIC C backend regression controls
	File: c-private-global-storage.bas
	Purpose: Drop unused private storage while retaining initializer dependencies.
	Responsibilities: Transitive pointer initializers, metadata-only uses, RTTI,
	public data, owned globals, DATA statements and an opaque assembler control.
	This file intentionally does NOT contain: Platform APIs or concurrent access.
'/
#lang "fb"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_PRIVATE_GLOBAL_STORAGE_FAILED", __LINE__
		end 1
	end if
#endmacro

extern public_value alias "fb_test_public_storage" as long
dim shared public_value as long = 47
declare function ExternalValue cdecl alias "fb_test_public_value" () as long

type LinkValue
	value as long
	next_value as LinkValue ptr
end type

dim shared as LinkValue tail_value = (73, 0)
dim shared as LinkValue middle_value = (41, @tail_value)
dim shared as LinkValue ptr head_value = @middle_value

dim shared as long initializer_only(0 to 2) = {11, 22, 33}
dim shared as long ptr live_pointer = @initializer_only(1)

'' These initializers mention storage whose value is never observed in C.
'' SIZEOF is a type query, so it must not become a runtime storage root.
dim shared as LinkValue dormant_tail = (5, 0)
dim shared as LinkValue ptr dormant_head = @dormant_tail
dim shared as long dormant_array(0 to 7) = {1, 2, 3, 4, 5, 6, 7, 8}
dim shared as long ptr dormant_pointer = @dormant_array(0)

type DormantClass extends object
	declare static function AddOne(byval value as long) as long
end type

function DormantClass.AddOne(byval value as long) as long
	return value + 1
end function

dim shared as long constructors, destructors

type OwnedGlobal
	marker as long
	declare constructor()
	declare destructor()
end type

constructor OwnedGlobal()
	constructors += 1
	marker = 19
end constructor

destructor OwnedGlobal()
	VERIFY(marker = 19)
	destructors += 1
	VERIFY(destructors = 1)
	print "C_PRIVATE_GLOBAL_DTOR_PASS"
end destructor

dim shared owned_global as OwnedGlobal
dim shared owned_text as string
owned_text = "retained"

#ifdef OPAQUE_ASM
asm
	nop
end asm
#endif

VERIFY(sizeof(dormant_head) = sizeof(any ptr))
VERIFY(sizeof(dormant_pointer) = sizeof(any ptr))
VERIFY(head_value->value = 41)
VERIFY(head_value->next_value->value = 73)
VERIFY(*live_pointer = 22)
VERIFY(ExternalValue() = 47)
VERIFY(constructors = 1)
VERIFY(DormantClass.AddOne(5) = 6)

dim as long first_value, second_value
read first_value, second_value
VERIFY(first_value = 17 and second_value = 29)
print "C_PRIVATE_GLOBAL_STORAGE_PASS"

data 17, 29

/' end of c-private-global-storage.bas '/
