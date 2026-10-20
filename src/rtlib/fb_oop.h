/*
 * Project: FreeBASIC runtime library
 * File: fb_oop.h
 * Purpose: Provide fb oop runtime support.
 * Responsibilities: Shared runtime types, ABI declarations and internal contracts.
 * This file intentionally does NOT contain: Platform implementations or compiler parsing.
 */
typedef struct _FB_RTTI {
	void				*stdlistVT;
	const char      		*id;
	struct _FB_RTTI 	*pRTTIBase;
} FB_RTTI;

typedef struct _FB_BASEVT {
	void				*nullPtr;
	FB_RTTI				*pRTTI;
} FB_BASEVT;

typedef struct _FB_OBJECT {
	FB_BASEVT			*pVT;
} FB_OBJECT;

/* Object class constructor & copy constructor */
void _ZN10fb_Object$C1Ev( FB_OBJECT* );
void _ZN10fb_Object$C1ERKS_( FB_OBJECT *this_, const FB_OBJECT *rhs );

/* end of fb_oop.h */
