'' FreeBASIC SQLite 3 example
'' File: sqlite3_test.bas
'' Purpose: Execute a query against the selected database.
'' Responsibilities: C callback widths, SQL nulls and SQLite-owned errors.
'' This file contains no database schema creation or SQLite implementation.
'' Translated by Nex/marzec.



#include once "sqlite3.bi"

const DEFAULT_DATABASE = "sqlite3_test.db"
const DEFAULT_QUERY	   = "select * from features"

declare sub showusage( )

declare function callback cdecl _
	( _
		byval NotUsed as any ptr, _
		byval argc as long, _
		byval argv as zstring ptr ptr, _
		byval colName as zstring ptr ptr _
	) as long

	dim as sqlite3 ptr db
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

	if sqlite3_open( database_name, @db ) then
		print "Can't open database";
		if db <> 0 then
			print ": "; *sqlite3_errmsg( db );
			sqlite3_close( db )
		end if
		print
		end 1
	end if

	print "Using database: "; database_name
	print

	dim as integer query_error = 0
	if sqlite3_exec( db, query, @callback, 0, @errMsg ) <> SQLITE_OK then
		query_error = 1
		print "SQL error";
		if errMsg <> 0 then print ": "; *errMsg;
		print
	end if
	' sqlite3_exec allocates this message through SQLite's allocator.
	if errMsg <> 0 then sqlite3_free(errMsg)

	if sqlite3_close(db) <> SQLITE_OK then
		print "Could not close the SQLite database"
		query_error = 1
	end if
	end query_error

'':::::
function callback cdecl _
	( _
		byval NotUsed as any ptr, _
		byval argc as long, _
		byval argv as zstring ptr ptr, _
		byval colName as zstring ptr ptr _
	) as long

	dim as integer i
	dim as string text
	' SQLite borrows these arrays only for the duration of the callback.
	if argc < 0 then return 1
	if argc > 0 then
		if argv = 0 orElse colName = 0 then return 1
	end if

	for i = 0 to argc - 1
		if colName[i] = 0 then return 1
		text = "NULL"
		if( argv[i] <> 0 ) then
			text = *argv[i]
		end if

		print *colName[i], " = '"; text; "'"
	next

	print

	function = 0

end function

'':::::
sub showusage( )
	print "Usage: "; command( 0 ); " database ""query"""
end sub
'' end of sqlite3_test.bas
