/'
	Project: FreeBASIC C backend
	File: backend/c/c-evaluated-values.bi
	Purpose: Preserve evaluation when a scalar destination is unobserved.
	Responsibilities: Supply a typed C value context and emit each helper once.
	This file intentionally does NOT contain: AST lifetime changes, expression
	optimization or native diagnostic suppression.
'/

'' One set of primitive helpers belongs to the current C output. No source
'' symbols are retained, and the next output starts with an empty set.
dim shared as integer c_evaluated_value_emitted(0 to FB_DATATYPES-1)

private sub hCEvaluatedValuesInit()
	for i as integer = 0 to FB_DATATYPES-1
		c_evaluated_value_emitted(i) = FALSE
	next
end sub

private function hCEvaluatedScalar(byval dtype as integer, byref value as string) as string
	var kind = typeGetDtAndPtrOnly(dtype)
	assert(kind >= 0 and kind < FB_DATATYPES)
	assert(typeIsPtr(kind) = FALSE)
	var helper = "fb_EvaluatedValue" + str(kind)
	if( c_evaluated_value_emitted(kind) = FALSE ) then
		'' GCC can omit floating arithmetic in a void expression even at -O0.
		'' A by-value argument retains the value context of the original store.
		'' Returning that value allows ordinary native inlining and dead-store
		'' optimization, without introducing a named unread destination.
		var typename = hEmitType(kind, NULL)
		var section = sectionGosub(0)
		hWriteLine("static inline " + typename + " " + helper + "(" + typename + " value)")
		hWriteLine("{ return value; }")
		sectionReturn(section)
		c_evaluated_value_emitted(kind) = TRUE
	end if
	return helper + "( (" + value + ") )"
end function

/' end of backend/c/c-evaluated-values.bi '/
