/'
	Project: FreeBASIC C backend regression controls
	File: c-static-initializer-reachability.bas
	Purpose: Keep live static initializer dependencies and omit unobserved data.
	Responsibilities: Constant reference chains, self references, empty global
	strings, runtime initialization, array descriptors and element cleanup.
	This file intentionally does NOT contain: Concurrent access, allocation
	failure injection or changes to source-level storage and callback types.
'/
#lang "fb"
#include once "crt/stdlib.bi"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_STATIC_INITIALIZER_REACHABILITY_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as integer initializations, constructions, destructions
dim shared as string empty_unused_text
dim shared empty_unused_array(0 to 2) as string
dim shared as ustring empty_unused_unicode
dim shared as string live_text
extern published_text alias "fb_static_published_text" as string
dim shared as string published_text

function InitialText() as string
	initializations += 1
	return "owned"
end function

dim shared as string initialized_text
initialized_text = "initialized"

type OwnedElement
	value as integer
	text as string
	declare constructor()
	declare destructor()
end type

constructor OwnedElement()
	constructions += 1
	value = constructions
	text = InitialText()
end constructor

destructor OwnedElement()
	VERIFY(value >= 1 and value <= 4)
	VERIFY(text = "owned")
	destructions += 1
end destructor

dim shared as OwnedElement global_element

private sub CheckShutdown cdecl()
	'' This callback is registered before the static array's cleanup. C exit
	'' callbacks run in reverse registration order, so the elements are gone.
	if destructions = 3 then
		print "C_STATIC_INITIALIZER_SHUTDOWN_PASS"
	else
		print "C_STATIC_INITIALIZER_SHUTDOWN_FAILED", destructions
	end if
end sub

sub CheckStaticStorage()
#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif
	static as integer unused_constant = 19
	static as integer unused_backing
	static as integer ptr unused_binding = @unused_backing
	static as integer ptr ptr unused_binding_chain = @unused_binding
	static as any ptr unused_self = @unused_self
	static as any ptr live_self = @live_self
	static byref unused_text_reference as string = live_text
	VERIFY(*cptr(any ptr ptr, live_self) = live_self)

	'' The body reads only the final pointer. Every earlier declaration must
	'' survive through initializer edges, independent of declaration order.
	static as integer live_value = 43
	static as integer first_value = 43
	static as any ptr link01 = @live_value
	static as any ptr link02 = @link01
	static as any ptr link03 = @link02
	static as any ptr link04 = @link03
	static as any ptr link05 = @link04
	static as any ptr link06 = @link05
	static as any ptr link07 = @link06
	static as any ptr link08 = @link07
	static as any ptr link09 = @link08
	static as any ptr link10 = @link09
	static as any ptr link11 = @link10
	static as any ptr link12 = @link11
	static as any ptr link13 = @link12
	static as any ptr link14 = @link13
	static as any ptr link15 = @link14
	static as any ptr link16 = @link15
	dim as any ptr cursor = link16
	for i as integer = 1 to 15
		cursor = *cptr(any ptr ptr, cursor)
	next
	VERIFY(*cptr(integer ptr, cursor) = first_value)
	*cptr(integer ptr, cursor) += 1

	VERIFY(*cptr(integer ptr, cursor) = first_value + 1)
	first_value += 1

	static as string runtime_text
	static as integer calls
	if calls = 0 then runtime_text = "runtime"
	calls += 1
	runtime_text += "x"
	VERIFY(len(runtime_text) = 7 + calls)

	static as OwnedElement owned_elements(0 to 2)
	VERIFY(constructions = 4)
	VERIFY(destructions = 0)
	for i as integer = 0 to 2
		VERIFY(owned_elements(i).value = i + 2)
		VERIFY(owned_elements(i).text = "owned")
	next
end sub

function SumElements(values() as integer) as integer
	dim as integer result_value
	for i as integer = lbound(values) to ubound(values)
		result_value += values(i)
	next
	return result_value
end function

sub CheckDescriptorUse()
	'' Descriptor-passed calls must retain metadata even though direct indexing
	'' and element constructors can use the backing data without a descriptor.
	static as integer values(2 to 4) = { 11, 13, 17 }
	VERIFY(SumElements(values()) = 41)
end sub

VERIFY(initializations = 1)
VERIFY(initialized_text = "initialized")
VERIFY(atexit(procptr(CheckShutdown)) = 0)
live_text = "live"
CheckStaticStorage()
CheckStaticStorage()
CheckDescriptorUse()
VERIFY(initializations = 4)
VERIFY(live_text = "live")
print "C_STATIC_INITIALIZER_REACHABILITY_PASS"

/' end of c-static-initializer-reachability.bas '/
