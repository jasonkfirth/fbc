'' File ownership: each successful Open is matched by Close before the handle
'' is reused.
const TEXT_FILE = "test.txt"

'' Write out text file:
dim as integer fileNumber = freefile

if open( TEXT_FILE for output access write as #fileNumber ) <> 0 then
	print "Unable to open "; TEXT_FILE; " for writing."
	end 1
end if

print #fileNumber, "Hello,"
print #fileNumber, ""
print #fileNumber, "this is an example text file,"
print #fileNumber, "generated with the help of FreeBASIC."

close #fileNumber

'' ---
'' Display text file line by line:

fileNumber = freefile

if open( TEXT_FILE for input access read as #fileNumber ) <> 0 then
	print "Unable to open "; TEXT_FILE; " for reading."
	end 1
end if

dim as string ln

do until( eof(fileNumber) )
	line input #fileNumber, ln
	print ln
loop

close #fileNumber
