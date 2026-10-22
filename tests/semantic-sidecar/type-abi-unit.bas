'' Project: FreeBASIC semantic datatype ABI tests
'' File: type-abi-unit.bas
'' Purpose: Check stable base and mangling fields independently of model parsing.
'' Responsibilities: Default modes, all five-bit identities and preserved flags.
'' This file intentionally does NOT compile or execute analyzed user programs.
#lang "fb"
#include once "tooling/semantic-private.bi"

'' This test links the production ABI module without the model writer.
function fbSemanticModelNumber(byval value as longint) as string
	return ltrim(str(value))
end function

dim as integer wanted_default = valint(command(1))
if( fbSemanticTypeAbiStable( ) <> wanted_default ) then end 1
dim as string stable_request = "stable-v1", native_request = "native", bad_request = "unknown"
if( fbSemanticTypeAbiConfigure(stable_request) = FALSE ) then end 1
if( fbSemanticTypeAbiConfigure(bad_request) <> FALSE ) then end 1
if( fbSemanticTypeAbiStable( ) <> TRUE ) then end 1

'' Stable-v1 preserves the original slots and appends the two wide integers.
'' Entries 29..31 are reserved native bit patterns, not accepted Y identities.
dim as integer expected_base(0 to 31) => { _
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, _
	27, 28, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 29, 30, 31 }
dim as ulong flag_patterns(0 to 3) => { 0, &h3fe00ul, &h80000ul, &hf0080000ul }
dim as longint checks
for base_type as integer = 0 to 31
	for mangled_type as integer = 0 to 31
		for depth as integer = 0 to 8
			for pattern as integer = 0 to ubound(flag_patterns)
				dim as ulong source_bits = culng(base_type) or _
					(culng(mangled_type) shl FB_DT_MANGLEPOS) or _
					(culng(depth) shl 5) or flag_patterns(pattern)
				dim as ulong expected_bits = culng(expected_base(base_type)) or _
					(culng(expected_base(mangled_type)) shl FB_DT_MANGLEPOS) or _
					(culng(depth) shl 5) or flag_patterns(pattern)
				if( fbSemanticModelTypeCode(clngint(source_bits)) <> clngint(expected_bits) ) then end 1
				if( fbSemanticTypeAbiConfigure(native_request) = FALSE ) then end 1
				if( fbSemanticModelTypeCode(clngint(source_bits)) <> clngint(source_bits) ) then end 1
				if( fbSemanticTypeAbiConfigure(stable_request) = FALSE ) then end 1
				checks += 2
			next
		next
	next
next
if( fbSemanticModelTypeCode(4294967295ll) <> 4294967295ll ) then end 1
if( fbSemanticModelTypeNumber(19) <> "17" ) then end 1
print "TYPE_ABI_UNIT_PASS checks="; checks
end 0
'' end of type-abi-unit.bas
