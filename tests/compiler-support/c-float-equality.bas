'' Project: FreeBASIC C comparison regression tests
'' File: c-float-equality.bas
'' Purpose: Compare emitted equality, inequality and Boolean conversions.
'' Responsibilities: Expose scalar operations and side-effecting operands.
'' This file intentionally does NOT use approximate comparisons.

#ifdef EQUALITY_BASELINE
	#define D_EQ "fb_eq_base_d_eq"
	#define D_NE "fb_eq_base_d_ne"
	#define S_EQ "fb_eq_base_s_eq"
	#define S_NE "fb_eq_base_s_ne"
	#define D_BOOL "fb_eq_base_d_bool"
	#define D_EFFECT "fb_eq_base_d_effect"
#else
	#define D_EQ "fb_eq_new_d_eq"
	#define D_NE "fb_eq_new_d_ne"
	#define S_EQ "fb_eq_new_s_eq"
	#define S_NE "fb_eq_new_s_ne"
	#define D_BOOL "fb_eq_new_d_bool"
	#define D_EFFECT "fb_eq_new_d_effect"
#endif

declare function operand_d cdecl alias "fb_eq_operand_d"( byval which as long ) as double

function compare_d_eq cdecl alias D_EQ( byval lhs as double, byval rhs as double ) as long
	return lhs = rhs
end function

function compare_d_ne cdecl alias D_NE( byval lhs as double, byval rhs as double ) as long
	return lhs <> rhs
end function

function compare_s_eq cdecl alias S_EQ( byval lhs as single, byval rhs as single ) as long
	return lhs = rhs
end function

function compare_s_ne cdecl alias S_NE( byval lhs as single, byval rhs as single ) as long
	return lhs <> rhs
end function

function compare_d_bool cdecl alias D_BOOL( byval value as double ) as long
	return cbool(value)
end function

function compare_d_effect cdecl alias D_EFFECT() as long
	return operand_d(0) = operand_d(1)
end function

'' end of c-float-equality.bas
