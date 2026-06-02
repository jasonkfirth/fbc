' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC runtime directory tests
'' ---------------------------------
''
'' File: dir-runtime.bas
''
'' Purpose:
''
''     Exercise DIR() and basic directory operations at runtime.
''
'' Responsibilities:
''
''     - create a private directory tree
''     - enumerate files and subdirectories with DIR()
''     - verify directory attributes are reported for subdirectories
''     - verify CHDIR/CURDIR round-trips through the runtime
''
'' This file intentionally does NOT contain:
''
''     - platform-specific filesystem permission checks
''     - symbolic link handling
''     - long-path stress testing
''

#include once "dir.bi"

const ROOT_DIR = "file/dir-runtime.tmp"
const CHILD_DIR = ROOT_DIR + "/child"
const ALPHA_FILE = ROOT_DIR + "/alpha.txt"
const BETA_FILE = ROOT_DIR + "/beta.dat"

sub delete_file_if_present( byref filename as string )
	dim as integer f = freefile()

	if( open( filename for input as #f ) = 0 ) then
		close #f
		kill filename
	end if
end sub

sub cleanup_tree()
	delete_file_if_present ALPHA_FILE
	delete_file_if_present BETA_FILE
	rmdir CHILD_DIR
	rmdir ROOT_DIR
end sub

sub write_text_file( byref filename as string, byref text as string )
	dim as integer f = freefile()

	if( open( filename for output as #f ) <> 0 ) then
		assert( 0 )
	end if

	print #f, text;
	close #f
end sub

function has_entry _
	( _
		byref pattern as string, _
		byref wanted as string, _
		byval attributes as long, _
		byval need_directory as integer _
	) as integer

	dim as string entry
	dim as long entry_attr

	entry = dir( pattern, attributes, entry_attr )
	while( entry <> "" )
		if( lcase( entry ) = lcase( wanted ) ) then
			if( need_directory <> 0 ) then
				return ( (entry_attr and fbDirectory) <> 0 )
			end if

			return -1
		end if

		entry = dir( entry_attr )
	wend

	return 0
end function

cleanup_tree()

assert( mkdir( ROOT_DIR ) = 0 )
assert( mkdir( CHILD_DIR ) = 0 )

write_text_file ALPHA_FILE, "alpha"
write_text_file BETA_FILE, "beta"

assert( has_entry( ROOT_DIR + "/*.txt", "alpha.txt", fbNormal, 0 ) )
assert( has_entry( ROOT_DIR + "/*.dat", "beta.dat", fbNormal, 0 ) )
assert( has_entry( ROOT_DIR + "/*", "child", fbDirectory, -1 ) )

dim as string original_dir = curdir()

assert( chdir( ROOT_DIR ) = 0 )
assert( has_entry( "*.txt", "alpha.txt", fbNormal, 0 ) )
assert( has_entry( "*", "child", fbDirectory, -1 ) )
assert( chdir( original_dir ) = 0 )

cleanup_tree()

'' end of dir-runtime.bas
