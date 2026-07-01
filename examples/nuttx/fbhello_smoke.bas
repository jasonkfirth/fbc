''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbhello.bas
''
'' Purpose:
''
''     Exercise the first small FreeBASIC runtime surface on NuttX/RISC-V.
''
'' Responsibilities:
''
''     - prove that generated C from a BASIC source can run as a NuttX app
''     - cover core scalar, string, array, heap, file, and filesystem helpers
''
'' This file intentionally does NOT contain:
''
''     - graphics, audio, threads, or networking
''     - target-specific compiler switches
''

sample_data:
data 10, 20

typed_data:
data "stored text", 6.25, 1234567890123

print "hello from FreeBASIC on NuttX RISC-V"

const limit_value as integer = 5

dim as integer i, total

for i = 1 to limit_value
    total += i
next

print "sum 1..5 ="; total
print "hex total = "; hex(total)
print "double sample ="; 3.5

dim as double math_x = 0.5
dim as double math_y = 9.0

print "sin sample ="; iif(sin(math_x) > 0.47 and sin(math_x) < 0.49, 1, 0)
print "cos sample ="; iif(cos(math_x) > 0.87 and cos(math_x) < 0.88, 1, 0)
print "sqr sample ="; iif(sqr(math_y) = 3.0, 1, 0)
print "int sample ="; iif(int(math_x + 3.2) = 3.0, 1, 0)
print "fix sample ="; iif(fix(-math_x - 3.2) = -3.0, 1, 0)

dim as byte width_b = -7
dim as ubyte width_ub = 250
dim as short width_s = -1234
dim as ushort width_us = 54321
dim as longint width_li = 1234567890123
dim as ulongint width_uli = 1234567890123

print "byte sample ="; width_b
print "ubyte sample ="; width_ub
print "short sample ="; width_s
print "ushort sample ="; width_us
print "longint sample ="; width_li
print "ulongint sample ="; width_uli
print "bool sample ="; true

dim as integer values(0 to 3) = { 2, 4, 6, 8 }

total = 0

for i = lbound(values) to ubound(values)
    total += values(i)
next

if total = 20 then
    print "array total ok"
else
    print "array total bad"
end if

select case total
case 20
    print "select ok"
case else
    print "select bad"
end select

dim as integer data_a, data_b
restore sample_data
read data_a, data_b
print "data read sum ="; data_a + data_b
restore
read data_a, data_b
print "data restore sum ="; data_a + data_b

dim as string data_text
dim as double data_double
dim as longint data_longint

restore typed_data
read data_text, data_double, data_longint

print "data string sample = "; data_text
print "data double sample ="; data_double
print "data longint sample ="; data_longint

print "abs sample ="; abs(-42)
print "len sample ="; len("nuttx")

dim as string text_left, text_right, text_full

text_left = "nut"
text_right = "tx"
text_full = text_left + text_right

print "concat sample = "; text_full
print "left sample = "; left(text_full, 3)
print "mid sample = "; mid(text_full, 2, 3)
print "right sample = "; right(text_full, 2)
print "instr sample ="; instr(text_full, "tt")
print "instrrev sample ="; instrrev("abcabc", "ab")
print "ucase sample = "; ucase(text_full)
print "lcase sample = "; lcase("NuTTx")
print "trim sample = "; trim("  pad  ")
print "val sample ="; val("45.5")
print "valint sample ="; valint("42")
print "valuint sample ="; valuint("42")
print "vallng sample ="; vallng("1234567890123")
print "valulng sample ="; valulng("1234567890123")
print "str sample = "; str(123)
print "chr sample = "; chr(65)
print "asc sample ="; asc("A")
print "space len ="; len(space(3))
print "string sample = "; string(3, 42)
print "tab sample below"
print "A"; tab(5); "B"
print "spc sample below"
print "C"; spc(3); "D"
print "using double below"
print using "###.##"; 12.5
print "using longint below"
print using "#####"; 42
print "using string below"
print using "&"; "nuttx"
dim fixed_text as string * 5
fixed_text = "abc"
print "fixed len ="; len(fixed_text)
print "fixed asc4 ="; asc(mid(fixed_text, 4, 1))
dim fixed_set_text as string * 6
lset fixed_set_text = "ab"
print "lset sample = ["; fixed_set_text; "]"
rset fixed_set_text = "cd"
print "rset sample = ["; fixed_set_text; "]"
print "bin sample = "; bin(10)
print "oct sample = "; oct(10)
print "hex byte sample = "; hex(cubyte(255))
dim as string mki_sample = mki$(1234)
dim as string mkl_sample = mkl$(12345678)
dim as string mks_sample = mks$(1.5)
dim as string mkd_sample = mkd$(2.5)
print "mki cvi sample ="; iif(cvi(mki_sample) = 1234, 1, 0)
print "mkl cvl sample ="; iif(cvl(mkl_sample) = 12345678, 1, 0)
print "mks cvs sample ="; iif(abs(cvs(mks_sample) - 1.5) < 0.01, 1, 0)
print "mkd cvd sample ="; iif(abs(cvd(mkd_sample) - 2.5) < 0.01, 1, 0)
dim as string mid_assign_sample = "abcdef"
mid(mid_assign_sample, 2, 3) = "XYZ"
print "mid assign sample = "; mid_assign_sample
print "ltrim sample = "; ltrim("  left")
print "rtrim sample = "; rtrim("right  ")
print "command zero sample ="; iif(len(command(0)) > 0, 1, 0)
print "command arg sample = "; command(1)
print "command all sample = "; command
print "exepath sample ="; iif(instr(exepath, "fbhello") > 0, 1, 0)
print "env missing len ="; len(environ("FBXL_NOT_SET"))
dim as long env_set_result = setenviron("FBXL_ENV_TEST=ok")
if env_set_result <> 0 then print "env set result ="; env_set_result
print "env set sample = "; environ("FBXL_ENV_TEST")
print "err initial ="; err
dim as integer shell_result = shell("echo fb-shell-ok")
print "shell sample ="; iif(shell_result = 0, 1, 0)
cls
locate 1, 1
color 7, 0
width 80, 25
locate 3, 4
print "pos sample ="; pos(0)
print "csrlin sample ="; csrlin
locate 1, 1
print "console control ok"
print "inkey len ="; len(inkey)

dim as string console_line
line input console_line
print "line input sample = "; console_line

dim as integer console_number
input console_number
print "input int sample ="; console_number

dim as string input_text
input input_text
print "input string sample = "; input_text

dim as double input_double
input input_double
print "input double sample ="; input_double

dim as longint input_longint
input input_longint
print "input longint sample ="; input_longint

randomize 1
print "rnd range ="; iif(rnd >= 0 and rnd < 1, 1, 0)
dim cmp_a as string = "abc"
dim cmp_b as string = "abd"
print "compare eq ="; iif(cmp_a = "abc", 1, 0)
print "compare ne ="; iif(cmp_a <> cmp_b, 1, 0)
print "compare lt ="; iif(cmp_a < cmp_b, 1, 0)

redim as integer nums(0 to 4)

total = 0

for i = lbound(nums) to ubound(nums)
    nums(i) = i * 3
    total += nums(i)
next

print "redim total ="; total
erase nums
print "erase sample ok"

redim as integer grid(1 to 2, 1 to 3)

grid(1, 1) = 7
grid(2, 3) = 11

if lbound(grid, 1) = 1 and ubound(grid, 1) = 2 and _
    lbound(grid, 2) = 1 and ubound(grid, 2) = 3 then
    print "redim2 bounds ok"
else
    print "redim2 bounds bad"
end if

print "redim2 total ="; grid(1, 1) + grid(2, 3)
erase grid

dim as double start_time

start_time = timer
sleep 1, 1
print "timer sample ="; iif(timer >= start_time, 1, 0)

dim as string date_sample = date
print "date sample ="; iif(len(date_sample) = 10, 1, 0)

dim as string time_sample = time
print "time sample ="; iif(len(time_sample) = 8, 1, 0)

dim as string file_line

open "/ram/fbhello.txt" for output as #1
print #1, "file line"
close #1

open "/ram/fbhello.txt" for input as #1
print "file eof start ="; iif(eof(1), 1, 0)
print "file size ="; lof(1)
print "file loc start ="; loc(1)
seek #1, 1
print "file seek ="; seek(1)
line input #1, file_line
print "file eof end ="; iif(eof(1), 1, 0)
close #1

print "file sample = "; file_line

open "/ram/fbappend.txt" for output as #1
print #1, "one"
close #1

open "/ram/fbappend.txt" for append as #1
print #1, "two"
close #1

dim as string append_a, append_b

open "/ram/fbappend.txt" for input as #1
line input #1, append_a
line input #1, append_b
close #1

print "append sample = "; append_a; ":"; append_b
kill "/ram/fbappend.txt"

open "/ram/fbreset.txt" for output as #1
print #1, "reset ok"
reset

open "/ram/fbreset.txt" for input as #1
line input #1, file_line
close #1

print "reset sample = "; file_line
kill "/ram/fbreset.txt"

open "/ram/fblock.bin" for binary as #1
lock #1, 1 to 1
unlock #1, 1 to 1
close #1
print "lock sample ok"
kill "/ram/fblock.bin"

open "/ram/fbinputdollar.txt" for output as #1
print #1, "abcdef";
close #1

open "/ram/fbinputdollar.txt" for input as #1
dim as string input_dollar_sample = input$(3, #1)
close #1

print "input dollar sample = "; input_dollar_sample
kill "/ram/fbinputdollar.txt"

open "/ram/inputfile.txt" for output as #1
print #1, "from file"
print #1, "456"
print #1, "7.5"
close #1

dim as string file_input_string
dim as integer file_input_int
dim as double file_input_double

open "/ram/inputfile.txt" for input as #1
input #1, file_input_string, file_input_int, file_input_double
close #1

print "file input string = "; file_input_string
print "file input int ="; file_input_int
print "file input double ="; file_input_double
kill "/ram/inputfile.txt"

open "/ram/fbwrite.txt" for output as #1
write #1, "abc", 123, 4.5
close #1

open "/ram/fbwrite.txt" for input as #1
line input #1, file_line
close #1
print "write sample = "; file_line

dim as string write_input_string
dim as integer write_input_int
dim as double write_input_double

open "/ram/fbwrite.txt" for input as #1
input #1, write_input_string, write_input_int, write_input_double
close #1
print "write input string = "; write_input_string
print "write input int ="; write_input_int
print "write input double ="; write_input_double
kill "/ram/fbwrite.txt"

open "/ram/fbwritetypes.txt" for output as #1
write #1, cbyte(-1), cubyte(2), cshort(-3), cushort(4), cuint(5), _
    clngint(-6), culngint(7), csng(1.25), true
close #1

open "/ram/fbwritetypes.txt" for input as #1
line input #1, file_line
close #1
print "write types sample = "; file_line

dim as byte write_byte
dim as ubyte write_ubyte
dim as short write_short
dim as ushort write_ushort
dim as uinteger write_uint
dim as longint write_longint
dim as ulongint write_ulongint
dim as single write_single
dim as boolean write_bool

open "/ram/fbwritetypes.txt" for input as #1
input #1, write_byte, write_ubyte, write_short, write_ushort, write_uint, _
    write_longint, write_ulongint, write_single, write_bool
close #1

print "write input types ok ="; iif(write_byte = -1 and write_ubyte = 2 and _
    write_short = -3 and write_ushort = 4 and write_uint = 5 and _
    write_longint = -6 and write_ulongint = 7 and _
    abs(write_single - 1.25) < 0.01 and (write_bool <> 0), 1, 0)
kill "/ram/fbwritetypes.txt"

dim as integer binary_value = &h12345678

open "/ram/fbhello.bin" for binary as #1
put #1, , binary_value
seek #1, 1
binary_value = 0
get #1, , binary_value
close #1

print "binary sample ="; iif(binary_value = &h12345678, 1, 0)

type sample_rec
    as integer n
    as string * 6 text
end type

dim as sample_rec rec
rec.n = 77
rec.text = "nuttx"

open "/ram/fbrandom.dat" for random as #1 len = len(sample_rec)
put #1, 1, rec
rec.n = 0
rec.text = ""
get #1, 1, rec
close #1

print "random file sample ="; iif(rec.n = 77 and trim(rec.text) = "nuttx", 1, 0)
kill "/ram/fbrandom.dat"

dim as string binary_text = "xyz"

open "/ram/fbrandomstr.dat" for random as #1 len = 8
binary_text = "qqqq"
put #1, 2, "nuttx"
binary_text = space(5)
get #1, 2, binary_text
close #1

print "random string sample = "; trim(binary_text)
kill "/ram/fbrandomstr.dat"

binary_text = "xyz"

open "/ram/fbhello-str.bin" for binary as #1
put #1, , binary_text
seek #1, 1
binary_text = space(3)
get #1, , binary_text
close #1

print "binary string sample = "; binary_text
kill "/ram/fbhello-str.bin"

mkdir "/ram/fbdir"
chdir "/ram/fbdir"
print "curdir sample ="; iif(instr(curdir, "fbdir") > 0, 1, 0)

open "killme.txt" for output as #1
print #1, "x"
close #1
kill "killme.txt"
print "kill sample ok"

open "oldname.txt" for output as #1
print #1, "x"
close #1
name "oldname.txt" as "newname.txt"
kill "newname.txt"
print "rename sample ok"

mkdir "remove-me"
rmdir "remove-me"
print "rmdir sample ok"

mkdir "dirprobe"
open "dirprobe/one.txt" for output as #1
print #1, "x"
close #1
open "dirprobe/two.txt" for output as #1
print #1, "x"
close #1
dim as string dir_first = dir("dirprobe/*.txt")
dim as string dir_second = dir()
dim as string dir_third = dir()
dim as integer dir_ok = 0
if (dir_first <> "") and (dir_second <> "") and (dir_first <> dir_second) and (dir_third = "") then
    if ((dir_first = "one.txt") or (dir_first = "two.txt")) and ((dir_second = "one.txt") or (dir_second = "two.txt")) then
        dir_ok = 1
    end if
end if
print "dir sample ="; dir_ok
kill "dirprobe/one.txt"
kill "dirprobe/two.txt"
rmdir "dirprobe"

dim as integer free_handle = freefile
open "freefile.txt" for output as #free_handle
print #free_handle, "free"
close #free_handle
print "freefile sample ="; iif(free_handle > 0, 1, 0)

dim as integer ptr heap_values = allocate(sizeof(integer) * 2)

if heap_values = 0 then
	print "heap allocate failed"
	end 1
end if

heap_values[0] = 7
heap_values[1] = 8
print "heap sum ="; heap_values[0] + heap_values[1]

dim as integer ptr resized_values = reallocate(heap_values, sizeof(integer) * 3)

if resized_values = 0 then
	deallocate heap_values
	print "heap reallocate failed"
	end 1
end if

heap_values = resized_values
heap_values[2] = 9
print "heap resize sum ="; heap_values[0] + heap_values[1] + heap_values[2]
deallocate heap_values

dim as ubyte ptr zero_values = callocate(4)

if zero_values = 0 then
	print "heap callocate failed"
	end 1
end if

print "callocate zero ="; zero_values[0]
deallocate zero_values

chdir "/"

'' end of fbhello.bas
