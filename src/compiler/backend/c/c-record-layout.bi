/'
Project: FreeBASIC C backend record layout
File: backend/c/c-record-layout.bi
Purpose: Check each emitted named field against its compiler-owned byte offset.
Responsibilities: Verify promoted anonymous fields without changing record declarations.
This file intentionally does NOT pack records, add fields or alter aggregate ABI types.
'/

private sub hCVerifyRecordFields(byval owner as FBSYMBOL ptr)
	'' Size alone can conceal a displaced member if trailing padding absorbs
	'' the difference. Check flattened fields using their original C names;
	'' anonymous struct/union members are promoted into this enclosing scope.
	var fld = symbUdtGetFirstField(owner)
	while( fld <> NULL )
		'' Dynamic array symbols are synthetic. Their descriptor fields have
		'' separate physical declarations and are verified on their own turn.
		if( symbIsDynamic(fld) = FALSE ) then
			hWriteStaticAssert("__builtin_offsetof(" + hGetUdtTag(owner) + hGetUdtId(owner) + ", " + _
			                   hGetCFieldId(fld) + ") == " + str(symbGetOfs(fld)))
		end if
		fld = symbUdtGetNextField(fld)
	wend
end sub

'' end of backend/c/c-record-layout.bi
