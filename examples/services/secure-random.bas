'' FreeBASIC runtime examples: secure-random.bas
'' Purpose: Request a complete 256-bit token from the checked system provider.
'' Responsibilities: Handle refusal, format bytes and clear the example buffer.
'' This example does not seed RND or supply a weaker fallback generator.
#lang "fb"
#include once "fbrandom.bi"

const TOKEN_BYTES as uinteger = 32
dim token(0 to TOKEN_BYTES - 1) as ubyte
'' SIZEOF(array) is the element size in BASIC. Pass the full byte count.
dim status as long = FBC.secureRandom(@token(0), TOKEN_BYTES)
if status <> FB.SECURE_RANDOM_OK then
	'' The API clears the complete valid span on a provider failure.
	print "No token was generated, status "; status
	end 1
end if
dim token_text as string = string(TOKEN_BYTES * 2, 0)
for index as long = lbound(token) to ubound(token)
	mid(token_text, index * 2 + 1, 2) = hex(token(index), 2)
	token(index) = 0
next
'' Pass the bytes or their encoded form to the protocol layer. Report only
'' the length here so generated values never enter the example's output.
print "Generated "; len(token_text); " hexadecimal characters."
'' end of secure-random.bas
