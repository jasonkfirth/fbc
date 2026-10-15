'' Project: FreeBASIC compiler examples
'' File: random.bas
'' Purpose: Write and read fixed-length records through a RANDOM file.
'' Responsibilities: own the file handle and demonstrate record positioning.
'' This file intentionally does NOT demonstrate a portable file format.
''
'' RANDOM files require the record length to match the value passed to PUT.
const RECORD_COUNT = 10
const RECORD_TO_READ = 2

dim i as long
dim as integer fileNumber = freefile

if open( "test.dat" for random as #fileNumber len = sizeof( i ) ) <> 0 then
	print "Unable to open test.dat."
	end 1
end if

for i = 1 to RECORD_COUNT
	put #fileNumber, , i
next

seek #fileNumber, RECORD_TO_READ
get #fileNumber, , i

print "data: "; i; " current record: "; loc(fileNumber); _
      " next: "; seek(fileNumber)

close #fileNumber

'' end of random.bas
