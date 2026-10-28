'' Project: FreeBASIC semantic parser tests
'' File: token-skip-guards.bas
'' Purpose: Execute the production skip helper against a controlled token stream.
'' Responsibilities: Delimiter nesting, exact consumption, omissions and buffered points.
'' This file intentionally does NOT reproduce the implementation of hSkipUntil.

#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "tooling/semantic-recovery.bi"

dim shared as LEX_CTX lex
dim shared as FBTOKEN stream(0 to 31)
dim shared as integer position, length, gets, skips, observations, observing = TRUE
dim shared as integer actual_stop, actual_consumed, actual_depth, actual_target, actual_eat
dim shared as longint actual_count
dim shared as string actual_reason

function errGetCount( ) as integer
	return 3
end function
function fbSemanticModelRecoveryEnabled( ) as integer
	return observing and (lex.ctx->semantic_probe = FALSE)
end function
function lexGetToken( byval flags as LEXCHECK ) as integer
	gets += 1
	if( position >= length ) then end 30
	lex.ctx->head = @stream(position)
	return stream(position).id
end function
sub lexSkipToken( byval flags as LEXCHECK )
	if( position >= length or stream(position).id = FB_TK_EOF ) then end 31
	position += 1
	skips += 1
end sub
function lexGetCurrentLocation( ) as LEX_LOCATION
	'' This API records identifier hints. Skipped tokens must not query it.
	end 32
end function
sub fbSemanticModelRecoverySkip( byval token as integer, byval eat as integer, _
	byval flags as integer, byval comma as integer, byval skipped as longint, _
	byval stopped_token as integer, byval consumed_stop as integer, byval nesting as integer, _
	byval errors_before as integer, byref first as LEX_LOCATION, byref stopping as LEX_LOCATION, _
	byref reason as const string )
	observations += 1
	actual_target = token
	actual_eat = eat
	actual_stop = stopped_token
	actual_consumed = consumed_stop
	actual_depth = nesting
	actual_count = skipped
	actual_reason = reason
	if( errors_before <> 3 or first.start_column <> 0 ) then end 33
	if( stopping.start_column <> position - abs(consumed_stop <> FALSE) ) then end 34
end sub

private sub prepare( values() as integer )
	position = 0
	gets = 0
	skips = 0
	observations = 0
	length = ubound(values) + 1
	for index as integer = 0 to length - 1
		stream(index).id = values(index)
		stream(index).source.start_column = index
	next
end sub

lex.ctx = @lex.ctxTB(0)
dim as integer nested(0 to 6) = { FB_TK_ID, CHAR_LPRNT, FB_TK_NUMLIT, CHAR_RPRNT, CHAR_COMMA, CHAR_RPRNT, FB_TK_EOF }
for observing = FALSE to TRUE step -1
	prepare(nested())
	hSkipUntil(CHAR_RPRNT, TRUE)
	if( position <> 6 or skips <> 6 or gets <> 9 ) then end 1
	if( observations <> abs(observing) ) then end 2
	if( observing ) then
		if( actual_count <> 6 or actual_stop <> CHAR_RPRNT or actual_consumed = FALSE or actual_depth ) then end 3
		if( actual_reason <> "target" ) then end 4
	end if
next
observing = TRUE

dim as integer comma_case(0 to 6) = { FB_TK_ID, CHAR_LPRNT, FB_TK_NUMLIT, CHAR_COMMA, CHAR_RPRNT, CHAR_COMMA, FB_TK_EOF }
prepare(comma_case())
hSkipUntil(CHAR_RPRNT, TRUE, , TRUE)
if( position <> 5 or actual_reason <> "comma" or actual_count <> 5 or actual_consumed ) then end 5
prepare(comma_case())
hSkipUntil(CHAR_COMMA, TRUE)
if( position <> 6 or actual_reason <> "comma" or actual_count <> 6 or actual_consumed = FALSE ) then end 6

dim as integer boundary(0 to 3) = { FB_TK_ID, FB_TK_STMTSEP, FB_TK_EOL, FB_TK_EOF }
prepare(boundary())
hSkipUntil(CHAR_RPRNT, FALSE)
if( position <> 1 or actual_reason <> "statement-boundary" ) then end 7
prepare(boundary())
hSkipUntil(FB_TK_EOL, TRUE)
if( position <> 3 or actual_reason <> "end-of-line" or actual_count <> 3 or actual_consumed = FALSE ) then end 8

dim as integer eof_case(0 to 1) = { FB_TK_ID, FB_TK_EOF }
prepare(eof_case())
hSkipUntil(FB_TK_EOF, TRUE)
if( position <> 1 or actual_reason <> "end-of-file" or actual_consumed or actual_count <> 1 ) then end 9

lex.ctx->semantic_probe = TRUE
prepare(nested())
hSkipUntil(CHAR_RPRNT, TRUE)
if( observations or position <> 6 or gets <> 9 ) then end 10
print "TOKEN_SKIP_GUARDS_PASS"

'' The test runner appends hSkipUntil verbatim from parser-helper.bas here.
'' end of token-skip-guards.bas
