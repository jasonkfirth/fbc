'' Project: FreeBASIC semantic sidecar tests
'' File: evaluation-capacity-guards.bas
'' Purpose: Exercise the production evaluation-context capacity guard.
'' Responsibilities: Named include depth, anonymous evaluation depth and probes.
'' This file intentionally does NOT contain: a replacement capacity algorithm.

const FB_MAXINCRECLEVEL = 16
type LEX_TKCTX
	marker as integer
end type
type LEX_CTX
	ctx as LEX_TKCTX ptr
	ctxTB(0 to FB_MAXINCRECLEVEL) as LEX_TKCTX
end type
type ENV_CTX
	includerec as integer
end type
dim shared as LEX_CTX lex
dim shared as ENV_CTX env

'' Native guard is inserted here.

for include_depth as integer = 0 to FB_MAXINCRECLEVEL - 1
	env.includerec = include_depth
	for slot as integer = 0 to FB_MAXINCRECLEVEL - 1
		lex.ctx = @lex.ctxTB(slot)
		if( hCanPushEvalCtx( ) = FALSE ) then end 1
	next
	lex.ctx = @lex.ctxTB(FB_MAXINCRECLEVEL)
	if( hCanPushEvalCtx( ) ) then end 2
next

env.includerec = FB_MAXINCRECLEVEL
for slot as integer = 0 to FB_MAXINCRECLEVEL
	lex.ctx = @lex.ctxTB(slot)
	if( hCanPushEvalCtx( ) ) then end 3
next

'' Pointer equality has a defined result for an independent probe allocation.
'' This does not grant probes permission to execute stateful macro callbacks.
dim as LEX_TKCTX probe
lex.ctx = @probe
env.includerec = 0
if( hCanPushEvalCtx( ) = FALSE ) then end 4
env.includerec = FB_MAXINCRECLEVEL
if( hCanPushEvalCtx( ) ) then end 5

print "EVALUATION_CAPACITY_PASS"
end 0

'' end of evaluation-capacity-guards.bas
