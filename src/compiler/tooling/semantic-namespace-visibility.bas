'' Project: FreeBASIC compiler - semantic namespace visibility
'' File: tooling/semantic-namespace-visibility.bas
'' Purpose: Retain source import order and native relationship lifetime.
'' Responsibilities: Copy live identities, distinguish lookup and lexical owners, close observations.
'' This file intentionally does NOT contain: independent resolution or inferred global visibility.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-namespace-visibility.bi"
#include once "tooling/semantic-constructs.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "tooling/semantic-macros.bi"
#include once "support/containers/hash.bi"

'' Module compilation is serial. The hash borrows immutable keys owned by
'' these entries; entries retain semantic IDs only, never native symbol pointers.
'' End the hash before releasing its keys, including on abandoned transactions.
private type SEMANTIC_NAMESPACE_IMPORT
	key as string
	identity as longint
	removed as integer
	next_import as SEMANTIC_NAMESPACE_IMPORT ptr
end type

'' Match the reader's source-event bound. The staging budget also bounds
'' total output. Four thousand buckets keep ordinary import sets inexpensive.
private const NAMESPACE_VISIBILITY_LIMIT = 1000000
private const NAMESPACE_VISIBILITY_BUCKETS = 4096
private dim shared as THASH namespace_imports
private dim shared as integer namespace_imports_ready
private dim shared as SEMANTIC_NAMESPACE_IMPORT ptr namespace_import_head, namespace_import_tail
private dim shared as longint namespace_using_count, namespace_import_count, namespace_remove_count, namespace_refresh_count
private dim shared as longint namespace_active_using, namespace_using_import_start, namespace_active_refresh
private dim shared as longint namespace_refresh_import_start
private dim shared as longint namespace_binding_count

sub fbSemanticModelResetNamespaceVisibility( )
	if( namespace_imports_ready ) then hashEnd(@namespace_imports)
	namespace_imports_ready = FALSE
	dim as SEMANTIC_NAMESPACE_IMPORT ptr item = namespace_import_head
	while( item <> NULL )
		dim as SEMANTIC_NAMESPACE_IMPORT ptr following = item->next_import
		delete item
		item = following
	wend
	namespace_import_head = NULL
	namespace_import_tail = NULL
	namespace_using_count = 0
	namespace_import_count = 0
	namespace_remove_count = 0
	namespace_refresh_count = 0
	namespace_active_using = 0
	namespace_using_import_start = 0
	namespace_active_refresh = 0
	namespace_refresh_import_start = 0
	namespace_binding_count = 0
end sub

private function hNext( byref count as longint ) as longint
	if( count >= NAMESPACE_VISIBILITY_LIMIT ) then
		fbSemanticModelFailAt("namespace visibility observation limit exceeded")
		return 0
	end if
	count += 1
	return count
end function

private sub hProperty( byref key as const string, byref value as const string )
	fbSemanticModelAppendDetail("K" + TABCHAR + "symbol" + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(@symbGetGlobalNamespc( ))) + TABCHAR + _
		key + TABCHAR + fbSemanticModelEscape(value))
end sub

private function hObservationContext( ) as string
	return fbSemanticModelNumber(fbSemanticModelCurrentStatement( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentSource( )) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelCurrentCompound( ))
end function

function fbSemanticModelUsingBegin _
	( byval ns as FBSYMBOL ptr, byref first as LEX_LOCATION, byref last as LEX_LOCATION, _
	  byval first_nonphysical as longint, byval last_nonphysical as longint, _
	  byval first_binding as longint, byval last_binding as longint ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	if( (namespace_active_using <> 0) or (namespace_active_refresh <> 0) or (ns = NULL) ) then
		fbSemanticModelFailAt("overlapping or missing USING observation")
		return 0
	end if
	dim as FBSYMBOLTB ptr table = symbGetCurrentSymTb( )
	if( (table = NULL) or (fbSemanticModelCurrentStatement( ) = 0) or _
	    (fbSemanticModelCurrentSource( ) = 0) or (ns->class <> FB_SYMBCLASS_NAMESPACE) ) then
		fbSemanticModelFailAt("USING observation has no accepted parser context")
		return 0
	end if
	dim as longint identity = hNext(namespace_using_count)
	if( identity = 0 ) then return 0
	namespace_active_using = identity
	namespace_using_import_start = namespace_import_count
	if( first_binding > last_binding ) then
		first_binding = 0
		last_binding = 0
	end if
	dim as string role = "namespace-using:" + fbSemanticModelNumber(identity)
	dim as string spelling = "written"
	if( (first_nonphysical <> last_nonphysical) or (first.is_physical = FALSE) or (last.is_physical = FALSE) ) then spelling = "expanded"
	hProperty(role, hObservationContext( ) + TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(table->owner)) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(symbGetCurrentNamespc( ))) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelSymbolId(ns)) + TABCHAR + _
		fbSemanticModelNumber(first_binding) + TABCHAR + fbSemanticModelNumber(last_binding) + TABCHAR + spelling)
	dim as LEX_LOCATION observed_first = first
	if( spelling = "expanded" ) then observed_first.is_physical = FALSE
	fbSemanticModelExportCoordinates("source-context", fbSemanticModelCurrentSource( ), role, observed_first, last)
	return identity
end function

sub fbSemanticModelUsingEnd( byval identity as longint, byval native_result as integer )
	if( identity = 0 ) then exit sub
	if( identity <> namespace_active_using ) then
		fbSemanticModelFailAt("USING observation closes another request")
		exit sub
	end if
	hProperty("namespace-using-result:" + fbSemanticModelNumber(identity), _
		fbSemanticModelNumber(abs(native_result <> FALSE)) + TABCHAR + _
		fbSemanticModelNumber(namespace_import_count - namespace_using_import_start))
	namespace_active_using = 0
end sub

private function hImport( byval imported as FBSYMBOL ptr, byref origin as const string ) as SEMANTIC_NAMESPACE_IMPORT ptr
	if( (imported = NULL) orElse (imported->class <> FB_SYMBCLASS_NSIMPORT) ) then
		fbSemanticModelFailAt("invalid native namespace import")
		return NULL
	end if
	dim as string key = fbSemanticModelNumber(fbSemanticModelSymbolId(imported))
	if( namespace_imports_ready ) then
		dim as SEMANTIC_NAMESPACE_IMPORT ptr existing = hashLookup(@namespace_imports, strptr(key))
		if( existing <> NULL ) then return existing
	end if
	dim as longint identity = hNext(namespace_import_count)
	if( identity = 0 ) then return NULL
	dim as SEMANTIC_NAMESPACE_IMPORT ptr item = new SEMANTIC_NAMESPACE_IMPORT
	if( item = NULL ) then
		fbSemanticModelFailAt("namespace import observation allocation failed")
		return NULL
	end if
	item->key = key
	item->identity = identity
	item->removed = FALSE
	item->next_import = NULL
	if( namespace_import_tail = NULL ) then
		namespace_import_head = item
	else
		namespace_import_tail->next_import = item
	end if
	namespace_import_tail = item
	if( namespace_imports_ready = FALSE ) then
		hashInit(@namespace_imports, NAMESPACE_VISIBILITY_BUCKETS, FALSE)
		namespace_imports_ready = TRUE
	end if
	if( hashAdd(@namespace_imports, strptr(item->key), item, hashHash(strptr(item->key))) = NULL ) then
		fbSemanticModelFailAt("namespace import observation index allocation failed")
		return NULL
	end if
	dim as longint owner = 0
	if( imported->symtb <> NULL ) then owner = fbSemanticModelSymbolId(imported->symtb->owner)
	'' The import's lexical table owns removal. exp_ns instead owns the native
	'' namespace import list; procedure/block USING can therefore have distinct
	'' lexical and lookup owners. Neither one alone implies global visibility.
	fbSemanticModelExportSymbolDetails(imported)
	hProperty("namespace-import:" + fbSemanticModelNumber(identity), item->key + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(imported->nsimp.imp_ns)) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(imported->nsimp.exp_ns)) + TABCHAR + _
		fbSemanticModelNumber(owner) + TABCHAR + fbSemanticModelNumber(namespace_active_using) + TABCHAR + _
		fbSemanticModelNumber(namespace_active_refresh) + TABCHAR + hObservationContext( ) + TABCHAR + origin)
	return item
end function

sub fbSemanticModelNamespaceImportAdded( byval imported as FBSYMBOL ptr )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	hImport(imported, "created")
end sub

sub fbSemanticModelNamespaceImportRemoved( byval imported as FBSYMBOL ptr, byval hashonly as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	'' Some compiler-initialized relationships can predate module capture.
	'' Snapshot those before removal without inventing an earlier creation.
	dim as SEMANTIC_NAMESPACE_IMPORT ptr item = hImport(imported, "preexisting")
	if( item = NULL ) then exit sub
	if( item->removed ) then
		fbSemanticModelFailAt("namespace import removed twice")
		exit sub
	end if
	item->removed = TRUE
	namespace_remove_count += 1
	hProperty("namespace-import-remove:" + fbSemanticModelNumber(item->identity), _
		hObservationContext( ) + TABCHAR + fbSemanticModelNumber(abs(hashonly <> FALSE)))
end sub

sub fbSemanticModelNamespaceRefresh( byval ns as FBSYMBOL ptr, byval entering as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( entering ) then
		if( (namespace_active_refresh <> 0) or (namespace_active_using <> 0) or (ns = NULL) ) then
			fbSemanticModelFailAt("overlapping namespace refresh observation")
			exit sub
		end if
		namespace_active_refresh = hNext(namespace_refresh_count)
		if( namespace_active_refresh = 0 ) then exit sub
		namespace_refresh_import_start = namespace_import_count
		hProperty("namespace-refresh:" + fbSemanticModelNumber(namespace_active_refresh), _
			fbSemanticModelNumber(fbSemanticModelSymbolId(ns)) + TABCHAR + hObservationContext( ))
	else
		if( namespace_active_refresh = 0 ) then
			fbSemanticModelFailAt("namespace refresh has no opening")
			exit sub
		end if
		hProperty("namespace-refresh-result:" + fbSemanticModelNumber(namespace_active_refresh), _
			fbSemanticModelNumber(namespace_import_count - namespace_refresh_import_start))
		namespace_active_refresh = 0
	end if
end sub

sub fbSemanticModelBindingScopeContext( byval binding as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( (binding <= 0) or (hNext(namespace_binding_count) = 0) ) then
		fbSemanticModelFailAt("binding scope context has no source occurrence")
		exit sub
	end if
	dim as FBSYMBOLTB ptr table = symbGetCurrentSymTb( )
	dim as FBHASHTB ptr hash_table = symbGetCurrentHashTb( )
	dim as longint lexical = 0, lookup_hash = 0
	if( table <> NULL ) then lexical = fbSemanticModelSymbolId(table->owner)
	if( hash_table <> NULL ) then lookup_hash = fbSemanticModelSymbolId(hash_table->owner)
	'' Capture the parser's live context, not the selected symbol's owner.
	'' Qualified lookup and WITH/member receivers remain separate facts. In
	'' particular, a procedure table does not become a lookup namespace.
	hProperty("namespace-binding:" + fbSemanticModelNumber(binding), hObservationContext( ) + TABCHAR + _
		fbSemanticModelNumber(lexical) + TABCHAR + _
		fbSemanticModelNumber(fbSemanticModelSymbolId(symbGetCurrentNamespc( ))) + TABCHAR + _
		fbSemanticModelNumber(lookup_hash))
end sub

sub fbSemanticModelFinishNamespaceVisibility( byval commit as integer )
	if( commit and fbSemanticModelFullEnabled( ) ) then
		if( (namespace_active_using <> 0) or (namespace_active_refresh <> 0) ) then
			fbSemanticModelFailAt("namespace visibility observation remains open")
		end if
		dim as SEMANTIC_NAMESPACE_IMPORT ptr item = namespace_import_head
		while( item <> NULL )
			hProperty("namespace-import-state:" + fbSemanticModelNumber(item->identity), iif(item->removed, "removed", "retained"))
			item = item->next_import
		wend
		hProperty("namespace-visibility-counts:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(namespace_using_count) + TABCHAR + fbSemanticModelNumber(namespace_import_count) + _
			TABCHAR + fbSemanticModelNumber(namespace_remove_count) + TABCHAR + fbSemanticModelNumber(namespace_refresh_count))
		hProperty("namespace-binding-count:" + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )), _
			fbSemanticModelNumber(namespace_binding_count))
	end if
	fbSemanticModelResetNamespaceVisibility( )
end sub

'' end of tooling/semantic-namespace-visibility.bas
