/'
    Project: FreeBASIC C backend regression tests
    File: c-wide-literal-target.bas
    Purpose: Exercise wide literal storage on targets with different WCHAR ABIs.
    Responsibilities: Check pooled literals, fixed buffers and code-unit data.
    This fixture intentionally does NOT depend on a platform window or device.
'/
#lang "fb"

const TEXT_CAPACITY = 8
const LATIN_CHARACTER = &hE9
const CJK_CHARACTER = &h4E2D
const SYMBOL_CHARACTER = &h263A

'' The module owns this buffer so the test exercises static initialization.
dim shared as wstring * TEXT_CAPACITY fixed_text = wchr(LATIN_CHARACTER, CJK_CHARACTER, SYMBOL_CHARACTER)
dim as wstring * TEXT_CAPACITY copy_text = wchr(LATIN_CHARACTER, CJK_CHARACTER, SYMBOL_CHARACTER)
if fixed_text <> copy_text then end 1
if (fixed_text[0] <> LATIN_CHARACTER) orelse (fixed_text[1] <> CJK_CHARACTER) orelse _
   (fixed_text[2] <> SYMBOL_CHARACTER) then end 2
if fixed_text[3] <> 0 then end 3

/' Use an expression separately so the compiler must materialize a literal. '/
if copy_text <> wchr(LATIN_CHARACTER, CJK_CHARACTER, SYMBOL_CHARACTER) then end 4
print "C_WIDE_LITERAL_TARGET_PASS"

/' end of c-wide-literal-target.bas '/
