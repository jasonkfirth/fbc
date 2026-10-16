'' FreeBASIC classic AmigaOS input qualification
'' --------------------------------------------
'' File: standard-input.bas
'' Purpose: Verify staged standard input through the native guest controller.
'' Responsibilities: Preserve redirected lines, keyboard fallback and EOF.
'' This file intentionally does NOT contain: shell or controller implementation.

dim first_line as string, second_line as string
line input first_line
line input second_line
if first_line <> "Amiga input fixture" then end 1
if second_line <> "second line" then end 2
'' A redirected stream uses buffered stdio throughout. SLEEP must leave the
'' next byte available to GETKEY, including data already buffered by LINE INPUT.
sleep
if getkey() <> asc("Z") then end 3
if getkey() <> -1 then end 4
if inkey$ <> "" then end 5
print "NATIVE_STANDARD_INPUT_OK"
end 0

'' end of standard-input.bas
