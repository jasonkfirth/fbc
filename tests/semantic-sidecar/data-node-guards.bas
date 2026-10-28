'' Project: FreeBASIC DATA payload tests
'' File: data-node-guards.bas
'' Purpose: Exercise real transient DATA constructors without the full parser.
'' Responsibilities: Active union tags, root count, literal guards and list order.
'' This file intentionally does NOT contain: alternate DATA constructors or serialization.

#include once "core/fbint.bi"
#include once "ast/ast.bi"

'' A bounded test arena replaces only allocation. Poisoned payload storage
'' proves the native constructors initialize their own active union members.
dim shared as ASTNODE arena(0 to 15)
dim shared as integer allocations, literal_queries

function astNewNode( byval class_ as integer, byval dtype as integer, byval subtype as FBSYMBOL ptr ) as ASTNODE ptr
	if( allocations > ubound(arena) ) then end 50
	dim as ASTNODE ptr result = @arena(allocations)
	allocations += 1
	result->class = class_
	result->dtype = dtype
	result->data.id = -1234567
	return result
end function

function astGetStrLitSymbol( byval node as ASTNODE ptr ) as FBSYMBOL ptr
	literal_queries += 1
	return node->sym
end function

'' Native constructors are inserted here.

dim as ASTNODE ptr root = astDataStmtBegin()
if( root->class <> AST_NODECLASS_DATASTMT or root->data.elmts <> 0 ) then end 1
dim as FBSYMBOL literal
dim as ASTNODE expressions(0 to 5)
dim as integer expected(0 to 6) => { FB_DATASTMT_ID_ZSTR, FB_DATASTMT_ID_WSTR, _
	FB_DATASTMT_ID_CONST, FB_DATASTMT_ID_OFFSET, FB_DATASTMT_ID_CONST, FB_DATASTMT_ID_CONST, FB_DATASTMT_ID_LINK }
expressions(0).dtype = FB_DATATYPE_CHAR
expressions(0).sym = @literal
expressions(1).dtype = FB_DATATYPE_WCHAR
expressions(1).sym = @literal
expressions(2).dtype = FB_DATATYPE_DOUBLE
'' A symbol on a nonstring expression must not be read as literal payload.
expressions(2).sym = @literal
expressions(3).class = AST_NODECLASS_OFFSET
expressions(3).dtype = FB_DATATYPE_INTEGER
expressions(4).dtype = FB_DATATYPE_CHAR
expressions(5).dtype = FB_DATATYPE_WCHAR
for index as integer = 0 to 6
	dim as ASTNODE ptr expression
	if( index < 6 ) then expression = @expressions(index)
	dim as ASTNODE ptr item = astDataStmtStore(root, expression)
	if( item->class <> AST_NODECLASS_DATASTMT or item->data.id <> expected(index) or item->l <> expression or item->r <> NULL ) then end 2
	if( root->data.elmts <> index + 1 or root->r <> item ) then end 3
next
if( literal_queries <> 4 ) then end 4
dim as ASTNODE ptr cursor = root->l
for index as integer = 0 to 6
	if( cursor = NULL ) then end 5
	if( cursor->data.id <> expected(index) ) then end 6
	cursor = cursor->r
next
if( cursor <> NULL or allocations <> 8 ) then end 7
print "DATA_NODE_GUARDS_PASS"

'' end of data-node-guards.bas
