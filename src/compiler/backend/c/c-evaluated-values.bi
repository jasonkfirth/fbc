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
dim shared as integer c_evaluated_pointer_emitted

private sub hCEvaluatedValuesInit()
	c_evaluated_pointer_emitted = FALSE
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
		'' An identity call can disappear after native inlining, including
		'' floating conversions and integer operations whose evaluation matters.
		'' A volatile value keeps that evaluation and destination rounding when
		'' the BASIC store has no reader. It does not access the removed storage.
		var typename = hEmitType(kind, NULL)
		var section = sectionGosub(0)
		var first = ctx.sections(0).bufferlen
		hWriteLine("static inline " + typename + " " + helper + "(" + typename + " value)")
		hWriteLine("{ volatile " + typename + " evaluated = value; return evaluated; }")
		hCLeafNativeSpan(helper, TRUE, first, ctx.sections(0).bufferlen)
		sectionReturn(section)
		c_evaluated_value_emitted(kind) = TRUE
	end if
	hCLeafNativeReference(helper)
	return helper + "( (" + value + ") )"
end function

'' The original typed RHS is evaluated before the standard object-pointer
'' conversion. No pointee is accessed, owned or converted through an integer.
'' The erased object type also avoids moving a local record tag into the header.
private function hCEvaluatedPointer(byref value as string) as string
	var helper = "fb_EvaluatedPointer"
	if( c_evaluated_pointer_emitted = FALSE ) then
		var section = sectionGosub(0)
		var first = ctx.sections(0).bufferlen
		hWriteLine("static inline const void* fb_EvaluatedPointer(const void* value)")
		hWriteLine("{ return value; }")
		hCLeafNativeSpan(helper, TRUE, first, ctx.sections(0).bufferlen)
		sectionReturn(section)
		c_evaluated_pointer_emitted = TRUE
	end if
	hCLeafNativeReference(helper)
	return "fb_EvaluatedPointer( (" + value + ") )"
end function

/' end of backend/c/c-evaluated-values.bi '/
