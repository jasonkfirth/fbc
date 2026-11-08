'' TEST_MODE : COMPILE_AND_RUN_OK
/'
Project: FreeBASIC C backend regression tests
File: c-storage-alignment-owner.bas
Purpose: Check member-array stride ownership in the actual alignment helper.
Responsibilities: Reject a member name used as a symbol, retain unknown origins,
				and check known, negative and unknown indices without invalid dereferences.
This file models the query's symbol interface. It does not copy the query.
'/
const NULL=0

const FB_SYMBCLASS_VAR=1, FB_SYMBCLASS_PARAM=2, FB_PARAMMODE_BYVAL=1
const FB_DATATYPE_INTEGER=0, FB_DATATYPE_BYTE=1, FB_DATATYPE_BOOLEAN=2
const FB_DATACLASS_INTEGER=0
enum
	EXPRCLASS_TEXT
	EXPRCLASS_IMM
	EXPRCLASS_SYM
	EXPRCLASS_ARRAYADDR
	EXPRCLASS_CAST
	EXPRCLASS_UOP
	EXPRCLASS_BOP
	EXPRCLASS_MEMBER
	EXPRCLASS_INDEX
end enum
enum
	AST_OP_DEREF
	AST_OP_ADDROF
	AST_OP_MUL
	AST_OP_AND
	AST_OP_SHL
	AST_OP_ADD
	AST_OP_SUB
end enum
type FBSYMBOL
	cls as integer
	dtype as integer
	bytes as integer
	array_flag as integer
	subtype as FBSYMBOL ptr
end type
type FBVALUE
	i as longint
end type
type EXPRNODE
	class as integer
	dtype as integer
	subtype as FBSYMBOL ptr
	l as EXPRNODE ptr
	r as EXPRNODE ptr
	union
		text as zstring ptr
		val as FBVALUE
		sym as FBSYMBOL ptr
		op as integer
	end union
end type
#define symbIsVar(s) ((s)->cls=FB_SYMBCLASS_VAR)
#define symbGetClass(s) ((s)->cls)
#define symbIsRef(s) FALSE
#define symbIsImport(s) FALSE
#define symbGetParamMode(s) FB_PARAMMODE_BYVAL
#define symbGetFullType(s) ((s)->dtype)
#define symbGetSubtype(s) ((s)->subtype)
#define symbIsCArray(s) ((s)->array_flag)
#define typeIsPtr(dtype) ((dtype) and 512)
#define typeGetClass(dtype) FB_DATACLASS_INTEGER
#define typeGetDtAndPtrOnly(dtype) ((dtype) and 511)
dim shared as FBSYMBOL array_symbol
dim shared as integer wrong_owner, size_queries
private function symbGetSizeOf(byval s as FBSYMBOL ptr) as longint
	size_queries+=1
	if s<>@array_symbol then
		wrong_owner+=1
		return 5
	end if
	return s->bytes
end function
private function typeCalcNaturalAlign(byval dtype as integer, byval subtype as FBSYMBOL ptr) as integer
	if subtype<>NULL then return 1
	if dtype=FB_DATATYPE_INTEGER then return 16
	return 1
end function
private function hNormalizeOffsetMod _
	( _
		byval ofs as longint, _
		byval align as integer _
	) as integer

	dim as integer n = any

	n = ofs mod align
	if( n < 0 ) then
		n += align
	end if

	function = n
end function
private function hCArrayIndexImmediate _
	( byval n as EXPRNODE ptr, byref value as longint ) as integer

	if( n = NULL ) then return FALSE
	if( typeGetClass(n->dtype) <> FB_DATACLASS_INTEGER or typeIsPtr(n->dtype) ) then return FALSE
	if( n->class = EXPRCLASS_IMM ) then
		value = n->val.i
		return TRUE
	end if
	'' Shift amounts are masked by _emitBop before reaching this tree.
	if( n->class = EXPRCLASS_BOP andalso n->op = AST_OP_AND ) then
		dim as longint left_value, right_value
		if( hCArrayIndexImmediate(n->l, left_value) andalso _
		    hCArrayIndexImmediate(n->r, right_value) ) then
			value = left_value and right_value
			return TRUE
		end if
	end if
	return FALSE
end function

'' Query nodes borrow stack storage. They must never be freed by the query.
private sub exprFreeNode(byval node as EXPRNODE ptr)
	if node<>NULL then end 10
end sub
type AlignmentFixtureContext
	unalignedid as uinteger
end type
dim shared as AlignmentFixtureContext ctx
#include once "../../src/compiler/backend/c/c-alignment.bi"

dim as FBSYMBOL root_symbol
root_symbol.cls=FB_SYMBCLASS_VAR
root_symbol.dtype=FB_DATATYPE_INTEGER
array_symbol.bytes=72
dim as zstring*64 field_name="ARRAY_FIELD_NAME_IS_NOT_A_SYMBOL"
dim as EXPRNODE origin, member_origin, member_offset, index_node, unknown_index
origin.class=EXPRCLASS_SYM:origin.sym=@root_symbol
member_offset.class=EXPRCLASS_IMM:member_offset.val.i=80
member_origin.class=EXPRCLASS_MEMBER:member_origin.l=@origin:member_origin.r=@member_offset
member_origin.text=@field_name
unknown_index.class=EXPRCLASS_TEXT:unknown_index.text=strptr("dynamic_index")
index_node.class=EXPRCLASS_INDEX:index_node.l=@member_origin:index_node.r=@unknown_index
index_node.sym=@array_symbol
var result=hCStorageOffsetMod(@index_node,8)

	if wrong_owner<>0 or result<>0 then end 2
	if hCStorageOffsetMod(@index_node,16)<>-1 then end 3
	dim as EXPRNODE known_index
	known_index.class=EXPRCLASS_IMM:known_index.val.i=3
	index_node.r=@known_index
	if hCStorageOffsetMod(@index_node,16)<>8 then end 4
	known_index.val.i=-1
	if hCStorageOffsetMod(@index_node,16)<>8 then end 5
	index_node.sym=0
	var previous_queries=size_queries
	if hCStorageOffsetMod(@index_node,8)<>-1 then end 6
	if size_queries<>previous_queries then end 7
	index_node.sym=@array_symbol:root_symbol.dtype=FB_DATATYPE_BYTE
	if hCStorageOffsetMod(@index_node,8)<>-1 then end 8
	if wrong_owner<>0 then end 9

' end of c-storage-alignment-owner.bas
