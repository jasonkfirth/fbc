/* Project: FreeBASIC runtime
 * File: fb_serialization.h
 * Purpose: Declare compiler-generated logical serialization entry points.
 * Responsibilities: Value and descriptor call ABI.
 * This file intentionally contains no user-defined type layouts or transports.
 * This file intentionally does NOT contain: unrelated runtime subsystems.
 */
#ifndef FB_SERIALIZATION_H
#define FB_SERIALIZATION_H

FBCALL FBSTRING *fb_Serialize( void *value, FBSTRING *schema, FBSTRING *program );
FBCALL ssize_t fb_Deserialize( void *value, FBSTRING *schema, FBSTRING *program, FBSTRING *payload );
FBCALL FBSTRING *fb_SerializeArray( FBARRAY *value, FBSTRING *schema, FBSTRING *program );
FBCALL ssize_t fb_DeserializeArray( FBARRAY *value, FBSTRING *schema, FBSTRING *program, FBSTRING *payload );

#endif

/* end of fb_serialization.h */
