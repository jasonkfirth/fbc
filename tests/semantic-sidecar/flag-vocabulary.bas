'' Project: FreeBASIC semantic sidecar tests
'' File: flag-vocabulary.bas
'' Purpose: Exercise the pure native flag mapper over bits and symbol classes.
'' Responsibilities: Zero/all masks, each individual bit and invalid class bounds.
'' This file intentionally does NOT initialize compiler state or publish a model.

#include once "tooling/semantic-flag-names.bi"

dim as string group_name = "stale", flag_name = "stale", classes = "stale"
dim as ulong definition_bits = &hffffffffUL
if fbSemanticFlagDefinition(-1, group_name, flag_name, definition_bits, classes) then end 1
if len(group_name) or len(flag_name) or len(classes) or definition_bits then end 2
if fbSemanticFlagDefinition(fbSemanticFlagDefinitionCount(), group_name, flag_name, definition_bits, classes) then end 3
if len(group_name) or len(flag_name) or len(classes) or definition_bits then end 4
dim as ulong invalid_unmapped
if len(fbSemanticNamedFlags(-1, 1, &hffffffffUL, invalid_unmapped)) then end 5
if invalid_unmapped <> &hffffffffUL then end 6
if len(fbSemanticNamedFlags(5, 1, &hffffffffUL, invalid_unmapped)) then end 7
if invalid_unmapped <> &hffffffffUL then end 8

for group_id as integer = 0 to 4
	for symbol_class as integer = 0 to 18
		for input_bit as integer = -2 to 31
			dim as ulong bits = 0
			if input_bit = -1 then
				bits = &hffffffffUL
			elseif input_bit >= 0 then
				bits = 1UL shl input_bit
			end if
			dim as ulong unmapped
			dim as string names = fbSemanticNamedFlags(group_id, symbol_class, bits, unmapped)
			print ltrim(str(group_id)); chr(9); ltrim(str(symbol_class)); chr(9); _
				  ltrim(str(input_bit)); chr(9); names; chr(9); ltrim(str(unmapped))
		next
	next
next

'' end of flag-vocabulary.bas
