'' FreeBASIC TRE examples
'' File: subexpr.bas
'' Purpose: Split filename text using numbered captures.
'' Responsibilities: Check optional capture_offsets offsets and release the compiled pattern.
'' This file contains no filesystem access or TRE implementation.

''
'' regular expression example
''

#include  "regex.bi"

enum eFilePartKind
	eFPK_Full
	eFPK_Path
	eFPK_File
	eFPK_Basename
	eFPK_Extension
end enum

private function captured_text( byref text as string, byref capture_offsets as regmatch_t ) as string
	if capture_offsets.rm_so < 0 or capture_offsets.rm_eo < capture_offsets.rm_so or capture_offsets.rm_eo > len(text) then return ""
	return mid(text, capture_offsets.rm_so+1, capture_offsets.rm_eo-capture_offsets.rm_so)
end function

function get_filepart( byref buffer as string, byval kind as eFilePartKind ) as string
	dim re as regex_t
	dim pm as regmatch_t
	dim pbuff as zstring ptr
	dim res as integer
	dim nsub as integer

	if len(buffer)=0 then exit function

	pbuff = strptr( buffer )

	'' compile the pattern
	if regcomp( @re, "^(.*/)*([^/.]*)(((\.[^.]*)*)(\.[^.]*))?", REG_EXTENDED or REG_ICASE )<>0 then
		exit function
	end if

	nsub = re.re_nsub + 1
	redim match(1 to nsub) as regmatch_t

	'' first match
	res = regexec( @re, pbuff, nsub, @match(1), 0 )
	if ( res = 0 ) then

		select case kind
		case eFPK_Full
			function = buffer
		case eFPK_Path
			function = captured_text(buffer, match(2))
		case eFPK_File
			function = captured_text(buffer, match(3)) + _
			           captured_text(buffer, match(4))
		case eFPK_Basename
			function = captured_text(buffer, match(3)) + _
			           captured_text(buffer, match(5))
		case eFPK_Extension
			function = captured_text(buffer, match(7))
		end select
	end if
	regfree(@re)
end function

private sub ShowAll( byref s as string )
	print "Full:", s
	print "Path:", get_filepart(s, eFPK_Path)
	print "File:", get_filepart(s, eFPK_File)
	print "Base:", get_filepart(s, eFPK_Basename)
	print "Ext :", get_filepart(s, eFPK_Extension)
	print
end sub

#ifndef REGEX_EXAMPLE_NO_MAIN
ShowAll "path/name.ext"
ShowAll "path/name.ext1.ext2"
ShowAll "path/name.ext1.ext2.ext3"
ShowAll "path/name"
ShowAll "path/"
ShowAll "name.ext"
ShowAll "name.ext1.ext2"
ShowAll "name.ext1.ext2.ext3"
ShowAll "name"
ShowAll ".ext"
ShowAll ".ext1.ext2"
ShowAll ".ext1.ext2.ext3"
ShowAll ""
#endif

'' end of subexpr.bas
