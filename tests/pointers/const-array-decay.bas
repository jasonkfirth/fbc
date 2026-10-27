/'
    Project: FreeBASIC C backend regression tests
    File: const-array-decay.bas
    Purpose: Preserve qualified element types when C arrays decay to pointers.
    Responsibilities: Check narrow, wide and nested pointer tables and slot writes.
    This file intentionally does NOT write literal bytes or change allocator ownership.
'/
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fb"

'' Two dimensions keep these accesses on the general array-address path.
'' Pointer slots remain writable; the text reached through them is read-only.
dim shared as const zstring ptr narrow_table(-1 to 0, 2 to 3) = { _
	{ @"alpha", @"beta" }, _
	{ @"gamma", @"delta" } _
}

dim shared as const wstring ptr wide_table(-1 to 0, 2 to 3) = { _
	{ @wstr( "wide-alpha" ), @wstr( "wide-beta" ) }, _
	{ @wstr( "wide-gamma" ), @wstr( "wide-delta" ) } _
}

'' Each entry points to a read-only view of a pointer slot. Replacing an
'' entry in this outer table does not permit writes through that inner view.
dim shared as const zstring const ptr ptr nested_table(-1 to 0, 2 to 3) = { _
	{ @narrow_table(-1, 2), @narrow_table(-1, 3) }, _
	{ @narrow_table(0, 2), @narrow_table(0, 3) } _
}

'' String-value lowering reads a generic raw-string pointer from this
'' one-dimensional table. This matches the compiler's register-name tables;
'' simply returning a typed pointer can use the separate typed-index path.
dim shared as const zstring ptr raw_narrow_table(-1 to 2) = { _
	@"alpha", @"beta", @"gamma", @"delta" _
}

private function read_narrow_value( byval table_index as integer ) as string

	return *raw_narrow_table(table_index)

end function

private function narrow_value_length( byval table_index as integer ) as integer

	return len( *raw_narrow_table(table_index) )

end function

private function concatenate_narrow_value( byval table_index as integer ) as string

	return "read: " + *raw_narrow_table(table_index)

end function

private function read_narrow( byval row_index as integer, byval column_index as integer ) as const zstring ptr

	return narrow_table(row_index, column_index)

end function

private function read_wide( byval row_index as integer, byval column_index as integer ) as const wstring ptr

	return wide_table(row_index, column_index)

end function

private function read_nested( byval row_index as integer, byval column_index as integer ) as const zstring const ptr ptr

	return nested_table(row_index, column_index)

end function

private sub inspect_narrow( byval text as const zstring ptr, byval expected as const zstring ptr )

	if( text = 0 ) then end 1
	if( *text <> *expected ) then end 2

end sub

private sub inspect_wide( byval text as const wstring ptr, byval expected as const wstring ptr )

	if( text = 0 ) then end 3
	if( *text <> *expected ) then end 4

end sub

private sub inspect_nested( byval slot_address as const zstring const ptr ptr, byval expected as const zstring ptr )

	if( slot_address = 0 ) then end 5
	if( *slot_address = 0 ) then end 6
	if( **slot_address <> *expected ) then end 7

end sub

private sub replace_narrow( byval slot_address as const zstring ptr ptr, byval replacement as const zstring ptr )

	*slot_address = replacement

end sub

private sub replace_wide( byval slot_address as const wstring ptr ptr, byval replacement as const wstring ptr )

	*slot_address = replacement

end sub

dim as integer row_index = -1, column_index = 2
inspect_narrow( read_narrow( row_index, column_index ), @"alpha" )
inspect_wide( read_wide( row_index, column_index ), @wstr( "wide-alpha" ) )
inspect_nested( read_nested( row_index, column_index ), @"alpha" )

row_index = 0
column_index = 3
inspect_narrow( read_narrow( row_index, column_index ), @"delta" )
inspect_wide( read_wide( row_index, column_index ), @wstr( "wide-delta" ) )
inspect_nested( read_nested( row_index, column_index ), @"delta" )

'' Address-taking must preserve writable pointer slots without allowing
'' a store into the literal text. Existing nested views observe the new slot.
dim as const zstring ptr ptr narrow_slot = @narrow_table(row_index, column_index)
dim as const wstring ptr ptr wide_slot = @wide_table(row_index, column_index)
replace_narrow( narrow_slot, @"replacement" )
replace_wide( wide_slot, @wstr( "wide-replacement" ) )
inspect_narrow( read_narrow( row_index, column_index ), @"replacement" )
inspect_wide( read_wide( row_index, column_index ), @wstr( "wide-replacement" ) )
inspect_nested( read_nested( row_index, column_index ), @"replacement" )

dim as const zstring const ptr ptr ptr nested_slot = @nested_table(row_index, column_index)
*nested_slot = @narrow_table(-1, 2)
inspect_nested( read_nested( row_index, column_index ), @"alpha" )

'' A mutable array remains mutable after the qualifier-preserving decay.
'' This backing object is separate storage, so changing it cannot touch a literal.
dim as zstring * 16 mutable_text = "mutable"
dim as zstring ptr mutable_table(-1 to 0, 2 to 3) = { _
	{ @mutable_text, @mutable_text }, _
	{ @mutable_text, @mutable_text } _
}
dim as zstring ptr ptr mutable_slot = @mutable_table(row_index, column_index)
(*mutable_slot)[0] = asc( "M" )
inspect_narrow( *mutable_slot, @"Mutable" )

dim as integer raw_index = -1
if( read_narrow_value( raw_index ) <> "alpha" ) then end 8
raw_index = 2
if( narrow_value_length( raw_index ) <> 5 ) then end 9
if( concatenate_narrow_value( raw_index ) <> "read: delta" ) then end 10
raw_index = 1
dim as string raw_copy = *raw_narrow_table(raw_index)
if( raw_copy <> "gamma" ) then end 11

print "const-array-decay-ok"
end 0

' end of const-array-decay.bas
