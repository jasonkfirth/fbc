'' File ownership: fileNumber owns the handle until the final Close.
const RECORD_COUNT = 10
const RECORD_TO_READ = 2

dim i as long
dim as integer fileNumber = freefile

if open( "test.dat" for random as #fileNumber ) <> 0 then
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
