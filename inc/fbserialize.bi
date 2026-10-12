'' FreeBASIC logical serialization
'' File: fbserialize.bi
'' Purpose: Opt in to compiler-assisted binary serialization.
'' Responsibilities: Public names and error constants.
'' This file intentionally contains no field registration or transport code.
#ifndef __FBSERIALIZE_BI__
#define __FBSERIALIZE_BI__

#define Serialize __FB_SERIALIZE
#define Deserialize __FB_DESERIALIZE

'' Serialize returns an empty string on failure and sets ERR.
'' Deserialize returns TRUE on success; on failure the destination is unchanged.
'' Existing runtime error numbers distinguish malformed input from exhaustion.
const FB_SERIALIZE_INVALID = 1
const FB_SERIALIZE_OUT_OF_MEMORY = 4

#endif
'' end of fbserialize.bi
