'' Project: FreeBASIC compiler - typed syntax tree nodes
'' -----------------------------------------
''
'' File: ast/nodes/ast-node-var.bas
''
'' Purpose:
''
''     Build and lower AST variable access nodes.
''
'' Responsibilities:
''
''     - check operand types and build the node family described below
''     - lower nodes through the common IR and runtime call interfaces
''
'' This file intentionally does NOT contain:
''
''     - token scanning or physical register allocation
''

'' AST variable access nodes

#include once "core/fbint.bi"
#include once "ast/ast.bi"
#include once "backend/ir.bi"
#include once "runtime/rtl.bi"

function astNewVAR _
	( _
		byval sym as FBSYMBOL ptr, _
		byval ofs as longint, _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval no_thread_local as integer _
	) as ASTNODE ptr

	dim as ASTNODE ptr n = any

	assert( iif( sym, symbIsField( sym ) = FALSE, TRUE ) )

	if( dtype = FB_DATATYPE_INVALID ) then
		select case( symbGetClass( sym ) )
		case FB_SYMBCLASS_LABEL
			dtype = FB_DATATYPE_VOID
			subtype = NULL
		case FB_SYMBCLASS_PROC
			dtype = FB_DATATYPE_FUNCTION
			if( symbGetIsFuncPtr( sym ) ) then
				subtype = sym
			else
				subtype = symbAddProcPtrFromFunction( sym )
			end if
		case else
			dtype = symbGetFullType( sym )
			subtype = symbGetSubtype( sym )
			if( symbIsParamVarByRef( sym ) or symbIsImport( sym ) or symbIsRef( sym ) ) then
				'' If it's a reference, then it's really a pointer
				'' (the caller is expected to do the implicit DEREF)
				dtype = typeAddrOf( dtype )
			end if
		end select
	end if

	if( sym andalso symbIsThreadLocal( sym ) andalso (no_thread_local = FALSE) andalso _
	    ((env.clopt.backend = FB_BACKEND_GAS) or _
	     (env.clopt.backend = FB_BACKEND_GAS64)) andalso _
	    (symbIsThreadLocalC( sym ) = FALSE orElse _
	     env.clopt.target <> FB_COMPTARGET_LINUX orElse _
	     fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB)) then
		dim as ASTNODE ptr call_node = astNewCALL( rtlProcLookup( @FB_RTL_TLSGETVAR, FB_RTL_IDX_TLSGETVAR ) )
		dim as integer pointer_integer_type = _
			iif( env.pointersize = 8, FB_DATATYPE_ULONGINT, FB_DATATYPE_ULONG )
		if( astNewARG( call_node, astNewCONSTstr( symbGetMangledName( sym ) ) ) = NULL ) then
			astDelTREE( call_node )
			return NULL
		end if
		if( astNewARG( call_node, astNewCONSTi( symbGetRealSize( sym ), FB_DATATYPE_ULONG ) ) = NULL ) then
			astDelTREE( call_node )
			return NULL
		end if
		dim as integer align = sym->var_.align
		if( align = 0 ) then align = typeCalcNaturalAlign( symbGetFullType( sym ), symbGetSubtype( sym ) )
		if( align < 1 ) then align = 1
		if( astNewARG( call_node, astNewCONSTi( align, FB_DATATYPE_ULONG ) ) = NULL ) then
			astDelTREE( call_node )
			return NULL
		end if
		if( symbGetTypeIniTree( sym ) andalso (symbIsExtern( sym ) = FALSE) ) then
			dim as ASTNODE ptr initial_address = astNewADDROF( _
				astNewVAR( sym, 0, FB_DATATYPE_INVALID, NULL, TRUE ) )
			initial_address = astNewCONV( pointer_integer_type, NULL, initial_address )
			if( astNewARG( call_node, initial_address ) = NULL ) then
				astDelTREE( call_node )
				return NULL
			end if
		else
			if( astNewARG( call_node, astNewCONSTi( 0, pointer_integer_type ) ) = NULL ) then
				astDelTREE( call_node )
				return NULL
			end if
		end if
		return astNewDEREF( call_node, dtype, subtype, ofs )
	end if

	'' alloc new node
	n = astNewNode( AST_NODECLASS_VAR, dtype, subtype )

	if( sym ) then
		if( symbIsVar( sym ) and symbIsTemp( sym ) ) then
			astDtorListAddRef( sym )
		end if
	end if

	n->sym = sym
	n->var_.ofs = ofs

	'' boolean? do conversion on load
	if( typeGetDtAndPtrOnly( dtype ) = FB_DATATYPE_BOOLEAN ) then
		n = astNewCONV( dtype, NULL, n )
	end if

	function = n
end function

function astLoadVAR( byval n as ASTNODE ptr ) as IRVREG ptr
	dim as FBSYMBOL ptr s = any
	dim as longint ofs = any
	dim as IRVREG ptr vr = NULL

	s = n->sym
	ofs = n->var_.ofs
	if( s <> NULL ) then
		symbSetIsAccessed( s )
		ofs += symbGetOfs( s )
	end if

	if( ast.doemit ) then
		vr = irAllocVRVAR( astGetFullType( n ), n->subtype, s, ofs )
		vr->vector = n->vector
	end if

	function = vr
end function

'' end of ast/nodes/ast-node-var.bas
