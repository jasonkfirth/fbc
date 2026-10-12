'' FreeBASIC serialization regression
'' File: roundtrip.bas
'' Purpose: Exercise generated traversal and transactional reconstruction.
'' Responsibilities: Deep ownership, numeric values, strings, arrays and failures.
'' This file intentionally contains no transport or handwritten field codec.

#include once "fbserialize.bi"

sub check( byval condition as integer, byref detail as const string )
    if( condition = 0 ) then
        print "FAIL: " & detail & " ERR=" & err
        end 1
    end if
end sub

function make_text( ) as string
    return "temporary" & chr(0) & "result"
end function

type Item
    label_text as string
    quantity as integer
end type

type PlayerState
    player_name as string
    health as integer
    inventory(any, any) as Item
    fixed_items(-2 to 1) as Item
    title_text as string * 7
    child as Item
    scores(any) as double
end type

type PackedState field = 1
    marker as ubyte
    text_value as string
    values(any) as Item
end type

type DifferentState
    wrong_name as string
    health as integer
end type

sub copy_unknown_rank( source_values() as Item, target_values() as Item )
    check(Deserialize(Serialize(source_values()), target_values()), "unknown BYDESC rank")
end sub

sub reject_unknown_rank_change( source_values() as Item, target_values() as Item )
    dim as string before = Serialize(target_values())
    check(Deserialize(Serialize(source_values()), target_values()) = 0, "descriptor capacity rejection")
    check(Serialize(target_values()) = before, "descriptor capacity transactional")
end sub

dim as PlayerState source, target
source.player_name = "Player" & chr(0) & "One"
source.health = 100
source.title_text = "abcdefg"
source.child.label_text = "nested"
source.child.quantity = -99
redim source.inventory(-3 to -1, 5 to 6)
redim source.scores(2 to 3)
source.scores(2) = 1.25
source.scores(3) = -9.5
for i as long = -3 to -1
    for j as integer = 5 to 6
        source.inventory(i,j).label_text = str(i) & ":" & str(j)
        source.inventory(i,j).quantity = i * 10 + j
    next
next
source.fixed_items(-2).label_text = "fixed"
target.player_name = "old"
redim target.inventory(0 to 0, 0 to 0)
target.inventory(0,0).label_text = "old element"
dim as string payload = Serialize(source)
check(len(payload) > 0 andalso err = 0, "serialize UDT")
check(Deserialize(payload, target), "deserialize UDT")
check(target.player_name = source.player_name, "embedded null")
check(target.health = 100 and target.child.quantity = -99, "nested primitive")
check(target.child.label_text = "nested", "nested string")
check(target.title_text = "abcdefg", "fixed string")
check(lbound(target.inventory,1) = -3 and ubound(target.inventory,1) = -1, "first bounds")
check(lbound(target.inventory,2) = 5 and ubound(target.inventory,2) = 6, "second bounds")
check(target.inventory(-1,6).quantity = -4, "dynamic UDT values")
check(target.fixed_items(-2).label_text = "fixed", "fixed UDT array")
check(target.scores(2) = 1.25 and target.scores(3) = -9.5, "floating array")
source.player_name = "changed"
source.inventory(-1,6).label_text = "changed"
source.fixed_items(-2).label_text = "changed"
check(target.player_name = "Player" & chr(0) & "One", "independent string")
check(target.inventory(-1,6).label_text = "-1:6", "independent dynamic element")
check(target.fixed_items(-2).label_text = "fixed", "independent fixed element")

'' Every prefix, including ones failing after allocated children, must leave
'' the existing destination's complete logical state unchanged.
dim as string before = Serialize(target)
for i as integer = 0 to len(payload) - 1
    check(Deserialize(left(payload,i), target) = 0, "truncated rejection")
    check(err = FB_SERIALIZE_INVALID, "truncated error")
    check(Serialize(target) = before, "transactional failure")
next
check(Deserialize(payload & chr(0), target) = 0, "trailing rejection")
check(Serialize(target) = before, "trailing transactional")
dim as DifferentState incompatible
incompatible.wrong_name = "preserve"
check(Deserialize(payload, incompatible) = 0, "schema mismatch")
check(incompatible.wrong_name = "preserve", "schema transactional")

dim as PlayerState empty_source
check(Deserialize(Serialize(empty_source), target), "empty owned members")
check(target.player_name = "" and ubound(target.inventory,0) = 0, "unallocated array")
redim target.inventory(-3 to -1, 5 to 6)
target.inventory(-3,5).label_text = "usable after empty decode"

dim as PackedState packed_source, packed_target
packed_source.marker = 17
packed_source.text_value = "unaligned"
redim packed_source.values(-1 to 1)
packed_source.values(0).label_text = "owned"
check(Deserialize(Serialize(packed_source), packed_target), "packed descriptors")
check(packed_target.marker = 17 and packed_target.text_value = "unaligned", "packed values")
check(packed_target.values(0).label_text = "owned", "packed array")

dim as string text_source = "", text_target = "old"
check(Deserialize(Serialize(text_source), text_target), "empty scalar string")
check(text_target = "", "empty scalar value")
text_source = "a" & chr(0) & "b"
check(Deserialize(Serialize(text_source), text_target), "scalar string")
check(text_source = text_target and strptr(text_source) <> strptr(text_target), "scalar ownership")
check(Deserialize(Serialize(text_source), text_source), "self replacement")
for i as integer = 0 to 512
    check(Deserialize(Serialize(make_text()), text_target), "temporary source ownership")
    check(text_target = "temporary" & chr(0) & "result", "temporary source value")
next
check(Deserialize(Serialize(Serialize(text_source)), text_target), "nested serializer source")
check(Deserialize(text_target, text_source), "nested serializer value")

dim as string * 4 fixed_source = "abcd", fixed_target
check(Deserialize(Serialize(fixed_source), fixed_target), "scalar fixed string")
check(fixed_target = "abcd", "scalar fixed value")

dim as integer n_source = -2147483647, n_target
check(Deserialize(Serialize(n_source), n_target), "native integer")
check(n_source = n_target, "native integer value")
dim as uinteger u_source = 4294967295ull, u_target
check(Deserialize(Serialize(u_source), u_target), "native unsigned")
check(u_source = u_target, "native unsigned value")
dim as longint q_source = &h8000000000000000ll, q_target
check(Deserialize(Serialize(q_source), q_target), "longint")
check(q_source = q_target, "longint minimum")
dim as ulongint v_source = &hffffffffffffffffull, v_target
check(Deserialize(Serialize(v_source), v_target), "ulongint")
check(v_source = v_target, "ulongint maximum")
dim as byte b_source = -128, b_target
check(Deserialize(Serialize(b_source), b_target), "byte")
check(b_source = b_target, "byte minimum")
dim as short s_source = -32768, s_target
check(Deserialize(Serialize(s_source), s_target), "short")
check(s_source = s_target, "short minimum")
dim as long l_source = -2147483647 - 1, l_target
check(Deserialize(Serialize(l_source), l_target), "long")
check(l_source = l_target, "long minimum")
dim as single f_source = -1.5, f_target
check(Deserialize(Serialize(f_source), f_target), "single")
check(f_source = f_target, "single value")
dim as double d_source = -0.125, d_target
check(Deserialize(Serialize(d_source), d_target), "double")
check(d_source = d_target, "double value")
dim as boolean bool_source = true, bool_target
check(Deserialize(Serialize(bool_source), bool_target), "boolean")
check(bool_target = true, "boolean value")
dim as ushort us_source = 65535, us_target
check(Deserialize(Serialize(us_source), us_target), "ushort")
check(us_source = us_target, "ushort maximum")
dim as ulong ul_source = 4294967295ul, ul_target
check(Deserialize(Serialize(ul_source), ul_target), "ulong")
check(ul_source = ul_target, "ulong maximum")
dim as ubyte ub_source = 255, ub_target
check(Deserialize(Serialize(ub_source), ub_target), "ubyte")
check(ub_source = ub_target, "ubyte maximum")
enum StateCode
    state_negative = -1
    state_positive = 17
end enum
dim as StateCode enum_source = state_negative, enum_target
check(Deserialize(Serialize(enum_source), enum_target), "enum")
check(enum_target = state_negative, "enum value")
enum DifferentCode
    state_other = 18
end enum
dim as DifferentCode different_enum = state_other
check(Deserialize(Serialize(enum_source), different_enum) = 0, "enum schema rejection")
check(different_enum = state_other, "enum schema transactional")
'' Preserve IEEE representations, including negative zero, infinity and NaN.
dim as ulongint float_bits(0 to 2) = { &h8000000000000000ull, &h7ff0000000000000ull, &h7ff8000000001234ull }
for i as integer = 0 to 2
    *cptr(ulongint ptr, @d_source) = float_bits(i)
    check(Deserialize(Serialize(d_source), d_target), "IEEE double bit pattern")
    check(*cptr(ulongint ptr, @d_target) = float_bits(i), "IEEE double preservation")
next

dim fixed_array_source(-2 to 1) as string
dim fixed_array_target(-2 to 1) as string
fixed_array_source(-1) = "inline"
fixed_array_target(-1) = "replace"
check(Deserialize(Serialize(fixed_array_source()), fixed_array_target()), "root fixed array")
check(fixed_array_target(-1) = "inline", "root fixed element")
dim dynamic_array_source(any, any) as Item
dim dynamic_array_target(any, any) as Item
redim dynamic_array_source(-2 to 0, 3 to 4)
dynamic_array_source(0,4).label_text = "root"
check(Deserialize(Serialize(dynamic_array_source()), dynamic_array_target()), "root dynamic array")
check(dynamic_array_target(0,4).label_text = "root", "root dynamic element")
dynamic_array_target(0,4).label_text = "different"
check(dynamic_array_source(0,4).label_text = "root", "root independent ownership")
dim one_dimension_source(-1 to 1) as Item
dim one_dimension_target(any) as Item
one_dimension_source(0).label_text = "unknown rank"
copy_unknown_rank(one_dimension_source(), one_dimension_target())
check(one_dimension_target(0).label_text = "unknown rank", "unknown rank value")
reject_unknown_rank_change(dynamic_array_source(), one_dimension_target())
dim never_allocated() as long
dim also_never_allocated() as long
check(Deserialize(Serialize(never_allocated()), also_never_allocated()), "unknown unallocated rank")

print "SERIALIZATION_ROUNDTRIP=PASS"
'' end of roundtrip.bas
