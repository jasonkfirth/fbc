/*
 * Project: FreeBASIC runtime library
 * File: objects/oop_object_copyctor.c
 * Purpose: Provide oop object copyctor runtime support.
 * Responsibilities: Object type support and logical object serialization.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/* constructor fb_Object$( byref rhs as const fb_Object$ ) */
void fb_ObjectCopyCtor( FB_OBJECT *this_, const FB_OBJECT *rhs )
{
	/* Just initialize the vptr properly (we cannot just copy it from the
	   rhs, because that may really be a different object type that just was
	   up-casted), nothing else to do. */
	fb_ObjectCtor( this_ );
}

/* end of objects/oop_object_copyctor.c */
