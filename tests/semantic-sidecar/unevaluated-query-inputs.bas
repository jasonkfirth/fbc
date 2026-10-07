'' Project: FreeBASIC semantic sidecar tests
'' File: unevaluated-query-inputs.bas
'' Purpose: Retain every parsed operand inside discarded type and size queries.
'' Responsibilities: Folded casts, nested queries, call arguments and live controls.
'' This file intentionally does NOT execute conversions or external calls.
#lang "fb"

declare function Consume(byval value as byte) as byte
sub CheckQueries(byval wide as longint)
	print cbyte(wide)
	print sizeof(Consume(cbyte(wide)))
	dim as typeof(Consume(cbyte(wide))) typed_value
	print sizeof(sizeof(cbyte(wide)))
	dim as typeof(sizeof(cbyte(wide))) typed_size
	print sizeof(typeof(cbyte(wide)))
	print sizeof(cbyte(100000)), typed_value, typed_size
	print len(str(cbyte(wide)))
end sub

'' end of unevaluated-query-inputs.bas
