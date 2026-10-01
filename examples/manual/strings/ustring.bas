'' Project: FreeBASIC examples
'' File: ustring.bas
'' Purpose: Introduce the built-in UTF-8 string type and scalar positions.
'' Responsibilities: Show construction, conversion, slicing, and indexing.
'' This file intentionally does NOT contain Unicode normalization examples.

'' UCHR constructs Unicode scalars, independently of source-file encoding.
dim as ustring text = "caf" + uchr(&hE9) + " " + uchr(&h1F600)
print text
print "Code points:"; len(text)

'' STRING keeps its existing byte length; conversion preserves UTF-8 bytes.
dim as string bytes = text
print "UTF-8 bytes:"; len(bytes)
dim as ustring decoded = ustring(bytes)
print "Round trip: "; decoded = text

'' Standard positions are one-based. Bracket indices are zero-based.
print "Fourth character: "; mid(text, 4, 1)
print "First four: "; left(text, 4)
print "Last character: "; right(text, 1)
print "Last code point: "; hex(asc(text, len(text)))
print "Fourth code point: "; hex(text[3])

'' Replacing a scalar can grow or shrink its encoded byte width.
text[3] = &h1F642
mid(text, 1, 3) = "tea"
print "Edited: "; text

'' WSTR converts UTF-8 to the target's wide-character representation.
dim as wstring * 32 wide = wstr(text)
dim as ustring fromWide = ustring(wide)
print "Wide round trip: "; fromWide = text

'' end of ustring.bas
