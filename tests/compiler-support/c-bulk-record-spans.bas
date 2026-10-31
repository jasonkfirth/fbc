/'
	Project: FreeBASIC C backend regression controls
	File: c-bulk-record-spans.bas
	Purpose: Preserve containing-object bounds for bulk field initialization.
	Responsibilities: Anonymous union vectors, copy and array constructors,
	packed storage, partial field groups, inherited state and single evaluation.
	This file intentionally does NOT contain: Out-of-bounds runtime accesses,
	allocation stress or native warning suppressions.
'/
#lang "fb"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_BULK_RECORD_SPANS_FAILED", __LINE__
		end 1
	end if
#endmacro

type Vec2
	union
		type
			x as single
			y as single
		end type
		values(0 to 1) as single
	end union
	declare constructor()
	declare constructor(byref source_value as const Vec2)
	declare constructor(source_values() as single)
end type

constructor Vec2()
end constructor
constructor Vec2(byref source_value as const Vec2)
	x = source_value.x
	y = source_value.y
end constructor
constructor Vec2(source_values() as single)
	x = source_values(0)
	y = source_values(1)
end constructor

type Vec3
	union
		type
			x as single
			y as single
			z as single
		end type
		values(0 to 2) as single
	end union
	declare constructor()
	declare constructor(byref source_value as const Vec3)
	declare constructor(source_values() as single)
end type

constructor Vec3()
end constructor
constructor Vec3(byref source_value as const Vec3)
	x = source_value.x
	y = source_value.y
	z = source_value.z
end constructor
constructor Vec3(source_values() as single)
	x = source_values(0)
	y = source_values(1)
	z = source_values(2)
end constructor

type GuardedVector
	before_bytes(0 to 15) as ubyte
	vector as Vec3
	after_bytes(0 to 15) as ubyte
end type

type PackedGroup field = 1
	prefix as ubyte = &hA5
	union
		type
			x as single
			y as single
		end type
		values(0 to 1) as single
	end union
	suffix as ubyte = &h5A
	declare constructor()
end type
constructor PackedGroup()
end constructor

type OwnedGroup
	text as string
	union
		type
			x as single
			y as single
		end type
		values(0 to 1) as single
	end union
	tail as long = 73
end type

dim shared as integer address_calls
private function GroupAddress(byval value as OwnedGroup ptr) as OwnedGroup ptr
	address_calls += 1
	return value
end function

type MarkerBase extends object
	marker as long
	declare constructor()
	declare virtual function ReadMarker() as long
end type
constructor MarkerBase()
	marker = 71
end constructor
function MarkerBase.ReadMarker() as long
	return marker
end function

type DerivedGroup extends MarkerBase
	union
		type
			x as single
			y as single
			z as single
		end type
		values(0 to 2) as single
	end union
	declare constructor()
end type
constructor DerivedGroup()
end constructor

#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif

dim as single values(0 to 2) = { 11.0!, 13.0!, 17.0! }
dim as Vec2 initial2
VERIFY(initial2.x = 0.0! and initial2.y = 0.0!)
dim as Vec2 array2 = Vec2(values())
dim as Vec2 copy2 = array2
VERIFY(copy2.values(0) = 11.0! and copy2.values(1) = 13.0!)
dim as Vec3 initial3
VERIFY(initial3.x = 0.0! and initial3.y = 0.0! and initial3.z = 0.0!)
dim as Vec3 array3 = Vec3(values())
dim as Vec3 copy3 = array3
VERIFY(copy3.x = 11.0! and copy3.y = 13.0! and copy3.z = 17.0!)

dim as GuardedVector guarded
for i as integer = 0 to 15
	guarded.before_bytes(i) = &hA5
	guarded.after_bytes(i) = &h5A
next
guarded.vector.x = 31.0!
guarded.vector.y = 37.0!
guarded.vector.z = 41.0!
'' Placement construction reuses this object's storage. It does not allocate
'' or transfer ownership, and must not touch either surrounding byte array.
dim as Vec3 ptr rebuilt = new (@guarded.vector) Vec3()
VERIFY(rebuilt = @guarded.vector)
VERIFY(rebuilt->x = 0.0! and rebuilt->y = 0.0! and rebuilt->z = 0.0!)
for i as integer = 0 to 15
	VERIFY(guarded.before_bytes(i) = &hA5)
	VERIFY(guarded.after_bytes(i) = &h5A)
next

dim as PackedGroup packed
VERIFY(sizeof(PackedGroup) = 10)
VERIFY(packed.prefix = &hA5 and packed.suffix = &h5A)
VERIFY(packed.x = 0.0! and packed.y = 0.0!)

scope
	dim as OwnedGroup first_value, second_value
	first_value.text = "owned"
	first_value.x = 43.0!
	first_value.y = 47.0!
	address_calls = 0
	*GroupAddress(@second_value) = first_value
	VERIFY(address_calls = 1)
	VERIFY(second_value.x = 43.0! and second_value.y = 47.0!)
	VERIFY(second_value.tail = 73)
	VERIFY(second_value.text = "owned")
	first_value.text = "changed"
	VERIFY(second_value.text = "owned")
end scope

dim as DerivedGroup derived
VERIFY(derived.x = 0.0! and derived.y = 0.0! and derived.z = 0.0!)
VERIFY(derived.marker = 71)
dim as MarkerBase ptr base_view = @derived
VERIFY(base_view->ReadMarker() = 71)
print "C_BULK_RECORD_SPANS_PASS"

/' end of c-bulk-record-spans.bas '/
