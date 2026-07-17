'' File ownership: sourceFile owns the handle until the final Close.
'' The input is treated as opaque bytes; this example defines no file format.
const ESC_KEY = 27

dim as integer sourceFile = freefile

if open( "binary.bas" for binary access read as #sourceFile ) <> 0 then
	print "Unable to open binary.bas."
	end 1
end if

dim b as byte

print "Reading each byte separately, press a key to continue to the next"

do until( eof(sourceFile) )
	get #sourceFile, , b

	'' Display corresponding ASCII character
	print chr( b );

	'' A blocking Sleep is intentional here: each keypress advances one byte.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL602 FBL-LOOP-010
	sleep
	if inkey() = chr(ESC_KEY) then exit do
	while inkey() <> "" : wend
loop

close #sourceFile
