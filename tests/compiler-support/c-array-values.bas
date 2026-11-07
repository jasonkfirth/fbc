/'
    Project: FreeBASIC C array value regressions
    File: c-array-values.bas
    Purpose: Preserve native scaling for actual fixed-array values.
    Responsibilities: Check typed loads, stores, callbacks, local bounds and packed fields.
    This file intentionally does NOT allocate large arrays or touch unowned storage.
'/
#lang "fb"

type PackedPair field = 1
    small_value as ushort
    large_value as ulong
end type

type WidePair
    first_value as ulongint
    second_value as ulongint
end type

type PackedOwner field = 1
    leading as ubyte
    items(0 to 2) as ulong
    trailing as ubyte
end type

declare function plus_ten cdecl(byval value as long) as long
declare function plus_twenty cdecl(byval value as long) as long

dim shared shorts(0 to 2) as ushort = {3, 17, 31}
dim shared longs(0 to 2) as long = {5, 19, 37}
dim shared wide_values(0 to 2) as ulongint = {7, 23, 41}
dim shared singles(0 to 2) as single = {0.5!, 1.25!, 2.5!}
dim shared doubles(0 to 2) as double = {0.5, 1.25, 2.5}
dim shared pointer_values(0 to 2) as any ptr
dim shared callbacks(0 to 2) as function cdecl(byval as long) as long = {@plus_ten, @plus_ten, @plus_ten}
dim shared packed_values(0 to 2) as PackedPair
dim shared wide_pairs(0 to 2) as WidePair
dim shared labels(0 to 2) as string
dim shared packed_owner as PackedOwner
dim shared as long target_value = 53
dim shared as uinteger index_calls

function plus_ten cdecl(byval value as long) as long
    return value + 10
end function

function plus_twenty cdecl(byval value as long) as long
    return value + 20
end function

function next_index(byval value as uinteger) as uinteger
    index_calls += 1
    return value
end function

function read_pair(byval index_value as uinteger) as PackedPair
    return packed_values(index_value)
end function

sub exercise_values(byval index_value as uinteger)
    if shorts(index_value) <> 17 then end 10
    if longs(index_value) <> 19 then end 11
    if wide_values(index_value) <> 23 then end 12
    if abs(singles(index_value) - 1.25!) > 0.00001! then end 13
    if abs(doubles(index_value) - 1.25) > 0.00001 then end 14
    if pointer_values(index_value) <> @target_value then end 15
    if callbacks(index_value)(5) <> 15 then end 16
    dim as PackedPair pair_copy = read_pair(index_value)
    if pair_copy.small_value <> 11 or pair_copy.large_value <> 13 then end 17
    if wide_pairs(index_value).first_value <> 29 or wide_pairs(index_value).second_value <> 31 then end 18
    if packed_owner.items(index_value) <> 43 then end 19
    if labels(index_value) <> "middle" then end 20

    shorts(index_value) = 61
    longs(index_value) = 67
    wide_values(index_value) = 71
    singles(index_value) = 2.5!
    doubles(index_value) = 2.5
    pointer_values(index_value) = 0
    callbacks(index_value) = @plus_twenty
    pair_copy.small_value = 73
    pair_copy.large_value = 79
    packed_values(index_value) = pair_copy
    wide_pairs(index_value).first_value = 83
    wide_pairs(index_value).second_value = 89
    packed_owner.items(index_value) = 97
    labels(index_value) = "changed"

    if shorts(1) <> 61 or longs(1) <> 67 or wide_values(1) <> 71 then end 21
    if abs(singles(1) - 2.5!) > 0.00001! or abs(doubles(1) - 2.5) > 0.00001 then end 22
    if pointer_values(1) <> 0 or callbacks(1)(5) <> 25 then end 23
    if packed_values(1).small_value <> 73 or packed_values(1).large_value <> 79 then end 24
    if wide_pairs(1).first_value <> 83 or wide_pairs(1).second_value <> 89 then end 25
    if packed_owner.items(1) <> 97 or packed_owner.leading <> &hA5 or packed_owner.trailing <> &h5A then end 26
    if labels(1) <> "changed" or labels(0) <> "left" or labels(2) <> "right" then end 27
    if shorts(0) <> 3 or shorts(2) <> 31 or longs(0) <> 5 or longs(2) <> 37 then end 28
    if wide_values(0) <> 7 or wide_values(2) <> 41 then end 29

    index_calls = 0
    longs(next_index(index_value)) = 101
    if longs(next_index(index_value)) <> 101 or index_calls <> 2 then end 30

    shorts(1) = 17
    longs(1) = 19
    wide_values(1) = 23
    singles(1) = 1.25!
    doubles(1) = 1.25
    pointer_values(1) = @target_value
    callbacks(1) = @plus_ten
    packed_values(1).small_value = 11
    packed_values(1).large_value = 13
    wide_pairs(1).first_value = 29
    wide_pairs(1).second_value = 31
    packed_owner.items(1) = 43
    labels(1) = "middle"
end sub

sub exercise_local_bounds(byval alias_bit as uinteger)
    dim negative_bounds(-3 to -1) as long = {5, 7, 11}
    dim positive_bounds(7 to 9) as ushort = {13, 17, 19}
    const minimum_bound as integer = cast(integer, cuint(1) shl (sizeof(uinteger) * 8 - 1))
    dim minimum_bounds(minimum_bound to minimum_bound + 2) as long = {23, 29, 31}
    dim as uinteger negative_index = cuint(-2) xor alias_bit
    dim as uinteger positive_index = cuint(8) xor alias_bit
    dim as uinteger minimum_index = cuint(minimum_bound + 1) xor alias_bit
    if negative_bounds(negative_index) <> 7 then end 40
    if positive_bounds(positive_index) <> 17 then end 41
    if minimum_bounds(minimum_index) <> 29 then end 42
    negative_bounds(negative_index) = 37
    positive_bounds(positive_index) = 41
    minimum_bounds(minimum_index) = 43
    if negative_bounds(-2) <> 37 or negative_bounds(-3) <> 5 or negative_bounds(-1) <> 11 then end 43
    if positive_bounds(8) <> 41 or positive_bounds(7) <> 13 or positive_bounds(9) <> 19 then end 44
    if minimum_bounds(minimum_bound + 1) <> 43 or minimum_bounds(minimum_bound) <> 23 or _
        minimum_bounds(minimum_bound + 2) <> 31 then end 45
end sub

pointer_values(1) = @target_value
packed_values(1).small_value = 11
packed_values(1).large_value = 13
wide_pairs(1).first_value = 29
wide_pairs(1).second_value = 31
packed_owner.leading = &hA5
packed_owner.items(1) = 43
packed_owner.trailing = &h5A
labels(0) = "left"
labels(1) = "middle"
labels(2) = "right"

dim as uinteger alias_bit = cuint(1) shl (sizeof(uinteger) * 8 - 1)
#ifdef CHECKED_PROOF
    alias_bit = 0
#endif
#ifdef EXPECT_BOUNDS_ERROR
    exercise_values(alias_bit or 1)
    end 99
#endif
exercise_values(1)
exercise_values(alias_bit or 1)
exercise_local_bounds(0)
exercise_local_bounds(alias_bit)
print "ARRAY_VALUE_MATRIX_PASS"

' end of c-array-values.bas
