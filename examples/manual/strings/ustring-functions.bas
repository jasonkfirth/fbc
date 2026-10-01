'' Project: FreeBASIC examples
'' File: ustring-functions.bas
'' Purpose: Demonstrate standard string functions with UTF-8 text.
'' Responsibilities: Show search, trim, case, repetition, alignment, and numbers.
'' This file intentionally does NOT contain file handling.

dim as ustring accent = uchr(&hE9), emoji = uchr(&h1F600)
dim as ustring text = accent + "A" + emoji + accent

print "INSTR: "; instr(text, emoji)
print "INSTR from position: "; instr(2, text, accent)
print "INSTR ANY: "; instr(text, any emoji + accent)
print "INSTRREV: "; instrrev(text, accent)
print "INSTRREV ANY: "; instrrev(text, any emoji + accent, 3)

dim as ustring padded = "  " + text + "  "
print "TRIM: ["; trim(padded); "]"
print "LTRIM: ["; ltrim(padded); "]"
print "RTRIM: ["; rtrim(padded); "]"
padded = emoji + text + emoji
print "TRIM a pattern: "; trim(padded, emoji)
print "LTRIM a pattern: "; ltrim(padded, emoji)
print "RTRIM a pattern: "; rtrim(padded, emoji)
print "TRIM ANY: "; trim(padded, any emoji + accent)
print "LTRIM ANY: "; ltrim(padded, any emoji + accent)
print "RTRIM ANY: "; rtrim(padded, any emoji + accent)

'' Default Unicode casing supports mappings that expand the text.
print "UCASE sharp s: "; ucase(uchr(&hDF))
print "LCASE dotted I: "; lcase(uchr(&h130))
print "ASCII-only UCASE: "; ucase(text, 1)
print "ASCII-only LCASE: "; lcase(text, 1)

print "Repeat a scalar: "; ustring(3, &h1F600)
print "Repeat the first character: "; string(3, accent)
dim as ustring slotText = ustring(6, 32)
lset slotText = text
print "LSET: ["; slotText; "]"
rset slotText = text
print "RSET: ["; slotText; "]"

dim as ustring numberText = "123.5"
print "VAL: "; val(numberText)
print "VALINT: "; valint(numberText)
print "VALLNG: "; vallng(numberText)
print "VALUINT: "; valuint(numberText)
print "VALULNG: "; valulng(numberText)
print "STR: "; str(numberText)

'' end of ustring-functions.bas
