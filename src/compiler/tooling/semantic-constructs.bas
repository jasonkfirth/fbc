'' Project: FreeBASIC compiler - semantic source constructs
'' File: tooling/semantic-constructs.bas
'' Purpose: Preserve statements and compound extents before their ASTs are flushed.
'' Responsibilities: Own observation stacks, source extents, and fact associations.
'' This file intentionally does NOT contain: grammar dispatch or control-flow inference.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-macros.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "tooling/semantic-constructs.bi"
#include once "parser/parser.bi"

'' -------------------------------------------------------------------------
'' Observation stacks
'' -------------------------------------------------------------------------

type SEMANTIC_SOURCE_STATEMENT
	identity as longint
	parent as longint
	compound as longint
	source as LEX_LOCATION
	token as integer
end type

type SEMANTIC_SOURCE_COMPOUND
	identity as longint
	statement as longint
	source as LEX_LOCATION
end type

const SEMANTIC_CONSTRUCT_MAX_DEPTH = 65536
dim shared as SEMANTIC_SOURCE_STATEMENT ptr semantic_statements
dim shared as integer semantic_statement_depth, semantic_statement_capacity
dim shared as SEMANTIC_SOURCE_COMPOUND ptr semantic_compounds
dim shared as integer semantic_compound_depth, semantic_compound_capacity

sub fbSemanticModelResetConstructs( )
	deallocate(semantic_statements)
	deallocate(semantic_compounds)
	semantic_statements = NULL
	semantic_compounds = NULL
	semantic_statement_depth = 0
	semantic_statement_capacity = 0
	semantic_compound_depth = 0
	semantic_compound_capacity = 0
end sub

private function hGrow( byref storage as any ptr, byref capacity as integer, byval depth as integer, byval element_bytes as integer ) as integer
	if( depth < capacity ) then return TRUE
	if( depth >= SEMANTIC_CONSTRUCT_MAX_DEPTH ) then
		fbSemanticModelFail( )
		return FALSE
	end if
	dim as integer next_capacity = iif(capacity = 0, 64, capacity * 2)
	dim as any ptr next_storage = reallocate(storage, next_capacity * element_bytes)
	if( next_storage = NULL ) then
		fbSemanticModelFail( )
		return FALSE
	end if
	storage = next_storage
	capacity = next_capacity
	return TRUE
end function

function fbSemanticModelCurrentStatement( ) as longint
	if( semantic_statement_depth = 0 ) then return 0
	return semantic_statements[semantic_statement_depth - 1].identity
end function

private function hCurrentCompound( ) as longint
	if( semantic_compound_depth = 0 ) then return 0
	return semantic_compounds[semantic_compound_depth - 1].identity
end function

private function hLocation( byref source as LEX_LOCATION ) as string
	return fbSemanticModelNumber(fbSemanticModelLocationIsPhysical(source)) + TABCHAR + _
		fbSemanticModelEscape(source.source_file) + TABCHAR + fbSemanticModelNumber(source.start_line) + TABCHAR + _
		fbSemanticModelNumber(source.start_column) + TABCHAR + fbSemanticModelNumber(source.end_line) + TABCHAR + _
		fbSemanticModelNumber(source.end_column)
end function

private function hCompoundKind( byval token as integer ) as string
	select case token
	case FB_TK_IF: return "if"
	case FB_TK_FOR: return "for"
	case FB_TK_DO: return "do"
	case FB_TK_WHILE: return "while"
	case FB_TK_SELECT: return "select"
	case FB_TK_WITH: return "with"
	case FB_TK_SCOPE: return "scope"
	case FB_TK_NAMESPACE: return "namespace"
	case FB_TK_EXTERN: return "extern"
	case FB_TK_FUNCTION: return "procedure"
	case FB_TK_TYPE: return "type"
	case FB_TK_UNION: return "union"
	case FB_TK_ENUM: return "enum"
	case FB_TK_ASM: return "assembly"
	case else: return "unknown"
	end select
end function

'' -------------------------------------------------------------------------
'' Parser boundaries and fact ownership
'' -------------------------------------------------------------------------

function fbSemanticModelStatementBegin( byref source as LEX_LOCATION, byval token as integer, byval token_class as integer ) as longint
	if( fbSemanticModelEnabled( ) = FALSE ) then return 0
	if( (token = FB_TK_EOL) or (token = FB_TK_EOF) or (token = FB_TK_COMMENT) or (token = FB_TK_REM) or _
	    (token = FB_TK_ELSE) and (semantic_statement_depth > 0) ) then return 0
	if( hGrow(semantic_statements, semantic_statement_capacity, semantic_statement_depth, sizeof(SEMANTIC_SOURCE_STATEMENT)) = FALSE ) then return 0
	dim as longint parent = fbSemanticModelCurrentStatement( ), compound = hCurrentCompound( )
	dim as longint owner = fbSemanticModelSymbolId(parser.currproc)
	dim as longint context = fbSemanticModelCurrentContext( )
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	with semantic_statements[semantic_statement_depth]
		.identity = identity
		.parent = parent
		.compound = compound
		.source = source
		.token = token
	end with
	semantic_statement_depth += 1
	fbSemanticModelAppendProvenance("ST" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(parent) + _
		TABCHAR + fbSemanticModelNumber(compound) + TABCHAR + fbSemanticModelNumber(owner) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(context) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )) + TABCHAR + fbSemanticModelNumber(parser.stmt.cnt) + _
		TABCHAR + fbSemanticModelNumber(token) + TABCHAR + fbSemanticModelNumber(token_class) + TABCHAR + hLocation(source))
	fbSemanticModelMacroOrigin("statement", identity, source.macro_identity, "start")
	return identity
end function

sub fbSemanticModelStatementEnd( byval identity as longint, byref route as const string, byref ending as LEX_LOCATION, byval errors_before as integer )
	if( identity = 0 ) then exit sub
	if( (semantic_statement_depth = 0) or (fbSemanticModelCurrentStatement( ) <> identity) ) then
		fbSemanticModelFail( )
		exit sub
	end if
	semantic_statement_depth -= 1
	dim as LEX_LOCATION start_site = semantic_statements[semantic_statement_depth].source
	dim as string outcome = iif(errGetCount( ) = errors_before, "parsed", "recovered")
	if( route = "unmatched" ) then outcome = "unmatched"
	fbSemanticModelAppendProvenance("STE" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + route + TABCHAR + outcome + TABCHAR + hLocation(ending))
	fbSemanticModelMacroOrigin("statement", identity, ending.macro_identity, "end")
	fbSemanticModelExportCoordinates("statement", identity, "range", start_site, ending)
end sub

function fbSemanticModelConstructBegin( byval token as integer ) as longint
	if( fbSemanticModelEnabled( ) = FALSE ) then return 0
	if( hGrow(semantic_compounds, semantic_compound_capacity, semantic_compound_depth, sizeof(SEMANTIC_SOURCE_COMPOUND)) = FALSE ) then return 0
	dim as longint parent = hCurrentCompound( ), statement = fbSemanticModelCurrentStatement( )
	dim as longint owner = fbSemanticModelSymbolId(parser.currproc), context = fbSemanticModelCurrentContext( )
	dim as LEX_LOCATION source = lexGetLastLocation( )
	if( semantic_statement_depth > 0 ) then source = semantic_statements[semantic_statement_depth - 1].source
	dim as string kind = hCompoundKind(token)
	if( (token = FB_TK_TYPE) and (semantic_statement_depth > 0) ) then
		if( semantic_statements[semantic_statement_depth - 1].token = FB_TK_UNION ) then kind = "union"
	end if
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	with semantic_compounds[semantic_compound_depth]
		.identity = identity
		.statement = statement
		.source = source
	end with
	semantic_compound_depth += 1
	fbSemanticModelAppendProvenance("BLK" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(parent) + _
		TABCHAR + fbSemanticModelNumber(statement) + TABCHAR + fbSemanticModelNumber(owner) + TABCHAR + kind + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(context) + TABCHAR + hLocation(source))
	fbSemanticModelMacroOrigin("construct", identity, source.macro_identity, "start")
	return identity
end function

sub fbSemanticModelConstructEnd( byval identity as longint, byref ending as LEX_LOCATION )
	if( identity = 0 ) then exit sub
	if( (semantic_compound_depth = 0) or (hCurrentCompound( ) <> identity) ) then
		fbSemanticModelFail( )
		exit sub
	end if
	semantic_compound_depth -= 1
	fbSemanticModelAppendProvenance("BEND" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + hLocation(ending))
	fbSemanticModelMacroOrigin("construct", identity, ending.macro_identity, "end")
	fbSemanticModelExportCoordinates("construct", identity, "range", semantic_compounds[semantic_compound_depth].source, ending)
end sub

sub fbSemanticModelAssociateStatement( byref domain as const string, byval subject as longint, byval statement as longint )
	if( statement = 0 ) then statement = fbSemanticModelCurrentStatement( )
	if( (statement = 0) or (subject = 0) ) then exit sub
	fbSemanticModelAppendProvenance("OWN" + TABCHAR + domain + TABCHAR + fbSemanticModelNumber(subject) + TABCHAR + fbSemanticModelNumber(statement))
end sub

sub fbSemanticModelStatementOperation( byref operation as const string )
	dim as longint statement = fbSemanticModelCurrentStatement( )
	if( (statement = 0) or (fbSemanticModelEnabled( ) = FALSE) ) then exit sub
	'' The caller is the grammar route that actually accepted this builtin,
	'' rather than a spelling lookup that could misclassify a user identifier.
	fbSemanticModelAppendProvenance("SOP" + TABCHAR + fbSemanticModelNumber(statement) + TABCHAR + operation)
end sub

'' end of tooling/semantic-constructs.bas
