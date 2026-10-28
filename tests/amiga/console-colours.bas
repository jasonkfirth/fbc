'' FreeBASIC native Amiga console qualification
'' File: console-colours.bas
'' Purpose: Check real console pens, geometry and bounded cell clearing.
'' Responsibilities: Exercise COLOR and VIEW PRINT through the native driver.
'' This file does not implement terminal operations or synthesize input.

extern "C"
    declare function amiga_console_resize() as long
    declare function amiga_console_ready() as long
    declare function amiga_console_columns() as long
    declare function amiga_console_rows() as long
    declare function amiga_console_cell(byval row as long, byval column as long, _
        byval foreground as ulong, byval background as ulong, byval need_foreground as long) as long
    declare function amiga_console_wrap() as long
    declare function amiga_console_palette_unchanged() as long
    declare function amiga_console_swap() as long
    declare function amiga_console_cursor(byval row as long, byval column as long) as long
end extern

if amiga_console_ready() = 0 then end 1
dim columns as long = amiga_console_columns()
dim rows as long = amiga_console_rows()
if loword(width) <> columns or hiword(width) <> rows then end 2
if columns < 10 or rows < 10 then end 3
dim wrapping as long = amiga_console_wrap()

dim previous as ulong = color(15, 0)
if previous <> 7 then end 4
cls
locate 3, 3, 0 : print "M";
if amiga_console_cell(3, 3, &hffffff, 0, 1) <> 0 then end 5

previous = color(0, 15)
if previous <> 15 then end 6
cls
locate 3, 3, 0 : print "M";
if amiga_console_cell(3, 3, 0, &hffffff, 1) <> 0 then end 7
if amiga_console_cell(10, 10, 0, &hffffff, 0) <> 0 then end 8

'' A viewport clear must preserve differently coloured cells outside it.
color 15, 0
locate 2, 2, 0 : print "M";
color 0, 15
view print 4 to 6
cls 2
if amiga_console_cell(2, 2, &hffffff, 0, 1) <> 0 then end 9
if amiga_console_cell(5, 10, 0, &hffffff, 0) <> 0 then end 10

'' Clearing the last column of the final row must not wrap or scroll.
view print rows to rows
cls 2
if amiga_console_cell(2, 2, &hffffff, 0, 1) <> 0 then end 11
if amiga_console_cell(rows, columns, 0, &hffffff, 0) <> 0 then end 12
if amiga_console_cursor(rows, 1) = 0 then end 13
if amiga_console_wrap() <> wrapping then end 14
if amiga_console_palette_unchanged() = 0 then end 15

'' User screen preferences can place black/white in different native pens.
if amiga_console_swap() = 0 then end 16
color 15, 0
color 0, 15
view print
cls
locate 3, 3, 0 : print "M";
if amiga_console_cell(3, 3, 0, &hffffff, 1) <> 0 then end 17
if amiga_console_cell(10, 10, 0, &hffffff, 0) <> 0 then end 18

'' Shrinking a real window must refresh dimensions and keep cached cursor
'' coordinates inside the smaller backing buffers before the next PRINT.
locate rows, columns, 0
if amiga_console_resize() = 0 then end 19
columns = amiga_console_columns() : rows = amiga_console_rows()
if loword(width) <> columns or hiword(width) <> rows then end 20
if pos(0) < 1 or pos(0) > columns or csrlin < 1 or csrlin > rows then end 21
print "resize";
view print
cls
locate 3, 3, 0 : print "M";
if amiga_console_cell(3, 3, 0, &hffffff, 1) <> 0 then end 22
end 0

'' end of console-colours.bas
