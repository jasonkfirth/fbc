/'
	Project: FreeBASIC C backend regression controls
	File: c-callback-ownership.bas
	Purpose: Check reachability of shared and unobserved native callback adapters.
	Responsibilities: Dynamic element lifetimes, string ownership, unused private
	clients, and typed graphics adapters that have no surviving caller.
	This file intentionally does NOT contain: Window creation or display tests.
'/
#lang "fb"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_CALLBACK_OWNERSHIP_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as integer created, destroyed

type Element
	payload as string
	declare constructor()
	declare destructor()
end type

constructor Element()
	payload = "created"
	created += 1
end constructor

destructor Element()
	VERIFY(len(payload) > 0)
	destroyed += 1
end destructor

type Owner
	items(any) as Element
end type

private sub LiveClientA()
	dim as Owner value
	redim value.items(0 to 1)
	value.items(0).payload = "alpha"
	value.items(1).payload = "beta"
	redim preserve value.items(0 to 2)
	VERIFY(value.items(0).payload = "alpha")
	VERIFY(value.items(1).payload = "beta")
	VERIFY(value.items(2).payload = "created")
end sub

private sub LiveClientB()
	dim values() as Element
	redim values(0 to 1)
	VERIFY(values(0).payload = "created")
	VERIFY(values(1).payload = "created")
	erase values
end sub

type Ghost
	payload as string
end type

private sub DeadArrayClient()
	dim values() as Ghost
	redim values(0 to 1)
	values(0).payload = "unreachable"
end sub

private function DeadBlender cdecl _
	(byval source_pixel as ulong, byval destination_pixel as ulong, _
	 byval context as long ptr) as ulong
	return source_pixel xor destination_pixel xor culng(*context)
end function

private sub DeadGraphicsClient(byval source_image as any ptr, byval destination_image as any ptr)
	dim as long context = 7
	put destination_image, (0, 0), source_image, custom, @DeadBlender, @context
end sub

private sub DeadCaller()
	'' Parsing this caller marks both clients as source-accessed. Native
	'' reachability must still distinguish those marks from an actual root.
	DeadArrayClient()
	DeadGraphicsClient(0, 0)
end sub

#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif

LiveClientA()
VERIFY(created = 3 and destroyed = 3)
LiveClientB()
VERIFY(created = 5 and destroyed = 5)
print "C_CALLBACK_OWNERSHIP_PASS"

/' end of c-callback-ownership.bas '/
