/*
 * Project: FreeBASIC runtime library
 * File: fb_oop.h
 * Purpose: Provide fb oop runtime support.
 * Responsibilities: Shared runtime types, ABI declarations and internal contracts.
 * This file intentionally does NOT contain: Platform implementations or compiler parsing.
 */
#include "fb_abi.h"
typedef struct FB_RTTI_ {
	void				*stdlistVT;
	const char      		*id;
	struct FB_RTTI_ 	*pRTTIBase;
} FB_RTTI;

typedef struct FB_BASEVT_ {
	void				*nullPtr;
	FB_RTTI				*pRTTI;
} FB_BASEVT;

typedef struct FB_OBJECT_ {
	FB_BASEVT			*pVT;
} FB_OBJECT;

/* Object class constructor & copy constructor */
extern FB_RTTI fb_ObjectRTTI FB_RTLIB_SYMBOL("__fb_ZTS6Object");

void fb_ObjectCtor( FB_OBJECT* ) FB_RTLIB_SYMBOL("_ZN10fb_Object$C1Ev");
void fb_ObjectCopyCtor( FB_OBJECT *this_, const FB_OBJECT *rhs ) FB_RTLIB_SYMBOL("_ZN10fb_Object$C1ERKS_");

/* Type tests use the existing object/vtable and RTTI layout. */
FBCALL int fb_IsTypeOf( FB_OBJECT *obj, FB_RTTI *typeRTTI );

/* end of fb_oop.h */
