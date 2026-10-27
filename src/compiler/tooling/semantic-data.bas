'' Project: FreeBASIC compiler - semantic DATA streams
'' File: tooling/semantic-data.bas
'' Purpose: Preserve DATA source order before descriptor initialization consumes it.
'' Responsibilities: Original values, native descriptors, READ targets and RESTORE selections.
'' This file intentionally does NOT contain: name resolution or runtime cursor inference.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-data.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "tooling/semantic-macros.bi"

declare sub fbSemanticModelExportExpressionDistinct _
	( byval expr as ASTNODE ptr, byref first as LEX_LOCATION, byref last as LEX_LOCATION, _
	  byval first_nonphysical as longint, byval last_nonphysical as longint )

'' Compilation and export are serial. These counters own no parser or AST
'' storage and reset with every module transaction, including parser restarts.
'' One million observations matches the reader's bounded source-event domain.
const SEMANTIC_DATA_LIMIT as longint = 1000000
dim shared as longint data_groups, data_items, data_descriptors, data_reads, data_restores

sub fbSemanticModelResetData( )
	data_groups = 0
	data_items = 0
	data_descriptors = 0
	data_reads = 0
	data_restores = 0
end sub

private function hNext( byref count as longint ) as longint
	if( count >= SEMANTIC_DATA_LIMIT ) then
		fbSemanticModelFailAt("DATA observation limit exceeded")
		return 0
	end if
	count += 1
	return count
end function

private sub hProperty( byref key as const string, byref value as const string )
	dim as longint owner = fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))
	if( owner = 0 ) then
		fbSemanticModelFailAt("DATA observation namespace is unavailable")
		exit sub
	end if
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + fbSemanticModelNumber(owner) + _
		TABCHAR + key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hSource( ) as string
	dim as longint statement = fbSemanticModelCurrentStatement( )
	dim as longint source_id = fbSemanticModelCurrentSource( )
	if( (statement = 0) or (source_id = 0) ) then
		fbSemanticModelFailAt("source DATA observation has no parser context")
	end if
	return fbSemanticModelNumber(statement) + TABCHAR + fbSemanticModelNumber(source_id)
end function

private sub hRange( byref role as const string, byref first as LEX_LOCATION, _
	byref last as LEX_LOCATION, byval first_nonphysical as longint, byval last_nonphysical as longint )
	'' Interior expansions cannot become an editable contiguous physical range.
	dim as LEX_LOCATION observed_first = first, observed_last = last
	if( first_nonphysical <> last_nonphysical ) then observed_first.is_physical = FALSE
	fbSemanticModelExportCoordinates("source-context", fbSemanticModelCurrentSource( ), role, observed_first, observed_last)
	dim as longint expansion = first.macro_identity
	if( expansion = 0 ) then expansion = last.macro_identity
	fbSemanticModelMacroOrigin("symbol", fbSemanticModelSymbolId(@symbGetGlobalNamespc( )), expansion, role)
end sub

function fbSemanticModelDataBegin( ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	return hNext(data_groups)
end function

sub fbSemanticModelDataItem _
	( byval group_id as longint, byval ordinal as integer, byval item as ASTNODE ptr, _
	  byval raw_text as integer, byref first as LEX_LOCATION, byref last as LEX_LOCATION, _
	  byval first_nonphysical as longint, byval last_nonphysical as longint )
	if( group_id = 0 ) then exit sub
	if( (group_id <> data_groups) or (ordinal < 0) or (item = NULL) ) then
		fbSemanticModelFailAt("invalid source DATA item")
		exit sub
	end if
	dim as ASTNODE ptr expr = item->l
	if( (expr = NULL) or (hNext(data_items) = 0) ) then
		fbSemanticModelFailAt("source DATA item is unavailable")
		exit sub
	end if
	dim as string kind, value_kind, value_text
	dim as longint expression_id = 0, address_id = 0, address_offset = 0
	select case item->data.id
	case FB_DATASTMT_ID_ZSTR, FB_DATASTMT_ID_WSTR
		kind = iif(raw_text, "qb-raw", iif(item->data.id = FB_DATASTMT_ID_ZSTR, "string", "wide-string"))
		dim as FBVALUE literal_value
		literal_value.s = astGetStrLitSymbol(expr)
		fbSemanticModelFormatValue(expr->dtype, @literal_value, value_kind, value_text)
	case FB_DATASTMT_ID_CONST
		kind = "number"
		if( astIsCONST(expr) = FALSE ) then
			fbSemanticModelFailAt("numeric DATA item is not a constant")
			exit sub
		end if
		fbSemanticModelFormatValue(expr->dtype, @expr->val.value, value_kind, value_text)
	case FB_DATASTMT_ID_OFFSET
		kind = "address"
		address_id = fbSemanticModelSymbolId(expr->sym)
		address_offset = expr->ofs.ofs
	case else
		fbSemanticModelFailAt("unknown source DATA item kind")
		exit sub
	end select
	'' QB reads uninterpreted tokens. Exporting an E there would incorrectly
	'' claim the compiler evaluated an expression or expanded a macro.
	if( raw_text = FALSE ) then
		fbSemanticModelExportExpressionDistinct(expr, first, last, first_nonphysical, last_nonphysical)
		expression_id = expr->semantic_expression
		if( expression_id = 0 ) then
			fbSemanticModelFailAt("source DATA expression snapshot is unavailable")
			exit sub
		end if
	end if
	dim as string role = "data-item:" + fbSemanticModelNumber(group_id) + ":" + fbSemanticModelNumber(ordinal)
	hProperty(role, kind + TABCHAR + fbSemanticModelTypeNumber(culng(expr->dtype)) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(expr->subtype)) + TABCHAR + _
		fbSemanticModelNumber(expression_id) + TABCHAR + value_kind + TABCHAR + fbSemanticModelEscape(value_text) + _
		TABCHAR + fbSemanticModelNumber(address_id) + TABCHAR + fbSemanticModelNumber(address_offset))
	'' Empty QB fields have a value but no written token extent.
	if( (first.start_line < last.end_line) or _
	    ((first.start_line = last.end_line) and (first.start_column < last.end_column)) ) then
		hRange(role, first, last, first_nonphysical, last_nonphysical)
	end if
end sub

sub fbSemanticModelDataDescriptor _
	( byval group_id as longint, byval array as FBSYMBOL ptr, byval previous as FBSYMBOL ptr, _
	  byval label as FBSYMBOL ptr, byval cells as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (array = NULL) or (cells < 1) or (hNext(data_descriptors) = 0) ) then
		fbSemanticModelFailAt("invalid lowered DATA descriptor")
		exit sub
	end if
	dim as longint array_id = fbSemanticModelSymbolId(array)
	dim as longint previous_id = fbSemanticModelSymbolId(previous)
	dim as longint label_id = fbSemanticModelSymbolId(label)
	hProperty("data-descriptor:" + fbSemanticModelNumber(array_id), fbSemanticModelNumber(previous_id) + _
		TABCHAR + fbSemanticModelNumber(label_id) + TABCHAR + fbSemanticModelNumber(group_id) + _
		TABCHAR + fbSemanticModelNumber(cells))
	if( previous <> NULL ) then fbSemanticModelExportRelation("symbol", previous_id, array, "data-next")
	if( group_id <> 0 ) then
		hProperty("data-group:" + fbSemanticModelNumber(group_id), hSource( ) + TABCHAR + _
			fbSemanticModelNumber(array_id) + TABCHAR + fbSemanticModelNumber(cells - 1))
	end if
end sub

sub fbSemanticModelDataPayload _
	( byval group_id as longint, byval ordinal as integer, byval tag as integer, _
	  byref numeric_text as const string )
	if( group_id = 0 ) then exit sub
	'' The runtime descriptor stores this field as an unsigned 16-bit tag.
	'' Keep the actual lowered tag, including native string length wrapping.
	hProperty("data-payload:" + fbSemanticModelNumber(group_id) + ":" + fbSemanticModelNumber(ordinal), _
		fbSemanticModelNumber(cushort(tag)) + TABCHAR + fbSemanticModelEscape(numeric_text))
end sub

function fbSemanticModelDataRead _
	( byval expr as ASTNODE ptr, byval ordinal as integer, byref first as LEX_LOCATION, _
	  byref last as LEX_LOCATION, byval first_nonphysical as longint, _
	  byval last_nonphysical as longint ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	if( (expr = NULL) or (ordinal < 0) ) then
		fbSemanticModelFailAt("invalid source READ destination")
		return 0
	end if
	dim as longint identity = hNext(data_reads)
	if( identity = 0 ) then return 0
	fbSemanticModelExportExpressionDistinct(expr, first, last, first_nonphysical, last_nonphysical)
	if( expr->semantic_expression = 0 ) then
		fbSemanticModelFailAt("source READ expression snapshot is unavailable")
		return 0
	end if
	dim as string role = "data-read:" + fbSemanticModelNumber(identity)
	hProperty(role, hSource( ) + TABCHAR + fbSemanticModelNumber(ordinal) + TABCHAR + _
		fbSemanticModelNumber(expr->semantic_expression))
	hRange(role, first, last, first_nonphysical, last_nonphysical)
	return identity
end function

sub fbSemanticModelDataReadRuntime( byval identity as longint, byval proc as FBSYMBOL ptr )
	if( identity = 0 ) then exit sub
	if( (identity <> data_reads) or (proc = NULL) ) then
		fbSemanticModelFailAt("source READ has no selected runtime procedure")
		exit sub
	end if
	dim as string role = "data-read-runtime:" + fbSemanticModelNumber(identity)
	hProperty(role, fbSemanticModelNumber(fbSemanticModelSymbolId(proc)))
	fbSemanticModelExportRelation("symbol", fbSemanticModelSymbolId(@symbGetGlobalNamespc( )), proc, role)
end sub

sub fbSemanticModelDataRestore _
	( byval statement as longint, byval label as FBSYMBOL ptr, byval array as FBSYMBOL ptr )
	'' Initialization calls also use rtlDataRestore. Only the parser supplies
	'' a statement, so generated cursor initialization is never a source RESTORE.
	if( (statement = 0) or (fbSemanticModelFullEnabled( ) = FALSE) ) then exit sub
	dim as longint identity = hNext(data_restores)
	if( identity = 0 ) then exit sub
	if( (statement <> fbSemanticModelCurrentStatement( )) or (array = NULL) ) then
		fbSemanticModelFailAt("invalid source RESTORE selection")
		exit sub
	end if
	hProperty("data-restore:" + fbSemanticModelNumber(identity), hSource( ) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(label)) + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(array)))
end sub

sub fbSemanticModelFinishData( )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	hProperty("data-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
		fbSemanticModelNumber(data_groups) + TABCHAR + fbSemanticModelNumber(data_items) + TABCHAR + _
		fbSemanticModelNumber(data_descriptors) + TABCHAR + fbSemanticModelNumber(data_reads) + TABCHAR + _
		fbSemanticModelNumber(data_restores))
end sub

'' end of tooling/semantic-data.bas
