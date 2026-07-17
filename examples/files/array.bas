''
'' full array saving and loading, as in VB
''
'' File ownership: each successful Open is matched by Close before the handle
'' is reused. The file contains exactly ENTRIES raw byte values.
''


sub test

const ENTRIES = 10

	'' save array
	dim as byte outarray(0 to ENTRIES-1)

	dim as integer f = freefile
	if open( "test.dat" for binary access write as #f ) <> 0 then
		print "Unable to open test.dat for writing."
		exit sub
	end if

	dim as integer i
	for i = 0 to ENTRIES-1
		outarray(i) = i
	next i

	put #f, , outarray()

	close #f

	'' load array
	dim as byte inarray(0 to ENTRIES-1)

	f = freefile
	if open( "test.dat" for binary access read as #f ) <> 0 then
		print "Unable to open test.dat for reading."
		exit sub
	end if

	get #f, , inarray()

	for i = 0 to ENTRIES-1
		print inarray(i); outarray(i)
	next i

	close #f

end sub

	test
