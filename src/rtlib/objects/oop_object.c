/*
 * Project: FreeBASIC runtime library
 * File: objects/oop_object.c
 * Purpose: Object class methods.
 * Responsibilities: Object type support and logical object serialization.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

	FB_RTTI fb_ObjectRTTI = { NULL, "6OBJECT", NULL };
	static FB_BASEVT Object_VT = { NULL, &fb_ObjectRTTI };

/* constructor fb_Object$( ) */
void fb_ObjectCtor( FB_OBJECT *this_ )
{
	this_->pVT = &Object_VT + 1;
}

/* end of objects/oop_object.c */
