'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-typed-members.bas
'' Purpose: Verify typed C member access and the retained raw-memory paths.
'' Responsibilities: Cover nested records, array layout, qualifiers, callbacks,
''     copies, single evaluation, unions, packed records and bitfields.
'' This file intentionally does NOT require a particular compiler optimization.
#lang "fb"

type member_leaf
	marker as short
	count as integer
	value as double
	payload as integer ptr
end type

type member_bundle
	lead as ubyte
	nested as member_leaf
	grid(2 to 3, -1 to 1) as member_leaf
	text as string
	tag as integer
	callback as function cdecl( byval as integer ) as integer
end type

type member_parent
	count as integer
end type
type member_child extends member_parent
	extra as integer
end type

type member_overlay
	prefix as integer
	union
		whole as ulongint
		type
			low as ulong
			high as ulong
		end type
	end union
end type

type member_packed field = 1
	lead as ubyte
	count as integer
	value as double
end type

type member_bits
	low:3 as uinteger
	high:5 as uinteger
	count as integer
end type

type member_initialized
	count as const integer = 91
	readonly_value as const integer ptr
	declare constructor( byref value as const integer )
end type
constructor member_initialized( byref value as const integer )
	readonly_value = @value
end constructor

dim shared as member_bundle storage
dim shared as integer pointer_evaluations
function bundle_address() as member_bundle ptr
	pointer_evaluations += 1
	return @storage
end function

function member_callback cdecl( byval value as integer ) as integer
	return value * 3
end function

function member_total( byref value as const member_bundle ) as integer
	return value.nested.count + value.grid(2, -1).count + value.grid(3, 1).count
end function

sub member_copy( byref target as member_bundle, byref source as const member_bundle )
	target = source
end sub

dim as integer payload_value = 37
storage.lead = 7
storage.nested.marker = -19
storage.nested.count = 101
storage.nested.value = 1.25
storage.nested.payload = @payload_value
storage.text = "typed members"
storage.tag = 113
storage.callback = @member_callback

'' Unequal dimensions and nonzero bounds expose transposed C array paths.
for row as integer = 2 to 3
	for column as integer = -1 to 1
		storage.grid(row, column).count = row * 100 + column
		storage.grid(row, column).marker = row * 10 + column
	next
next
if( storage.grid(2, -1).count <> 199 ) then end 1
if( storage.grid(2, 0).count <> 200 ) then end 2
if( storage.grid(2, 1).count <> 201 ) then end 3
if( storage.grid(3, -1).count <> 299 ) then end 4
if( storage.grid(3, 0).count <> 300 ) then end 5
if( storage.grid(3, 1).count <> 301 ) then end 6
storage.grid(3, 0).count = 701
dim as integer changing_row = 3, changing_column = 0
if( storage.grid(changing_row, changing_column).count <> 701 ) then end 7

'' The typed path must evaluate its base once for both reads and writes.
bundle_address()->nested.count += 5
if( pointer_evaluations <> 1 or storage.nested.count <> 106 ) then end 8
dim as integer observed = bundle_address()->grid(3, 1).count
if( pointer_evaluations <> 2 or observed <> 301 ) then end 9
if( member_total(storage) <> 606 ) then end 10
if( *storage.nested.payload <> 37 or storage.callback(14) <> 42 ) then end 11

dim as member_bundle copied
member_copy(copied, storage)
if( copied.text <> "typed members" or copied.tag <> 113 ) then end 12
if( copied.nested.marker <> -19 or copied.nested.value <> 1.25 ) then end 13
if( copied.nested.payload <> @payload_value or copied.callback(9) <> 27 ) then end 14
if( copied.grid(3, 0).count <> 701 or copied.grid(2, 1).marker <> 21 ) then end 15
storage.text = "changed"
if( copied.text <> "typed members" ) then end 16

dim as member_child child
child.count = 23
child.extra = 29
if( child.count + child.extra <> 52 ) then end 17

dim as member_overlay overlay
overlay.prefix = 41
overlay.whole = &h1122334455667788ull
'' These native runs target the little-endian Windows ABIs.
if( overlay.low <> &h55667788ul or overlay.high <> &h11223344ul ) then end 18
overlay.high = &hAABBCCDDul
if( overlay.whole <> &hAABBCCDD55667788ull or overlay.prefix <> 41 ) then end 19

dim as member_packed packed_value
packed_value.lead = 3
packed_value.count = -12345
packed_value.value = 6.25
if( packed_value.lead <> 3 or packed_value.count <> -12345 ) then end 20
if( packed_value.value <> 6.25 ) then end 21
if( cuint(@packed_value.count) - cuint(@packed_value) <> 1 ) then end 22

dim as member_bits bits
bits.low = 5
bits.high = 19
bits.count = 73
if( bits.low <> 5 or bits.high <> 19 or bits.count <> 73 ) then end 23

dim as member_initialized initialized = member_initialized(payload_value)
if( initialized.count <> 91 or *initialized.readonly_value <> 37 ) then end 24

'' Incompatible floating/integer views must retain the raw-memory path.
dim as ulongint bit_pattern = &h3FF0000000000000ull
dim as double floating_view = *cptr(double ptr, @bit_pattern)
if( floating_view <> 1.0 ) then end 25

print "TYPED_MEMBERS_PASS"
end 0
'' end of c-typed-members.bas
