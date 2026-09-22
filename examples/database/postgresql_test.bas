''
'' PostgreSQL (www.postgresql.org) example, translated from C by dr0p (dr0p[-at-]perfectbg.com)
''

#include "postgresql/libpq-fe.bi"

'' main
    dim as string conninfo
    dim as PGconn ptr conn
    dim as PGresult ptr res
    dim as integer nFields, i, j

    '' Set FBC_PG_CONNINFO when the server needs an authenticated login or another host.
    conninfo = Environ("FBC_PG_CONNINFO")
    If conninfo = "" Then
        conninfo = "dbname=postgres user=postgres"
    End If

    conn = PQconnectdb(conninfo)
	If conn = NULL Then
		Print "Connection allocation failed"
		End 1
	End If

    if (PQstatus(conn) <> CONNECTION_OK) then
        print "Connection to database failed: "; *PQerrorMessage(conn)
        PQfinish(conn)
        end 1
    end if


    res = PQexec(conn, "SELECT * FROM pg_database")
	If res = NULL Then
		Print "Command execution failed"
		PQfinish(conn)
		End 1
	End If
    ? PQresultStatus(res)
    if (PQresultStatus(res) <> PGRES_TUPLES_OK) then
        print "Command failed: "; *PQerrorMessage(conn)
        PQclear(res)
        PQfinish(conn)
        end 1
	end if

	nFields = PQnfields(res)-1
	for i = 0 to nFields
		print *PQfname(res, i),
	next
	print
	print

	for i = 0 to PQntuples(res)-1
		for j = 0 to nFields
			print *PQgetvalue(res, i, j),
		next
		print
	next

    PQclear(res)

    PQfinish(conn)
	end 0
