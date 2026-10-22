'' Project: FreeBASIC compiler semantic datatype ABI
'' File: tooling/semantic-type-abi.bi
'' Purpose: Declare the datatype encoding boundary used by semantic serializers.
'' Responsibilities: ABI configuration and checked raw-type conversion entry points.
'' This file intentionally does NOT contain: native enum values presented as a stable ABI.

#pragma once

declare function fbSemanticTypeAbiConfigure(byref request as const string) as integer
declare function fbSemanticTypeAbiStable( ) as integer
declare function fbSemanticModelTypeCode(byval dtype as longint) as longint
declare function fbSemanticModelTypeNumber(byval dtype as longint) as string

'' end of tooling/semantic-type-abi.bi
