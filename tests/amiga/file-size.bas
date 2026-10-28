' Project: FreeBASIC Amiga file regression
' File: file-size.bas
' Purpose: Verify that read-only LOF preserves input and avoids seeking.
' Responsibilities: Cover buffering, EOF, encoding, external growth and fallback.
' This file intentionally does not replace any file-device callback.
extern "C"
    declare sub audit_reset()
    declare function audit_moves() as long
    declare function append_byte(byval name as zstring ptr) as long
end extern
dim shared as long failures, cases, queries, moves
sub require(byval condition as integer, byref description as string)
    cases += 1
    if condition = 0 then
        failures += 1
        dim as long report = freefile
        open "failures.txt" for append as #report
        print #report,description
        close #report
    end if
end sub
function measured_size(byval number as long) as longint
    audit_reset()
    dim as longint value = lof(number)
    moves += audit_moves()
    queries += 1
    return value
end function
dim as string payload=""
for index as long = 0 to 4100
    payload += chr(65 + index mod 26)
next
dim as long fixture = freefile
open "fixture.bin" for binary access write as #fixture
put #fixture,,payload
close #fixture
open "fixture.bin" for binary access read shared as #fixture
dim as longint positions(0 to 7) => {1,2,1023,1024,1025,2049,4097,4101}
for index as long=0 to 7
    seek #fixture,positions(index)
    for repeat as long=1 to 4
        dim as longint before=seek(fixture)
        require(measured_size(fixture)=4101,"raw length")
        require(seek(fixture)=before,"LOF changed buffered input position")
    next
    dim as ubyte value
    get #fixture,,value
    require(value=asc(mid(payload,positions(index),1)),"LOF changed next input byte")
next
seek #fixture,4201
require(measured_size(fixture)=4101,"past EOF changed length")
require(seek(fixture)=4201,"past EOF cursor lost")
close #fixture
open "fixture.bin" for binary access read shared as #fixture
require(append_byte(strptr("fixture.bin"))=0,"external append failed")
require(measured_size(fixture)=4102,"read-only length cached external append")
require(seek(fixture)=1,"metadata query moved input after external growth")
close #fixture
fixture=freefile
open "unicode.txt" for binary access write as #fixture
dim as string encoded=chr(239,187,191) & "first" & chr(10) & "second" & chr(10)
put #fixture,,encoded
close #fixture
open "unicode.txt" for input encoding "utf8" as #fixture
dim as string linevalue
line input #fixture,linevalue
require(linevalue="first","first UTF8 line")
require(measured_size(fixture)=16,"UTF8 raw byte length")
line input #fixture,linevalue
require(linevalue="second","LOF disturbed second UTF8 line")
close #fixture
for unit_width as long=2 to 4 step 2
    dim as string widebytes, encodingname, plain="Hello world"
    if unit_width=2 then
        widebytes=chr(255,254): encodingname="utf16"
    else
        widebytes=chr(255,254,0,0): encodingname="utf32"
    end if
    for index as long=1 to len(plain)
        widebytes += mid(plain,index,1) & string(unit_width-1,chr(0))
    next
    fixture=freefile
    open "wide.txt" for binary access write as #fixture
    put #fixture,,widebytes
    close #fixture
    open "wide.txt" for input encoding encodingname as #fixture
    require(measured_size(fixture)=unit_width*12,"wide raw byte length before input")
    line input #fixture,linevalue
    require(linevalue="Hello world","LOF disturbed wide input")
    require(measured_size(fixture)=unit_width*12,"wide raw byte length after input")
    close #fixture
next
' Writable streams retain their normal seek/flush behavior.
open "writable.bin" for binary as #fixture
put #fixture,,payload
audit_reset()
require(lof(fixture)=4101,"pending writable file length")
require(audit_moves()>0,"writable stream bypassed existing fallback")
close #fixture
require(lof(fixture)=0,"closed file length")
dim as long report=freefile
open "file-size-results.tsv" for output as #report
print #report,"CASES";chr(9);cases;chr(9);"FAILURES";chr(9);failures;chr(9);"QUERIES";chr(9);queries;chr(9);"SEEKS";chr(9);moves
close #report
if failures<>0 then end 73
if moves<>0 then end 72
end 0
' end of file-size.bas
