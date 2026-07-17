''
'' FUNCTIONs can return their result BYREF, similar to BYREF parameters,
'' which can be used to avoid unnecessary copies, or to allow modification
'' of the variable/object returned from a function.
''

const DAT_FIRST = 0
const DAT_LAST = 4

dim shared dat(DAT_FIRST to DAT_LAST) as zstring * 512 = _
{ _
	( "stand still"        ), _
	( "jump the mountains" ), _
	( "walk the streets"   ), _
	( "swim the oceans"    ), _
	( "fly like the wind"  ) _
}

randomize( timer( ) )

function accessRandomElement( ) byref as zstring
	dim as integer index = DAT_FIRST + _
	                       int( rnd( ) * (DAT_LAST - DAT_FIRST + 1) )
	function = dat( index )
end function

const RANDOM_ACCESS_COUNT = 10

	for i as integer = 1 to RANDOM_ACCESS_COUNT
		dim byref element as zstring = accessRandomElement( )
		print element
		'' The table uses fixed-capacity ZSTRING entries, so this does not reallocate.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL503
		element += " +1"
	next
