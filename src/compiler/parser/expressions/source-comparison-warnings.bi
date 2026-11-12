/'
	Project: FreeBASIC source diagnostics
	File: parser/expressions/source-comparison-warnings.bi
	Purpose: Warn about integer comparisons fixed by the declared operand range.
	Responsibilities: Inspect source operands before promotion without changing
	their evaluation, overload resolution or generated branch condition.
	This file intentionally does NOT infer ranges across assignments or control flow.
'/

private function hSourceComparisonFixed(byval op as integer, byval l as ASTNODE ptr, byval r as ASTNODE ptr) as integer
	if( env.clopt.warninglevel > FB_WARNINGMSGS_DEFAULT_LEVEL ) then return FALSE
	select case as const op
	case AST_OP_EQ, AST_OP_NE, AST_OP_LT, AST_OP_LE, AST_OP_GT, AST_OP_GE
	case else
		return FALSE
	end select
	if( l = NULL or r = NULL ) then return FALSE
	if( l->class = AST_NODECLASS_CONST ) then
		swap l, r
		select case as const op
		case AST_OP_LT: op = AST_OP_GT
		case AST_OP_LE: op = AST_OP_GE
		case AST_OP_GT: op = AST_OP_LT
		case AST_OP_GE: op = AST_OP_LE
		end select
	end if
	if( r->class <> AST_NODECLASS_CONST ) then return FALSE
	select case as const l->class
	case AST_NODECLASS_VAR, AST_NODECLASS_FIELD, AST_NODECLASS_IDX, AST_NODECLASS_DEREF
	case else
		return FALSE
	end select
	var dtype = astGetDataType(l)
	var cdtype = astGetDataType(r)
	if( typeIsPtr(dtype) or typeIsPtr(cdtype) ) then return FALSE
	if( typeGetClass(dtype) <> FB_DATACLASS_INTEGER or typeGetClass(cdtype) <> FB_DATACLASS_INTEGER ) then return FALSE
	'' Wider signed constants retain their value. Negative constants and the
	'' 64-bit/128-bit boundaries need the full mixed-signedness rules, so leave
	'' those comparisons alone rather than guessing from their storage bits.
	if( typeGetSize(dtype) <= 0 or typeGetSize(dtype) > 4 or typeGetSize(cdtype) > 8 ) then return FALSE
	if( dtype = FB_DATATYPE_BOOLEAN or typeIsInteger128Scalar(cdtype) ) then return FALSE
	'' An unsigned common type can map negative source values into its upper
	'' half. Do not claim a continuous signed range across that conversion.
	if( typeIsSigned(dtype) and (typeIsSigned(cdtype) = FALSE) ) then
		if( typeGetSize(cdtype) >= typeGetSize(FB_DATATYPE_INTEGER) and _
		    typeGetSize(cdtype) >= typeGetSize(dtype) ) then return FALSE
	end if
	var value = r->val.i
	if( value < 0 ) then return FALSE
	dim as longint minimum = 0, maximum = (1ll shl typeGetBits(dtype)) - 1
	if( typeIsSigned(dtype) ) then
		maximum = (1ll shl (typeGetBits(dtype) - 1)) - 1
		minimum = -maximum - 1
	end if
	dim as integer fixed_result = FALSE
	select case as const op
	case AST_OP_EQ, AST_OP_NE
		fixed_result = (value < minimum) or (value > maximum)
	case AST_OP_LT, AST_OP_GE
		fixed_result = (value <= minimum) or (value > maximum)
	case AST_OP_LE, AST_OP_GT
		fixed_result = (value < minimum) or (value >= maximum)
	end select
	return fixed_result
end function

'' end of parser/expressions/source-comparison-warnings.bi
