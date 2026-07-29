/'
    FreeBASIC compiler regression test
    ----------------------------------

    File: lrset-udt-length.bas

    Purpose:

        Verify that LSET and RSET use an overloaded LEN() when a UDT extends
        ZSTRING or WSTRING.

    Responsibilities:

        - preserve embedded NUL characters covered by the overloaded length
        - evaluate a source expression, its cast, and its LEN() exactly once
        - cover both narrow and wide string runtime entry points

    This file intentionally does NOT contain:

        - dynamic STRING LSET/RSET coverage
        - structure-to-structure LSET coverage
'/

' TEST_MODE : COMPILE_AND_RUN_OK

dim shared as integer z_get_calls
dim shared as integer z_cast_calls
dim shared as integer z_len_calls
dim shared as integer w_get_calls
dim shared as integer w_cast_calls
dim shared as integer w_len_calls

type ZBOX extends zstring
	public:
		data as zstring * 16
		logical_length as integer

		declare operator cast( ) byref as zstring
end type

operator ZBOX.cast( ) byref as zstring
	z_cast_calls += 1
	operator = *cast( zstring ptr, @data )
end operator

operator len( byref value as const ZBOX ) as integer
	z_len_calls += 1
	return value.logical_length
end operator

dim shared as ZBOX z_source

private function getZSource( ) byref as ZBOX
	z_get_calls += 1
	return z_source
end function

type WBOX extends wstring
	public:
		data as wstring * 16
		logical_length as integer

		declare operator cast( ) byref as wstring
end type

operator WBOX.cast( ) byref as wstring
	w_cast_calls += 1
	operator = *cast( wstring ptr, @data )
end operator

operator len( byref value as const WBOX ) as integer
	w_len_calls += 1
	return value.logical_length
end operator

dim shared as WBOX w_source

private function getWSource( ) byref as WBOX
	w_get_calls += 1
	return w_source
end function

private function checkZBytes _
	( _
		byref value as ZBOX, _
		byval c0 as integer, _
		byval c1 as integer, _
		byval c2 as integer, _
		byval c3 as integer, _
		byval c4 as integer _
	) as integer

	return (value.data[0] = c0) and _
	       (value.data[1] = c1) and _
	       (value.data[2] = c2) and _
	       (value.data[3] = c3) and _
	       (value.data[4] = c4) and _
	       (value.data[5] = 0)
end function

private function checkWBytes _
	( _
		byref value as WBOX, _
		byval c0 as integer, _
		byval c1 as integer, _
		byval c2 as integer, _
		byval c3 as integer, _
		byval c4 as integer _
	) as integer

	return (value.data[0] = c0) and _
	       (value.data[1] = c1) and _
	       (value.data[2] = c2) and _
	       (value.data[3] = c3) and _
	       (value.data[4] = c4) and _
	       (value.data[5] = 0)
end function

scope
	dim as ZBOX dst
	dst.data = "xxxxx"
	dst.logical_length = 5

	z_source.data[0] = asc( "A" )
	z_source.data[1] = 0
	z_source.data[2] = asc( "B" )
	z_source.data[3] = asc( "C" )
	z_source.data[4] = 0
	z_source.logical_length = 4

	lset dst = getZSource( )
	if( checkZBytes( dst, asc( "A" ), 0, asc( "B" ), asc( "C" ), asc( " " ) ) = FALSE ) then
		end 1
	end if

	if( (z_get_calls <> 1) or (z_cast_calls <> 2) or (z_len_calls <> 2) ) then
		end 2
	end if

	dst.data = "xxxxx"
	rset dst = getZSource( )
	if( checkZBytes( dst, asc( " " ), asc( "A" ), 0, asc( "B" ), asc( "C" ) ) = FALSE ) then
		end 3
	end if

	if( (z_get_calls <> 2) or (z_cast_calls <> 4) or (z_len_calls <> 4) ) then
		end 4
	end if
end scope

scope
	dim as WBOX dst
	dst.data = wstr( "xxxxx" )
	dst.logical_length = 5

	w_source.data[0] = asc( "A" )
	w_source.data[1] = 0
	w_source.data[2] = asc( "B" )
	w_source.data[3] = asc( "C" )
	w_source.data[4] = 0
	w_source.logical_length = 4

	lset dst = getWSource( )
	if( checkWBytes( dst, asc( "A" ), 0, asc( "B" ), asc( "C" ), asc( " " ) ) = FALSE ) then
		end 5
	end if

	if( (w_get_calls <> 1) or (w_cast_calls <> 2) or (w_len_calls <> 2) ) then
		end 6
	end if

	dst.data = wstr( "xxxxx" )
	rset dst = getWSource( )
	if( checkWBytes( dst, asc( " " ), asc( "A" ), 0, asc( "B" ), asc( "C" ) ) = FALSE ) then
		end 7
	end if

	if( (w_get_calls <> 2) or (w_cast_calls <> 4) or (w_len_calls <> 4) ) then
		end 8
	end if
end scope

end 0

/' end of lrset-udt-length.bas '/
