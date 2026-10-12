'' FreeBASIC SQLite 2 example
'' File: sqlite2_test.bas
'' Purpose: Execute a query against the selected database.
'' Responsibilities: C callback widths, SQL nulls and SQLite-owned errors.
'' This file contains no database schema creation or SQLite implementation.



#include once "sqlite2.bi"

const DEFAULT_DATABASE = "sqlite2_test.db"
const DEFAULT_QUERY    = "select * from test"

declare sub showusage( )

declare function callback cdecl ( byval NotUsed as any ptr, _
								  byval argc as long, _
								  byval argv as zstring ptr ptr, _
								  byval colName as zstring ptr ptr ) as long

	dim as sqlite ptr db
	dim as zstring ptr errMsg
	dim as string database_name, query

	if( __FB_ARGC__ > 3 ) then
		showusage
		end 1
	end if

	if( __FB_ARGC__ >= 2 ) then
		database_name = command( 1 )
	else
		database_name = DEFAULT_DATABASE
	end if

	if( __FB_ARGC__ = 3 ) then
		query = command( 2 )
	else
		query = DEFAULT_QUERY
	end if

	'' SQLite 2 retains this legacy mode argument but ignores its value.
	db = sqlite_open( database_name, 0, @errMsg )

	if db = 0 then
		print "Can't open database";
		if errMsg <> 0 then
			print ": "; *errMsg;
			sqlite_freemem(errMsg)
		end if
		print
		end 1
	end if

	print "Using database: "; database_name
	print

	dim as integer query_error = 0
	if sqlite_exec( db, query, @callback, 0, @errMsg ) <> SQLITE_OK then
		query_error = 1
		print "SQL error";
		if errMsg <> 0 then
			print ": "; *errMsg;
			sqlite_freemem(errMsg)
		end if
		print
	end if

	sqlite_close(db)
	end query_error

'':::::
function callback cdecl ( byval NotUsed as any ptr, _
						  byval argc as long, _
						  byval argv as zstring ptr ptr, _
						  byval colName as zstring ptr ptr ) as long

	dim as integer i
	'' SQLite borrows these arrays for the callback. A SQL NULL is a null
	'' element pointer, while an empty string still has a nonnull pointer.
	if argc < 0 then return 1
	if argc > 0 then
		if argv = 0 orElse colName = 0 then return 1
	end if

	for i = 0 to argc - 1
		print *colName[i], " = '";
		if argv[i] <> 0 then
			print *argv[i];
		else
			print "NULL";
		end if
		print "'"
	next

	print

	function = 0

end function

'':::::
sub showusage( )
	print "Usage: "; command( 0 ); " database ""query"""
end sub

'' end of sqlite2_test.bas
