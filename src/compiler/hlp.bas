'' misc helpers
''
''


#include once "fb.bi"
#include once "fbint.bi"
#include once "ir.bi"
#include once "lex.bi"
#include once "dstr.bi"

'':::::
function hHexUInt _
	( _
		byval value as uinteger _
	) as zstring ptr static

	static as zstring * 8 + 1 res
	dim as zstring ptr p
	dim as integer lgt, maxlen
	const USHORT_MAX = 65535

	static as integer hexTB(0 to 15) = _
	{ _
		asc( "0" ), asc( "1" ), asc( "2" ), asc( "3" ), _
		asc( "4" ), asc( "5" ), asc( "6" ), asc( "7" ), _
		asc( "8" ), asc( "9" ), asc( "A" ), asc( "B" ), _
		asc( "C" ), asc( "D" ), asc( "E" ), asc( "F" ) _
	}

	maxlen = 4
	if( value > USHORT_MAX ) then
		maxlen = 8
	end if

	p = @res + 8-1
	lgt = 0

	do
		*p = hexTB( value and &h0000000F )

		lgt +=1
		if( lgt = maxlen ) then
			exit do
		end if

		p -= 1
		value shr= 4
	loop

	function = p

end function

function hFloatToHex _
	( _
		byval value as double, _
		byval dtype as integer _
	) as string

	'' Emit the raw bits that make up the float
	if( typeGet( dtype ) = FB_DATATYPE_DOUBLE ) then
		function = "0x" + hex( *cptr( ulongint ptr, @value ), 16 )
	else
		dim as single singlevalue = value
		'' Using an intermediate uinteger to allow compiling with FB
		'' versions before the overload resolution overhaul
		function = "0x" + hex( cuint( *cptr( ulong ptr, @singlevalue ) ), 8 )
	end if
end function

function hFloatToHex_C99 _
	( _
		byval value as double _
	) as string

	'' float hex format defined in C99 spec: e.g. 0x1.fp+3
	const DOUBLE_EXPONENT_BIAS = 1023
	const DOUBLE_MANTISSA_HEX_DIGITS = 13

	dim as ulongint n = *cptr( ulongint ptr, @value )

	dim as integer sign = n shr 63
	dim as integer exp2 = (n shr 52) and (1u shl 11 - 1)
	dim as ulongint mantissa = n and (1ull shl 52 - 1)

	dim as string ret

	if( sign <> 0 ) then
		'' negative
		ret = "-0x"
	else
		'' positive
		ret = "0x"
	end if

	exp2 -= DOUBLE_EXPONENT_BIAS
	if( exp2 > -DOUBLE_EXPONENT_BIAS ) then
		'' normalized
		ret += "1." + hex( mantissa, DOUBLE_MANTISSA_HEX_DIGITS )
		if right( ret, 1 ) = "0" then ret = rtrim( ret, "0" )
	else
		if mantissa = 0 then
			'' zero
			ret += "0"
			exp2 = 0
		else
			'' denormed
			exp2 += 1
			ret += "0." + hex( mantissa, DOUBLE_MANTISSA_HEX_DIGITS )
			if right( ret, 1 ) = "0" then ret = rtrim( ret, "0" )
		end if
	end if

	ret += "p" & (*iif( exp2 >= 0, @"+", @"-" )) + str( abs( exp2 ) )

	return ret

end function

'':::::
function hFBrelop2IRrelop _
	( _
		byval tk as integer _
	) as integer static

	dim as integer op = any

	select case as const tk
	case FB_TK_EQ
		op = AST_OP_EQ
	case FB_TK_GT
		op = AST_OP_GT
	case FB_TK_LT
		op = AST_OP_LT
	case FB_TK_NE
		op = AST_OP_NE
	case FB_TK_LE
		op = AST_OP_LE
	case FB_TK_GE
		op = AST_OP_GE
	case else
		errReport( FB_ERRMSG_EXPECTEDRELOP )
		'' error recovery: fake an op
		op = AST_OP_EQ
	end select

	function = op

end function

'':::::
function hFileExists _
	( _
		byval filename as zstring ptr _
	) as integer static
	dim f as integer
#if defined( __FB_CYGWIN__ )
	dim as string normalized
	normalized = pathNormalizeHost( *filename )
	filename = strptr( normalized )
#endif

	f = freefile

	if( open( *filename, for input, as #f ) = 0 ) then
		function = TRUE
		close #f
	else
		function = FALSE
	end if

end function

'':::::
sub hUcase _
	( _
		byval src as const zstring ptr, _
		byval dst as zstring ptr _
	) static

	dim as integer c
	dim as const zstring ptr s
	dim as zstring ptr d
	const ASCII_CASE_OFFSET = CHAR_ALOW - CHAR_AUPP

	s = cast( zstring ptr, src )
	d = dst

	do
		c = *s
		if( c >= CHAR_ALOW ) then
			if( c <= CHAR_ZLOW ) then
				c -= ASCII_CASE_OFFSET
			end if
		elseif( c = 0 ) then
			exit do
		end if

		*d = c

		s += 1
		d += 1
	loop

	'' null-term
	*d = 0

end sub

'':::::
sub hClearName _
	( _
		byval src as zstring ptr _
	) static

	if( src = NULL ) then
		exit sub
	end if

	do
		select case as const *src
		case 0
			exit do

		case CHAR_AUPP to CHAR_ZUPP, CHAR_ALOW to CHAR_ZLOW, CHAR_0 to CHAR_9, CHAR_UNDER

		case else
			*src = CHAR_ZLOW
		end select

		src += 1
	loop

end sub

'' Searches backwards for the last '.' while still behind '/' or '\'.
private function hFindExtBegin( byref path as string ) as integer
	for i as integer = len( path )-1 to 0 step -1
		select case( path[i] )
		case asc( "." )
			return i
#if defined( __FB_WIN32__ ) or defined( __FB_DOS__ )
		case asc( "\" ), asc( "/" )
#else
		case asc( "/" )
#endif
			exit for
		end select
	next
	function = len( path )
end function

function hStripExt( byref path as string ) as string
	function = left( path, hFindExtBegin( path ) )
end function

'':::::
function hStripPath _
	( _
		byval filename as zstring ptr _
	) as string static

	dim as integer lp, p_found, p(1 to 2)

	lp = 0
	do
		p(1) = instr( lp+1, *filename, RSLASH )
		p(2) = instr( lp+1, *filename, "/" )
		if p(1)=0 or (p(2)>0 and p(2)<p(1)) then
			p_found = p(2)
		else
			p_found = p(1)
		end if
		if( p_found = 0 ) then
			exit do
		end if
		lp = p_found
	loop

	if( lp > 0 ) then
		function = mid( *filename, lp+1 )
	else
		function = *filename
	end if

end function

'':::::
function hStripFilename _
	( _
		byval filename as zstring ptr _
	) as string static

	dim as integer lp, p_found, p(1 to 2)

	lp = 0
	do
		p(1) = instr( lp+1, *filename, RSLASH )
		p(2) = instr( lp+1, *filename, "/" )
		if p(1)=0 or (p(2)>0 and p(2)<p(1)) then
			p_found = p(2)
		else
			p_found = p(1)
		end if
		if( p_found = 0 ) then
			exit do
		end if
		lp = p_found
	loop

	if( lp > 0 ) then
		function = left( *filename, lp )
	else
		function = ""
	end if

end function

'':::::
function hGetFileExt _
	( _
		byval fname as zstring ptr _
	) as string static

	dim as integer p, lp
	dim as string res

	lp = 0
	do
		p = instr( lp+1, *fname, "." )
		if( p = 0 ) then
			exit do
		end if
		lp = p
	loop

	if( lp = 0 ) then
		function = ""
	else
		res = lcase( mid( *fname, lp+1 ) )
		if instr( res, RSLASH ) > 0 or instr( res, "/" ) > 0 then
			'' We had a folder with a "." inside ...
			function = ""
		elseif( len(res) > 0 ) then
			'' . or .. dirs?
			if( res[0] = asc( RSLASH ) or res[0] = asc( "/" ) ) then
				function = ""
			else
				function = res
			end if
		end if
	end if

end function

sub hReplaceSlash( byval s as zstring ptr, byval char as integer )
	for i as integer = 0 to len( *s ) - 1
		if( (s[i] = CHAR_RSLASH) or (s[i] = CHAR_SLASH) ) then
			s[i] = char
		end if
	next
end sub

#if defined( __FB_CYGWIN__ )
private function hCygwinDrivePrefix( ) as string
	static as integer initialized = FALSE
	static as string prefix

	if( initialized = FALSE ) then
		/'
			Cygwin uses /cygdrive/c/... for Win32 drive paths, but
			MSYS2 uses /c/...

			The compiler can be run from either shell when building
			cross packages on Windows.  If we always normalize C:
			paths to /cygdrive/c under MSYS2, hFileExists() can find
			a relative include first, then the later absolute open()
			fails after the path has been rewritten to a mount point
			that does not exist in that process environment.
		'/
		if( len( environ( "MSYSTEM" ) ) > 0 ) then
			prefix = "/"
		else
			prefix = "/cygdrive/"
		end if

		initialized = TRUE
	end if

	function = prefix
end function
#endif

function pathNormalizeHost( byref path as string ) as string
#if defined( __FB_CYGWIN__ )
	dim as string normalized
	const WINDOWS_DRIVE_PATH_PREFIX_LENGTH = 2

	normalized = path

	hReplaceSlash( strptr( normalized ), asc( "/" ) )

	if( len( normalized ) >= WINDOWS_DRIVE_PATH_PREFIX_LENGTH ) then
		if( normalized[1] = asc( ":" ) ) then
			dim as integer drive = normalized[0]

			if( ((drive >= asc( "A" )) and (drive <= asc( "Z" ))) or _
			    ((drive >= asc( "a" )) and (drive <= asc( "z" ))) ) then
				dim as string drivepath = hCygwinDrivePrefix( ) + lcase( chr( drive ) )

				if( len( normalized ) = WINDOWS_DRIVE_PATH_PREFIX_LENGTH ) then
					function = drivepath
					exit function
				elseif( normalized[WINDOWS_DRIVE_PATH_PREFIX_LENGTH] = asc( "/" ) ) then
					function = drivepath + mid( normalized, WINDOWS_DRIVE_PATH_PREFIX_LENGTH + 1 )
					exit function
				end if
			end if
		end if
	end if

	function = normalized
#else
	function = path
#endif
end function

function pathStripDiv( byref path as string ) as string
	dim as integer length = len( path )
	if( length > 0 ) then
		length -= 1
		select case( path[length] )
#if defined( __FB_WIN32__ ) or defined( __FB_DOS__ )
		case asc("/"), asc("\")
#else
		case asc("/")
#endif
			return left( path, length )
		end select
	end if
	function = path
end function

function pathIsAbsolute( byval path as zstring ptr ) as integer
#if defined( __FB_WIN32__ ) or defined( __FB_DOS__ )
	if( (*path)[0] <> 0 ) then
		select case( (*path)[1] )
		case asc( ":" )
			'' C:...
			function = TRUE
#ifdef __FB_WIN32__
		case asc( "\" )
			'' \\... UNC path
			function = ((*path)[0] = asc( "\" ))
#endif
		end select
	end if
#else
	'' /...
	function = ((*path)[0] = asc( "/" ))
#endif
end function

function hCheckFileFormat( byval f as integer ) as integer
	dim as ubyte BOM(0 to 3)
	dim as FBFILE_FORMAT fmt
	const FILE_START_POSITION = 1
	const BOM_FIRST_BYTE = 0
	const BOM_SECOND_BYTE = 1
	const BOM_THIRD_BYTE = 2
	const BOM_FOURTH_BYTE = 3
	const UTF8_BOM_LENGTH = 3
	const UTF16_BOM_LENGTH = 2
	const UTF32_BOM_LENGTH = 4
	static as ubyte UTF32BE_BOM(0 to UTF32_BOM_LENGTH - 1) => { &h00, &h00, &hFE, &hFF }
	static as ubyte UTF32LE_BOM(0 to UTF32_BOM_LENGTH - 1) => { &hFF, &hFE, &h00, &h00 }
	static as ubyte UTF8_BOM(0 to UTF8_BOM_LENGTH - 1) => { &hEF, &hBB, &hBF }
	static as ubyte UTF16LE_BOM(0 to UTF16_BOM_LENGTH - 1) => { &hFF, &hFE }
	static as ubyte UTF16BE_BOM(0 to UTF16_BOM_LENGTH - 1) => { &hFE, &hFF }

	fmt = FBFILE_FORMAT_ASCII

	if( get( #f, 0, BOM() ) = 0 ) then
		if( (BOM(BOM_FIRST_BYTE) = UTF32BE_BOM(BOM_FIRST_BYTE)) andalso _
		    (BOM(BOM_SECOND_BYTE) = UTF32BE_BOM(BOM_SECOND_BYTE)) andalso _
		    (BOM(BOM_THIRD_BYTE) = UTF32BE_BOM(BOM_THIRD_BYTE)) andalso _
		    (BOM(BOM_FOURTH_BYTE) = UTF32BE_BOM(BOM_FOURTH_BYTE)) ) then
			fmt = FBFILE_FORMAT_UTF32BE

		elseif( (BOM(BOM_FIRST_BYTE) = UTF32LE_BOM(BOM_FIRST_BYTE)) andalso _
		        (BOM(BOM_SECOND_BYTE) = UTF32LE_BOM(BOM_SECOND_BYTE)) andalso _
		        (BOM(BOM_THIRD_BYTE) = UTF32LE_BOM(BOM_THIRD_BYTE)) andalso _
		        (BOM(BOM_FOURTH_BYTE) = UTF32LE_BOM(BOM_FOURTH_BYTE)) ) then
		    fmt = FBFILE_FORMAT_UTF32LE

		else
			if( (BOM(BOM_FIRST_BYTE) = UTF8_BOM(BOM_FIRST_BYTE)) andalso _
			    (BOM(BOM_SECOND_BYTE) = UTF8_BOM(BOM_SECOND_BYTE)) andalso _
			    (BOM(BOM_THIRD_BYTE) = UTF8_BOM(BOM_THIRD_BYTE)) ) then
				fmt = FBFILE_FORMAT_UTF8

			else
				if( (BOM(BOM_FIRST_BYTE) = UTF16LE_BOM(BOM_FIRST_BYTE)) andalso _
				    (BOM(BOM_SECOND_BYTE) = UTF16LE_BOM(BOM_SECOND_BYTE)) ) then
					fmt = FBFILE_FORMAT_UTF16LE

				elseif( (BOM(BOM_FIRST_BYTE) = UTF16BE_BOM(BOM_FIRST_BYTE)) andalso _
				        (BOM(BOM_SECOND_BYTE) = UTF16BE_BOM(BOM_SECOND_BYTE)) ) then
					fmt = FBFILE_FORMAT_UTF16BE
				end if
			end if
		end if

		select case fmt
		case FBFILE_FORMAT_ASCII
			seek #f, FILE_START_POSITION

		case FBFILE_FORMAT_UTF8
			seek #f, FILE_START_POSITION + UTF8_BOM_LENGTH

		case FBFILE_FORMAT_UTF16LE, _
			 FBFILE_FORMAT_UTF16BE
			seek #f, FILE_START_POSITION + UTF16_BOM_LENGTH

		case FBFILE_FORMAT_UTF32LE, _
			 FBFILE_FORMAT_UTF32BE
			seek #f, FILE_START_POSITION + UTF32_BOM_LENGTH
		end select
	end if

	function = fmt
end function

function hCurDir( ) as string
	'' curdir() usually won't be terminated with a path separator,
	'' except when it points to the file system root, instead of
	'' some directory (e.g. C:\ on Win32 or / on Unix).
	function = pathStripDiv( pathNormalizeHost( curdir( ) ) )
end function

function pathStripCurdir( byref path as string ) as string
	var pwd = hCurdir( ) + FB_HOST_PATHDIV
	if( left( path, len( pwd ) ) = pwd ) then
		function = right( path, len( path ) - len( pwd ) )
	else
		function = path
	end if
end function

function hIsValidSymbolName( byval sym as zstring ptr ) as integer

	if( sym = NULL ) then exit function

	var symlen = len( *sym )

	if( symlen = 0 ) then exit function

	if( (hIsChar(sym[0]) orelse (sym[0] = asc("_"))) = FALSE ) then exit function

	for i as integer = 1 to symlen-1
		if( ((hIsChar(sym[i])) orelse (sym[i] = asc("_")) orelse (hIsCharNumeric(sym[i]))) = FALSE ) then exit function
	next

	function = TRUE

end function

'' Checks whether a string starts with and ends in [double-]quotes.
private function strIsQuoted(byref s as string) as integer
	dim as integer last = len(s) - 1
	if (last < 1) then
		return FALSE
	end if

	return (((s[0] = asc("""")) and (s[last] = asc(""""))) or _
	        ((s[0] = asc("'" )) and (s[last] = asc("'" ))))
end function

function strUnquote(byref s as string) as string
	if (strIsQuoted(s)) then
		return mid(s, 2, len(s) - 2)
	end if
	return s
end function
