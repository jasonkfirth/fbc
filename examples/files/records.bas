'' File ownership: each successful Open is matched by Close.
'' This is a FreeBASIC-native record layout, not a portable interchange format.

type testrecord field=1
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-STR-009
	namefield  as string * 20
	scorefield as single
end type

const RECORD_COUNT = 10

dim filebuffer as testrecord
dim as integer fileNumber = freefile

'' Write out some test data
if open( "testdat.dat" for random access write as #fileNumber _
         len = len(filebuffer) ) <> 0 then
	print "Unable to open testdat.dat for writing."
	end 1
end if

for i as integer = 1 to RECORD_COUNT
	filebuffer.namefield = "name" + ltrim(str(i))
	filebuffer.scorefield = i
	put #fileNumber, i, filebuffer
next
close #fileNumber

'' Read it back in
fileNumber = freefile

if open( "testdat.dat" for random access read as #fileNumber _
         len = len(filebuffer) ) <> 0 then
	print "Unable to open testdat.dat for reading."
	end 1
end if

for i as integer = 1 to RECORD_COUNT
	get #fileNumber, i, filebuffer
	print i, filebuffer.namefield, str( filebuffer.scorefield ), filebuffer.scorefield
next
close #fileNumber
