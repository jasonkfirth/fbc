'' Project: FreeBASIC compiler - semantic preprocessing
'' File: tooling/semantic-preprocessor.bas
'' Purpose: Preserve actual conditional decisions and scanned inactive regions.
'' Responsibilities: Own observation identities, nesting, and source associations.
'' This file intentionally does NOT contain: directive evaluation or name lookup.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-preprocessor.bi"
#include once "tooling/semantic-macros.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "preprocessor/pp.bi"

'' -------------------------------------------------------------------------
'' Conditional observation state
'' -------------------------------------------------------------------------

type SEMANTIC_PP_CONDITIONAL
	groupid as longint
	branchid as longint
	parentid as longint
	ordinal as longint
end type

'' Active conditions have the parser's depth limit. The inactive scanner can
'' encounter deeper nesting without evaluating it, so observation storage grows
'' independently. The export limit bounds memory without changing parser state.
const SEMANTIC_PP_MAX_CONDITIONAL_DEPTH = 65536
dim shared as SEMANTIC_PP_CONDITIONAL ptr semantic_pp_conditions
dim shared as integer semantic_pp_depth, semantic_pp_capacity

sub fbSemanticModelResetPreprocessor( )
	deallocate(semantic_pp_conditions)
	semantic_pp_conditions = NULL
	semantic_pp_depth = 0
	semantic_pp_capacity = 0
end sub

function fbSemanticModelPPCurrentBranch( ) as longint
	if( semantic_pp_depth = 0 ) then return 0
	return semantic_pp_conditions[semantic_pp_depth - 1].branchid
end function

private function hGrowConditions( ) as integer
	if( semantic_pp_depth < semantic_pp_capacity ) then return TRUE
	if( semantic_pp_depth >= SEMANTIC_PP_MAX_CONDITIONAL_DEPTH ) then
		fbSemanticModelFail( )
		return FALSE
	end if
	dim as integer capacity = iif(semantic_pp_capacity = 0, 64, semantic_pp_capacity * 2)
	dim as SEMANTIC_PP_CONDITIONAL ptr storage = reallocate(semantic_pp_conditions, capacity * sizeof(SEMANTIC_PP_CONDITIONAL))
	if( storage = NULL ) then
		fbSemanticModelFail( )
		return FALSE
	end if
	semantic_pp_conditions = storage
	semantic_pp_capacity = capacity
	return TRUE
end function

private function hDirectiveKind( byval token as integer ) as string
	select case token
	case FB_TK_PP_IF: return "if"
	case FB_TK_PP_IFDEF: return "ifdef"
	case FB_TK_PP_IFNDEF: return "ifndef"
	case FB_TK_PP_ELSEIF: return "elseif"
	case FB_TK_PP_ELSEIFDEF: return "elseifdef"
	case FB_TK_PP_ELSEIFNDEF: return "elseifndef"
	case FB_TK_PP_ELSE: return "else"
	end select
	return ""
end function

private function hLocation( byref source as LEX_LOCATION ) as string
	return fbSemanticModelNumber(fbSemanticModelLocationIsPhysical(source)) + TABCHAR + _
		fbSemanticModelEscape(source.source_file) + TABCHAR + _
		fbSemanticModelNumber(source.start_line) + TABCHAR + fbSemanticModelNumber(source.start_column) + TABCHAR + _
		fbSemanticModelNumber(source.end_line) + TABCHAR + fbSemanticModelNumber(source.end_column)
end function

'' -------------------------------------------------------------------------
'' Events emitted only from actual compiler decisions
'' -------------------------------------------------------------------------

function fbSemanticModelPPBranch _
	( byval token as integer, byref source as LEX_LOCATION, byval first_branch as integer ) as longint
	if( fbSemanticModelEnabled( ) = FALSE ) then return 0
	if( first_branch ) then
		if( hGrowConditions( ) = FALSE ) then return 0
		dim as longint parentid = fbSemanticModelPPCurrentBranch( )
		semantic_pp_depth += 1
		with semantic_pp_conditions[semantic_pp_depth - 1]
			.parentid = parentid
			.ordinal = 0
		end with
	elseif( semantic_pp_depth = 0 ) then
		fbSemanticModelFail( )
		return 0
	else
		semantic_pp_conditions[semantic_pp_depth - 1].ordinal += 1
	end if
	'' Snapshot before allocating the branch identity: configuration records
	'' themselves use the same transaction's detail sequence.
	dim as longint contextid = fbSemanticModelCurrentContext( )
	dim as longint branchid = fbSemanticModelNextDetailIdentity( )
	with semantic_pp_conditions[semantic_pp_depth - 1]
		.branchid = branchid
		if( first_branch ) then .groupid = branchid
		fbSemanticModelAppendProvenance("PPB" + TABCHAR + fbSemanticModelNumber(branchid) + TABCHAR + _
			fbSemanticModelNumber(.parentid) + TABCHAR + fbSemanticModelNumber(.groupid) + TABCHAR + _
			fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(contextid) + _
			TABCHAR + fbSemanticModelNumber(.ordinal) + TABCHAR + hDirectiveKind(token) + TABCHAR + hLocation(source))
	end with
	fbSemanticModelMacroOrigin("conditional", branchid, source.macro_identity, "directive")
	fbSemanticModelExportCoordinates("conditional", branchid, "directive", source, source)
	return branchid
end function

sub fbSemanticModelPPDecision _
	( byval branchid as longint, byref evaluation as const string, _
	  byval result as integer, byval selected as integer, byref ending as LEX_LOCATION )
	if( branchid = 0 ) then exit sub
	dim as string truth = ""
	if( evaluation = "evaluated" ) then truth = fbSemanticModelNumber(abs(result <> FALSE))
	fbSemanticModelAppendProvenance("PPD" + TABCHAR + fbSemanticModelNumber(branchid) + TABCHAR + _
		evaluation + TABCHAR + truth + TABCHAR + fbSemanticModelNumber(abs(selected <> FALSE)) + TABCHAR + hLocation(ending))
end sub

sub fbSemanticModelPPEnd( byref source as LEX_LOCATION )
	if( (fbSemanticModelEnabled( ) = FALSE) or (semantic_pp_depth = 0) ) then exit sub
	fbSemanticModelAppendProvenance("PPE" + TABCHAR + _
		fbSemanticModelNumber(semantic_pp_conditions[semantic_pp_depth - 1].groupid) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + hLocation(source))
	semantic_pp_depth -= 1
	fbSemanticModelExportCoordinates("conditional-end", semantic_pp_conditions[semantic_pp_depth].groupid, "directive", source, source)
end sub

sub fbSemanticModelPPDefined _
	( byval sym as FBSYMBOL ptr, byref spelling as const string, byref source as LEX_LOCATION )
	if( fbSemanticModelEnabled( ) = FALSE ) then exit sub
	dim as longint symbolid = fbSemanticModelSymbolId(sym)
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	fbSemanticModelAppendProvenance("PPT" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelPPCurrentBranch( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + fbSemanticModelNumber(symbolid) + TABCHAR + _
		fbSemanticModelEscape(spelling) + TABCHAR + fbSemanticModelNumber(abs(sym <> NULL)) + TABCHAR + hLocation(source))
	fbSemanticModelMacroOrigin("defined-probe", identity, source.macro_identity, "name")
	fbSemanticModelExportCoordinates("defined-probe", identity, "range", source, source)
end sub

sub fbSemanticModelPPSkipped _
	( byval branchid as longint, byref start_site as LEX_LOCATION, byref end_site as LEX_LOCATION )
	if( branchid = 0 ) then exit sub
	dim as LEX_LOCATION source = start_site
	if( (source.source_file <> end_site.source_file) or (end_site.start_line < source.start_line) ) then exit sub
	source.end_line = end_site.start_line
	source.end_column = end_site.start_column
	source.is_physical and= end_site.is_physical
	if( (source.end_line = source.start_line) and (source.end_column <= source.start_column) ) then exit sub
	dim as longint identity = fbSemanticModelNextDetailIdentity( )
	fbSemanticModelAppendProvenance("PPS" + TABCHAR + fbSemanticModelNumber(identity) + TABCHAR + _
		fbSemanticModelNumber(branchid) + TABCHAR + fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + hLocation(source))
end sub

'' end of tooling/semantic-preprocessor.bas
