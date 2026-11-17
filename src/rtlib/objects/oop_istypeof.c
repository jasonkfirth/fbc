/*
 * Project: FreeBASIC runtime library
 * File: objects/oop_istypeof.c
 * Purpose: is operator.
 * Responsibilities: Object type support and logical object serialization.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_IsTypeOf( FB_OBJECT *obj, FB_RTTI *typeRTTI )
{
	if( (obj == NULL) || (obj->pVT == NULL) || (typeRTTI == NULL) )
		return FB_FALSE;
	
	FB_RTTI *objRTTI = (obj->pVT - 1)->pRTTI;
	while( objRTTI != NULL )
	{	
		/* note: can't compare just the address because object or type could be declared in a DLL */
		if( strcmp( objRTTI->id, typeRTTI->id ) == 0 ) 
			return FB_TRUE;
			
		objRTTI = objRTTI->pRTTIBase;
	}
	
	return FB_FALSE;
}

/* end of objects/oop_istypeof.c */
