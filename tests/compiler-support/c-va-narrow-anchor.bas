'' FreeBASIC C variadic regression, c-va-narrow-anchor.bas
'' Checks argument traversal behind narrow integer and Single anchors.
'' Preserves each named parameter's source ABI and caller-visible value.
'' The native C dialect selects the va_start builtin.
#lang "fb"

#ifdef DEAD_ONLY
private function unusedByte cdecl(byval anchor as byte, ...) as longint
	dim as cva_list args = any
	cva_start(args, anchor)
	var result = cva_arg(args, longint)
	cva_end(args)
	return result
end function

'' Keep this body available for the C dependency graph's pruning check.
asm nop
#else
declare function checkNativeCalls cdecl alias "fb_test_va_anchor_calls"() as long

function behindByte cdecl alias "fb_test_anchor_byte"(byval anchor as byte, ...) as longint
	if anchor <> -7 then end 11
	dim as cva_list args = any
	cva_start(args, anchor)
	var result = cva_arg(args, longint)
	cva_end(args)
	return result
end function

function behindShort cdecl alias "fb_test_anchor_short"(byval anchor as short, ...) as longint
	if anchor <> -513 then end 12
	dim as cva_list args = any
	cva_start(args, anchor)
	var result = cva_arg(args, longint)
	cva_end(args)
	return result
end function

function behindSingle cdecl alias "fb_test_anchor_single"(byval anchor as single, ...) as longint
	if anchor <> 1.25 then end 13
	dim as cva_list args = any
	cva_start(args, anchor)
	var result = cva_arg(args, longint)
	cva_end(args)
	return result
end function

if behindByte(-7, clngint(101)) <> 101 then end 1
if behindShort(-513, clngint(202)) <> 202 then end 2
if behindSingle(1.25, clngint(303)) <> 303 then end 3
if checkNativeCalls() <> 0 then end 4
#endif
print "VA_NARROW_ANCHOR_PASS"

'' end of c-va-narrow-anchor.bas
