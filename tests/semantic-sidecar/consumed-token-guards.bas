'' Project: FreeBASIC consumed-token tests
'' File: consumed-token-guards.bas
'' Purpose: Exercise production token copying without lexer input APIs.
'' Responsibilities: Active unions, rotating-buffer lifetime, exclusions and bounds.
'' This file intentionally does NOT tokenize text or perform symbol lookup.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-tokens.bi"
#include once "tooling/semantic-constructs.bi"
#include once "parser/parser.bi"
#include once "preprocessor/pp.bi"
dim shared as PP_CTX pp

dim shared as LEX_CTX lex
dim shared as PARSERCTX parser
dim shared as integer observing = TRUE, failures, writes, points, fail_closed
dim shared as integer full_mode, symbol_requests
dim shared as string captured

function fbSemanticModelTypedExpressionsEnabled( ) as integer
	return observing
end function
function fbSemanticModelFullEnabled( ) as integer
	return full_mode
end function
function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	if( sym = NULL ) then return 0
	symbol_requests += 1
	return sym->class
end function
function fbSemanticModelModuleIdentity( ) as longint
	return 1
end function
function fbSemanticModelNumber( byval value as longint ) as string
	return ltrim(str(value))
end function
function fbSemanticModelTypeCode( byval value as longint ) as longint
	return value
end function
function fbSemanticModelTypeNumber( byval value as longint ) as string
	return fbSemanticModelNumber(value)
end function
function fbSemanticModelEscape( byref value as const string ) as string
	return value
end function
function fbSemanticModelCurrentStatement( ) as longint
	return 8
end function
function fbSemanticModelCurrentContext( ) as longint
	return 9
end function
function fbSemanticModelCurrentSource( ) as longint
	return 1
end function
function lexGetLastLocation( ) as LEX_LOCATION
	points += 1
	return lex.ctx->last_source
end function
sub fbSemanticModelAppendProvenance( byref value as const string )
	writes += 1
	if( writes <= 32 ) then captured += value + chr(10)
end sub
sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
	if( fail_closed ) then observing = FALSE
end sub

lex.ctx = @lex.ctxTB(0)
full_mode = TRUE
dim as FBTOKEN token
token.id = FB_TK_ID
token.class = FB_TKCLASS_IDENTIFIER
token.dtype = FB_DATATYPE_INVALID
token.text = "Copied"
token.len = 6
token.prdpos = 0
token.suffixchar = CHAR_NULL
token.semantic_lookup = 7
fbSemanticModelConsumedToken(@token)
token.text = "Changed"
if( instr(captured, "Copied") = 0 or instr(captured, "Changed") ) then end 1

token.class = FB_TKCLASS_NUMLITERAL
token.id = FB_DATATYPE_LONG
token.dtype = FB_DATATYPE_LONG
token.text = "123"
token.len = 3
token.hassuffix = TRUE
token.suffixchar = 999999
'' An inactive identifier suffix slot must not be read for numeric tokens.
token.numeric_suffix = "l"
token.numeric_original = "123"
token.numeric_original_length = 3
token.numeric_original_digits = 3
fbSemanticModelConsumedToken(@token)
if( instr(captured, "999999") ) then end 2

token.class = FB_TKCLASS_STRLITERAL
token.id = FB_TK_STRLIT
token.dtype = FB_DATATYPE_WCHAR
token.textw = wstr("Hi")
token.len = 2
token.hasesc = FALSE
fbSemanticModelConsumedToken(@token)
if( instr(captured, "0000004800000069") = 0 ) then end 3

token.class = FB_TKCLASS_OPERATOR
token.id = asc("+")
token.dtype = token.id
token.text = "+"
token.len = 1
fbSemanticModelConsumedToken(@token)
token.class = FB_TKCLASS_UNKNOWN
token.text = "?"
fbSemanticModelConsumedToken(@token)
fbSemanticModelFinishTokens(TRUE)
if( failures or writes <> 7 ) then end 4

writes = 0
full_mode = FALSE
fbSemanticModelConsumedToken(NULL)
full_mode = TRUE
lex.ctx->semantic_probe = TRUE
fbSemanticModelConsumedToken(NULL)
lex.ctx->semantic_probe = FALSE
lex.ctx->reclevel = 1
fbSemanticModelConsumedToken(NULL)
lex.ctx->reclevel = 0
pp.skipping = TRUE
fbSemanticModelConsumedToken(NULL)
pp.skipping = FALSE
parser.options = FB_PARSEROPT_ISPP
fbSemanticModelConsumedToken(NULL)
parser.options = 0
if( writes or failures ) then end 5

fbSemanticModelConsumedToken(NULL)
token.len = FB_MAXLITLEN + 1
fbSemanticModelConsumedToken(@token)
token.len = 1
token.class = FB_TKCLASS_UNKNOWN + 1
fbSemanticModelConsumedToken(@token)
if( failures <> 3 ) then end 6
fbSemanticModelFinishTokens(FALSE)

token.class = FB_TKCLASS_OPERATOR
writes = 0
captured = ""
for ordinal as integer = 1 to 1000000
	fbSemanticModelConsumedToken(@token)
next
if( failures <> 3 or writes <> 1000000 ) then end 7
fbSemanticModelConsumedToken(@token)
if( failures <> 4 or writes <> 1000000 ) then end 8
fbSemanticModelFinishTokens(FALSE)
writes = 0
captured = ""
fbSemanticModelConsumedToken(@token)
fbSemanticModelFinishTokens(TRUE)
if( writes <> 3 or instr(captured, "consumed-token:1") = 0 ) then end 9
print "CONSUMED_TOKEN_GUARDS_PASS"

'' end of consumed-token-guards.bas
