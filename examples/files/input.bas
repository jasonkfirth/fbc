'' This example demonstrates how the Input statement can be used to parse and
'' retrieve data from a file.
''
'' File ownership: each routine closes the handle it opens.

sub doinput( byref file as string )
	dim as integer i
	dim as double f
	dim as string s
	dim as integer fileNumber = freefile

	if open( file for input access read as #fileNumber ) <> 0 then
		print "Unable to open "; file; " for reading."
		exit sub
	end if

	'' Input # intentionally reads the BASIC-formatted data written below.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL517
	input #fileNumber, s
	print s

	'' FB-LINTER: DISABLE-NEXT-LINE FBL517
	input #fileNumber, i, f, s
	print i, f, s

	close #fileNumber
end sub

'' Create a test data file...
const TEST_INTEGER = 1234
const TEST_REAL = 5678.901

dim as integer fileNumber = freefile

if open( "test.dat" for output access write as #fileNumber ) <> 0 then
	print "Unable to open test.dat for writing."
	end 1
end if

print #fileNumber, "abc def"
print #fileNumber, TEST_INTEGER, TEST_REAL, "xyz zzz"
close #fileNumber

'' Read it in
doinput "test.dat"
