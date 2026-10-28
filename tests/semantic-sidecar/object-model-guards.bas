'' Project: FreeBASIC semantic sidecar tests
'' File: object-model-guards.bas
'' Purpose: Exercise the production object observer without compiler initialization.
'' Responsibilities: Exact copied pointers, invalid shapes, reset and inactive hooks.
'' This file intentionally does NOT implement object layout or virtual dispatch.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-object-model.bi"

dim shared as SYMBCTX symb
dim shared as FBENV env
dim shared as FBSYMBOL owner_type, base_type, base_field, table_symbol, rtti_symbol, base_rtti, name_literal, member_symbol
dim shared as FB_STRUCTEXT owner_ext, base_ext
dim shared as FB_PROCEXT member_ext
dim shared as ASTNODE table_tree, table_open, table_values(0 to 2), table_expr(0 to 2), table_close
dim shared as ASTNODE rtti_tree, rtti_open, rtti_values(0 to 2), rtti_expr(0 to 2), rtti_close, placeholder
dim shared as integer observing = TRUE, failures, identity_queries
dim shared as string captured

function fbSemanticModelFullEnabled( ) as integer
	return observing
end function

function fbSemanticModelNumber( byval value as longint ) as string
	return ltrim(str(value))
end function

function fbSemanticModelEscape( byref value as const string ) as string
	return value
end function

sub fbSemanticModelAppendDetail( byref value as string )
	captured += value + chr(10)
end sub

sub fbSemanticModelFailAt( byref reason as const string )
	failures += 1
end sub

function fbSemanticModelModuleIdentity( ) as longint
	return 1
end function

function fbSemanticModelSymbolId( byval sym as FBSYMBOL ptr ) as longint
	identity_queries += 1
	if sym = NULL then return 0
	if sym = @symb.globnspc then return 1
	if sym = @base_type then return 2
	if sym = @owner_type then return 3
	if sym = @table_symbol then return 4
	if sym = @rtti_symbol then return 5
	if sym = @base_rtti then return 6
	if sym = @name_literal then return 7
	if sym = @member_symbol then return 8
	end 10
end function

function symbProcGetVtableIndex( byval proc as FBSYMBOL ptr ) as integer
	if proc <> @member_symbol then end 11
	return member_ext.vtableindex
end function

function symbProcGetOverridden( byval proc as FBSYMBOL ptr ) as FBSYMBOL ptr
	if proc <> @member_symbol then end 12
	return member_ext.overridden
end function

private sub fresh( )
	fbSemanticModelResetObjectModel( )
	captured = ""
	failures = 0
	identity_queries = 0
end sub

'' Native table entries contain pointer-width assignments even when the
'' table symbol is an array. RTTI uses three scalar pointer fields.
env.pointersize = 8
owner_type.class = FB_SYMBCLASS_STRUCT
base_type.class = FB_SYMBCLASS_STRUCT
base_field.class = FB_SYMBCLASS_FIELD
base_field.subtype = @base_type
table_symbol.class = FB_SYMBCLASS_VAR
rtti_symbol.class = FB_SYMBCLASS_VAR
base_rtti.class = FB_SYMBCLASS_VAR
name_literal.class = FB_SYMBCLASS_VAR
member_symbol.class = FB_SYMBCLASS_PROC
member_symbol.proc.ext = @member_ext
member_ext.vtableindex = 2
owner_ext.vtable = @table_symbol
owner_ext.rtti = @rtti_symbol
owner_ext.vtableelements = 3
base_type.udt.ext = @base_ext
base_ext.rtti = @base_rtti
base_ext.vtableelements = 2
symb.rtti.fb_object = @base_type
table_tree.class = AST_NODECLASS_TYPEINI
table_tree.l = @table_open
table_open.class = AST_NODECLASS_TYPEINI_SCOPEINI
table_open.sym = @table_symbol
table_open.r = @table_values(0)
table_close.class = AST_NODECLASS_TYPEINI_SCOPEEND
table_close.sym = @table_symbol
rtti_tree.class = AST_NODECLASS_TYPEINI
rtti_tree.l = @rtti_open
rtti_open.class = AST_NODECLASS_TYPEINI_SCOPEINI
rtti_open.sym = @rtti_symbol
rtti_open.r = @rtti_values(0)
rtti_close.class = AST_NODECLASS_TYPEINI_SCOPEEND
rtti_close.sym = @rtti_symbol
placeholder.class = AST_NODECLASS_NOP
for index as integer = 0 to 2
	table_values(index).class = AST_NODECLASS_TYPEINI_ASSIGN
	table_values(index).l = @table_expr(index)
	table_values(index).typeini.ofs = index * env.pointersize
	table_values(index).typeini.bytes = env.pointersize
	table_expr(index).dtype = typeAddrOf(FB_DATATYPE_VOID)
	rtti_values(index).class = AST_NODECLASS_TYPEINI_ASSIGN
	rtti_values(index).l = @rtti_expr(index)
	rtti_values(index).typeini.ofs = index * env.pointersize
	rtti_values(index).typeini.bytes = env.pointersize
	rtti_expr(index).dtype = typeAddrOf(FB_DATATYPE_VOID)
	if index < 2 then
		table_values(index).r = @table_values(index + 1)
		rtti_values(index).r = @rtti_values(index + 1)
	end if
next
table_values(2).r = @table_close
rtti_values(2).r = @rtti_close
table_expr(0).class = AST_NODECLASS_CONST
table_expr(1).class = AST_NODECLASS_OFFSET
table_expr(1).sym = @rtti_symbol
table_expr(2).class = AST_NODECLASS_OFFSET
table_expr(2).sym = @member_symbol
rtti_expr(0).class = AST_NODECLASS_CONST
rtti_expr(1).class = AST_NODECLASS_OFFSET
rtti_expr(1).sym = @name_literal
rtti_expr(2).class = AST_NODECLASS_OFFSET
rtti_expr(2).sym = @base_rtti

'' A plain UDT's inactive extension fields must not be consulted.
fbSemanticModelObjectSnapshot(@owner_type, 3)
if failures or instr(captured, "plain" + chr(9) + "0" + chr(9) + "0") = 0 then end 1
owner_type.udt.ext = @owner_ext
owner_type.udt.base = @base_field
owner_type.stats or= FB_SYMBSTATS_HASRTTI
symbGetCompSymbTb(@owner_type).head = @member_symbol
fbSemanticModelObjectSnapshot(@owner_type, 3)
fbSemanticModelObjectVtable(@owner_type, @table_symbol, 2, 0, @table_tree)
fbSemanticModelObjectRtti(@owner_type, @rtti_symbol, "initialized", @rtti_tree)
fbSemanticModelFinishObjectModel(TRUE)
if failures then end 2
if instr(captured, "virtual-table-slot:1:2" + chr(9) + "address" + chr(9) + "8") = 0 then end 2
if instr(captured, "object-model-counts:1" + chr(9) + "2" + chr(9) + "1" + chr(9) + "3" + chr(9) + "1" + chr(9) + "1") = 0 then end 2
if table_values(2).l <> @table_expr(2) or member_ext.vtableindex <> 2 then end 2

fresh( )
fbSemanticModelObjectSnapshot(NULL, 0)
fbSemanticModelObjectSnapshot(@member_symbol, 8)
fbSemanticModelObjectSnapshot(@owner_type, 0)
table_symbol.class = FB_SYMBCLASS_PROC
fbSemanticModelObjectSnapshot(@owner_type, 3)
table_symbol.class = FB_SYMBCLASS_VAR
if failures <> 4 or len(captured) then end 3

fresh( )
fbSemanticModelObjectVtable(NULL, NULL, 0, 0, NULL)
fbSemanticModelObjectVtable(@owner_type, NULL, 0, 0, NULL)
fbSemanticModelObjectVtable(@owner_type, @table_symbol, -1, 0, @table_tree)
fbSemanticModelObjectVtable(@owner_type, @table_symbol, 2, -1, @table_tree)
if failures <> 4 or len(captured) then end 4

'' Failed element decoding must stop before reading unrelated union fields.
for broken as integer = 0 to 3
	fresh( )
	select case broken
	case 0: table_expr(2).ofs.ofs = 1
	case 1: table_expr(2).class = AST_NODECLASS_CALL
	case 2: table_values(2).class = AST_NODECLASS_TYPEINI_CTORCALL
	case 3: table_values(2).r = NULL
	end select
	fbSemanticModelObjectVtable(@owner_type, @table_symbol, 2, 0, @table_tree)
	if failures <> 1 then end 5
	table_expr(2).ofs.ofs = 0
	table_expr(2).class = AST_NODECLASS_OFFSET
	table_values(2).class = AST_NODECLASS_TYPEINI_ASSIGN
	table_values(2).r = @table_close
next

fresh( )
fbSemanticModelObjectRtti(NULL, NULL, "initialized", NULL)
fbSemanticModelObjectRtti(@owner_type, NULL, "initialized", @rtti_tree)
fbSemanticModelObjectRtti(@owner_type, @rtti_symbol, "pretend", @rtti_tree)
fbSemanticModelObjectRtti(@owner_type, @rtti_symbol, "deferred", @rtti_tree)
if failures <> 4 or len(captured) then end 6
failures = 0
fbSemanticModelObjectRtti(@owner_type, @rtti_symbol, "deferred", @placeholder)
if failures or instr(captured, "deferred") = 0 or instr(captured, "unavailable" + chr(9) + "0") = 0 then end 6
fresh( )
rtti_expr(0).val.i = 1
fbSemanticModelObjectRtti(@owner_type, @rtti_symbol, "initialized", @rtti_tree)
if failures <> 1 then end 6
rtti_expr(0).val.i = 0

fresh( )
fbSemanticModelObjectSnapshot(@owner_type, 3)
dim as string previous_capture = captured
fbSemanticModelFinishObjectModel(FALSE)
if failures or captured <> previous_capture then end 7
captured = ""
fbSemanticModelObjectSnapshot(@owner_type, 3)
if instr(captured, "object-model-snapshot:1" + chr(9)) = 0 then end 7

fresh( )
observing = FALSE
fbSemanticModelObjectSnapshot(NULL, 0)
fbSemanticModelObjectVtable(NULL, NULL, -1, -1, NULL)
fbSemanticModelObjectRtti(NULL, NULL, "pretend", NULL)
fbSemanticModelFinishObjectModel(TRUE)
if failures or len(captured) or identity_queries then end 8
print "OBJECT_MODEL_GUARDS_PASS"

'' end of object-model-guards.bas
