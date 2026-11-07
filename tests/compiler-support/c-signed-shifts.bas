/'
    Project: FreeBASIC C backend regression
    File: c-signed-shifts.bas
    Purpose: Check signed left shifts and negative pointer offsets.
    Responsibilities: Exercise operand widths, counts and single evaluation.
    This fixture intentionally does NOT dereference synthetic pointers.
'/
dim shared as integer calls
private function next_value() as longint
    calls += 1
    return -3
end function
private function next_count() as integer
    calls += 1
    return 1
end function

dim as byte small = -64
dim as short medium = -16384
dim as long word = -1073741824
dim as longint wide = -4611686018427387904ll
dim as longint minus_one = -1
dim as longint minimum = -9223372036854775807ll - 1
dim as longint positive = 4611686018427387904ll
dim as integer count = 1
if (small shl count) <> -128 then end 1
if (medium shl count) <> -32768 then end 2
if (word shl count) <> -2147483648ll then end 3
if (wide shl count) <> minimum then end 4
if (next_value() shl next_count()) <> -6 then end 5
if calls <> 2 then end 6
count = 2
if (positive shl count) <> 0 then end 7
count = -1
if (minus_one shl count) <> minimum then end 8
if (word shr 1) <> -536870912 then end 9

dim as integer ptr p = cptr(integer ptr, &h1000)
dim as integer index_value = -1
dim as integer ptr first_index = @index_value, second_index = first_index
if @p[*first_index + *second_index] <> cptr(integer ptr, &h1000 - 2 * sizeof(integer)) then end 10
end 0
' end of c-signed-shifts.bas
