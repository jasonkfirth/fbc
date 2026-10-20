/*
 * Project: FreeBASIC runtime library
 * File: objects/oop_object.c
 * Purpose: Object class methods.
 * Responsibilities: Object type support and logical object serialization.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

	FB_RTTI __fb_ZTS6Object = { NULL, "6OBJECT", NULL };
	static FB_BASEVT Object_VT = { NULL, &__fb_ZTS6Object };

/* constructor fb_Object$( ) */
void _ZN10fb_Object$C1Ev( FB_OBJECT *this_ )
{
	this_->pVT = (FB_BASEVT *)(((unsigned char *)&Object_VT) + sizeof( FB_BASEVT ));
}

/* end of objects/oop_object.c */
