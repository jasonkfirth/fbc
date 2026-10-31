'' Project: FreeBASIC compiler - C source backend
'' -----------------------------------------
''
'' File: backend/c/ir-hlc.bas
''
'' Purpose:
''
''     Emit C types, declarations, procedures, and operations from typed IR.
''
'' Responsibilities:
''
''     - serialize typed IR, data layouts, and control flow as C
''     - apply C ABI and compiler interoperability requirements
''
'' This file intentionally does NOT contain:
''
''     - BASIC grammar or execution of the external C compiler
''

''
'' "high level" IR interface for emitting C code
''
'' The C backend is called "high level" in comparison to the ASM backend, but
'' actually produces pretty low-level, ABI-dependant C code, using gcc as the
'' "assembler". It works with mostly the same low-level operations that would
'' be sent to the ASM backend (for example: labels and jumps instead of if/else
'' blocks).
''
'' - Some math operations are transparently implemented using gcc __builtin_*()
''   functions. For others, we let the AST know that we can't handle them here,
''   and it will call RTL functions instead.
''
'' - Float to int conversions need special treatment to achieve FB's rounding
''   behaviour. Simple C casting as in '(int)floatvar' cannot be used because it
''   just truncates instead of rounding to nearest. Thus we use 4 helper
''   routines (float|double -> int32|int64) that are implemented in x86 ASM
''   (as done by the ASM backend) or using __builtin_nearbyint[f]().
''
'' - Field accesses, pointer indexing, struct layout/field alignment, etc. is
''   all still calculated on the FB side, i.e. the generated C code is
''   ABI-dependant. sizeof()/offsetof() are evaluated purely on the FB side,
''   it's impossible to pass all constant expressions on to the C backend.
''   (constants, array bounds, fixstr lengths, multi-dim indexing, ...)
''
'' - va_* vararg macros can't be supported with the C backend, they are too
''   different from C's va_*() macros.
''   1. For example, FB's va_first/va_arg can be called repeatedly, that's not
''      possible with C's va_start/va_arg. Acccessing the current arg and
''      advancing to the next is two separate functions in FB, but combined in
''      one in C. It's impossible to reliably & automatically translate from one
''      to the other.
''   2. It's not possible to implement va_first() as "address-of last named
''      parameter" as done for the ASM backend, because gcc sometimes puts
''      parameters into temp vars, and then addrof on that parameter returns the
''      temp var, not the parameter.
''   3. On x86 va_list is just a pointer (exactly what's needed for va_first())
''      but for x86_64 it's not that easy. Varargs may be passed in registers.
''   4. It'd be "nice" to be able to read out all var args into a buffer and
''      allow that to be accessed through FB's va_* macros, but that's not
''      possible because there's no way to know how many varargs there are.
''
'' - Calling conventions/name mangling:
''
''    * We sometimes make use of gcc's asm("nameToUseInAsm") feature which is
''      similar to ALIAS.
''
''    * Cdecl and Stdcall (stdcall with @N) are easily emitted for GCC on
''      individual functions.
''
''    * Special case for stdcall + functions returning UDTs, where we already
''      had to "lower the AST on the FB-side" to use a hidden pointer parameter
''      and pointer result, instead of plain UDT result (see hGetReturnType()):
''
''      gcc doesn't know that the "hidden" result parameter is special and thus
''      calculates it into the @N suffix. We need to use an asm() alias to avoid
''      that.
''
''    * StdcallMs (stdcall without @N) is not directly supported by gcc, only at
''      the linker level through ld --kill-at etc. We need it for individual
''      functions though, not the entire executable/DLL. We can achieve that by
''      using asm() aliases.
''
''      Because gcc inserts these asm() names into DLL export tables as-is,
''      without stripping the underscore prefix, we must emit the exports
''      manually using inline ASM instead of __attribute__((dllexport)) to get
''      them to work correctly.
''
''    * Pascal is like StdcallMs except that arguments are pushed left-to-right
''      (same order as written in code, not reversed like Cdecl/Stdcall). The
''      symbGetProc*Param() macros take care of changing the order when cycling
''      through parameters of Pascal functions, and by together with such
''      functions being emitted as Stdcall this results in a double-reverse
''      resulting in the proper ABI.
''
''    * For non-x86, there's no need to emit cdecl/stdcall/... at all because
''      they don't exist (on x86_64 or ARM etc.) and gcc ignores the attributes.
''
'' - "boolean" is implemented as int8, instead of C99's _Bool type (even though
''   FB's boolean is supposed to be compatible with C's _Bool), because that way
''   we can avoid undefined behaviour if values other than 0/1 are stored in the
''   boolean, and get the same behaviour as with the ASM backend. Of course that
''   also means we have to add code for converting int/float to 0/1 when casting
''   them to boolean.
''
'' - Avoiding C's undefined behaviour where possible. This is important because
''   otherwise we can't rely on gcc to generate meaningful code when
''   "optimizations" are enabled. As a (hopefully nice-to-have) side-effect,
''   some of the FB features/behaviour that originates from the x86 ASM backend
''   are guaranteed to work just the same with the C backend, no matter what
''   architecture.
''    - using gcc's -fwrapv to get well-defined signed integer overflow
''    - emitting the extra AND mask operation on the rhs of SHL/SHR, to ensure
''      that the shift amount is in the range 0..sizeof(lhs)*8-1.
''      (astNewBOP() already handles this for constant shift amounts)
''    - relying on gcc to provide well-defined signed shl on negative integers
''      (https://gcc.gnu.org/onlinedocs/gcc/Integers-implementation.html)
''

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "tooling/semantic-link.bi"
#include once "tooling/semantic-emission-names.bi"
#include once "tooling/semantic-backend-abi.bi"
#include once "tooling/semantic-call-abi.bi"
#include once "tooling/semantic-startup.bi"
#include once "backend/ir.bi"
#include once "runtime/rtl.bi"
#include once "support/containers/flist.bi"
#include once "lexer/lex.bi"
#include once "parser/parser.bi"
#include once "backend/ir-private.bi"
#include once "crt/string.bi"
#include once "backend/c/ir-hlc-platform.bi"

'' The stack of nested sections allows us to go back and emit text to
'' the headers of parent sections, while already working on emitting
'' something else in an inner section.
'' (most commonly used for UDT declarations, which are only emitted
''  when they're needed by something else that's being emitted)
''
'' index 0 is the "toplevel" section,
'' index 1 is the "body" where procedures are emitted into,
'' the rest is used for nested procedure/scope blocks.
''
'' "body" is separate from "toplevel" to allow adding declarations to
'' "toplevel", while the procedures are appended to "body", one after
'' another. Then, once all procedures are emitted, "body" is closed,
'' and is appended to "toplevel". At that point we're done emitting
'' anyways and don't need to add stuff to toplevel's header anymore.
''
'' Procedures and scopes also use a header/body pair. Pure storage declarations
'' belong before statements; runtime initialization and calls stay in their original body
'' position. This preserves evaluation order and lexical variable lifetimes.
'' For the toplevel emitting all at once makes sense because
'' it is more efficient to check the symbol tables for called procedures only
'' once during _emitEnd() instead of once during every _emitProcBegin().
'' Note that _emitBegin() is called before parsing has even started,
'' so the global declarations can't be emitted from there already.

const MAX_SECTIONS = FB_MAXSCOPEDEPTH * 2 + 1
const SECTION_BUFFER_INITIAL_CAPACITY = 8192
const SECTION_TAB_BUFFER_LENGTH = 64

type SECTIONENTRY
	buffer          as ubyte ptr
	bufferlen       as uinteger
	buffercapacity  as uinteger
	old             as integer '' old junk text (that is only kept around to keep the buffer allocated)?
	indent          as integer '' current indendation level to be used when emitting lines into this section
end type

enum
	EXPRCLASS_TEXT = 0
	EXPRCLASS_IMM
	EXPRCLASS_SYM
	EXPRCLASS_CAST
	EXPRCLASS_UOP
	EXPRCLASS_BOP
	EXPRCLASS_MACRO
	EXPRCLASS_MEMBER
	EXPRCLASS_INDEX
	EXPRCLASS_ARRAYADDR
	EXPRCLASS_VTABLE
end enum

const C_MEMBER_VIEW_VALUE = 1
const C_MEMBER_VIEW_STORAGE = 2
const C_MEMBER_VIEW_WRITE = 3

type EXPRNODE
	class       as integer  '' EXPRCLASS_*

	'' This expression's type, to determine whether CASTs are needed or not
	dtype       as integer
	subtype     as FBSYMBOL ptr

	l           as EXPRNODE ptr  '' CAST/UOP/BOP
	r           as EXPRNODE ptr  '' BOP
	member_view as integer      '' MEMBER uses an alignment-one record view; an empty path dereferences it.

	union
		text    as zstring ptr  '' TEXT
		val     as FBVALUE      '' IMM
		sym     as FBSYMBOL ptr '' SYM
		op      as integer      '' UOP/BOP
	end union
end type

type EXPRCACHENODE
	'' Each cache entry associates an expression tree with a vreg id,
	'' allowing expressions to be looked up for certain vreg accesses,
	'' instead of having to be emitted as #defines or temp vars.
	''
	'' Having a separate list for the cache is faster than cycling through
	'' the whole ctx.exprnodes list. Often there will be only 1 (UOPs) or
	'' 2 (BOPs) expression trees cached, since the AST usually accesses
	'' expression results right when emitting the next expression/statement.
	vregid      as integer
	expr        as EXPRNODE ptr
end type

type CALIGNVIEW
	dtype      as integer
	subtype    as FBSYMBOL ptr
	section    as integer
	emitted    as integer
end type

'' Owned C spellings outlive the local symbols from which they were emitted.
type CPOINTERVIEW
	original   as DZSTRING
	aliasname  as DZSTRING
	section    as integer
end type

type CGFXBLENDER
	function_type as DZSTRING
	parameter_type as DZSTRING
	identifier as uinteger
end type

type CARRAYCALLBACK
	target_name as DZSTRING
	target_type as DZSTRING
	identifier  as uinteger
end type

'' Initializer nodes borrow their symbols. Only the stack allocation belongs
'' to the emitter; closing a child aggregate counts as one parent element.
type CVARINISCOPE
	previous as CVARINISCOPE ptr
	sym as FBSYMBOL ptr
	udt as FBSYMBOL ptr
	is_array as integer
	emitted_elements as longint
end type

enum
	BUILTIN_F2I           = (1 shl 0)
	BUILTIN_F2L           = (1 shl 1)
	BUILTIN_F2UL          = (1 shl 2)
	BUILTIN_D2I           = (1 shl 3)
	BUILTIN_D2L           = (1 shl 4)
	BUILTIN_D2UL          = (1 shl 5)
	BUILTIN_STATICASSERT  = (1 shl 6)
	BUILTIN_F2I128        = (1 shl 7)
	BUILTIN_F2U128        = (1 shl 8)
	BUILTIN_D2I128        = (1 shl 9)
	BUILTIN_D2U128        = (1 shl 10)
	BUILTIN_F32_EQUAL     = (1 shl 11)
	BUILTIN_F64_EQUAL     = (1 shl 12)
end enum

type IRHLCCTX
	sections(0 to MAX_SECTIONS-1)   as SECTIONENTRY
	section                 as integer '' Current section to write to
	sectiongosublevel       as integer

	linenum                 as integer
	escapedinputfilename    as string
	usedbuiltins            as uinteger  '' BUILTIN_*

	anonstack               as TLIST  '' stack of nested anonymous structs/unions in a struct/union

	varini                  as string
	vtablemembers           as string  '' Typed C fields for the active compiler-owned vtable.
	vtableelements          as integer
	varinisym               as FBSYMBOL ptr '' Non-NULL while emitting a static initializer.
	variniscopelevel        as integer
	variniscopes            as CVARINISCOPE ptr
	varinifailed            as integer

	fbctinf                 as string
	exports                 as string

	exprnodes               as TLIST   '' EXPRNODE
	exprtext                as string  '' buffer used by exprFlush() to build the final text
	exprcache               as TLIST   '' EXPRCACHENODE
	alignmentviews          as TLIST   '' CALIGNVIEW, owned until backend shutdown.
	arraycallbacks          as TLIST   '' CARRAYCALLBACK, owns C names for one emission unit.
	arraycallbackid         as uinteger
	gfxblenders             as TLIST   '' CGFXBLENDER, owns adapter type spellings per C output.
	gfxblenderid            as uinteger
	gfxcontextid            as uinteger
	unalignedid             as uinteger '' Per-emission byte-copy temporary names, reset by _emitBegin().
	localassertid           as uinteger '' Unique local enum names for pre-C11 layout checks.
	codeptrid               as uinteger '' Typed representation-transfer temporaries.

	globalvarpass           as integer  '' Global var emitting is done in 2 passes; this allows the callbacks to identify the current pass.
	computedgotoaddress     as integer  '' A real label address or required entry reference has been emitted.
	computedgotoentry       as integer  '' Only Clang needs an otherwise unreferenced entry address.
	computedgotodispatch    as integer  '' One target slot and dispatch block per C activation.
	computedgotoopaque      as integer  '' Raw text may expose labels without a symbol expression.
	currproc                as FBSYMBOL ptr  '' Borrowed procedure symbol while emitting its body.
	procentrylabel          as FBSYMBOL ptr  '' Borrowed from the procedure AST while emitting its body.
	procentrysection        as integer
	procentryoffset         as uinteger
	procentrybytes          as uinteger
end type

declare function hEmitType _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval emit_const as integer = FALSE _
	) as string

declare sub hEmitStruct( byval s as FBSYMBOL ptr, byval is_ptr as integer )

declare sub _emitDBG _
	( _
		byval op as integer, _
		byval proc as FBSYMBOL ptr, _
		byval lnum as integer, _
		ByVal filename As zstring ptr = 0 _
	)

declare function hCUnalignedTempName() as string
declare function exprTryCRecordArrayAddress(byval dtype as integer, byval subtype as FBSYMBOL ptr, byval value as EXPRNODE ptr) as EXPRNODE ptr
declare function hIsScalarLoadType(byval dtype as integer) as integer

declare sub hCRememberViewType( byval dtype as integer, byval subtype as FBSYMBOL ptr )
declare sub hCClearPointerViews( byval firstsection as integer )
declare function hCPointerViewSection( byval dtype as integer, byval subtype as FBSYMBOL ptr ) as integer
declare function hCPointerViewName( byref original as string, byval declsection as integer ) as string

declare sub exprFreeNode( byval n as EXPRNODE ptr )
declare function hIsArrayCallbackParam( byval proc as FBSYMBOL ptr, byval param_index as integer ) as integer
declare sub hEmitArrayCallbackDefinitions( )
declare sub hClearArrayCallbacks( )
declare function hGfxBlenderType( byval proc as FBSYMBOL ptr, byval param_index as integer ) as FBSYMBOL ptr
declare sub hEmitGfxBlenderDefinitions( )
declare sub hClearGfxBlenders( )
declare sub hCReadOnlyFinish()
declare sub hCReadOnlyLocalsFinish()
declare sub hCUnobservedFinish()
declare sub hCLiteralStorageInit()
declare sub hCLiteralStorageFinish()
declare sub hCLiteralStoragePrepare(byval buffer as ubyte ptr, byval bytes as uinteger)
#if __FB_DEBUG__
declare sub exprDump( byval n as EXPRNODE ptr )
#endif

'' globals
dim shared as IRHLCCTX ctx


'' Coverage of the current procedure AST for local storage declarations.
'' Reset at procedure entry; debug, opaque assembly and naked procedures keep locals.
dim shared as integer c_local_refs_complete
'' Each procedure owns a pointer-keyed reference index, separate from the parser's
'' historical ACCESSED flags. Keys are owned; local symbols are only borrowed.
dim shared as THASH c_local_refs
'' Minimum-alignment pointee typedefs own their spellings for one C output.
'' Record definitions keep their natural fields, including recursive pointers.
dim shared as TLIST c_pointer_views
dim shared as integer c_pointer_view_id, c_pointer_raw_types
#include once "backend/c/asm-names.bi"
#include once "backend/c/c-identifiers.bi"
#include once "backend/c/c-generated-leaves.bi"
#include once "backend/c/c-abi-parameters.bi"
#include once "backend/c/c-auto-descriptors.bi"
declare function hCStaticArrayBacking(byval sym as FBSYMBOL ptr) as FBSYMBOL ptr
declare function hCStaticScalarCandidate(byval sym as FBSYMBOL ptr) as integer
declare sub hCStaticArrayInitializers(byval proc as FBSYMBOL ptr)
#include once "backend/c/c-unused-scalar-stores.bi"
#include once "backend/c/c-integer-ranges.bi"
#include once "backend/c/c-address-comparisons.bi"
#include once "backend/c/c-comparison-folding.bi"

declare sub hBuildStrLit( byref ln as string, byval z as zstring ptr, _
	byval length as longint, byval paddedlength as longint )

private sub hAppendCObjectAlias( byref ln as string, byref object_name as string )
	ln += " __asm__("
	hBuildStrLit( ln, strptr(object_name), len(object_name) + 1, 0 )
	ln += ")"
end sub


'' Same order as FB_DATATYPE. The C frontends provide native 128-bit typedefs
'' without system headers. Their signedness, layout and calling convention are
'' the native ABI; hEmitType selects the two-word representation on 32-bit CPUs.
dim shared as const zstring ptr dtypeName(0 to FB_DATATYPES-1) = _
{ _
	@"void"     , _ '' void
	@"boolean"  , _ '' boolean
	@"int8"     , _ '' byte
	@"uint8"    , _ '' ubyte
	@"char"     , _ '' char
	@"int16"    , _ '' short
	@"uint16"   , _ '' ushort
	NULL        , _ '' wchar
	NULL        , _ '' integer
	NULL        , _ '' uinteger
	NULL        , _ '' enum
	@"int32"    , _ '' long
	@"uint32"   , _ '' ulong
	@"int64"    , _ '' longint
	@"uint64"   , _ '' ulongint
	@"__int128_t", _ '' integer128
	@"__uint128_t", _ '' uinteger128
	@"float"    , _ '' single
	@"double"   , _ '' double
	@"FBSTRING" , _ '' string
	NULL        , _ '' fix-len string
	@"__builtin_va_list"  , _ '' va_list
	NULL        , _ '' struct
	NULL        , _ '' namespace
	NULL        , _ '' function
	@"void"     , _ '' fwdref (needed for any un-resolved fwdrefs)
	NULL        , _ '' pointer
	NULL        , _ '' xmmword
	@"FBSTRING"   _ '' ustring
}

private sub _init( )
	irhlInit( )
	listInit( @c_pointer_views, 8, sizeof(CPOINTERVIEW), LIST_FLAGS_CLEAR )
	listInit(@c_auto_descriptors, 8, sizeof(CAUTODESC), LIST_FLAGS_CLEAR)
	listInit(@c_scalar_stores, 32, sizeof(CSCALARSTORE), LIST_FLAGS_CLEAR)
	listInit(@c_scalar_dependencies, 32, sizeof(CSCALARDEPENDENCY), LIST_FLAGS_CLEAR)
	listInit( @ctx.anonstack, 8, sizeof( FBSYMBOL ptr ), LIST_FLAGS_NOCLEAR )
	listInit( @ctx.exprnodes, 32, sizeof( EXPRNODE ), LIST_FLAGS_CLEAR )
	listInit( @ctx.exprcache, 8, sizeof( EXPRCACHENODE ), LIST_FLAGS_NOCLEAR )
	listInit( @ctx.alignmentviews, 8, sizeof(CALIGNVIEW), LIST_FLAGS_CLEAR )
	listInit( @ctx.arraycallbacks, 8, sizeof(CARRAYCALLBACK), LIST_FLAGS_CLEAR )
	listInit( @ctx.gfxblenders, 8, sizeof(CGFXBLENDER), LIST_FLAGS_CLEAR )
	irSetOption( IR_OPT_FPUIMMEDIATES or IR_OPT_MISSINGOPS )

	if( fbIs64bit( ) ) then
		dtypeName(FB_DATATYPE_INTEGER) = dtypeName(FB_DATATYPE_LONGINT)
		dtypeName(FB_DATATYPE_UINT   ) = dtypeName(FB_DATATYPE_ULONGINT)
	else
		dtypeName(FB_DATATYPE_INTEGER) = dtypeName(FB_DATATYPE_LONG)
		dtypeName(FB_DATATYPE_UINT   ) = dtypeName(FB_DATATYPE_ULONG)
	end if
end sub

private sub _end( )
	hCReadOnlyLocalsFinish()
	hCLeafFinish( )
	hCAbiFinish( )
	hCReadOnlyFinish()
	hCUnobservedFinish()
	hCClearPointerViews(0)
	listEnd(@c_pointer_views)
	hCAutoDescClear()
	listEnd(@c_auto_descriptors)
	hCScalarStoresClear()
	listEnd(@c_scalar_dependencies)
	listEnd(@c_scalar_stores)
	if( c_local_refs.list <> NULL ) then hashEnd(@c_local_refs)
	''
	'' Section buffers are retained while the compiler processes input modules so
	'' later modules can reuse their capacity.  The backend owns these allocations
	'' and releases them when the backend itself shuts down.
	''
	for i as integer = 0 to MAX_SECTIONS-1
		with( ctx.sections(i) )
			if( .buffer <> NULL ) then
				deallocate( .buffer )
				.buffer = NULL
			end if
			.bufferlen = 0
			.buffercapacity = 0
			.old = TRUE
		end with
	next

	hClearArrayCallbacks( )
	listEnd( @ctx.arraycallbacks )
	hClearGfxBlenders( )
	listEnd( @ctx.gfxblenders )
	listEnd( @ctx.alignmentviews )
	listEnd( @ctx.exprcache )
	listEnd( @ctx.exprnodes )
	listEnd( @ctx.anonstack )
	irhlEnd( )
end sub

'' "Begin/end" to be used to opening/closing sections whenever opening/closing
'' procs/scopes and also for the special sections 0 (header) and 1 (body).
private sub sectionBegin( )
	ctx.section += 1
	assert( ctx.section < MAX_SECTIONS )
	hCClearPointerViews(ctx.section)
	'' Tell next hWriteLine() to overwrite instead of appending,
	'' to overwrite pre-existing data while keeping the buffer allocated
	with( ctx.sections(ctx.section) )
		.old = TRUE
		if( ctx.section > 0 ) then
			'' Use at least the parent section's indentation
			'' (some emitting functions will temporarily increase
			'' it for code nested inside {} etc.)
			.indent = ctx.sections(ctx.section-1).indent
		else
			'' Start indendation at zero TAB's
			.indent = 0
		end if
	end with
end sub

'' Append bytes to a section while growing its buffer geometrically.
private sub sectionAppend _
	( _
		byval e as SECTIONENTRY ptr, _
		byval src as const any ptr, _
		byval bytes as uinteger _
	)
	dim as uinteger required = any
	dim as uinteger newcapacity = any
	dim as uinteger doubledcapacity = any
	dim as ubyte ptr newbuffer = any

	if( bytes = 0 ) then
		exit sub
	end if

	'' Unsigned wrap would otherwise turn the capacity check into an undersized
	'' allocation followed by an out-of-bounds copy.
	required = e->bufferlen + bytes
	if( required < e->bufferlen ) then
		error( 4 )
		exit sub
	end if

	if( required > e->buffercapacity ) then
		newcapacity = e->buffercapacity
		if( newcapacity = 0 ) then
			'' Eight KiB avoids repeated startup allocations without retaining a
			'' significant block for the many short-lived nested sections.
			newcapacity = SECTION_BUFFER_INITIAL_CAPACITY
		end if

		do while( newcapacity < required )
			doubledcapacity = newcapacity shl 1

			'' Once doubling would wrap, grow only to the exact final request.
			if( doubledcapacity <= newcapacity ) then
				newcapacity = required
				exit do
			end if

			newcapacity = doubledcapacity
		loop

		'' Keep the old pointer intact until Reallocate succeeds.  This avoids
		'' leaking the existing section and copying through NULL on failure.
		newbuffer = reallocate( e->buffer, newcapacity )
		if( newbuffer = NULL ) then
			error( 4 )
			exit sub
		end if

		e->buffer = newbuffer
		e->buffercapacity = newcapacity
	end if

	memcpy( e->buffer + e->bufferlen, src, bytes )
	e->bufferlen = required
end sub

'' Write line to current section (indentation & newline are automatically added)
private sub sectionWriteLine( byref s as string )
	static as zstring * (SECTION_TAB_BUFFER_LENGTH + 1) tabs
	dim as SECTIONENTRY ptr e = @ctx.sections(ctx.section)
	dim as integer indentation = e->indent

	'' Reuse one fixed tab block instead of allocating a temporary STRING for
	'' every indented output line.
	if( tabs[0] = 0 ) then
		for i as integer = 0 to SECTION_TAB_BUFFER_LENGTH-1
			tabs[i] = asc( TABCHAR )
		next
		tabs[SECTION_TAB_BUFFER_LENGTH] = 0
	end if

	if( e->old ) then
		e->bufferlen = 0
		e->old = FALSE
	end if

	do while( indentation > SECTION_TAB_BUFFER_LENGTH )
		sectionAppend( e, @tabs[0], SECTION_TAB_BUFFER_LENGTH )
		indentation -= SECTION_TAB_BUFFER_LENGTH
	loop

	if( indentation > 0 ) then
		sectionAppend( e, @tabs[0], indentation )
	end if

	sectionAppend( e, strptr( s ), len( s ) )
	sectionAppend( e, @NEWLINE, len( NEWLINE ) )
end sub

private sub sectionIndent( )
	ctx.sections(ctx.section).indent += 1
end sub

private sub sectionUnindent( )
	assert( ctx.sections(ctx.section).indent > 0 )
	ctx.sections(ctx.section).indent -= 1
end sub

private function sectionInsideProc( ) as integer
	'' 0 and 1 are toplevel, 2+ means inside proc
	function = (ctx.section >= 2)
end function

private sub sectionEnd( )
	dim as SECTIONENTRY ptr parent = any, child = any

	assert( ctx.section >= 0 )

	if( ctx.section > 0 ) then
		'' Append to parent section, if anything was written
		parent = @ctx.sections(ctx.section-1)
		child = @ctx.sections(ctx.section)
		if( child->old = FALSE ) then
			if( parent->old ) then
				parent->bufferlen = 0
				parent->old = FALSE
			end if
			sectionAppend( parent, child->buffer, child->bufferlen )
		end if
	end if

	ctx.section -= 1
end sub

'' "Gosub" for temporarily writing to another section than the current one
private function sectionGosub( byval section as integer ) as integer
	assert( (section >= 0) and (section <= ctx.section) )
	function = ctx.section
	ctx.section = section
	ctx.sectiongosublevel += 1
end function

'' "Return" to restore the previous current section
private sub sectionReturn( byval section as integer )
	assert( ctx.sectiongosublevel > 0 )
	ctx.sectiongosublevel -= 1
	ctx.section = section
end sub

'' Main emitting function
'' Writes out line of code to current section, and adds #line's
private sub hWriteLine( byref s as string, byval noline as integer = FALSE )
	static as string ln

	if( env.clopt.debuginfo and (noline = FALSE) ) then
		ln = "#line " + str( ctx.linenum )
		ln += " """ + ctx.escapedinputfilename + """"
		sectionWriteLine( ln )
	end if

	sectionWriteLine( s )
end sub

private sub hWriteDeclaration( byref s as string )
	'' The preceding section is this block's declaration header. Moving only
	'' storage declarations leaves calls and runtime initialization in order.
	if( ctx.section >= 3 ) then
		var section = sectionGosub( ctx.section - 1 )
		hWriteLine( s )
		sectionReturn( section )
	else
		hWriteLine( s )
	end if
end sub

#include once "backend/c/c-evaluated-values.bi"

private sub hUpdateCurrentFileName( byval filename as zstring ptr )
	ctx.escapedinputfilename = hReplace( filename, "\", $"\\" )
end sub

private sub hWriteStaticAssert( byref expr as string )
	dim as integer section = any

	if( sectionInsideProc( ) ) then
		'' A block-scope extern object is diagnosed as an unused variable.
		'' Keep the ABI check as a real C11 assertion. Older C dialects use
		'' an enum constraint, which reserves no storage or external symbol.
		ctx.localassertid += 1
		if( ctx.localassertid = 0 ) then
			errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
			exit sub
		end if
		hWriteLine( "#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)", TRUE )
		hWriteLine( "_Static_assert( (" + expr + "), ""FreeBASIC ABI mismatch"" );" )
		hWriteLine( "#else", TRUE )
		hWriteLine( "enum { fb_c_local_assert_" + str(ctx.localassertid) + _
		            " = 1 / ((" + expr + ") ? 1 : 0) };" )
		hWriteLine( "#endif", TRUE )
		exit sub
	end if

	if( (ctx.usedbuiltins and BUILTIN_STATICASSERT) = 0 ) then
		ctx.usedbuiltins or= BUILTIN_STATICASSERT

		'' Emit the #define into the header section, not inside procedures,
		'' and above the 1st use (can't be emitted from _emitEnd() because
		'' then it could appear behind struct declarations...)
		section = sectionGosub( 0 )
		hWriteLine( "#define fb_c_static_assert_check( expr ) extern int fb_c_static_assert[(expr) ? 1 : -1]", TRUE )
		sectionReturn( section )
	end if

	hWriteLine( "fb_c_static_assert_check( " + expr + " );" )
end sub

private sub hAppendClobber _
	( _
		byref ln as string, _
		byval regname as const zstring ptr _
	)
	ln += ", """
	ln += *regname
	ln += """"
end sub

private sub hAppendProcAsmClobbers( byref ln as string )
	select case( fbGetCpuFamily( ) )
	case FB_CPUFAMILY_X86
		hAppendClobber( ln, @"eax" )
		if( env.clopt.pic = FALSE ) then
			hAppendClobber( ln, @"ebx" )
		end if
		hAppendClobber( ln, @"ecx" )
		hAppendClobber( ln, @"edx" )
		hAppendClobber( ln, @"edi" )
		hAppendClobber( ln, @"esi" )

		if( env.clopt.fputype = FB_FPUTYPE_SSE ) then
			hAppendClobber( ln, @"mm0" )
			hAppendClobber( ln, @"mm1" )
			hAppendClobber( ln, @"mm2" )
			hAppendClobber( ln, @"mm3" )
			hAppendClobber( ln, @"mm4" )
			hAppendClobber( ln, @"mm5" )
			hAppendClobber( ln, @"mm6" )
			hAppendClobber( ln, @"mm7" )
			hAppendClobber( ln, @"xmm0" )
			hAppendClobber( ln, @"xmm1" )
			hAppendClobber( ln, @"xmm2" )
			hAppendClobber( ln, @"xmm3" )
			hAppendClobber( ln, @"xmm4" )
			hAppendClobber( ln, @"xmm5" )
			hAppendClobber( ln, @"xmm6" )
			hAppendClobber( ln, @"xmm7" )
		end if

	case FB_CPUFAMILY_X86_64
		hAppendClobber( ln, @"rax" )
		hAppendClobber( ln, @"rbx" )
		hAppendClobber( ln, @"rcx" )
		hAppendClobber( ln, @"rdx" )
		hAppendClobber( ln, @"rdi" )
		hAppendClobber( ln, @"rsi" )
		hAppendClobber( ln, @"r8" )
		hAppendClobber( ln, @"r9" )
		hAppendClobber( ln, @"r10" )
		hAppendClobber( ln, @"r11" )
		hAppendClobber( ln, @"r12" )
		hAppendClobber( ln, @"r13" )
		hAppendClobber( ln, @"r14" )
		hAppendClobber( ln, @"r15" )

		if( env.clopt.fputype = FB_FPUTYPE_SSE ) then
			hAppendClobber( ln, @"mm0" )
			hAppendClobber( ln, @"mm1" )
			hAppendClobber( ln, @"mm2" )
			hAppendClobber( ln, @"mm3" )
			hAppendClobber( ln, @"mm4" )
			hAppendClobber( ln, @"mm5" )
			hAppendClobber( ln, @"mm6" )
			hAppendClobber( ln, @"mm7" )
			hAppendClobber( ln, @"xmm0" )
			hAppendClobber( ln, @"xmm1" )
			hAppendClobber( ln, @"xmm2" )
			hAppendClobber( ln, @"xmm3" )
			hAppendClobber( ln, @"xmm4" )
			hAppendClobber( ln, @"xmm5" )
			hAppendClobber( ln, @"xmm6" )
			hAppendClobber( ln, @"xmm7" )
			hAppendClobber( ln, @"xmm8" )
			hAppendClobber( ln, @"xmm9" )
			hAppendClobber( ln, @"xmm10" )
			hAppendClobber( ln, @"xmm11" )
			hAppendClobber( ln, @"xmm12" )
			hAppendClobber( ln, @"xmm13" )
			hAppendClobber( ln, @"xmm14" )
			hAppendClobber( ln, @"xmm15" )
		end if

	case FB_CPUFAMILY_ARM
		hAppendClobber( ln, @"r0" )
		hAppendClobber( ln, @"r1" )
		hAppendClobber( ln, @"r2" )
		hAppendClobber( ln, @"r3" )
		hAppendClobber( ln, @"r4" )
		hAppendClobber( ln, @"r5" )
		hAppendClobber( ln, @"r6" )
		hAppendClobber( ln, @"r8" )
		hAppendClobber( ln, @"r9" )
		hAppendClobber( ln, @"r10" )
		hAppendClobber( ln, @"r11" )
		hAppendClobber( ln, @"r12" )
		hAppendClobber( ln, @"r14" )
		hAppendClobber( ln, @"r15" )

	case FB_CPUFAMILY_AARCH64
		hAppendClobber( ln, @"x0" )
		hAppendClobber( ln, @"x1" )
		hAppendClobber( ln, @"x2" )
		hAppendClobber( ln, @"x3" )
		hAppendClobber( ln, @"x4" )
		hAppendClobber( ln, @"x5" )
		hAppendClobber( ln, @"x6" )
		hAppendClobber( ln, @"x7" )
		hAppendClobber( ln, @"x8" )
		hAppendClobber( ln, @"x9" )
		hAppendClobber( ln, @"x10" )
		hAppendClobber( ln, @"x11" )
		hAppendClobber( ln, @"x12" )
		hAppendClobber( ln, @"x13" )
		hAppendClobber( ln, @"x14" )
		hAppendClobber( ln, @"x15" )
		hAppendClobber( ln, @"x16" )
		hAppendClobber( ln, @"x17" )
		hAppendClobber( ln, @"x18" )
		hAppendClobber( ln, @"x19" )
		hAppendClobber( ln, @"x20" )
		hAppendClobber( ln, @"x21" )
		hAppendClobber( ln, @"x22" )
		hAppendClobber( ln, @"x23" )
		hAppendClobber( ln, @"x24" )
		hAppendClobber( ln, @"x25" )
		hAppendClobber( ln, @"x26" )
		hAppendClobber( ln, @"x27" )
		hAppendClobber( ln, @"x28" )
		hAppendClobber( ln, @"x30" )

	case FB_CPUFAMILY_RISCV32, FB_CPUFAMILY_RISCV64
		hAppendClobber( ln, @"ra" )
		hAppendClobber( ln, @"gp" )
		hAppendClobber( ln, @"tp" )
		hAppendClobber( ln, @"t0" )
		hAppendClobber( ln, @"t1" )
		hAppendClobber( ln, @"t2" )
		hAppendClobber( ln, @"s0" )
		hAppendClobber( ln, @"s1" )
		hAppendClobber( ln, @"a0" )
		hAppendClobber( ln, @"a1" )
		hAppendClobber( ln, @"a2" )
		hAppendClobber( ln, @"a3" )
		hAppendClobber( ln, @"a4" )
		hAppendClobber( ln, @"a5" )
		hAppendClobber( ln, @"a6" )
		hAppendClobber( ln, @"a7" )
		hAppendClobber( ln, @"s2" )
		hAppendClobber( ln, @"s3" )
		hAppendClobber( ln, @"s4" )
		hAppendClobber( ln, @"s5" )
		hAppendClobber( ln, @"s6" )
		hAppendClobber( ln, @"s7" )
		hAppendClobber( ln, @"s8" )
		hAppendClobber( ln, @"s9" )
		hAppendClobber( ln, @"s10" )
		hAppendClobber( ln, @"s11" )
		hAppendClobber( ln, @"t3" )
		hAppendClobber( ln, @"t4" )
		hAppendClobber( ln, @"t5" )
		hAppendClobber( ln, @"t6" )

	case FB_CPUFAMILY_MIPS32, FB_CPUFAMILY_MIPS32EL, _
	     FB_CPUFAMILY_MIPS64, FB_CPUFAMILY_MIPS64EL
		'' Registers 26 and 27 are kernel-reserved, 28 is the ABI global
		'' pointer, 29 is the stack pointer, and 30 is the frame pointer.
		'' GCC rejects the ABI pointers in a clobber list, including for
		'' non-PIC translation units because MIPS abicalls use register 28.
		dim regname as string
		for regnum as integer = 1 to 25
			regname = "$" + str( regnum )
			hAppendClobber( ln, strptr( regname ) )
		next
		hAppendClobber( ln, @"$31" )

	case FB_CPUFAMILY_S390X
		hAppendClobber( ln, @"r0" )
		hAppendClobber( ln, @"r1" )
		hAppendClobber( ln, @"r2" )
		hAppendClobber( ln, @"r3" )
		hAppendClobber( ln, @"r4" )
		hAppendClobber( ln, @"r5" )
		hAppendClobber( ln, @"r6" )
		hAppendClobber( ln, @"r7" )
		hAppendClobber( ln, @"r8" )
		hAppendClobber( ln, @"r9" )
		hAppendClobber( ln, @"r10" )
		hAppendClobber( ln, @"r11" )
		hAppendClobber( ln, @"r12" )
		hAppendClobber( ln, @"r13" )
		hAppendClobber( ln, @"r14" )

	case FB_CPUFAMILY_LOONGARCH64
		hAppendClobber( ln, @"ra" )
		hAppendClobber( ln, @"tp" )
		hAppendClobber( ln, @"a0" )
		hAppendClobber( ln, @"a1" )
		hAppendClobber( ln, @"a2" )
		hAppendClobber( ln, @"a3" )
		hAppendClobber( ln, @"a4" )
		hAppendClobber( ln, @"a5" )
		hAppendClobber( ln, @"a6" )
		hAppendClobber( ln, @"a7" )
		hAppendClobber( ln, @"t0" )
		hAppendClobber( ln, @"t1" )
		hAppendClobber( ln, @"t2" )
		hAppendClobber( ln, @"t3" )
		hAppendClobber( ln, @"t4" )
		hAppendClobber( ln, @"t5" )
		hAppendClobber( ln, @"t6" )
		hAppendClobber( ln, @"t7" )
		hAppendClobber( ln, @"t8" )
		hAppendClobber( ln, @"s0" )
		hAppendClobber( ln, @"s1" )
		hAppendClobber( ln, @"s2" )
		hAppendClobber( ln, @"s3" )
		hAppendClobber( ln, @"s4" )
		hAppendClobber( ln, @"s5" )
		hAppendClobber( ln, @"s6" )
		hAppendClobber( ln, @"s7" )
		hAppendClobber( ln, @"s8" )
		hAppendClobber( ln, @"fp" )
	end select
end sub

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

enum EMITPROC_OPTIONS
	EMITPROC_ISPROTO   = &h1
	EMITPROC_ISPROCPTR = &h2
end enum

private sub hAppendCtorAttrib _
	( _
		byref ln as string, _
		byval proc as FBSYMBOL ptr, _
		byval in_front as integer _
	)

	dim as integer priority = any

	'' NetBSD finalizers are emitted as ordered array entries after the body.
	if( (env.clopt.target = FB_COMPTARGET_NETBSD) and _
	    (proc->stats and FB_SYMBSTATS_GLOBALDTOR) ) then exit sub

	if( proc->stats and (FB_SYMBSTATS_GLOBALCTOR or FB_SYMBSTATS_GLOBALDTOR) ) then
		dim as integer semantic_start = len(ln)
		if( in_front = FALSE ) then
			ln += " "
		end if
		ln += "__attribute__(( "
		if( proc->stats and FB_SYMBSTATS_GLOBALCTOR ) then
			ln += "constructor"
		else
			ln += "destructor"
		end if

		priority = symbGetProcPriority( proc )
		if( priority <> 0 ) then
			ln += "( " + str( priority ) + " )"
		end if

		ln += " ))"
		if( in_front ) then
			ln += " "
		end if
		fbSemanticModelStartupEmission(proc, iif(symbGetIsGlobalCtor(proc), "constructor", "destructor"), _
			"c", iif(in_front, "definition-attribute", "declaration-attribute"), _
			iif(priority = 0, "", str(priority)), "function-attribute", mid(ln, semantic_start + 1))
	end if
end sub

'' Helper function to add underscore prefix or @N stdcall suffix to mangled
'' procedure names (because symb-mangling doesn't do it for -gen gcc), for use
'' in inline ASM and such.
private function hGetMangledNameForASM _
	( _
		byval sym as FBSYMBOL ptr, _
		byval underscore_prefix as integer _
	) as string

	dim as string mangled
	dim as string libcname
	if( irUsesClangCCompiler() ) then
		libcname = irGetBuiltinLibcName( sym )
	end if

	'' Builtins with a libc ABI must also resolve when used as addresses.
	if( len( libcname ) > 0 ) then
		mangled = libcname
	elseif( ((symbGetMangling(sym) = FB_MANGLING_RTLIB) or _
	     ((symbGetMangling(sym) = FB_MANGLING_CDECL) and _
	      ((sym->stats and FB_SYMBSTATS_HASALIAS) <> 0))) andalso _
	    (sym->id.alias <> NULL) ) then
		mangled = *sym->id.alias
	else
		mangled = *symbGetMangledName( sym )
	end if

	if( underscore_prefix and env.underscoreprefix ) then
		mangled  = "_" + mangled
	end if

	'' Add the @N suffix for x86 STDCALL
	if( (fbGetCpuFamily( ) = FB_CPUFAMILY_X86) and symbIsProc( sym ) ) then
		if( symbGetProcMode( sym ) = FB_FUNCMODE_STDCALL ) then
			mangled += "@"
			mangled += str( symbProcCalcStdcallSuffixN( sym ) )
		end if
	end if

	function = mangled
end function

private function hNeedAbiAlias( byval proc as FBSYMBOL ptr ) as integer
	function = FALSE

	if( irUsesClangCCompiler() ) then
		if( len( irGetBuiltinLibcName( proc ) ) > 0 ) then return TRUE
	end if

	if( (symbGetMangling(proc) = FB_MANGLING_CDECL) andalso _
	    ((proc->stats and FB_SYMBSTATS_HASALIAS) <> 0) andalso _
	    (left(*proc->id.alias, 10) <> "__builtin_") ) then
		return TRUE
	end if

	'' rtlib mangling?  Expect we always need the asm alias
	if( symbGetMangling( proc ) = FB_MANGLING_RTLIB ) then
		function = TRUE
		exit function
	end if

	'' Only on systems where gcc would use the @N suffix
	if( fbGetCpuFamily( ) <> FB_CPUFAMILY_X86 ) then
		exit function
	end if

	select case( env.clopt.target )
	case FB_COMPTARGET_WIN32, FB_COMPTARGET_CYGWIN, _
	     FB_COMPTARGET_XBOX

		if( symbGetProcMode( proc ) = FB_FUNCMODE_FASTCALL ) then
			exit function
		end if

	case else
		exit function
	end select

	select case( symbGetProcMode( proc ) )
	'' stdcallms/pascal must be emitted as stdcall and need the alias to
	'' avoid the @N suffix.
	case FB_FUNCMODE_STDCALL_MS, FB_FUNCMODE_PASCAL
		function = TRUE

	'' For stdcall with @N suffix, if the function has a hidden UDT result
	'' pointer parameter, or UDT's passed byval, we need the alias to get
	'' the correct @N suffix. (gcc could calculate the wrong value into the
	'' @N suffix, since it doesn't known about the special result parameter
	'' or byval UDTs).  It should be safe to always generate the alias
	'' ourselves since we already must control for the special cases.
	case FB_FUNCMODE_STDCALL
		function = TRUE
	case else
		'' Other conventions do not need a separately emitted alias.
	end select
end function

private function hNeedAlias( byval proc as FBSYMBOL ptr ) as integer
	return hNeedAbiAlias(proc) orelse hCNameNeedsEncoding(symbGetMangledName(proc))
end function

private function hEmitMutableLibcPtrParam _
	( _
		byref mangled as string, _
		byval param_index as integer _
	) as integer

	function = FALSE

	if( param_index <> 0 ) then
		exit function
	end if

	'' DEALLOCATE()/REALLOCATE accept const pointers in FB source, but the
	'' C library prototypes are free(void *) and realloc(void *, size_t).
	'' Emit the libc ABI spelling so crt.bi and the built-ins can coexist.
	select case( mangled )
	case "free", "realloc"
		function = TRUE
	end select
end function

private function hEmitLibcSizeParam _
	( _
		byref mangled as string, _
		byval param_index as integer _
	) as integer

	function = FALSE

	select case( mangled )
	case "malloc"
		function = (param_index = 0)

	case "calloc"
		function = (param_index = 0) or (param_index = 1)

	case "realloc"
		function = (param_index = 1)
	end select
end function

private function hGetLibcSizeDtype( ) as integer

	'' The RTL intrinsics use uinteger for FreeBASIC's pointer-sized count,
	'' but libc prototypes use C size_t.  On LP64 targets Clang compares the
	'' spelling against unsigned long, not an unsigned long long typedef.
	dim as integer dtype = FB_DATATYPE_UINT

	if( fbIs64bit( ) andalso ((env.target.options and FB_TARGETOPT_UNIX) <> 0) ) then
		dtype = typeSetMangleDt( dtype, FB_DATATYPE_UINT )
	elseif( env.clopt.target = FB_COMPTARGET_DOS ) then
		dtype = typeSetMangleDt( FB_DATATYPE_ULONG, FB_DATATYPE_UINT )
	end if

	function = dtype
end function

private sub hAdjustCParamType _
	( _
		byref mangled as string, _
		byval param_index as integer, _
		byref dtype as integer, _
		byref subtype as FBSYMBOL ptr _
	)

	'' Prototypes and argument casts must use the same existing C ABI type.
	'' In particular, FB accepts const pointers for DEALLOCATE/REALLOCATE,
	'' while their C entry points require a mutable void pointer.
	if( hEmitMutableLibcPtrParam( mangled, param_index ) ) then
		dtype = typeAddrOf( FB_DATATYPE_VOID )
		subtype = NULL
	elseif( hEmitLibcSizeParam( mangled, param_index ) ) then
		dtype = hGetLibcSizeDtype( )
		subtype = NULL
	elseif( mangled = "fb_ArrayRedimTo" andalso param_index = 1 ) then
		'' fb_array.h and array_redimto.c both take const FBARRAY*. The
		'' intrinsic's source descriptor must retain that same C contract.
		if( typeGetDtAndPtrOnly(dtype) <> typeAddrOf(FB_DATATYPE_STRUCT) or subtype = NULL ) then
			errReportEx(FB_ERRMSG_INTERNAL, __FUNCTION__)
			exit sub
		end if
		dtype = typeAddrOf(typeSetIsConst(typeDeref(dtype)))
	end if

end sub

private function hEmitLibcProcPtrParam _
	( _
		byref mangled as string, _
		byval param_index as integer _
	) as string

	function = ""

	if( param_index <> 0 ) then
		exit function
	end if

	'' Match the C library prototype.  Some hosted libc headers are included
	'' by the generated-C smoke harness, and gcc diagnoses a conflicting
	'' atexit(void *) declaration before it gets to the BASIC test body.
	select case( mangled )
	case "atexit"
		function = "void (*)( void )"
	end select
end function

'' A C identifier is not necessarily its object symbol. Explicit asm aliases
'' take precedence over the platform C prefix and convention decoration.
'' Win32 fastcall is delegated to the C compiler; its byte suffix describes
'' the emitted C formals, including our explicit hidden-result pointer and
'' pointer lowering of non-trivial BYVAL records. The native FBC stdcall
'' suffix calculator deliberately follows a different ABI contract there.
private function hGetProcObjectName _
	( _
		byval proc as FBSYMBOL ptr, _
		byref identifier as const string _
	) as string
	dim as string object_name
	if( hNeedAbiAlias(proc) ) then
		object_name = hGetMangledNameForASM(proc, TRUE)
	else
		object_name = identifier
		dim as integer fastcall_target = FALSE
		if( (fbGetCpuFamily( ) = FB_CPUFAMILY_X86) and (symbGetProcMode(proc) = FB_FUNCMODE_FASTCALL) ) then
			select case env.clopt.target
			case FB_COMPTARGET_WIN32, FB_COMPTARGET_CYGWIN, FB_COMPTARGET_XBOX
				fastcall_target = TRUE
			end select
		end if
		dim as longint bytes = 0
		if( fastcall_target ) then
			if( symbProcReturnsOnStack(proc) ) then bytes = env.pointersize
			dim as integer count = 0, dtype = 0
			dim as FBSYMBOL ptr subtype = NULL, param = symbGetProcHeadParam(proc)
			do while( param <> NULL )
				count += 1
				if( count > symbGetProcParams(proc) ) then
					fbSemanticLinkFail( )
					return ""
				end if
				if( symbGetParamMode(param) = FB_PARAMMODE_VARARG ) then
					'' The C compiler ignores fastcall on a variadic signature.
					fastcall_target = FALSE
					exit do
				end if
				symbGetRealParamDtype(param, dtype, subtype)
				dim as longint length = symbCalcLen(dtype, subtype)
				if( (length <= 0) or (length > &h7fffffff) ) then
					fbSemanticLinkFail( )
					return ""
				end if
				'' Win32 C argument slots are rounded up to four bytes.
				bytes += ((length + 3) \ 4) * 4
				param = param->next
			loop
		end if
		if( fastcall_target ) then
			object_name = "@" + object_name + "@" + str(bytes)
		elseif( env.underscoreprefix ) then
			object_name = "_" + object_name
		end if
	end if
	return object_name
end function

private sub hObserveProcObjectName( byval proc as FBSYMBOL ptr, byref identifier as const string )
	if( (fbSemanticLinkProcedureObjectNeeded(proc) = FALSE) and _
	    (fbSemanticModelObjectNameNeeded(proc) = FALSE) ) then exit sub
	if( (left(identifier, 10) = "__builtin_") and (hNeedAbiAlias(proc) = FALSE) ) then
		'' The C compiler can inline or rewrite a builtin. Its identifier is
		'' not evidence of an object symbol with that spelling.
		fbSemanticModelEmissionName(proc, "object", "", "unavailable", "backend-intrinsic")
		exit sub
	end if
	var object_name = hGetProcObjectName( proc, identifier )
	fbSemanticModelEmissionName(proc, "object", object_name)
	fbSemanticLinkProcedureObject( proc, object_name )
end sub

private function hProcDoesNotReturn( byval proc as FBSYMBOL ptr ) as integer
	if( symbGetProcIsNoReturn( proc ) ) then return TRUE
	if( symbGetIsRTL( proc ) andalso (*symbGetMangledName( proc ) = FB_RTL_END) ) then
		'' END, SYSTEM and STOP share the runtime's terminating exit() path.
		return TRUE
	end if
	return FALSE
end function

private function hIsCLongDoubleType _
	( byval dtype as integer, byval subtype as FBSYMBOL ptr ) as integer

	if( typeGetDtAndPtrOnly( dtype ) <> FB_DATATYPE_STRUCT ) then return FALSE
	if( subtype = NULL ) then return FALSE
	return (*symbGetName( subtype ) = "CLONGDOUBLE")
end function

private function hUsesCLongDoubleCABI( byval proc as FBSYMBOL ptr ) as integer
	if( proc = NULL ) then return FALSE
	return (symbGetProcMode( proc ) = FB_FUNCMODE_CDECL)
end function

private function hCLongDoubleFromNativeExpression _
	( byref native_expression as string, byval subtype as FBSYMBOL ptr ) as string

	var identifier = "fb_c_clongdouble_native_result_" + str( symbUniqueId( ) )
	var native_identifier = identifier + "_native"
	var type_name = hEmitType( FB_DATATYPE_STRUCT, subtype )
	function = "__extension__ ({ " + type_name + " " + identifier + "; " + _
		"long double " + native_identifier + " = " + native_expression + "; " + _
		"__builtin_memset( (void*)&" + identifier + ", 0, sizeof( " + identifier + " ) ); " + _
		"__builtin_memcpy( (void*)&" + identifier + ", (const void*)&" + native_identifier + ", " + _
		"(sizeof( " + identifier + " ) < sizeof( " + native_identifier + " ) ? " + _
		"sizeof( " + identifier + " ) : sizeof( " + native_identifier + " )) ); " + _
		identifier + "; })"
end function

private function hCLongDoubleToNativeExpression _
	( byref value_expression as string, byval subtype as FBSYMBOL ptr ) as string

	var identifier = "fb_c_clongdouble_native_argument_" + str( symbUniqueId( ) )
	var type_name = hEmitType( FB_DATATYPE_STRUCT, subtype )
	function = "__extension__ ({ " + type_name + " " + identifier + " = " + _
		value_expression + "; fb_LongDoubleToNative( (const void*)&" + identifier + ", " + _
		str( subtype->lgt ) + "u ); })"
end function

#include once "backend/c/c-readonly-parameters.bi"
#include once "backend/c/c-readonly-locals.bi"
#include once "backend/c/c-unobserved-arguments.bi"

private function hEmitProcHeader _
	( _
		byval proc as FBSYMBOL ptr, _
		byval options as EMITPROC_OPTIONS, _
		byval readonly_variant as CREADONLYPROC ptr = NULL _
	) as string

	dim as string ln, mangled
	dim as CABIPARAM ptr abi_first = NULL, abi_last = NULL
	dim as uinteger abi_prefix = 0
	dim as integer dtype = any
	dim as FBSYMBOL ptr subtype = any
	dim as string semantic_result, semantic_convention, semantic_role

	if( options = 0 ) then
		'' ctor/dtor flags on bodies
		hAppendCtorAttrib( ln, proc, TRUE )
	end if

	if( (options and EMITPROC_ISPROCPTR) = 0 ) then
		if( symbIsPrivate(proc) or readonly_variant <> NULL ) then
			ln += "static "
		end if
	end if

	'' Function result type (is 'void' for subs)
	dim as integer cabi_clongdouble_result = _
		hUsesCLongDoubleCABI( proc ) andalso _
		hIsCLongDoubleType( symbGetProcRealType( proc ), _
		                     symbGetProcRealSubtype( proc ) )
	if( cabi_clongdouble_result ) then
		semantic_result = "long double"
	else
		semantic_result = hEmitType( typeUnsetIsConst( symbGetProcRealType( proc ) ), _
						symbGetProcRealSubtype( proc ), TRUE )
	end if
	ln += semantic_result

	'' Calling convention if needed (for function pointers it's usually not
	'' put in this place, but should work nonetheless)
	if( fbGetCpuFamily( ) = FB_CPUFAMILY_X86 ) then
		select case( symbGetProcMode( proc ) )
		case FB_FUNCMODE_STDCALL, FB_FUNCMODE_STDCALL_MS, FB_FUNCMODE_PASCAL
			select case( env.clopt.target )
			case FB_COMPTARGET_WIN32, FB_COMPTARGET_XBOX
				'' MinGW recognizes this shorter & prettier version
				semantic_convention = " __stdcall"
			case else
				'' Linux GCC only accepts this
				semantic_convention = " __attribute__((stdcall))"
			end select
		case FB_FUNCMODE_THISCALL
			select case( env.clopt.target )
			case FB_COMPTARGET_WIN32, FB_COMPTARGET_XBOX
				semantic_convention = " __thiscall"
			case else
				semantic_convention = " __attribute__((thiscall))"
			end select
		case FB_FUNCMODE_FASTCALL
			select case( env.clopt.target )
			case FB_COMPTARGET_WIN32, FB_COMPTARGET_XBOX
				semantic_convention = " __fastcall"
			case else
				semantic_convention = " __attribute__((fastcall))"
			end select
		end select
	end if
	ln += semantic_convention
	semantic_role = iif((options and EMITPROC_ISPROCPTR) <> 0, "pointer-type", _
		iif((options and EMITPROC_ISPROTO) <> 0, "declaration", "definition"))
	dim as longint semantic_signature = 0
	if( readonly_variant = NULL ) then semantic_signature = fbSemanticModelBackendAbiBegin(proc, "c", semantic_role, semantic_result, semantic_convention)

	if( (options and EMITPROC_ISPROCPTR) = 0 ) then
		if( hProcDoesNotReturn( proc ) ) then ln += " __attribute__((noreturn))"
	end if
	ln += " "

	mangled = *symbGetMangledName( proc )

	'' Identifier
	if( options and EMITPROC_ISPROCPTR ) then
		ln += "(*"
		ln += hGetCId( proc )
		ln += ")"
	else
		var identifier = hGetCId(proc)
		if( readonly_variant <> NULL ) then identifier = *readonly_variant->identifier
		ln += identifier
		if( readonly_variant = NULL ) then fbSemanticModelEmissionName(proc, "c", identifier)
	end if

	'' Parameter list
	ln += "( "

	'' If returning a struct, there's an extra parameter
	dim as FBSYMBOL ptr hidden = NULL
	if( symbProcReturnsOnStack( proc ) andalso (cabi_clongdouble_result = FALSE) ) then
		dim as integer semantic_hidden_dtype
		dim as FBSYMBOL ptr semantic_hidden_subtype
		if( options and EMITPROC_ISPROTO ) then
			hidden = symbGetSubType( proc )
			semantic_hidden_dtype = typeAddrOf( symbGetType( hidden ) )
			semantic_hidden_subtype = hidden
		else
			hidden = proc->proc.ext->res
			semantic_hidden_dtype = typeAddrOf( symbGetType( hidden ) )
			semantic_hidden_subtype = symbGetSubtype( hidden )
		end if
		dim as string semantic_hidden = hEmitType(semantic_hidden_dtype, semantic_hidden_subtype)
		ln += semantic_hidden
		fbSemanticModelBackendAbiParameter(semantic_signature, NULL, 0, 1, "hidden-result", semantic_hidden, _
			semantic_hidden_dtype, semantic_hidden_subtype)
		if( (options and EMITPROC_ISPROTO) = 0 ) then ln += " " + hGetCId(hidden)

		if( symbGetProcParams( proc ) > 0 ) then
			ln += ", "
		end if
	end if

	var param = symbGetProcLastParam( proc )
	dim as integer param_index = 0
	dim as DZSTRING params
	DZstrZero( params )

	if( (hidden = NULL) and (param = NULL) ) then
		ln += "void"
	end if

	while( param )
		if( symbGetParamMode( param ) = FB_PARAMMODE_VARARG ) then
			DZstrConcatAssign( params, "..." )
			fbSemanticModelBackendAbiParameter(semantic_signature, param, 0, 0, "variadic", "...")
		else
			'' Emit clang-compatible datatype for main()'s argv,
			'' clang is very strict about this...
			if( param->stats and FB_SYMBSTATS_ARGV ) then
				DZstrConcatAssign( params, "char**" )
				fbSemanticModelBackendAbiParameter(semantic_signature, param, 0, 1, "argv", "char**")
			else
				symbGetRealParamDtype( param, dtype, subtype )
			if( hCReadOnlyParam(readonly_variant, param) ) then dtype = typeAddrOf(typeSetIsConst(typeDeref(dtype)))
				dim as integer semantic_original_dtype = dtype
				dim as FBSYMBOL ptr semantic_original_subtype = subtype
				if( (options and EMITPROC_ISPROTO) <> 0 ) then
					dim as string libc_procptr = hEmitLibcProcPtrParam( mangled, param_index )
					var gfx_blender = hGfxBlenderType(proc, param_index)
					if( gfx_blender <> NULL ) then libc_procptr = hEmitType(typeAddrOf(FB_DATATYPE_FUNCTION), gfx_blender, TRUE)
					if( hIsArrayCallbackParam( proc, param_index ) ) then
						libc_procptr = "void (*)( void* )"
					end if
					if( len( libc_procptr ) > 0 ) then
						DZstrConcatAssign( params, libc_procptr )
						fbSemanticModelBackendAbiParameter(semantic_signature, param, 0, 1, "libc-callback", libc_procptr)
						param = symbGetProcPrevParam( proc, param )
						param_index += 1
						if( param ) then
							DZstrConcatAssign( params, ", " )
						end if
						continue while
					end if
					hAdjustCParamType( mangled, param_index, dtype, subtype )
				end if
				dim as integer cabi_clongdouble_parameter = _
					hUsesCLongDoubleCABI( proc ) andalso _
					hIsCLongDoubleType( dtype, subtype )
				dim as string semantic_parameter
				if( cabi_clongdouble_parameter ) then
					semantic_parameter = "long double"
				else
					semantic_parameter = hEmitType(dtype, subtype, TRUE)
				end if
				DZstrConcatAssign( params, semantic_parameter )
				dim as string semantic_kind = "value"
				if( (dtype <> semantic_original_dtype) or (subtype <> semantic_original_subtype) ) then semantic_kind = "libc-adjusted"
				fbSemanticModelBackendAbiParameter(semantic_signature, param, 0, 1, semantic_kind, semantic_parameter, dtype, subtype)
			end if

			if( (options and EMITPROC_ISPROTO) = 0 ) then
				var parameter_identifier = hGetCId( symbGetParamVar( param ) )
				if( hUsesCLongDoubleCABI( proc ) andalso _
				    hIsCLongDoubleType( dtype, subtype ) ) then
					parameter_identifier = "fb_c_native_" + parameter_identifier
				end if
				DZstrConcatAssign( params, " " + parameter_identifier )
				if( options = 0 andalso readonly_variant = NULL ) then hCAbiParameter(abi_first, abi_last, param, len(*params.data))
			end if
		end if

		param = symbGetProcPrevParam( proc, param )
		param_index += 1
		if( param ) then
			DZstrConcatAssign( params, ", " )
		end if
	wend

	abi_prefix = len(ln)
	if( params.data <> NULL ) then
		ln += *params.data
	end if
	DZstrAllocate( params, 0 )

	ln += " )"
	if( (options and EMITPROC_ISPROCPTR) = 0 andalso readonly_variant = NULL ) then hObserveProcObjectName(proc, mangled)

	if( ((options and EMITPROC_ISPROCPTR) = 0) and _
	    ((options and EMITPROC_ISPROTO) <> 0)        ) then
		'' Add asm("mangledname") alias if needed.
		'' asm() can only be be used on prototypes.
		if( hNeedAlias(proc) andalso readonly_variant = NULL ) then
			var object_name = hGetProcObjectName( proc, mangled )
			hAppendCObjectAlias( ln, object_name )
		end if

		'' MinGW GCC takes constructor priorities from the first declaration.
		'' Alias prototypes must retain them before the attributed definition.
		hAppendCtorAttrib( ln, proc, FALSE )
	end if

	if( abi_first <> NULL ) then
		hCAbiDefinition(proc, ln, abi_prefix, abi_first, semantic_signature)
	else
		fbSemanticModelBackendAbiEnd(semantic_signature, ln)
	end if
	function = ln
end function

private function hCReadOnlyCallee(byval proc as FBSYMBOL ptr) as CREADONLYPROC ptr
	var entry = hCReadOnlyFind(proc)
	if( entry <> NULL ) then return entry
	'' The current AST is being consumed. Its complete proof ran at entry;
	'' never infer a contract from only the unconsumed suffix of a body.
	if( proc = ctx.currproc ) then return NULL
	var body = ast.proc.head
	while( body <> NULL )
		if( body->sym = proc ) then exit while
		body = body->next
	wend
	if( body = NULL ) then return NULL
	entry = hCReadOnlyPrepare(proc, body->l)
	if( entry = NULL ) then return NULL
	'' A caller may be emitted before the definition, including module-level
	'' code. Place the internal prototype in the enclosing output section.
	var section = sectionGosub(0)
	var first = ctx.sections(0).bufferlen
	hWriteLine(hEmitProcHeader(proc, EMITPROC_ISPROTO, entry) + ";")
	hCLeafAuxPrototype(proc, first)
	sectionReturn(section)
	return entry
end function


private function hGetUdtTag( byval sym as FBSYMBOL ptr ) as string
	if( symbIsStruct( sym ) ) then
		if( symbGetUDTIsUnion( sym ) ) then
			function = "union "
		else
			function = "struct "
		end if
	end if
end function

private function hGetUdtId( byval sym as FBSYMBOL ptr ) as string
	dim as string id
	dim as zstring ptr mangled

	if( typeGetMangleDt( symbGetFullType( sym ) ) = FB_DATATYPE_VA_LIST ) then
		'' gcc's __builtin_va_list needs an exact name
		select case symbGetValistType( symbGetFullType( sym ), symbGetSubtype( sym ) )
		case FB_CVA_LIST_BUILTIN_C_STD, FB_CVA_LIST_BUILTIN_PPC, _
		     FB_CVA_LIST_BUILTIN_AARCH64, FB_CVA_LIST_BUILTIN_ARM, _
		     FB_CVA_LIST_BUILTIN_S390X
				function = *sym->id.alias
				exit function
		end select
	end if

	'' The mangled name may start with a number. Use a C identifier prefix
	'' for these private type names, leaving linker/RTTI names unchanged.
	'' Internal names can also contain $. Encode every non-alphanumeric byte,
	'' including the escape marker _, so distinct mangled names stay distinct.
	id = "fb_type_"
	mangled = symbGetMangledName( sym )
	for i as integer = 0 to len( *mangled ) - 1
		dim as ubyte ch = cubyte( (*mangled)[i] )
		select case ch
		case asc( "A" ) to asc( "Z" ), asc( "a" ) to asc( "z" ), asc( "0" ) to asc( "9" )
			id += chr( ch )
		case else
			id += "_" + hex( ch, 2 )
		end select
	next
	function = id
end function

private function hGetUdtName( byval sym as FBSYMBOL ptr ) as string
	function = hGetUdtTag( sym ) + hGetUdtId( sym )
end function

private sub hEmitUDT( byval s as FBSYMBOL ptr, byval is_ptr as integer )
	dim as integer section = any

	if( s = NULL ) then
		return
	end if

	if( symbGetIsEmitted( s ) ) then
		return
	end if

	if( symbIsLocal( s ) ) then
		'' Write declaration to corresponding scope
		'' (FB_MAINSCOPE=0 maps to section index 1)
		section = 1 + symbGetScope( s )

		'' Local to FB main? Convert to explicit main() function...
		'' (should only happen while emitting main(), since we won't
		'' see main's locals from elsewhere)
		if( symbGetScope( s ) = FB_MAINSCOPE ) then
			section += 1

		'' global namespace due the implicit MAIN?
		elseif( symbGetNamespace( s ) = @symbGetGlobalNamespc( ) ) then
			section += 1

		end if

		'' BASIC scope numbers predate the separate C declaration and body
		'' sections. Map the old scope to its declaration section so a local
		'' record is complete before its hoisted variables. Clamp to the
		'' current scope when static locals need a child type in their parent.
		if( ctx.section >= 2 ) then
			var current_scope_section = (ctx.section \ 2) + 1
			if( section > current_scope_section ) then section = current_scope_section
			if( section >= 2 ) then section = (section - 1) * 2
		elseif( section > ctx.section ) then
			section = ctx.section
		end if
	else
		'' Write to toplevel
		section = 0
	end if

	section = sectionGosub( section )

	select case as const symbGetClass( s )
	case FB_SYMBCLASS_ENUM
		symbSetIsEmitted( s )
		'' no subtype, to avoid infinite recursion
		hWriteLine( "typedef " + hEmitType( FB_DATATYPE_ENUM, NULL ) + " " + hGetUdtName( s ) + ";" )
		hCRememberViewType(FB_DATATYPE_ENUM, s)

	case FB_SYMBCLASS_STRUCT
		hEmitStruct( s, is_ptr )

	'' We're emitting procptr subtypes as typedefs, instead of expanding them in-place,
	'' because that way we can keep doing the "type id" syntax for local vars, parameters, etc.,
	'' while when expanding procptrs it would become more complicated ("returntype (*id)(parameters)").
	case FB_SYMBCLASS_PROC
		if( symbGetIsFuncPtr( s ) ) then
			'' While emitting a procptr typedef, we may emit a UDT that references this procptr typedef.
			'' For example:
			''     type FBSYMBOL_ as FBSYMBOL
			''     type FBRTLCALLBACK as function(byval as FBSYMBOL_ ptr) as integer
			''     type FBSYMBOL
			''         callback as FBRTLCALLBACK
			''     end type
			''     dim callback as FBRTLCALLBACK
			'' In a case like that, we should take care not to emit the typedef twice, because that
			'' isn't allowed by older gcc versions.

			'' 1. Build the procedure declaration and emit its dependencies...
			var decl = hEmitProcHeader( s, EMITPROC_ISPROTO or EMITPROC_ISPROCPTR )

			'' 2. Emit this procptr typedef if it wasn't emitted by step 1
			if( symbGetIsEmitted( s ) = FALSE ) then
				hWriteLine( "typedef " + decl + ";" )
				symbSetIsEmitted( s )
				hCRememberViewType(FB_DATATYPE_FUNCTION, s)
			end if
		end if
	end select

	sectionReturn( section )
end sub

'' Returns "[N]" (N = array size) if the symbol is an array or a fixlen string.
private function hEmitArrayDecl( byval sym as FBSYMBOL ptr ) as string
	dim as string s
	dim as DZSTRING dimensions
	DZstrZero( dimensions )

	'' even dllimport'ed arrays are emitted as pointers, not arrays
	if( symbIsImport( sym ) ) then
		exit function
	end if

	'' Emit all array dimensions individually
	'' (This lets array initializers rely on gcc to fill uninitialized
	'' elements with zeroes)
	select case( symbGetClass( sym ) )
	case FB_SYMBCLASS_VAR, FB_SYMBCLASS_FIELD
		if( symbGetIsDynamic( sym ) = FALSE ) then
			for i as integer = 0 to symbGetArrayDimensions( sym ) - 1
				'' elements = ubound( array, d ) - lbound( array, d ) + 1
				DZstrConcatAssign( dimensions, _
					"[" + str( symbArrayUbound( sym, i ) - symbArrayLbound( sym, i ) + 1 ) + "]" )
			next
		end if
	end select
	if( dimensions.data <> NULL ) then
		s = *dimensions.data
	end if
	DZstrAllocate( dimensions, 0 )

	if( symbIsRef( sym ) = FALSE ) then
		'' If it's a fixed-length string, add an extra array dimension
		'' (zstring * 5 becomes char[5])
		'' (string * 5 becomes char[5])
		dim as longint length = 0
		select case( symbGetType( sym ) )
		case FB_DATATYPE_FIXSTR, FB_DATATYPE_CHAR
			length = symbGetSizeOf( sym )
		case FB_DATATYPE_WCHAR
			length = symbGetWstrLength( sym ) + 1
		end select
		if( length > 0 ) then
			s += "[" + str( length ) + "]"
		end if
	end if

	function = s
end function

private sub hEmitVarDecl _
	( _
		byval use_extern as integer, _
		byval sym as FBSYMBOL ptr, _
		byval varini as zstring ptr _
	)

	dim as string ln
	dim as integer static_storage = FALSE
	var is_vtable = ((sym->stats and FB_SYMBSTATS_VTABLE) <> 0)
	dim as string vtabletag
	if( is_vtable ) then
		vtabletag = "fb_c_vtable_" + hGetCId( sym )
		if( use_extern ) then
			'' External tables need only an address, so their record can stay incomplete.
			hWriteDeclaration( "struct " + vtabletag + ";" )
		else
			if( (ctx.varinisym <> sym) or (varini = NULL) or _
			    (ctx.vtableelements <> symbGetArrayElements(sym)) ) then
				errReportEx( FB_ERRMSG_INTERNAL, "invalid C virtual table initializer" )
				exit sub
			end if
			'' The native ABI uses consecutive pointer-sized slots. A C record keeps
			'' each method's real prototype without converting its address to void*.
			hWriteDeclaration( "struct " + vtabletag + " { " + ctx.vtablemembers + " };" )
			hWriteStaticAssert( "sizeof(struct " + vtabletag + ") == " + _
				str(ctx.vtableelements) + " * sizeof(void*)" )
			for i as integer = 0 to ctx.vtableelements - 1
				hWriteStaticAssert( "__builtin_offsetof(struct " + vtabletag + _
					", slot_" + str(i) + ") == " + str(i) + " * sizeof(void*)" )
			next
		end if
	end if

	if( use_extern ) then
		ln += "extern "
	elseif( symbIsStatic( sym ) and ((symbGetAttrib( sym ) and (FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_EXTERN)) = 0) ) then
		ln += "static "
		static_storage = TRUE
	end if
	if( symbIsThreadLocal( sym ) ) then ln += "__thread "

	dim dtype as integer
	dim subtype as FBSYMBOL ptr
	symbGetRealType( sym, dtype, subtype )

	if( is_vtable ) then
		ln += "struct " + vtabletag
	elseif( use_extern and (typeGetPtrCnt(dtype) = 0) and _
	    (typeGet(dtype) = FB_DATATYPE_FWDREF) ) then
		'' C allows an extern object declaration with an incomplete type.
		'' The C backend only needs a concrete spelling to form its address;
		'' accesses that require the missing layout are rejected by the parser.
		ln += "char"
	else
		'' C permits the pointee of a local pointer to remain const even though
		'' top-level const on the BASIC variable itself is not emitted here.
		dtype = hCReadOnlyLocalType(sym, dtype)
		ln += hEmitType( typeUnsetIsConst( dtype ), sym->subtype, TRUE )
	end if
	var identifier = hGetCId( sym )
	ln += " " + identifier
	fbSemanticModelEmissionName(sym, "c", identifier)
	if( is_vtable = FALSE ) then ln += hEmitArrayDecl( sym )
	dim as string selected_object_name
	if( sym->class = FB_SYMBCLASS_VAR andalso symbIsLocal(sym) = FALSE ) then
		if( hCNameNeedsEncoding(symbGetMangledName(sym)) andalso _
		    (hCUseEmulatedTlsName(sym, symbGetMangledName(sym)) = FALSE) ) then
			'' Global C tokens may differ; retain the original object identity.
			var object_name = hGetMangledNameForASM( sym, TRUE )
			hAppendCObjectAlias( ln, object_name )
			selected_object_name = object_name
		end if
	end if
	if( fbSemanticModelObjectNameNeeded(sym) ) then
		if( symbIsThreadLocal(sym) ) then
			fbSemanticModelEmissionName(sym, "object", "", "unavailable", "backend-tls")
		elseif( symbIsLocal(sym) ) then
			if( symbIsStatic(sym) ) then
				fbSemanticModelEmissionName(sym, "object", "", "unavailable", "c-local-static")
			else
				fbSemanticModelEmissionName(sym, "object", "", "none", "automatic-storage")
			end if
		else
			if( len(selected_object_name) = 0 ) then
				selected_object_name = identifier
				if( env.underscoreprefix ) then selected_object_name = "_" + selected_object_name
			end if
			fbSemanticModelEmissionName(sym, "object", selected_object_name)
		end if
	end if

	'' dllimport's are handled manually, emitted as pointers and deref'ed
	'' where needed, as with the ASM backend, as opposed to using
	'' __attribute__((dllimport)). The _imp__ prefix will be added by fbc's
	'' name mangling.

	if( symbIsCommon( sym ) and (not use_extern) ) then
		'' WebAssembly object files do not support C COMMON symbols.
		'' Weak zero-initialized definitions preserve the one-definition
		'' behavior needed by BASIC COMMON blocks without tripping clang.
		if( env.clopt.target = FB_COMPTARGET_JS ) then
			ln += " __attribute__((weak))"
		else
			ln += " __attribute__((common))"
		end if
	end if

	if( varini ) then
		ln += " = " + *varini
	end if

	ln += ";"
	'' C static initializers are constant data. Their declaration can precede
	'' statements; BASIC runtime initialization remains in the emitted body.
	if( (varini = NULL) orelse static_storage ) then
		hWriteDeclaration( ln )
	else
		hWriteLine( ln )
	end if
end sub

private function hHasCLocalReference( byval sym as FBSYMBOL ptr ) as integer
	if( c_local_refs.list = NULL ) then return FALSE
	var key = str(culngint(sym))
	return (hashLookup(@c_local_refs, strptr(key)) <> NULL)
end function

private sub hRememberCLocalReference( byval sym as FBSYMBOL ptr )
	if( sym = NULL ) then exit sub
	if( symbIsVar(sym) = FALSE orelse symbIsLocal(sym) = FALSE ) then exit sub
	symbSetIsAccessed(sym)
	var key = str(culngint(sym))
	if( hashLookup(@c_local_refs, strptr(key)) = NULL ) then
		hashAdd(@c_local_refs, ZstrDup(strptr(key)), sym, hashHash(strptr(key)))
	end if
end sub

private sub hClearCLocalReferences( )
	if( c_local_refs.list <> NULL ) then hashEnd(@c_local_refs)
end sub

private sub hMarkReferencedLocalInTree( byval n as ASTNODE ptr )
	if( n = NULL ) then exit sub

	'' LIT nodes load as comments; only assembler text can hide storage uses.
	if( n->class = AST_NODECLASS_ASM ) then
		'' Assembly can name storage without an ordinary variable node.
		c_local_refs_complete = FALSE
		ctx.computedgotoopaque = TRUE
	elseif( n->class <> AST_NODECLASS_DECL ) then
		'' A declaration alone does not use storage. Its initialization children
		'' still do, including constructor calls and runtime clearing.
		hRememberCLocalReference(n->sym)
	end if

	if( n->class = AST_NODECLASS_CALL ) then
		'' The loader materializes these addresses outside the ordinary L/R
		'' trees: hidden ABI return slots, string copyback and profiling calls.
		hRememberCLocalReference(n->call.tmpres)
		var item = n->call.strtail
		while( item <> NULL )
			hRememberCLocalReference(item->sym)
			hMarkReferencedLocalInTree(item->srctree)
			item = item->prev
		wend
		hMarkReferencedLocalInTree(n->call.profbegin)
		hMarkReferencedLocalInTree(n->call.profend)
	end if

	'' NEXT links statement roots; only the caller walks that list.
	hMarkReferencedLocalInTree(n->l)
	hMarkReferencedLocalInTree(n->r)
end sub

private sub hMarkReferencedCStatement( byval n as ASTNODE ptr, byref fallthrough as integer )
	if( n = NULL ) then exit sub
	if( n->class = AST_NODECLASS_LINK ) then
		hMarkReferencedCStatement(n->l, fallthrough)
		hMarkReferencedCStatement(n->r, fallthrough)
		exit sub
	end if
	if( n->class = AST_NODECLASS_LABEL ) then
		if( symbGetIsAccessed(n->sym) ) then fallthrough = TRUE
	end if
	'' C flushing discards executable children of unreachable declarations.
	'' Only their lexical scope survives, so those children cannot keep storage.
	if( fallthrough = FALSE ) then exit sub
	hMarkReferencedLocalInTree(n)
	if( astCStatementEndsFlow(n) ) then fallthrough = FALSE
end sub

private sub hMarkReferencedLocals( byval n as ASTNODE ptr )
	dim as integer fallthrough = TRUE
	while( n <> NULL )
		hMarkReferencedCStatement(n, fallthrough)
		n = n->next
	wend
end sub

#include once "backend/c/c-static-array-storage.bi"

private sub hMaybeEmitLocalVar( byval sym as FBSYMBOL ptr )
	assert( symbIsLocal( sym ) )

	'' Fake dynamic array symbol? Descriptor will be emitted instead
	if( symbGetIsDynamic( sym ) ) then
		exit sub
	end if

	if( hCAutoDescUnused(sym) or hCAutoDescUnusedData(sym) ) then
		if( symbGetType(sym) = FB_DATATYPE_STRUCT ) then hEmitUDT(symbGetSubtype(sym), FALSE)
		exit sub
	end if

	if( hCStaticArrayStorageUnused(sym) or hCStaticScalarStorageUnused(sym) ) then
		'' Keep type declarations and ABI assertions when only storage is omitted.
		if( symbGetType(sym) = FB_DATATYPE_STRUCT ) then hEmitUDT(symbGetSubtype(sym), FALSE)
		exit sub
	end if

	'' A fixed array descriptor inherits ACCESSED from its data array when
	'' created. Direct indexing uses that data without using the descriptor.
	'' Only an actual optimized-AST descriptor reference requires this private
	'' constant metadata. Keep dynamic arrays and lifetime wrappers intact.
	if( c_local_refs_complete andalso symbIsStatic(sym) andalso symbIsDescriptor(sym) ) then
		var backing = sym->var_.desc.array
		if( (backing <> NULL) andalso symbIsVar(backing) andalso symbIsLocal(backing) andalso _
		    symbIsStatic(backing) andalso (symbIsDynamic(backing) = FALSE) andalso _
		    (symbGetArrayDimensions(backing) > 0) andalso _
		    ((backing->attrib and (FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_EXTERN or _
		      FB_SYMBATTRIB_SHARED or FB_SYMBATTRIB_COMMON)) = 0) andalso _
		    ((sym->stats and FB_SYMBSTATS_IMPLICIT) <> 0) andalso _
		    (symbHasCtor(backing) = FALSE) andalso (symbHasDtor(backing) = FALSE) ) then
			'' Keep dormant data/descriptor pairs until all initializer dependencies
			'' can be proved. Removing just their descriptor would strand data.
			if( hHasCLocalReference(backing) andalso (hHasCLocalReference(sym) = FALSE) ) then
				'' Keep descriptor type declarations and their ABI assertions;
				'' only this unreferenced storage declaration is omitted.
				hEmitUDT( symbGetSubtype(sym), FALSE )
				exit sub
			end if
		end if
	end if

	'' _emitProcBegin() scans the complete optimized procedure AST before
	'' declarations are emitted, so an unreferenced static without an
	'' initializer cannot be needed by a later expression.
	if( symbIsStatic( sym ) andalso _
	    (symbGetIsAccessed( sym ) = FALSE) andalso _
	    (symbGetTypeIniTree( sym ) = NULL) ) then
		exit sub
	end if

	'' Only surviving storage references keep automatic locals. Parser access
	'' flags can still describe expressions that have since been lowered away.
	'' Initialization, destruction and hidden call storage are indexed above;
	'' function result variables keep their ABI role even without a VAR node.
	if( c_local_refs_complete andalso (symbIsStatic(sym) = FALSE) andalso _
	    ((hHasCLocalReference(sym) = FALSE) orelse hCScalarStoreUnusedValue(sym)) andalso _
	    ((symbGetTypeIniTree(sym) = NULL) orelse hCScalarStoreUnusedBinding(sym)) andalso _
	    ((symbGetAttrib(sym) and FB_SYMBATTRIB_FUNCRESULT) = 0) andalso _
	    (symbGetArrayDimensions(sym) = 0) ) then
		if( symbGetType(sym) = FB_DATATYPE_STRUCT ) then hEmitUDT(symbGetSubtype(sym), FALSE)
		exit sub
	end if

	if( (symbGetTypeIniTree( sym ) <> NULL) and symbIsStatic( sym ) ) then
		irhlFlushStaticInitializer( sym )
	else
		hEmitVarDecl( FALSE, sym, NULL )
	end if
end sub

private sub hAllocGlobalVar( byval sym as FBSYMBOL ptr )
	if( symbGetTypeIniTree( sym ) ) then
		'' Unused private global? Don't bother emitting it
		if( (not symbIsPublic( sym )) and (not symbGetIsAccessed( sym )) ) then
			exit sub
		end if
		irhlFlushStaticInitializer( sym )
	else
		hEmitVarDecl( FALSE, sym, NULL )
	end if
end sub

private sub hMaybeEmitGlobalVar( byval sym as FBSYMBOL ptr )
	assert( symbIsLocal( sym ) = FALSE )

	'' String literals? Emitted inline instead of as global vars
	'' Unused EXTERN? Don't bother emitting it
	'' Fake dynamic array symbol? Descriptor will be emitted instead
	if( symbGetIsLiteral( sym ) or _
	    (symbIsExtern( sym ) and (not symbGetIsAccessed( sym ))) or _
	    symbGetIsDynamic( sym ) ) then
		exit sub
	end if

	'' Private globals which are never referenced do not need a C declaration.
	'' Public and COMMON symbols remain visible to other translation units.
	if( (symbIsPublic( sym ) = FALSE) andalso _
	    (symbIsCommon( sym ) = FALSE) andalso _
	    (symbGetIsAccessed( sym ) = FALSE) ) then
		exit sub
	end if

	if( hCGlobalStorageNeeded(sym) = FALSE ) then exit sub

	select case( ctx.globalvarpass )
	case 1
		if( symbGetAttrib( sym ) and (FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_EXTERN) ) then
			'' Emit externs as prototypes only for now;
			'' their initializers may reference other not-yet-emitted globals
			hEmitVarDecl( TRUE, sym, NULL )
		else
			'' Emitted other globals normally
			hAllocGlobalVar( sym )
		end if

	case 2
		'' Emit allocated externs
		if( symbGetAttrib( sym ) and (FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_PUBLIC) ) then
			hAllocGlobalVar( sym )
		end if

	end select
end sub

private sub hMaybeEmitGlobalVarExceptDataStmt( byval sym as FBSYMBOL ptr )
	'' DATA descriptor? Handled by irForEachDataStmt()
	if( symbIsDataDesc( sym ) ) then
		exit sub
	end if
	hMaybeEmitGlobalVar( sym )
end sub

private sub hMaybeEmitProcProto( byval s as FBSYMBOL ptr )
	dim as integer section = any

	'' Empty module bodies are discarded before the final prototype pass.
	'' Their constructor status records startup intent, not an emitted body.
	if( symbIsPrivate( s ) andalso symbGetIsParsed( s ) andalso _
	    symbGetIsModLevelProc( s ) andalso _
	    (symbGetProcIsEmitted( s ) = FALSE) ) then
		exit sub
	end if

	'' Public definitions may only be called from another module. Declare each
	'' emitted body without adding unused extern declarations to the header.
	if( symbGetIsFuncPtr( s ) or _
	    ((not symbGetIsAccessed( s )) and (not symbGetProcIsEmitted( s ))) ) then
		exit sub
	end if

	if( symbGetMangledName( s ) = NULL ) then
		exit sub
	end if

	'' Clang already knows this intrinsic's uint64_t prototype. Its C spelling
	'' is unsigned long on Linux but unsigned long long on Darwin/OpenBSD,
	'' even though all three have LP64 data models. Use Clang's declaration
	'' rather than guessing from the target word size; BASIC keeps ULONGINT.
	if( irUsesClangCCompiler() andalso _
	    (not symbGetIsParsed( s )) andalso _
	    (symbGetProcMode( s ) = FB_FUNCMODE_CDECL) andalso _
	    (s->id.alias <> NULL) ) then
		if( *s->id.alias = "__builtin_bswap64" ) then
			exit sub
		end if
	end if

	'' All procedure declarations go into the toplevel header
	section = sectionGosub( 0 )

	var prototype = hEmitProcHeader( s, EMITPROC_ISPROTO ) + ";"
	var first = ctx.sections(0).bufferlen
	hWriteLine( prototype )
	hCLeafPrototype( s, first )

	sectionReturn( section )
end sub

private sub hEmitFieldTypes( byval udt as FBSYMBOL ptr )
	dim as FBSYMBOL ptr fld = any
	fld = symbUdtGetFirstField( udt )
	while( fld )
		hEmitUDT( symbGetSubtype( fld ), typeIsPtr( symbGetType( fld ) ) )
		fld = symbUdtGetNextField( fld )
	wend
end sub

private function hFindParentAnonAlreadyOnStack _
	( _
		byval fld as FBSYMBOL ptr _
	) as FBSYMBOL ptr ptr

	dim as FBSYMBOL ptr ptr anonnode = any
	dim as FBSYMBOL ptr parent = any

	'' For each parent, starting with the inner-most...
	parent = fld->parent
	do
		'' Check whether it's already on the stack...
		anonnode = listGetTail( @ctx.anonstack )
		while( anonnode )
			if( *anonnode = parent ) then
				return anonnode
			end if
			anonnode = listGetPrev( anonnode )
		wend

		parent = parent->parent
	loop while( parent )

	function = NULL
end function

private sub hPushAnonParents _
	( _
		byval baseparent as FBSYMBOL ptr, _
		byval parent as FBSYMBOL ptr _
	)

	if( parent = baseparent ) then
		exit sub
	end if

	'' Recurse
	hPushAnonParents( baseparent, parent->parent )

	'' Push parents in top-down order
	assert( symbIsStruct( parent ) )
	if( symbGetUDTIsUnion( parent ) ) then
		hWriteLine( "union {", TRUE )
	else
		hWriteLine( "struct {", TRUE )
	end if
	sectionIndent( )
	*cptr( FBSYMBOL ptr ptr, listNewNode( @ctx.anonstack ) ) = parent

end sub

private sub hPopAnonParents( byval anonnode as FBSYMBOL ptr ptr )
	while( listGetTail( @ctx.anonstack ) <> anonnode )
		sectionUnindent( )
		hWriteLine( "};", TRUE )
		listDelNode( @ctx.anonstack, listGetTail( @ctx.anonstack ) )
	wend
end sub

private function hFieldNeedsRuntimeAlignment _
	( _
		byval fld as FBSYMBOL ptr _
	) as integer

	if( typeIsDynamicString( typeGetDtAndPtrOnly( symbGetType( fld ) ) ) ) then
		return TRUE
	end if

	if( (symbGetAttrib( fld ) and FB_SYMBATTRIB_DESCRIPTOR) <> 0 ) then
		return TRUE
	end if

	function = FALSE
end function

private sub hEmitStructWithFields( byval s as FBSYMBOL ptr )
	dim as integer skip = any, dtype = any, align = any
	dim as FBSYMBOL ptr subtype = any, fld = any
	dim as FBSYMBOL ptr ptr anonnode = any
	dim as DZSTRING ln
	DZstrZero( ln )

	'' Write out the fields
	fld = symbUdtGetFirstField( s )
	while( fld )

		if( fld->parent = s ) then
			'' Field from main UDT
			hPopAnonParents( NULL )
		else
			'' Field from a nested anonymous union/struct.
			'' Check the stack to decide whether we have to start
			'' nesting further, or instead go upwards, or stay at
			'' the current level.

			'' Find the field's inner-most parent that's already on
			'' stack, if any.
			anonnode = hFindParentAnonAlreadyOnStack( fld )

			'' a) Pop the stack until we reach the proper level,
			''    or stay at the current level.
			'' b) Reset the stack to the main UDT's level
			hPopAnonParents( anonnode )

			'' a) Push any parents that are inside the one that's on stack
			'' b) Push each new nested anon struct/union
			hPushAnonParents( iif( anonnode, *anonnode, s ), fld->parent )
		end if

		'' Don't emit fake dynamic array fields
		skip = symbIsDynamic( fld )

		if( skip = FALSE ) then
			'' Preserve pointee qualifiers while leaving the field itself writable
			'' for BASIC initialization and constructor assignments.
			dtype = symbGetType( fld ) or typeGetConstMask( symbGetFullType( fld ) )
			dtype = typeUnsetIsConst( dtype )
			subtype = symbGetSubtype( fld )
			DZstrAssign( ln, hEmitType( dtype, subtype, TRUE ) )
			DZstrConcatAssign( ln, " " + hGetCFieldId( fld ) )
			DZstrConcatAssign( ln, hEmitArrayDecl( fld ) )

			'' Field alignment (FIELD = N)?
			align = symbGetUDTAlign( s )
			if( env.clopt.target = FB_COMPTARGET_RISCOS ) then
				'' The enclosing RISC OS structure is packed to remove GCC's
				'' four-byte aggregate minimum. Restore each field's effective
				'' FreeBASIC alignment explicitly inside that packed body.
				var effectivealign = typeCalcNaturalAlign( dtype, subtype )
				if( (hFieldNeedsRuntimeAlignment( fld ) = FALSE) andalso _
				    (align > 0) andalso (align < effectivealign) ) then
					effectivealign = align
				end if
				DZstrConcatAssign( ln, _
					" __attribute__((aligned(" + str( effectivealign ) + ")))" )
			elseif( align > 0 ) then
				'' The aligned(N) attribute alone increases the alignment,
				'' together with packed it decreases it.
				'' FIELD = N in FB only decreases alignment, but never increases it.
				skip = (align >= typeCalcNaturalAlign( dtype, subtype ))
				skip or= hFieldNeedsRuntimeAlignment( fld )

				'' Don't add unnecessary attributes on nested structures
				'' that are already packed to the same alignment,
				'' gcc would show a warning in that case.
				if( typeGet( dtype ) = FB_DATATYPE_STRUCT ) then
					var fieldudtalign = symbGetUDTAlign( subtype )
					if( fieldudtalign > 0 ) then
						skip or= (align >= fieldudtalign)
					end if
				end if

				if( skip = FALSE ) then
					DZstrConcatAssign( ln, _
						" __attribute__((packed, aligned(" + str( align ) + ")))" )
				end if
			end if

			'' The alignment of nested structures which are packed with a
			'' smaller alignment than the natural or specified alignment of the
			'' parent structure has to be specified explicitly,
			'' otherwise the field will be packed too.
			if ( align <= 0 orelse skip ) then
				if( typeGet( dtype ) = FB_DATATYPE_STRUCT ) then
					var effectivealign = typeCalcNaturalAlign( dtype, subtype )
					if ( align > 0 andalso align < effectivealign ) then
						effectivealign = align
					end if
					var fieldudtalign = symbGetUDTAlign( subtype )
					if( fieldudtalign > 0 andalso effectivealign > fieldudtalign ) then
						DZstrConcatAssign( ln, _
							" __attribute__((aligned(" + str( effectivealign ) + ")))" )
					end if
				end if
			end if

			DZstrConcatAssign( ln, ";" )
			hWriteLine( *ln.data, TRUE )
		end if

		fld = symbUdtGetNextField( fld )
	wend

	'' Close any remaining nested anonymous structs/unions
	hPopAnonParents( NULL )

	assert( listGetHead( @ctx.anonstack ) = NULL )
	DZstrAllocate( ln, 0 )
end sub

private sub hEmitStruct( byval s as FBSYMBOL ptr, byval is_ptr as integer )
	dim as integer emit_fields = any
	dim as string ln

	'' Already in the process of emitting this UDT?
	if( symbGetIsBeingEmitted( s ) ) then
		'' This means there is a circular dependency with another UDT.
		'' One of the references can be a pointer only though,
		'' because UDTs cannot contain each-other, so this can always
		'' be solved by using a forward reference.
		if( is_ptr ) then
			'' Emit a forward reference for this struct (if not yet done).
			'' The symbol's accessed bit prevents emitting this struct reference twice.
			if( symbGetIsAccessed( s ) = FALSE ) then
				symbSetIsAccessed( s )
				hWriteLine( hGetUdtName( s ) + ";" )
			end if
			exit sub
		end if
	end if

	symbSetIsBeingEmitted( s )
	'' GCC can drop a lowered alignment on an incomplete record typedef.
	'' Keep recursive field declarations natural and create pointee views only
	'' after the full record has been emitted. These aliases do not own layout.
	c_pointer_raw_types += 1

	'' Should we emit the fields individually (for better debug info), or
	'' just use a byte array as structure body (to avoid having to worry
	'' about any layout issues)?
	''
	'' (Technically we don't need to emit fields explicitly, since we don't
	'' access them by name, but only by offset as calculated by FB.)
	''
	'' Currently we prefer emitting fields explicitly. Bitfields are the
	'' only case where it's currently not possible, because FB's bitfields
	'' are currently not ABI-compatible to GCC's.
	'' Future work: emit ABI-compatible C bitfields so field-level debug information
	'' remains available for UDTs which contain bitfield members.
	'' This sacrifices field-level debug information for bitfield members.
	emit_fields = not symbGetUdtHasBitfield( s )

	if( emit_fields ) then
		hEmitFieldTypes( s )
	end if

	'' Has this UDT been emitted in the mean time?
	'' (due to one of the fields causing a circular dependency)
	if( symbGetIsEmitted( s ) ) then
		c_pointer_raw_types -= 1
		exit sub
	end if

	'' Emit it now
	symbSetIsEmitted( s )

	'' Header: struct|union [attributes...] id {
	ln = hGetUdtTag( s )

	'' Work-around mingw32 gcc bug 52991; packing is broken for ms_struct
	'' stucts, which is the default under -mms-bitfields, which is on by
	'' default in mingw32 gcc 4.7.
	if( (env.clopt.target = FB_COMPTARGET_WIN32) and _
	    (symbGetUDTAlign( s ) > 0) ) then
		ln += "__attribute__((gcc_struct)) "
	end if

	ln += hGetUdtId( s )
	ln += " {"
	hWriteLine( ln, TRUE )
	sectionIndent( )

	if( emit_fields ) then
		hEmitStructWithFields( s )
	else
		hWriteLine( "uint8 __fb_struct_body[" & symbGetSizeOf( s ) & "];", TRUE )
	end if

	'' Close the UDT body. Give RISC OS aggregates the effective alignment
	'' already used for FreeBASIC's size calculation. The packed attribute
	'' removes GCC's otherwise unavoidable four-byte aggregate minimum.
	sectionUnindent( )
	if( env.clopt.target = FB_COMPTARGET_RISCOS ) then
		var effectivealign = s->udt.natalign
		if( (symbGetUDTAlign( s ) > 0) andalso _
		    (symbGetUDTAlign( s ) < effectivealign) ) then
			effectivealign = symbGetUDTAlign( s )
		end if
		hWriteLine( "} __attribute__((packed, aligned(" + _
		            str( effectivealign ) + ")));", TRUE )
	else
		hWriteLine( "};", TRUE )
	end if

	c_pointer_raw_types -= 1
	symbResetIsBeingEmitted( s )
	hCRememberViewType(FB_DATATYPE_STRUCT, s)

	'' Static assertion to ensure the struct has been emitted correctly,
	'' at least with the correct sizeof(), because if it'd be too small,
	'' that could easily cause stack trashing etc., because local vars
	'' allocated by gcc would be smaller than expected, etc.
	hWriteStaticAssert( "sizeof( " + hGetUdtTag( s ) + hGetUdtId( s ) + " ) == " + str( culngint( symbGetSizeOf( s ) ) ) )
end sub

private sub hWriteX86F2I _
	( _
		byref fname as string, _
		byval rtype as integer, _
		byval ptype as integer _
	)

	var rtype_str = hEmitType( rtype, NULL )
	var ptype_str = hEmitType( ptype, NULL )

	dim as string rtype_suffix, ptype_suffix
	if( env.clopt.asmsyntax = FB_ASMSYNTAX_ATT ) then
		select case( typeGetSize( rtype ) )
		case 4
			rtype_suffix = "l"
		case 8
			rtype_suffix = "q"
		end select
		if( ptype = FB_DATATYPE_SINGLE ) then
			ptype_suffix = "s"
		else
			ptype_suffix = "l"
		end if
	else
		select case( typeGetSize( rtype ) )
		case 4
			rtype_suffix = " dword ptr"
		case 8
			rtype_suffix = " qword ptr"
		end select
		if( ptype = FB_DATATYPE_SINGLE ) then
			ptype_suffix = " dword ptr"
		else
			ptype_suffix = " qword ptr"
		end if
	end if

	hWriteLine( "", TRUE )
	hWriteLine( "static inline " + rtype_str + " fb_" + fname +  "( " + ptype_str + " value )", TRUE )
	hWriteLine( "{", TRUE )
	sectionIndent( )
		hWriteLine( "volatile " + rtype_str + " result;", TRUE )
		hWriteLine( "__asm__(", TRUE )
		sectionIndent( )
			hWriteLine( """fld" + ptype_suffix + " %1;"""  , TRUE )
			hWriteLine( """fistp" + rtype_suffix + " %0;""", TRUE )
			hWriteLine( ":""=m"" (result)", TRUE )
			hWriteLine( ":""m"" (value)"  , TRUE )
			hWriteLine( ":""st"""  , TRUE )
		sectionUnindent( )
		hWriteLine( ");", TRUE )
		hWriteLine( "return result;", TRUE )
	sectionUnindent( )
	hWriteLine( "}", TRUE )

end sub

private sub hWriteGenericF2I _
	( _
		byref fname as string, _
		byval rtype as integer, _
		byval ptype as integer _
	)

	dim as string callname
	if( ptype = FB_DATATYPE_SINGLE ) then
		callname = "nearbyintf"
	else
		callname = "nearbyint"
	end if

	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_RISCOS ) then
		'' GCC 4.7's nearbyint expansion loses exact odd integers at the
		'' APCS precision boundary. Round through UnixLib floor() without
		'' the large floating-point bias used by nearbyint().
		hWriteLine( "double floor( double );", TRUE )
		hWriteLine( "static inline " + hEmitType( rtype, NULL ) + _
			" fb_" + fname + "( " + hEmitType( ptype, NULL ) + " value )", TRUE )
		hWriteLine( "{", TRUE )
		sectionIndent( )
			hWriteLine( "double rounded = floor( (double)value );", TRUE )
			hWriteLine( "double fraction = (double)value - rounded;", TRUE )
			hWriteLine( "if( fraction > 0.5 )", TRUE )
			sectionIndent( )
				hWriteLine( "rounded += 1.0;", TRUE )
			sectionUnindent( )
			hWriteLine( "else if( fraction == 0.5 ) {", TRUE )
			sectionIndent( )
				hWriteLine( "if( rounded >= 0.0 ) {", TRUE )
				sectionIndent( )
					hWriteLine( "if( ((uint64)rounded & 1ull) != 0 ) rounded += 1.0;", TRUE )
				sectionUnindent( )
				hWriteLine( "} else {", TRUE )
				sectionIndent( )
					hWriteLine( "if( ((int64)rounded & 1ll) != 0 ) rounded += 1.0;", TRUE )
				sectionUnindent( )
				hWriteLine( "}", TRUE )
			sectionUnindent( )
			hWriteLine( "}", TRUE )
			hWriteLine( "return (" + hEmitType( rtype, NULL ) + ")rounded;", TRUE )
		sectionUnindent( )
		hWriteLine( "}", TRUE )
	elseif( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_NUTTX ) then
		''
		'' The NuttX loadable-module path supplies small local math helpers
		'' from the FreeBASIC runtime. Calling them by name keeps generated
		'' code away from target libm/fenv paths that may not be module-safe.
		hWriteLine( hEmitType( ptype, NULL ) + " " + callname + "( " + hEmitType( ptype, NULL ) + " );", TRUE )
		hWriteLine( "#define fb_" + fname +  "( value ) ((" + hEmitType( rtype, NULL ) + ")" + callname + "( value ))", TRUE )
	elseif( (rtype = FB_DATATYPE_INTEGER128) or (rtype = FB_DATATYPE_UINTEGER128) ) then
		'' Native 128-bit types have a separate C extension and ABI contract.
		'' Keep their existing single-cast form outside the 32/64-bit helpers.
		hWriteLine( "#define fb_" + fname +  "( value ) ((" + hEmitType( rtype, NULL ) + ")__builtin_" + callname + "( value ))", TRUE )
	else
		'' Keep the floating result typed until the integer conversion.
		'' The input is evaluated once; nearbyint retains the active rounding mode.
		hWriteLine( "static inline " + hEmitType( rtype, NULL ) + _
			" fb_" + fname + "( " + hEmitType( ptype, NULL ) + " value )", TRUE )
		hWriteLine( "{", TRUE )
		sectionIndent( )
			hWriteLine( hEmitType( ptype, NULL ) + " rounded = __builtin_" + callname + "( value );", TRUE )
			hWriteLine( "return (" + hEmitType( rtype, NULL ) + ")rounded;", TRUE )
		sectionUnindent( )
		hWriteLine( "}", TRUE )
	end if

end sub

private sub hWriteF2I _
	( _
		byref fname as string, _
		byval rtype as integer, _
		byval ptype as integer _
	)

	'' We have inline ASM routines for some cases
	if( fbGetCpuFamily( ) = FB_CPUFAMILY_X86 ) then
		select case( rtype )
		case FB_DATATYPE_LONG, FB_DATATYPE_LONGINT
			hWriteX86F2I( fname, rtype, ptype )
			exit sub
		end select
	end if

	hWriteGenericF2I( fname, rtype, ptype )

end sub

private sub hMaybeEmitProcExport( byval proc as FBSYMBOL ptr )
	if( symbIsExport( proc ) = FALSE ) then
		exit sub
	end if

	'' Code we want in the final ASM file:
	''
	''  .section .drectve
	''  .ascii " -export:<escaped mangled procedure name>"
	''
	'' Since that includes double-quotes and backslashes we need to do
	'' lots of escaping when emitting this in strings in GCC inline ASM.

	ctx.exports += !"\t"""
	ctx.exports += $"\t.ascii "
	'' The escaped quote after -export: is assembler text, not a drive path.
	ctx.exports += $"\"" -export:" + _
		$"\\\"""
	ctx.exports += hGetMangledNameForASM( proc, FALSE )
	ctx.exports += $"\\\""\"""
	ctx.exports += $"\n"
	ctx.exports += !"""\n"
end sub

private function _emitBegin( ) as integer
	hCReadOnlyLocalsInit()
	hCEvaluatedValuesInit()
	hCLiteralStorageInit()
	if( hFileExists( env.outf.name ) ) then
		if( kill( env.outf.name ) <> 0 ) then
			return FALSE
		end if
	end if

	env.outf.num = freefile
	if( open( env.outf.name, for binary, access read write, as #env.outf.num ) <> 0 ) then
		return FALSE
	end if

	hCLeafInit( )
	hCAbiInit( )
	hCReadOnlyInit()
	hCUnobservedInit()
	hCClearPointerViews(0)
	c_pointer_view_id = 0
	c_pointer_raw_types = 0
	'' Declaration metadata belongs to one C output; retain only list capacity.
	do while( listGetHead(@ctx.alignmentviews) <> NULL )
		listDelNode(@ctx.alignmentviews, listGetHead(@ctx.alignmentviews))
	loop
	hClearArrayCallbacks( )
	ctx.arraycallbackid = 0
	hClearGfxBlenders( )
	ctx.gfxblenderid = 0
	ctx.gfxcontextid = 0
	ctx.section = -1
	ctx.sectiongosublevel = 0
	ctx.linenum = 0
	ctx.usedbuiltins = 0
	ctx.unalignedid = 0
	ctx.localassertid = 0
	ctx.codeptrid = 0
	ctx.varinisym = NULL
	ctx.globalvarpass = 0
	hUpdateCurrentFileName( env.inf.name )

	'' header
	sectionBegin( )

	if( env.clopt.debuginfo ) then
		_emitDBG( AST_OP_DBG_LINEINI, NULL, 0 )
	end if

	hWriteLine( "typedef   signed char       int8;", TRUE )
	hWriteLine( "typedef unsigned char      uint8;", TRUE )
	hWriteLine( "typedef   signed short      int16;", TRUE )
	hWriteLine( "typedef unsigned short     uint16;", TRUE )
	hWriteLine( "typedef   signed int        int32;", TRUE )
	hWriteLine( "typedef unsigned int       uint32;", TRUE )
	hWriteLine( "typedef   signed long long  int64;", TRUE )
	hWriteLine( "typedef unsigned long long uint64;", TRUE )
	if( fbGetBits( ) = 32 ) then
		'' Use the shared two-word runtime representation instead of relying on
		'' native __int128 support. Match target layout, including the smaller
		'' alignment required by Windows CE and classic m68k object formats.
		hWriteLine( "typedef struct __attribute__((aligned(" + _
		            str( typeCalcNaturalAlign( FB_DATATYPE_INTEGER128, NULL ) ) + _
		            "))) { uint64 low; uint64 high; } FB_INT128;", TRUE )
	end if
	if( fbIs64bit( ) ) then
		hWriteLine( "typedef struct { char *data; int64 len; int64 size; } FBSTRING;", TRUE )
	else
		hWriteLine( "typedef struct { char *data; int32 len; int32 size; } FBSTRING;", TRUE )
	end if
	hCRememberViewType(FB_DATATYPE_STRING, NULL)
	hWriteLine( "typedef int8 boolean;", TRUE )
	hWriteLine( "extern long double fb_LongDoubleToNative(const void *, unsigned int);", TRUE )

	'' body
	sectionBegin( )

	function = TRUE
end function

private sub hWriteFloatEqual( byref suffix as string, byval dtype as integer )
	var typename = hEmitType( dtype, NULL )
	hWriteLine( "static inline int fb_c_" + suffix + "_equal(" + typename + " left, " + typename + " right)", TRUE )
	hWriteLine( "{", TRUE )
	'' Exact BASIC equality is quiet for quiet NaNs and equates signed zero.
	'' These type-generic predicates preserve that unordered behavior. The
	'' typed arguments evaluate each operand once, including volatile reads.
	'' A pair of ordinary ordered comparisons would raise on quiet NaNs.
	hWriteLine( "return !__builtin_islessgreater(left, right) && !__builtin_isunordered(left, right);", TRUE )
	hWriteLine( "}", TRUE )
end sub

private sub _emitEnd( )
	hAsmEndNames( )
	dim as integer section = any
	dim as integer output_error = FALSE

	hUpdateCurrentFileName( env.inf.name )

	'' Switch to header section temporarily
	section = sectionGosub( 0 )

	if( fbGetOption( FB_COMPOPT_MODEVIEW ) = FB_MODEVIEW_GUI ) then
		hWriteLine( "#if defined(__GNUC__)", TRUE )
		hWriteLine( "__attribute__((weak)) const int __fb_app_mode_gui_enabled = 1;", TRUE )
		hWriteLine( "#else", TRUE )
		hWriteLine( "const int __fb_app_mode_gui_enabled = 1;", TRUE )
		hWriteLine( "#endif", TRUE )
	end if

	if( ctx.usedbuiltins and BUILTIN_F2I  ) then hWriteF2I( "F2I" , FB_DATATYPE_LONG    , FB_DATATYPE_SINGLE )
	if( ctx.usedbuiltins and BUILTIN_F2L  ) then hWriteF2I( "F2L" , FB_DATATYPE_LONGINT , FB_DATATYPE_SINGLE )
	if( ctx.usedbuiltins and BUILTIN_F2UL ) then hWriteF2I( "F2UL", FB_DATATYPE_ULONGINT, FB_DATATYPE_SINGLE )
	if( ctx.usedbuiltins and BUILTIN_D2I  ) then hWriteF2I( "D2I" , FB_DATATYPE_LONG    , FB_DATATYPE_DOUBLE )
	if( ctx.usedbuiltins and BUILTIN_D2L  ) then hWriteF2I( "D2L" , FB_DATATYPE_LONGINT , FB_DATATYPE_DOUBLE )
	if( ctx.usedbuiltins and BUILTIN_D2UL ) then hWriteF2I( "D2UL", FB_DATATYPE_ULONGINT, FB_DATATYPE_DOUBLE )
	if( ctx.usedbuiltins and BUILTIN_F2I128 ) then hWriteF2I( "F2I128", FB_DATATYPE_INTEGER128, FB_DATATYPE_SINGLE )
	if( ctx.usedbuiltins and BUILTIN_F2U128 ) then hWriteF2I( "F2U128", FB_DATATYPE_UINTEGER128, FB_DATATYPE_SINGLE )
	if( ctx.usedbuiltins and BUILTIN_D2I128 ) then hWriteF2I( "D2I128", FB_DATATYPE_INTEGER128, FB_DATATYPE_DOUBLE )
	if( ctx.usedbuiltins and BUILTIN_D2U128 ) then hWriteF2I( "D2U128", FB_DATATYPE_UINTEGER128, FB_DATATYPE_DOUBLE )
	if( ctx.usedbuiltins and BUILTIN_F32_EQUAL ) then hWriteFloatEqual( "f32", FB_DATATYPE_SINGLE )
	if( ctx.usedbuiltins and BUILTIN_F64_EQUAL ) then hWriteFloatEqual( "f64", FB_DATATYPE_DOUBLE )

	'' Append global declarations to the header of the toplevel section.
	'' This must be done during _emitEnd() instead of _emitBegin() because
	'' _emitBegin() is called even before any input code is parsed.

	'' Emit procedure prototypes first (because of function pointer
	'' initializers taking the address of procedures)
	symbForEachGlobal( FB_SYMBCLASS_PROC, @hMaybeEmitProcProto )
	'' Callback bodies follow the original typed procedure prototypes.
	hEmitArrayCallbackDefinitions( )
	hEmitGfxBlenderDefinitions( )

	''
	'' Emit global vars based on the order they were added to the global
	'' symbol table. It's the order they were found in the FB source code,
	'' and any initializers were only allowed to reference already declared
	'' variables, so we should never run into a case where a variable
	'' initializer references a global that wasn't declared yet.
	''
	'' Tricky case: Externs can be split into 2 declarations in the FB code:
	''        extern p as integer ptr
	''        dim shared x as integer
	''        dim shared p as integer ptr = @x
	'' essentially becomes
	''        public dim shared p as integer ptr = @x
	''        dim shared x as integer
	'' I.e. initializers of PUBLIC may have forward references.
	''
	'' To work around this, we must emit globals in 2 passes, essentially
	'' undoing the merging of extern prototype and allocation.
	''
	'' Special case: DATA descriptors, which must be emitted manually using
	'' their own list, to ensure they're emitted in the right order, because
	'' their internal initializers can reference each-other in different
	'' order than the declaration order.
	''

	hCGlobalStoragePrepare()
	ctx.globalvarpass = 1
	symbForEachGlobal( FB_SYMBCLASS_VAR, @hMaybeEmitGlobalVarExceptDataStmt )
	irForEachDataStmt( @hMaybeEmitGlobalVar )

	ctx.globalvarpass = 2
	symbForEachGlobal( FB_SYMBCLASS_VAR, @hMaybeEmitGlobalVarExceptDataStmt )
	irForEachDataStmt( @hMaybeEmitGlobalVar )

	sectionReturn( section )

	'' even though we are putting this asm section at the end of the emitted C file
	'' gcc might move it around when compiling the C lising and the ASM block ends up
	'' (typically?) at the beginning of the C source.  Appears to be a change in if
	'' gcc locates this before or after the first '.text. directive when it writes out
	'' the ASM listing from the C listing: somewhere between gcc 7.1 and gcc 7.4.
	'' Explicity close the .drectve section by switching to .text section and presume
	'' that it comensates for the gcc'a choice of section location.
	'' DLL export table
	if( env.clopt.export and (env.target.options and FB_TARGETOPT_EXPORT) ) then
		symbForEachGlobal( FB_SYMBCLASS_PROC, @hMaybeEmitProcExport )
		if( len( ctx.exports ) > 0 ) then
			hWriteLine( !"\n__asm__( \n\t\".section .drectve\\n\"\n" + ctx.exports + ");", TRUE )
			hWriteLine( "__asm__( "".text"" );" )
		end if
		ctx.exports = ""
	end if

	'' Global initializer addresses are roots too. Close the private graph
	'' before deciding which deferred ABI entry points need definitions.
	hCLeafReachable()
	hCReadOnlyWritePrivateWrappers()
	hCLeafPrune( )
	hCLeafFinish( )

	'' body (is appended to header section)
	sectionEnd( )

	'' Emit & close the main section
	if( (ctx.sections(0).old = FALSE) andalso _
	    (ctx.sections(0).bufferlen > 0) ) then
		hCLiteralStoragePrepare(ctx.sections(0).buffer, ctx.sections(0).bufferlen)
		if( hCAbiWriteOutput(ctx.sections(0).buffer, ctx.sections(0).bufferlen, c_leaf_opaque) = FALSE ) then
			output_error = TRUE
		end if
	end if
	hCAbiFinish( )
	hCReadOnlyFinish()
	hCUnobservedFinish()
	hCReadOnlyLocalsFinish()
	hCLiteralStorageFinish()
	sectionEnd( )

	if( close( #env.outf.num ) <> 0 ) then
		output_error = TRUE
	end if
	env.outf.num = 0
	if( output_error ) then
		errReportEx( FB_ERRMSG_FILEACCESSERROR, env.outf.name, -1 )
	end if

	assert( ctx.sectiongosublevel = 0 )
	assert( ctx.section = -1 )

	assert( listGetHead( @ctx.exprcache ) = NULL )
	assert( listGetHead( @ctx.exprnodes ) = NULL )

end sub

'':::::
private function _getOptionValue _
	( _
		byval opt as IR_OPTIONVALUE _
	) as integer

	select case opt
	case IR_OPTIONVALUE_MAXMEMBLOCKLEN
		return 0

	case else
		errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )

	end select

end function

private function _supportsOp _
	( _
		byval op as integer, _
		byval dtype as integer _
	) as integer
	'' Only these aren't available as either C ops or __builtin_*'s
	select case as const( op )
	case AST_OP_SGN, AST_OP_FIX, AST_OP_FRAC, AST_OP_RSQRT, AST_OP_RCP
		function = FALSE
	case else
		function = TRUE
	end select
end function

private sub _procBegin( byval proc as FBSYMBOL ptr )
	proc->proc.ext->dbg.iniline = lexLineNum( )
end sub

private sub _procEnd( byval proc as FBSYMBOL ptr )
	proc->proc.ext->dbg.endline = lexLineNum( )
end sub

private sub _scopeBegin( byval s as FBSYMBOL ptr )
end sub

private sub _scopeEnd( byval s as FBSYMBOL ptr )
end sub

private function hIsStaticWithDtor( byval sym as FBSYMBOL ptr ) as integer
	function = symbIsStatic( sym ) and (not symbIsRef( sym )) and symbHasDtor( sym )
end function

private sub _procAllocStaticVars( byval sym as FBSYMBOL ptr )
	dim as FBSYMBOL ptr desc = any
	dim as integer section = any

	''
	'' Emit all statics with dtor into the toplevel header section,
	'' so their dtor wrappers can see them.
	''
	'' This can't be done for all statics, since they can use local UDTs,
	'' and emitting those as globals too would be hard. For static with
	'' dtors though we can be sure they're not using local UDTs, because
	'' UDTs with dtors aren't allowed inside scopes.
	''

	section = sectionGosub( 0 )

	while( sym )
		select case( symbGetClass( sym ) )
		'' scope block? recursion..
		case FB_SYMBCLASS_SCOPE
			_procAllocStaticVars( symbGetScopeSymbTbHead( sym ) )

		'' variable?
		case FB_SYMBCLASS_VAR
			'' static with dtor?
			if( hIsStaticWithDtor( sym ) ) then
				hMaybeEmitLocalVar( sym )

				''
				'' Check whether it's a dynamic array with a corresponding
				'' descriptor that needs to be emitted instead.
				'' (it won't be detected by above check itself,
				'' as it's of FB_ARRAYDESC type)
				''
				'' It's the descriptor that matters for dynamic
				'' arrays - the dynamic array symbol itself is
				'' not even emitted by hMaybeEmitLocalVar().
				''
				'' Note that for static locals the descriptor and the
				'' descriptor UDT will be local too, but since we're
				'' emitting to the toplevel section, the descriptor
				'' will end up there, and hEmitUDT() isn't allowed
				'' to emit the descriptor UDT locally.
				'' (this way we force it to be emitted globally)
				''
				desc = symbGetArrayDescriptor( sym )
				if( desc ) then
					hMaybeEmitLocalVar( desc )
				end if
			end if
		end select

		sym = symbGetNext( sym )
	wend

	sectionReturn( section )
end sub

private sub _setVregDataType _
	( _
		byval vreg as IRVREG ptr, _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr _
	)

	if( vreg <> NULL ) then
		vreg->dtype = dtype
		vreg->subtype = subtype
	end if

end sub

private function hEmitType _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval emit_const as integer _
	) as string

	dim as string s
	dim as integer ptrcount = any

	'' replace type with __builtin_va_list if
	'' the mangle modifier was given
	if( symbIsBuiltinVaListType( dtype, subtype ) ) then
		select case symbGetValistType( dtype, subtype )
		case FB_CVA_LIST_BUILTIN_POINTER, FB_CVA_LIST_BUILTIN_VOID_POINTER
			dtype = typeJoinDtOnly( typeDeref(dtype), FB_DATATYPE_VA_LIST )
		case else
			dtype = typeJoinDtOnly( dtype, FB_DATATYPE_VA_LIST )
		end select
	end if
	ptrcount = typeGetPtrCnt( dtype )

	select case as const( typeGetDtOnly( dtype ) )
	case FB_DATATYPE_INTEGER128, FB_DATATYPE_UINTEGER128
		if( fbGetBits( ) = 32 ) then
			s = "FB_INT128"
		else
			s = *dtypeName(typeGetDtOnly( dtype ))
		end if

	case FB_DATATYPE_STRUCT, FB_DATATYPE_ENUM
		if( subtype ) then
			hEmitUDT( subtype, (ptrcount > 0) )
			s = hGetUdtName( subtype )
		elseif( typeGetDtOnly( dtype ) = FB_DATATYPE_ENUM ) then
			s = *dtypeName(typeGetRemapType( typeGetDtOnly( dtype ) ))
		else
			s = *dtypeName(FB_DATATYPE_VOID)
		end if

	case FB_DATATYPE_FUNCTION
		assert( ptrcount > 0 )
		ptrcount -= 1
		hEmitUDT( subtype, (ptrcount > 0) )
		s = hGetCId( subtype )

	case FB_DATATYPE_WCHAR
		'' Emit ubyte/ushort/uinteger instead of wchar_t
		s = *dtypeName(typeGetRemapType( typeGetDtOnly( dtype ) ))

	case FB_DATATYPE_FIXSTR
		'' Ditto (but typeGetRemapType() returns FB_DATATYPE_FIXSTR,
		'' so do it manually)
		s = *dtypeName(FB_DATATYPE_UBYTE)

	case else
		select case typeGetMangleDt( dtype )
		case FB_DATATYPE_CHAR
			s = "char"
		case FB_DATATYPE_BOOLEAN
			s = "_Bool"
		case FB_DATATYPE_INTEGER
			s = "long"
		case FB_DATATYPE_UINT
			s = "unsigned long"
		case else
			s = *dtypeName(typeGetDtOnly( dtype ))
		end select
	end select

	if( emit_const andalso typeIsConstAt( dtype, ptrcount ) ) then
		s = "const " + s
	end if

	if( ptrcount > 0 ) then
		var declsection = 0
		var useviews = (c_pointer_raw_types = 0) and (typeGetDtOnly(dtype) <> FB_DATATYPE_VA_LIST)
		if( useviews ) then declsection = hCPointerViewSection(dtype, subtype)
		for i as integer = ptrcount - 1 to 0 step -1
			if( useviews ) then
				'' Each extra indirection addresses a pointer-sized object too.
				'' Keep its real pointed-to type and all intervening const levels.
				var needsview = (i < ptrcount - 1) or (typeGetDtOnly(dtype) = FB_DATATYPE_FUNCTION)
				if( needsview = FALSE ) then
					needsview = (typeGetDtOnly(dtype) <> FB_DATATYPE_VOID) and (s <> "void") and (s <> "const void")
					if( typeGetSize(typeGetDtOnly(dtype)) = 1 ) then needsview = FALSE
				end if
				if( needsview ) then s = hCPointerViewName(s, declsection)
			end if
			s += "*"
			if( emit_const andalso typeIsConstAt( dtype, i ) ) then
				s += " const"
			end if
		next
	end if

	function = s
end function

#include once "backend/c/c-alignment-views.bi"
#include once "backend/c/c-pointer-views.bi"

private function exprNew _
	( _
		byval class_ as integer, _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr _
	) as EXPRNODE ptr

	dim as EXPRNODE ptr n = any

	n = listNewNode( @ctx.exprnodes )
	n->class = class_
	n->dtype = dtype
	n->subtype = subtype

	function = n
end function

private sub exprFreeNode( byval n as EXPRNODE ptr )
	if( (n->class = EXPRCLASS_TEXT) or (n->class = EXPRCLASS_MEMBER) ) then
		ZstrFree( n->text )
	end if
	listDelNode( @ctx.exprnodes, n )
end sub

private sub exprFreeTree( byval n as EXPRNODE ptr )
	if( n->l ) then
		exprFreeTree( n->l )
	end if
	if( n->r ) then
		exprFreeTree( n->r )
	end if
	exprFreeNode( n )
end sub

private function exprNewTEXT _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval s as const zstring ptr _
	) as EXPRNODE ptr

	dim as EXPRNODE ptr n = any

	n = exprNew( EXPRCLASS_TEXT, dtype, subtype )
	n->text = ZstrDup( s )

	function = n
end function

private function exprNewIMMi _
	( _
		byval i as longint, _
		byval dtype as integer = FB_DATATYPE_INTEGER _
	) as EXPRNODE ptr

	dim as EXPRNODE ptr n = any

	'' Integer literals can only be emitted as either 32bit int or 64bit long long,
	'' if other types are needed, an exprNewCAST() should be done afterwards.
	if( typeGetSize( dtype ) = 8 ) then
		dtype = iif( typeIsSigned( dtype ), FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT )
	else
		dtype = iif( typeIsSigned( dtype ), FB_DATATYPE_LONG, FB_DATATYPE_ULONG )
	end if

	n = exprNew( EXPRCLASS_IMM, dtype, NULL )
	n->val.i = i

	function = n
end function

private function exprNewIMMf _
	( _
		byval f as double, _
		byval dtype as integer _
	) as EXPRNODE ptr

	dim as EXPRNODE ptr n = any

	n = exprNew( EXPRCLASS_IMM, dtype, NULL )
	n->val.f = f

	function = n
end function

private function symbIsCArray( byval sym as FBSYMBOL ptr ) as integer
	'' No bydesc/byref, those are emitted as pointers...
	if( symbIsRef( sym ) or symbIsParamVarBydescOrByref( sym ) or symbIsImport( sym ) ) then
		return FALSE
	end if

	select case( symbGetClass( sym ) )
	case FB_SYMBCLASS_VAR, FB_SYMBCLASS_FIELD
		'' No dynamic arrays, they're just descriptor structs
		if( symbIsDynamic( sym ) ) then
			return FALSE
		end if

		if( symbGetArrayDimensions( sym ) <> 0 ) then
			return TRUE
		end if
	end select

	'' Fixed-length strings are emitted as arrays,
	'' string literals are emitted as string literals,
	'' both are pointers in C
	select case( symbGetType( sym ) )
	case FB_DATATYPE_FIXSTR, FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR
		return TRUE
	end select

	return FALSE
end function

private function exprNewCAST _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval l as EXPRNODE ptr _
	) as EXPRNODE ptr

	'' Array addresses are assembled after ADDROF, while combining the byte
	'' displacement. Recover their declared storage at this final pointer cast.
	if( typeIsPtr(dtype) andalso typeIsPtr(l->dtype) ) then
		var address = exprTryCRecordArrayAddress(dtype, subtype, l)
		if( address <> NULL ) then return address
	end if

	'' Don't add a CAST if l already has the desired type
	if( (dtype = l->dtype) and (subtype = l->subtype) ) then
		return l
	end if

	'' Don't cast if l has a compatible type (e.g. 32bit int vs. 32bit long)
	'' (same class, same size, same signedness, no pointers involved,
	'' both booleans or none). C char is distinct from BASIC's unsigned
	'' character value because plain char signedness depends on the target.
	if( (typeGetSizeType( l->dtype ) = typeGetSizeType( dtype )) and _
	    (not typeIsPtr( l->dtype )) and (not typeIsPtr( dtype )) and _
	    ((typeGetDtAndPtrOnly(l->dtype) = FB_DATATYPE_CHAR) = _
	     (typeGetDtAndPtrOnly(dtype) = FB_DATATYPE_CHAR)) ) then
		return l
	end if

	'' "(foo*)(bar*)"? Discard the bar* cast and cast only to foo*,
	'' pointers are pointers, such double casts are useless.
	if( l->class = EXPRCLASS_CAST ) then
		if( (typeGetPtrCnt( dtype ) > 0) and (typeGetPtrCnt( l->dtype ) > 0) ) then
			l->dtype = dtype
			l->subtype = subtype
			return l
		end if
	end if

	'' casting zstring to some other type? Try to force unsigned char by
	'' casting to unsigned char first.  If char is signed (in gcc) then
	'' the sign is extended and will give negative numbers when promoted
	'' which we probably don't want when converting ascii to integer.
	if( typeGetDtAndPtrOnly( l->dtype ) = FB_DATATYPE_CHAR ) then
		var n = exprNew( EXPRCLASS_CAST, FB_DATATYPE_UBYTE, NULL )
		n->l = l
		l = n
	end if

	var n = exprNew( EXPRCLASS_CAST, dtype, subtype )
	n->l = l
	function = n
end function

private function exprNewValueCAST _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval value as EXPRNODE ptr _
	) as EXPRNODE ptr

	'' Cached IR values have no addressable C binding. Keep a qualified
	'' object pointer qualified while it flows through arithmetic and other
	'' values; its eventual store or call still owns any mutable conversion.
	'' Nested pointers and changes of pointee type retain their usual casts.
	if( typeGetPtrCnt(dtype) = 1 andalso typeGetPtrCnt(value->dtype) = 1 ) then
		if( typeGetDtAndPtrOnly(dtype) = typeGetDtAndPtrOnly(value->dtype) andalso _
		    subtype = value->subtype andalso typeIsConstAt(value->dtype, 1) ) then
			if( typeGetDtOnly(dtype) <> FB_DATATYPE_FUNCTION ) then
				dtype = typeAddrOf(typeSetIsConst(typeDeref(dtype)))
			end if
		end if
	end if
	return exprNewCAST(dtype, subtype, value)
end function

private function exprNewMACRO _
	( _
		byval op as AST_OP, _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval l as EXPRNODE ptr, _
		byval r as EXPRNODE ptr _
	) as EXPRNODE ptr

	var n = exprNew( EXPRCLASS_MACRO, dtype, subtype )
	n->op = op
	n->l = l
	n->r = r
	function = n
end function

private function exprNewSYM( byval sym as FBSYMBOL ptr ) as EXPRNODE ptr
	dim as EXPRNODE ptr n = any
	dim as integer dtype = any
	dim as FBSYMBOL ptr subtype = any

	'' Dynamic array (fake dynamic array symbol)? Remap to array descriptor,
	'' since that's what it will be emitted as.
	if( symbIsVar( sym ) and symbIsDynamic( sym ) ) then
		sym = sym->var_.array.desc
		assert( sym )
	end if

	if( symbIsLabel( sym ) ) then
		'' &&label is a void* in GCC
		'' This is handled as a single SYM instead of ADDROF( SYM ),
		'' because a label is not a proper expression on its own.
		n = exprNew( EXPRCLASS_SYM, typeAddrOf( FB_DATATYPE_VOID ), NULL )
		n->sym = sym

	elseif( symbIsProc( sym ) ) then
		'' &proc
		'' Similar to labels above, this is only used to take the
		'' address of functions, not to call them, so the '&' is
		'' part of the SYM.
		n = exprNew( EXPRCLASS_SYM, typeAddrOf( FB_DATATYPE_FUNCTION ), sym )
		n->sym = sym

	elseif( (sym->stats and FB_SYMBSTATS_VTABLE) <> 0 ) then
		'' Compiler-owned tables have typed C record storage. Keep their ABI
		'' address opaque to the ordinary homogeneous-array index recovery.
		n = exprNew( EXPRCLASS_VTABLE, typeAddrOf(symbGetFullType(sym)), symbGetSubtype(sym) )
		n->sym = sym

	'' Array? Add CAST to make it a pointer to the first element,
	'' instead of a pointer to the array.
	elseif( symbIsCArray( sym ) ) then
		n = exprNew( EXPRCLASS_SYM, FB_DATATYPE_INVALID, NULL )
		n->sym = sym

		n = exprNewCAST( typeAddrOf( symbGetFullType( sym ) ), symbGetSubtype( sym ), n )

	'' main()'s argv? It's emitted as int8** (char**), not as uint8** (zstring ptr ptr),
	'' so specify the type manually instead of deriving it from the FB symbol.
	'' exprNewVREG() will add the cast to uint8** where needed.
	elseif( (symbIsVar( sym ) or (sym->class = FB_SYMBCLASS_PARAM)) and _
	        ((sym->stats and FB_SYMBSTATS_ARGV) <> 0) ) then
		n = exprNew( EXPRCLASS_SYM, typeMultAddrOf( FB_DATATYPE_BYTE, 2 ), NULL )
		n->sym = sym

	else
		symbGetRealType( sym, dtype, subtype )
		dtype = hCReadOnlyLocalType(sym, dtype)

		n = exprNew( EXPRCLASS_SYM, dtype, subtype )
		n->sym = sym
	end if

	function = n
end function

#include once "backend/c/c-pointer-arithmetic.bi"
#include once "backend/c/c-member-access.bi"

private function typeCBop _
	( _
		byval op as integer, _
		byval a as integer, _
		byval b as integer _
	) as integer

	'' Result of relational/comparison operators is int
	if( astOpIsRelational( op ) ) then
		return FB_DATATYPE_LONG
	end if

	'' This tries to do C operand type promotion (and is probably not
	'' 100% accurate), in order to figure out the result type of BOP/UOP
	'' in the C output code, to allow the expression emitting decide
	'' whether it needs to insert casts in the C output code or not.
	''
	'' This might only actually make a difference in rare cases;
	'' it depends on what kind of BOPs the AST tries to emit.
	''
	'' 1. Operands < int/uint (i.e. byte, short) are promoted to int/uint.
	'' 2. For operands >= int/uint, one operand is promoted to match the
	''    other, if necessary. (except for bitshifts, where the rhs' type
	''    isn't taken into account, unlike FB)

	a = typeGet( a )
	b = typeGet( b )

	'' Float types take precedence (?)
	if( (a = FB_DATATYPE_DOUBLE) or (b = FB_DATATYPE_DOUBLE) ) then
		return FB_DATATYPE_DOUBLE
	end if
	if( (a = FB_DATATYPE_SINGLE) or (b = FB_DATATYPE_SINGLE) ) then
		return FB_DATATYPE_SINGLE
	end if

	'' Promote 8bit/16bit types to 32bit,
	'' and normalize 32bit types to FB_DATATYPE_LONG
	if( typeGetSize( a ) <= 4 ) then
		a = iif( typeIsSigned( a ), FB_DATATYPE_LONG, FB_DATATYPE_ULONG )
	end if
	if( typeGetSize( b ) <= 4 ) then
		b = iif( typeIsSigned( b ), FB_DATATYPE_LONG, FB_DATATYPE_ULONG )
	end if

	'' Promote signed to unsigned
	if( (not typeIsSigned( a )) or (not typeIsSigned( b )) ) then
		a = typeToUnsigned( a )
		b = typeToUnsigned( b )
	end if

	'' Promote to 64bit, iff a 64bit operand is involved,
	'' and normalize to FB_DATATYPE_LONGINT
	if( (typeGetSize( a ) = 8) or (typeGetSize( b ) = 8) ) then
		a = iif( typeIsSigned( a ), FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT )
		b = iif( typeIsSigned( b ), FB_DATATYPE_LONGINT, FB_DATATYPE_ULONGINT )
	end if

	'' Promote signed to unsigned
	if( (not typeIsSigned( a )) or (not typeIsSigned( b )) ) then
		a = typeToUnsigned( a )
		b = typeToUnsigned( b )
	end if

	function = a
end function

private function exprNewUOP _
	( _
		byval op as integer, _
		byval l as EXPRNODE ptr _
	) as EXPRNODE ptr

	dim as EXPRNODE ptr n = any
	dim as integer dtype = any, solved_out = any

	solved_out = FALSE

	'' Similar to BOPs, the C type promotion rules should be applied
	'' to determine the UOP's result type.
	select case as const( op )
	case AST_OP_ADDROF
		'' peep-hole optimization:
		'' ADDROF( DEREF( x ) ) -> x
		if( l->class = EXPRCLASS_UOP ) then
			solved_out = (l->op = AST_OP_DEREF)
		end if

		dtype = l->dtype
		dtype = typeAddrOf( dtype )

	case AST_OP_DEREF
		'' peep-hole optimization:
		'' DEREF( ADDROF( x ) ) -> x
		if( l->class = EXPRCLASS_UOP ) then
			solved_out = (l->op = AST_OP_ADDROF)
		end if

		dtype = l->dtype
		assert( typeGetPtrCnt( dtype ) > 0 )
		dtype = typeDeref( dtype )

	case AST_OP_NEG, AST_OP_NOT, AST_OP_BOOLNOT
		'' peep-hole optimization:
		''    -(-(foo)) -> foo
		''    ~(~(foo)) -> foo
		if( l->class = EXPRCLASS_UOP ) then
			solved_out = (l->op = op)
		end if

		dtype = typeCBop( op, l->dtype, l->dtype )


	case AST_OP_ABS, AST_OP_FLOOR, _
	     AST_OP_SIN, AST_OP_ASIN, _
	     AST_OP_COS, AST_OP_ACOS, _
	     AST_OP_TAN, AST_OP_ATAN, _
	     AST_OP_SQRT, AST_OP_LOG, AST_OP_EXP
		'' Builtin float ops (sin/cos/tan etc.) return what they're given,
		'' abs() works with long & longint too, but same behaviour
		dtype = l->dtype

	case else
		errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
		dtype = l->dtype
	end select

	if( solved_out ) then
		n = l->l
		exprFreeNode( l )
		return n
	end if

	n = exprNew( EXPRCLASS_UOP, dtype, l->subtype )
	n->l = l
	n->op = op

	function = n
end function

private function exprIsNullPointerConstant( byval n as EXPRNODE ptr ) as integer

	if( n->class <> EXPRCLASS_CAST ) then
		return FALSE
	end if
	if( typeIsPtr( n->dtype ) = FALSE ) then
		return FALSE
	end if

	var literal = n->l
	return (literal->class = EXPRCLASS_IMM) andalso _
	       (typeGetClass( literal->dtype ) = FB_DATACLASS_INTEGER) andalso _
	       (literal->val.i = 0)
end function

private function exprNewBOP _
	( _
		byval op as integer, _
		byval l as EXPRNODE ptr, _
		byval r as EXPRNODE ptr _
	) as EXPRNODE ptr

	dim as EXPRNODE ptr n = any
	dim as integer dtype = any
	dim as FBSYMBOL ptr subtype = NULL

	'' To find out the BOPs result type, apply C type promotion rules
	dtype = typeCBop( op, l->dtype, r->dtype )
	'' C pointer arithmetic does not use the usual numeric promotions. Keep
	'' its actual result type so the vreg conversion can insert any required
	'' integer-address cast, and a later dereference retains its record type.
	select case as const( op )
	case AST_OP_ADD, AST_OP_SUB
		if( typeIsPtr(l->dtype) ) then
			if( typeIsPtr(r->dtype) ) then
				if( op = AST_OP_SUB ) then dtype = FB_DATATYPE_INTEGER
			elseif( typeGetClass(r->dtype) = FB_DATACLASS_INTEGER ) then
				dtype = l->dtype
				subtype = l->subtype
			end if
		elseif( op = AST_OP_ADD andalso typeIsPtr(r->dtype) ) then
			if( typeGetClass(l->dtype) = FB_DATACLASS_INTEGER ) then
				dtype = r->dtype
				subtype = r->subtype
			end if
		end if
	end select

	'' Integer zero is a C null pointer constant in equality comparisons. The
	'' IR can erase pointer qualifiers here, so retaining its typed zero cast
	'' can make nested const pointers appear incompatible. Argument casts keep
	'' their pointer type and width because they do not pass through this path.
	if( (op = AST_OP_EQ) or (op = AST_OP_NE) ) then
		if( typeIsPtr( l->dtype ) andalso exprIsNullPointerConstant( r ) ) then
			var ptrcast = r
			r = r->l
			r->dtype = FB_DATATYPE_LONG
			exprFreeNode( ptrcast )
		elseif( typeIsPtr( r->dtype ) andalso exprIsNullPointerConstant( l ) ) then
			var ptrcast = l
			l = l->l
			l->dtype = FB_DATATYPE_LONG
			exprFreeNode( ptrcast )
		end if
	end if

	'' if both are pointers, and either has a mangle modifier, then
	'' cast both sides to ubyte*, otherwise, gcc may warn about comparing
	'' distinct pointer types without a cast.  For example with pointers
	'' to __builtin_va_list types.  AST should have already rejected
	'' invalid comparisons or warned on suspicious comparisons.
	select case as const( op )
	case AST_OP_EQ, AST_OP_NE, AST_OP_GT, AST_OP_LT, AST_OP_GE, AST_OP_LE
		if( typeIsPtr( l->dtype ) and typeIsPtr( r->dtype ) ) then
			if( (typeGetMangleDt( l->dtype ) = FB_DATATYPE_VA_LIST) or _
				(typeGetMangleDt( r->dtype ) = FB_DATATYPE_VA_LIST) ) then
				l = exprNewCAST( hCBytePointerType(l->dtype), NULL, l )
				r = exprNewCAST( hCBytePointerType(r->dtype), NULL, r )
			end if
		end if
	end select

	if( op = AST_OP_ADD ) then
		var address = hCByteOffsetChain( l, r )
		if( address ) then
			'' Correct the native signed offset before taking one pointer step.
			'' A separate pointer step can leave the object even when the final
			'' address is valid. The C backend's -fwrapv contract preserves the
			'' native integer arithmetic; neither operand is evaluated twice.
			r->dtype = FB_DATATYPE_INTEGER
			address->r = exprNewBOP( AST_OP_ADD, address->r, r )
			return l
		end if
	end if

	n = exprNew( EXPRCLASS_BOP, dtype, subtype )
	n->l = l
	n->r = r
	n->op = op

	function = n
end function

#include once "backend/c/c-array-index.bi"

'' Add expression root node to cache list, with the corresponding vreg id,
'' allowing it to be looked up later (when the AST accesses that vreg).
private sub exprCache( byval vregid as integer, byval expr as EXPRNODE ptr )
	dim as EXPRCACHENODE ptr entry = any
	entry = listNewNode( @ctx.exprcache )
	entry->vregid = vregid
	entry->expr = expr
end sub

private function exprLookup( byval vregid as integer ) as EXPRNODE ptr
	dim as EXPRCACHENODE ptr entry = any

	'' Find the node corresponding to that vreg, if any.
	entry = listGetHead( @ctx.exprcache )
	while( entry )
		if( entry->vregid = vregid ) then
			exit while
		end if
		entry = listGetNext( entry )
	wend

	if( entry ) then
		function = entry->expr
		listDelNode( @ctx.exprcache, entry )
	else
		function = NULL
	end if
end function

private function hEmitInt _
	( _
		byval dtype as integer, _
		byval value as longint _
	) as string

	dim as string s

	if( typeIsSigned( dtype ) ) then
		s = str( value )

		'' Prevent GCC warnings for INT_MIN/LLONG_MIN:
		'' The '-' minus sign doesn't count as part of the number
		'' literal, and 2147483648 is too big to fit into a signed int,
		'' so according to C99 it is an unsigned int. We must add a cast
		'' to make it signed.
		if( (typeGetSize( dtype ) = 8) or typeIsInteger128Scalar( dtype ) ) then
			if( value = -9223372036854775808ull ) then
				s = "(int64)" + s + "u"
			end if
			s += "ll"
		else
			if( value = -2147483648u ) then
				s = "(int32)" + s + "u"
			end if
		end if
	else
		if( (typeGetSize( dtype ) = 8) or typeIsInteger128Scalar( dtype ) ) then
			s = str( culngint( value ) ) + "ull"
		else
			s = str( culng( value ) ) + "u"
		end if
	end if

	function = s
end function

private function hEmitFloat _
	( _
		byval dtype as integer, _
		byval value as double _
	) as string

	dim as string s
	dim as ulong expval = any

	expval = (*cast( ulongint ptr, @value )) shr 32

	select case( expval )
	'' +/- infinity?
	case &h7FF00000UL, &hFFF00000UL
		if( dtype = FB_DATATYPE_DOUBLE ) then
			if( expval and &h80000000ul ) then
				s += "(-__builtin_inf())"
			else
				s += "__builtin_inf()"
			end if
		else
			if( expval and &h80000000ul ) then
				s += "(-__builtin_inff())"
			else
				s += "__builtin_inff()"
			end if
		end if

	'' +/- NaN? Quiet-NaN's only
	case &h7FF80000UL, &hFFF80000UL
		if( irHlcPlatformEmitNan _
			( dtype, (expval and &h80000000ul), s ) = FALSE ) then
			if( dtype = FB_DATATYPE_DOUBLE ) then
				if( expval and &h80000000ul ) then
					s += "(-__builtin_nan( """" ))"
				else
					s += "__builtin_nan( """" )"
				end if
			else
				if( expval and &h80000000ul ) then
					s += "(-__builtin_nanf( """" ))"
				else
					s += "__builtin_nanf( """" )"
				end if
			end if
		end if

	case else

		'' Convert to exact representation using C99-compatible hex format
		s = hFloatToHex_C99( value )

		'' float type suffix
		if( dtype = FB_DATATYPE_SINGLE ) then
			s += "f"
		end if

	end select

	function = s
end function

private sub hBuildStrLit _
	( _
		byref ln as string, _
		byval z as zstring ptr, _
		byval length as longint, _  '' including null terminator
		byval paddedlength as longint _
	)

	dim as integer ch = any
	dim as DZSTRING buffer

	DZstrZero( buffer )
	DZstrAssign( buffer, strptr( ln ) )

	'' Convert the string to something suitable for C
	'' (assuming internal escape sequences have already been solved out
	'' using hUnescape())
	'' Non-ASCII characters and also \ or " must be escaped, but also care
	'' must be taken when normal chars following an escape sequence would
	'' be seen as part of that escape sequence. This is handled by splitting
	'' the string literal in two at that position.

	DZstrConcatAssign( buffer, """" )

	'' Don't bother emitting the null terminator explicitly - gcc will add
	'' it automatically already
	for i as integer = 0 to length - 2
		ch = (*z)[i]

		if( hCharNeedsEscaping( ch, asc( """" ) ) ) then
			'' Emit in \xNN escape form
			DZstrConcatAssign( buffer, $"\x" + hex( ch, 2 ) )

			'' Is there an 0-9, a-f or A-F char following?
			if( hIsValidHexDigit( (*z)[i+1] ) ) then
				'' Split up the string literal to prevent
				'' the compiler from treating this following
				'' char as part of the escape sequence
				DZstrConcatAssign( buffer, """ """ )
			end if
		elseif( ch = asc( "?" ) ) then
			DZstrConcatAssignC( buffer, asc( "?" ) )
			'' If the following string literal content would form a
			'' trigraph, it must be escaped
			if( (*z)[i+1] = asc( "?" ) ) then
				assert( (i+2) < length )  '' null terminator not yet reached
				select case( (*z)[i+2] )
				case asc( "=" ), asc( "/" ), asc( "'" ), _
				     asc( "(" ), asc( ")" ), asc( "!" ), _
				     asc( "<" ), asc( ">" ), asc( "-" )
					'' Split up the string literal between the two '??', ditto
					DZstrConcatAssign( buffer, """ """ )
				end select
			end if
		else
			'' Emit as-is
			DZstrConcatAssignC( buffer, ch )
		end if
	next

	if( paddedlength > length ) then
		DZstrConcatAssign( buffer, space( paddedlength-length ) )
	end if

	DZstrConcatAssign( buffer, """" )

	if( buffer.data <> NULL ) then
		ln = *buffer.data
	else
		ln = ""
	end if
	DZstrAllocate( buffer, 0 )
end sub

private function hUseNumericWstrLit( ) as integer
	'' Some COFF targets may use a 32-bit FreeBASIC WCHAR while the C
	'' compiler's L"..." literals are not 32-bit wchar_t strings.  Emit
	'' numeric target-WCHAR arrays there so WSTRING literals are described
	'' by FreeBASIC's target table instead of the host C wide-character ABI.
	function = fbTargetWcharIsUtf32( ) and fbTargetSupportsCOFF( )
end function

private sub hBuildNumericWstrLit _
	( _
		byref ln as string, _
		byval w as wstring ptr, _
		byval length as longint _  '' including null terminator
	)

	dim as uinteger ch = any
	dim as wstring ptr src = any, src_end = any
	dim as DZSTRING buffer

	DZstrZero( buffer )
	DZstrAssign( buffer, strptr( ln ) )

	DZstrConcatAssign( buffer, "(" + hEmitType( env.target.wchar, NULL ) + "[]){ " )

	src = w
	src_end = w + len( *w )
	for i as longint = 0 to length - 1
		if( i > 0 ) then
			DZstrConcatAssign( buffer, ", " )
		end if

		if( src < src_end ) then
			ch = hReadWstrChar( src, src_end )
		else
			ch = 0
		end if

		DZstrConcatAssign( buffer, hEmitInt( env.target.wchar, ch ) )
	next

	DZstrConcatAssign( buffer, " }" )

	if( buffer.data <> NULL ) then
		ln = *buffer.data
	else
		ln = ""
	end if
	DZstrAllocate( buffer, 0 )
end sub

private sub hBuildWstrLit _
	( _
		byref ln as string, _
		byval w as wstring ptr, _
		byval length as longint _  '' including null terminator
	)

	dim as uinteger ch = any
	dim as integer wcharsize = any
	dim as wstring ptr src = any, src_end = any
	dim as const zstring ptr strstart = any
	dim as DZSTRING buffer

	'' (ditto)

	if( hUseNumericWstrLit( ) ) then
		hBuildNumericWstrLit( ln, w, length )
		exit sub
	end if

	wcharsize = fbGetTargetWcharSize( )
	'' On targets with 1-byte wstring data, C wide strings/chars are too
	'' wide, so don't use them.
	if( wcharsize = 1 ) then
		strstart = @""""
	else
		strstart = @"L"""
	end if

	DZstrZero( buffer )
	DZstrAssign( buffer, strptr( ln ) )
	DZstrConcatAssign( buffer, strstart )

	'' Don't bother emitting the null terminator explicitly - gcc will add
	'' it automatically already
	src = w
	src_end = w + len( *w )
	for i as integer = 0 to length - 2
		if( src >= src_end ) then
			exit for
		end if

		ch = hReadWstrChar( src, src_end )

		'' A UTF-16 target needs two C wide-literal code units for a
		'' supplementary Unicode code point. The compiler's host WSTRING may
		'' hold that code point in one UTF-32 element, so split it explicitly.
		if( (wcharsize = 2) and (ch >= &h10000ul) and (ch <= &h10FFFFul) ) then
			ch -= &h10000ul
			DZstrConcatAssign( buffer, $"\x" + hex( (ch shr 10) + &hD800ul, 4 ) )
			DZstrConcatAssign( buffer, chr( 34 ) + " " + *strstart )
			DZstrConcatAssign( buffer, $"\x" + hex( (ch and &h3FFul) + &hDC00ul, 4 ) )
			DZstrConcatAssign( buffer, chr( 34 ) + " " + *strstart )
			continue for
		end if

		if( hCharNeedsEscaping( ch, asc( """" ) ) ) then
			DZstrConcatAssign( buffer, $"\x" + hex( ch, wcharsize * 2 ) )
			DZstrConcatAssign( buffer, chr( 34 ) + " " + *strstart )
		elseif( ch = asc( "?" ) ) then
			DZstrConcatAssignC( buffer, asc( "?" ) )
			if( (src < src_end) andalso (*src = asc( "?" )) ) then
				select case( cint( src[1] ) )
				case asc( "=" ), asc( "/" ), asc( "'" ), _
				     asc( "(" ), asc( ")" ), asc( "!" ), _
				     asc( "<" ), asc( ">" ), asc( "-" )
					DZstrConcatAssign( buffer, chr( 34 ) + " " + *strstart )
				end select
			end if
		else
			DZstrConcatAssignC( buffer, ch )
		end if
	next

	DZstrConcatAssign( buffer, """" )

	if( buffer.data <> NULL ) then
		ln = *buffer.data
	else
		ln = ""
	end if
	DZstrAllocate( buffer, 0 )
end sub

private function hBopToStr( byval op as integer ) as const zstring ptr
	select case as const( op )
	case AST_OP_ADD : function = @" + "
	case AST_OP_SUB : function = @" - "
	case AST_OP_MUL : function = @" * "
	case AST_OP_DIV : function = @" / "
	case AST_OP_INTDIV : function = @" / "
	case AST_OP_MOD : function = @" % "
	case AST_OP_SHL : function = @" << "
	case AST_OP_SHR : function = @" >> "
	case AST_OP_AND : function = @" & "
	case AST_OP_OR  : function = @" | "
	case AST_OP_XOR : function = @" ^ "
	case AST_OP_EQ  : function = @" == "
	case AST_OP_GT  : function = @" > "
	case AST_OP_LT  : function = @" < "
	case AST_OP_NE  : function = @" != "
	case AST_OP_GE  : function = @" >= "
	case AST_OP_LE  : function = @" <= "
	case else
		errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
		function = @""
	end select
end function

private function hFloatUopToStr( byval op as integer, byval dtype as integer ) as const zstring ptr
	function = @""

	'' Ignore any const qualifier.
	if( typeGetDtAndPtrOnly( dtype ) = FB_DATATYPE_SINGLE ) then
		select case as const( op )
		case AST_OP_SIN   : function = @"__builtin_sinf"
		case AST_OP_ASIN  : function = @"__builtin_asinf"
		case AST_OP_COS   : function = @"__builtin_cosf"
		case AST_OP_ACOS  : function = @"__builtin_acosf"
		case AST_OP_TAN   : function = @"__builtin_tanf"
		case AST_OP_ATAN  : function = @"__builtin_atanf"
		case AST_OP_SQRT  : function = @"__builtin_sqrtf"
		case AST_OP_LOG   : function = @"__builtin_logf"
		case AST_OP_EXP   : function = @"__builtin_expf"
		case AST_OP_FLOOR : function = @"__builtin_floorf"
		case else          : errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
		end select
	else
		assert( typeGetDtAndPtrOnly( dtype ) = FB_DATATYPE_DOUBLE )
		select case as const( op )
		case AST_OP_SIN   : function = @"__builtin_sin"
		case AST_OP_ASIN  : function = @"__builtin_asin"
		case AST_OP_COS   : function = @"__builtin_cos"
		case AST_OP_ACOS  : function = @"__builtin_acos"
		case AST_OP_TAN   : function = @"__builtin_tan"
		case AST_OP_ATAN  : function = @"__builtin_atan"
		case AST_OP_SQRT  : function = @"__builtin_sqrt"
		case AST_OP_LOG   : function = @"__builtin_log"
		case AST_OP_EXP   : function = @"__builtin_exp"
		case AST_OP_FLOOR : function = @"__builtin_floor"
		case else          : errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
		end select
	end if
end function

private function hUopToStr _
	( _
		byval op as integer, _
		byval dtype as integer, _
		byref is_builtin as integer _
	) as const zstring ptr

	is_builtin = FALSE

	select case( op )
	case AST_OP_ADDROF : function = @"&"
	case AST_OP_DEREF  : function = @"*"
	case AST_OP_NEG    : function = @"-"
	case AST_OP_NOT    : function = @"~"
	case AST_OP_BOOLNOT: function = @"!"

	case AST_OP_ABS
		is_builtin = TRUE

		select case as const( typeGetSizeType( dtype ) )
		case FB_SIZETYPE_FLOAT32
			function = @"__builtin_fabsf"
		case FB_SIZETYPE_FLOAT64
			function = @"__builtin_fabs"
		case FB_SIZETYPE_INT64, FB_SIZETYPE_UINT64
			function = @"__builtin_llabs"
		case else
			function = @"__builtin_abs"
		end select

	case else
		is_builtin = TRUE
		function = hFloatUopToStr( op, dtype )
	end select

end function

private sub hImm2Text( byref s as string, byval n as EXPRNODE ptr )
	if( typeGetClass( n->dtype ) = FB_DATACLASS_FPOINT ) then
		s += hEmitFloat( n->dtype, n->val.f )
	else
		assert( (n->dtype <> FB_DATATYPE_BOOLEAN) orelse ((n->val.i = 1) or (n->val.i = 0)) )
		s += hEmitInt( n->dtype, n->val.i )
	end if
end sub

#include once "backend/c/c-literal-storage.bi"

private sub hSym2Text( byref s as string, byval sym as FBSYMBOL ptr, byval native_storage as integer = FALSE )
	hCLeafReference( sym )
	'' String literal?
	if( symbGetIsLiteral( sym ) ) then
		if( native_storage ) then
			s += hCLiteralStorageName(sym)
		elseif( symbGetType(sym) = FB_DATATYPE_WCHAR ) then
			hBuildWstrLit(s, symbGetVarLitTextW(sym), symbGetWstrLength(sym) + 1)
		else
			hBuildStrLit(s, hUnescape(symbGetVarLitText(sym)), symbGetStrLength(sym) + 1, 0)
		end if
	else
		if( symbIsLabel( sym ) ) then
			if( sym = ctx.procentrylabel ) then symbSetIsAccessed( sym )
			s += "&&"
		elseif( symbIsProc( sym ) ) then
			hCAbiAddress(sym)
			s += "&"
		end if
		s += hGetCId( sym )
	end if
end sub

'' Builds up final expression text, walking the EXPRNODE tree
private sub hExprFlush( byval n as EXPRNODE ptr, byval need_parens as integer )
	dim as integer is_builtin = any

	select case as const( n->class )
	case EXPRCLASS_TEXT
		ctx.exprtext += *n->text

	case EXPRCLASS_MEMBER
		if( n->member_view = C_MEMBER_VIEW_VALUE andalso hCMemberViewIsAggregate(n) ) then
			var tempname = hCUnalignedTempName()
			ctx.exprtext += "__extension__ ({ " + hEmitType(typeUnsetIsConst(n->dtype), n->subtype) + _
			                " " + tempname + "; __builtin_memcpy( &" + tempname + ", (void const*) &("
			'' Emit this owned storage as an address, not as another value copy.
			n->member_view = C_MEMBER_VIEW_STORAGE
			hExprFlush(n, FALSE)
			n->member_view = C_MEMBER_VIEW_VALUE
			ctx.exprtext += "), sizeof(" + tempname + ") ); " + tempname + "; })"
			exit sub
		end if
		'' A whole-record value uses the same view as a field path. This keeps
		'' BYVAL argument loads from promising the original natural alignment.
		if( n->member_view andalso len(*n->text) = 0 ) then ctx.exprtext += "*"
		ctx.exprtext += "("
		if( n->member_view ) then
			var viewdtype = typeDeref(n->l->dtype)
			var viewname = hCAlignmentViewName(viewdtype, n->l->subtype)
			assert(len(viewname) > 0)
			ctx.exprtext += "("
			'' BASIC CONST storage is emitted as writable C storage so its
			'' runtime initializer can assign individual members. The frontend
			'' has already checked whether a store is permitted; retain const
			'' on read and address views, but not on that checked store view.
			if( typeIsConstAt(viewdtype, 0) andalso n->member_view <> C_MEMBER_VIEW_WRITE ) then ctx.exprtext += "const "
			ctx.exprtext += viewname + "*)("
			hExprFlush(n->l, FALSE)
			ctx.exprtext += ")"
		else
			hExprFlush( n->l, FALSE )
		end if
		ctx.exprtext += ")" + *n->text

	case EXPRCLASS_INDEX
		ctx.exprtext += "("
		hExprFlush(n->l, FALSE)
		ctx.exprtext += ")["
		hExprFlush(n->r, FALSE)
		ctx.exprtext += "]"

	case EXPRCLASS_ARRAYADDR
		dim as longint offset
		var origin = hCArrayStorageOrigin(n->l, offset)
		assert(origin <> NULL)
		'' ARRAYADDR is constructed only for a resolved storage origin.
		'' Preserve the internal-error path if an invalid tree reaches output.
		if( origin = NULL ) then
			errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
			exit sub
		end if
		ctx.exprtext += "(" + hEmitType(typeUnsetIsConst(n->dtype), n->subtype, TRUE) + ")((" + _
		                hEmitType(FB_DATATYPE_UINT, NULL) + ")"
		hExprFlush(origin->l, TRUE)
		ctx.exprtext += " + ("
		hExprFlush(origin->r, FALSE)
		ctx.exprtext += ") * " + hEmitInt(FB_DATATYPE_UINT, symbGetSizeOf(origin->sym))
		if( offset <> 0 ) then ctx.exprtext += " + " + hEmitInt(FB_DATATYPE_UINT, offset)
		ctx.exprtext += ")"

	case EXPRCLASS_IMM
		hImm2Text( ctx.exprtext, n )

	case EXPRCLASS_VTABLE
		ctx.exprtext += "(" + hEmitType(n->dtype, n->subtype) + ")&"
		hSym2Text( ctx.exprtext, n->sym, TRUE )

	case EXPRCLASS_SYM
		'' The symbol formatter also serves expression dumps. Only an address
		'' flushed into C satisfies Clang's computed-goto requirement.
		if( symbIsLabel( n->sym ) and sectionInsideProc( ) ) then
			ctx.computedgotoaddress = TRUE
		end if
		hSym2Text( ctx.exprtext, n->sym, TRUE )

	case EXPRCLASS_CAST
		hCLiteralMutableCast(n->dtype, n->l)
		if( typeIsPtr(n->dtype) andalso typeIsPtr(n->l->dtype) andalso _
		    ((typeGetDtAndPtrOnly(n->dtype) = typeAddrOf(FB_DATATYPE_FUNCTION)) xor _
		     (typeGetDtAndPtrOnly(n->l->dtype) = typeAddrOf(FB_DATATYPE_FUNCTION))) andalso _
		    sectionInsideProc() andalso (ctx.varinisym = NULL) ) then
			'' BASIC library lookups and callback APIs carry native code addresses
			'' in ANY PTR. Copy the representation between the exact declared types
			'' instead of relying on C's object/function pointer cast extension.
			'' Different pointer sizes cannot satisfy this native ABI contract.
			ctx.codeptrid += 1
			if( ctx.codeptrid = 0 ) then
				errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
				exit sub
			end if
			var inputname = "fb_c_codeptr_input_" + str(ctx.codeptrid)
			var resultname = "fb_c_codeptr_result_" + str(ctx.codeptrid)
			var inputsubtype = n->l->subtype
			if( typeGetDtAndPtrOnly(n->l->dtype) = typeAddrOf(FB_DATATYPE_FUNCTION) ) then
				'' A direct procedure address names the procedure symbol, not
				'' a procedure-pointer typedef. Intern its complete signature
				'' before declaring the source representation carrier.
				inputsubtype = symbAddProcPtrFromFunction(inputsubtype)
			end if
			hWriteDeclaration( hEmitType(typeUnsetIsConst(n->l->dtype), inputsubtype, TRUE) + " " + inputname + ";" )
			hWriteDeclaration( hEmitType(typeUnsetIsConst(n->dtype), n->subtype, TRUE) + " " + resultname + ";" )
			hWriteStaticAssert( "sizeof(" + inputname + ") == sizeof(" + resultname + ")" )

			'' Keep evaluation at the original expression site, including inside
			'' a lazy enclosing expression. The input is evaluated exactly once.
			'' Each null pointer maps to a typed null of the destination kind.
			ctx.exprtext += "((void)(" + inputname + " = ("
			hExprFlush( n->l, FALSE )
			ctx.exprtext += ")), " + inputname + " == 0 ? (" + resultname + " = 0) : ("
			ctx.exprtext += "(void)__builtin_memcpy(&" + resultname + ", &" + inputname + ", sizeof(" + resultname + ")), "
			ctx.exprtext += resultname + "))"
			exit sub
		end if

		'' Keep pointee qualifiers, including nested pointer constness, so
		'' argument casts agree with the emitted C prototype. Top-level const
		'' does not qualify the converted value and is unnecessary in a cast.
		ctx.exprtext += "(" + hEmitType( typeUnsetIsConst( n->dtype ), n->subtype, TRUE ) + ")"
		hExprFlush( n->l, TRUE )

	case EXPRCLASS_MACRO
		select case( n->op )
		case AST_OP_VA_ARG
			'' cva_arg(l, type) := __builtin_va_arg(l, type)
			ctx.exprtext += "__builtin_va_arg( "
			hExprFlush( n->l, TRUE )
			ctx.exprtext += ", "
			ctx.exprtext += hEmitType( n->dtype, n->subtype )
			ctx.exprtext += ")"

		case AST_OP_VA_START
			'' cva_start(l, r) := __builtin_va_start(l, r)
			ctx.exprtext += "__builtin_va_start( "
			hExprFlush( n->l, TRUE )
			ctx.exprtext += ", "
			hExprFlush( n->r, TRUE )
			ctx.exprtext += ")"

		case AST_OP_VA_END
			'' cva_end(l) := __builtin_va_end(l)
			ctx.exprtext += "__builtin_va_end( "
			hExprFlush( n->l, TRUE )
			ctx.exprtext += ")"

		case AST_OP_VA_COPY
			'' cva_copy(l, r) := __builtin_va_copy(l, r)
			ctx.exprtext += "__builtin_va_copy( "
			hExprFlush( n->l, TRUE )
			ctx.exprtext += ", "
			hExprFlush( n->r, TRUE )
			ctx.exprtext += ")"

		case else
			errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
		end select


	case EXPRCLASS_UOP
		ctx.exprtext += *hUopToStr( n->op, n->dtype, is_builtin )

		'' Add parentheses around UOPs to avoid -(-(foo)) looking like
		'' --foo which looks like the -- operator to gcc. Or, add the
		'' parentheses for __builtin_* calls.
		need_parens = (n->l->class = EXPRCLASS_UOP) or is_builtin
		if( need_parens ) then
			ctx.exprtext += "("
			if( is_builtin ) then
				ctx.exprtext += " "
			end if
		end if
		hExprFlush( n->l, TRUE )
		if( need_parens ) then
			if( is_builtin ) then
				ctx.exprtext += " "
			end if
			ctx.exprtext += ")"
		end if

	case EXPRCLASS_BOP
		select case( n->op )
		case AST_OP_ATAN2
			if( n->dtype = FB_DATATYPE_SINGLE ) then
				ctx.exprtext += "__builtin_atan2f"
			else
				ctx.exprtext += "__builtin_atan2"
			end if
			ctx.exprtext += "("
			hExprFlush( n->l, FALSE )
			ctx.exprtext += ", "
			hExprFlush( n->r, FALSE )
			ctx.exprtext += ")"
		case else
			var left_type = typeGetDtAndPtrOnly( n->l->dtype )
			var right_type = typeGetDtAndPtrOnly( n->r->dtype )
			if( ((n->op = AST_OP_EQ) or (n->op = AST_OP_NE)) andalso _
			    ((left_type = FB_DATATYPE_SINGLE) or (left_type = FB_DATATYPE_DOUBLE) or _
			     (right_type = FB_DATATYPE_SINGLE) or (right_type = FB_DATATYPE_DOUBLE)) ) then
				if( n->op = AST_OP_NE ) then ctx.exprtext += "!"
				if( (left_type = FB_DATATYPE_DOUBLE) or (right_type = FB_DATATYPE_DOUBLE) ) then
					ctx.exprtext += "fb_c_f64_equal("
					ctx.usedbuiltins or= BUILTIN_F64_EQUAL
				else
					ctx.exprtext += "fb_c_f32_equal("
					ctx.usedbuiltins or= BUILTIN_F32_EQUAL
				end if
				hExprFlush( n->l, FALSE )
				ctx.exprtext += ", "
				hExprFlush( n->r, FALSE )
				ctx.exprtext += ")"
				exit select
			end if
			'' Add parentheses around BOPs if the parent needs it
			'' (looks like parentheses are unnecessary for all the other
			'' expressions though, CAST/UOP should work fine without
			'' parentheses around their operand)
			if( need_parens ) then
				ctx.exprtext += "("
			end if
			hExprFlush( n->l, TRUE )
			ctx.exprtext += *hBopToStr( n->op )
			var leftdtype = typeGetDtAndPtrOnly(n->l->dtype)
			var rightdtype = typeGetDtAndPtrOnly(n->r->dtype)
			var unsigned_offset = _
				((n->op = AST_OP_ADD) or (n->op = AST_OP_SUB)) andalso _
				((leftdtype = FB_DATATYPE_UINT) or (leftdtype = FB_DATATYPE_ULONG) or _
				 (leftdtype = FB_DATATYPE_ULONGINT)) andalso _
				(typeGetSize(n->l->dtype) = env.pointersize) andalso _
				(typeHasMangleDt(n->l->dtype) = FALSE) andalso _
				(typeHasMangleDt(n->r->dtype) = FALSE) andalso _
				((rightdtype = FB_DATATYPE_INTEGER) or (rightdtype = FB_DATATYPE_LONG) or _
				 (rightdtype = FB_DATATYPE_LONGINT)) andalso _
				(typeGetSize(n->r->dtype) = env.pointersize)
			if( unsigned_offset ) then
				'' This is the conversion C already performs in the native
				'' unsigned domain. UINT and same-width ULONG/ULONGINT have the
				'' same C spelling. Keep this in formatting so byte-offset
				'' recovery still sees the original signed IR expression.
				ctx.exprtext += "(" + hEmitType(FB_DATATYPE_UINT, NULL) + ")("
				hExprFlush( n->r, FALSE )
				ctx.exprtext += ")"
			else
				hExprFlush( n->r, TRUE )
			end if
			if( need_parens ) then
				ctx.exprtext += ")"
			end if
		end select
	end select
end sub

private function exprFlush _
	( _
		byval n as EXPRNODE ptr, _
		byval need_parens as integer = FALSE _
	) as string

	hExprFlush( n, need_parens )

	function = ctx.exprtext
	ctx.exprtext = ""

	exprFreeTree( n )
end function

#if __FB_DEBUG__
private sub exprDump( byval n as EXPRNODE ptr )
	static as integer level
	dim as string s

	level += 1

	'' Indentation
	s += space( (level - 1) * 4 )

	static names(0 to ...) as const zstring ptr => { _
		@"TEXT", _ '' EXPRCLASS_TEXT
		@"IMM" , _ '' EXPRCLASS_IMM
		@"SYM" , _ '' EXPRCLASS_SYM
		@"CAST", _ '' EXPRCLASS_CAST
		@"UOP" , _ '' EXPRCLASS_UOP
		@"BOP" , _ '' EXPRCLASS_BOP
		@"MACRO", _ '' EXPRCLASS_MACRO
		@"MEMBER", _ '' EXPRCLASS_MEMBER
		@"INDEX", _ '' EXPRCLASS_INDEX
		@"ARRAYADDR", _ '' EXPRCLASS_ARRAYADDR
		@"VTABLE" _ '' EXPRCLASS_VTABLE
	}

	s += *names(n->class)
	s += typeDumpToStr( n->dtype, n->subtype )
	s += " "

	select case as const( n->class )
	case EXPRCLASS_TEXT, EXPRCLASS_MEMBER
		s += *n->text

	case EXPRCLASS_IMM
		hImm2Text( s, n )

	case EXPRCLASS_SYM, EXPRCLASS_VTABLE
		hSym2Text( s, n->sym )

	case EXPRCLASS_CAST
		s += hEmitType( n->dtype, n->subtype )

	case EXPRCLASS_MACRO
		s += hEmitType( n->dtype, n->subtype )

	case EXPRCLASS_UOP
		s += *hUopToStr( n->op, n->dtype, FALSE )

	case EXPRCLASS_BOP
		select case( n->op )
		case AST_OP_ATAN2
			if( n->dtype = FB_DATATYPE_SINGLE ) then
				s += "__builtin_atan2f"
			else
				s += "__builtin_atan2"
			end if
		case else
			s += *hBopToStr( n->op )
		end select

	end select

	print s

	select case( n->class )
	case EXPRCLASS_CAST, EXPRCLASS_UOP, EXPRCLASS_MEMBER, EXPRCLASS_ARRAYADDR
		exprDump( n->l )
	case EXPRCLASS_BOP, EXPRCLASS_INDEX
		exprDump( n->l )
		exprDump( n->r )
	case EXPRCLASS_MACRO
		exprDump( n->l )
		exprDump( n->r )
	end select

	level -= 1
end sub
#endif

private function exprNewOFFSET _
	( _
		byval sym as FBSYMBOL ptr, _
		byval ofs as longint, _
		byval memberdtype as integer = FB_DATATYPE_INVALID, _
		byval membersubtype as FBSYMBOL ptr = NULL _
	) as EXPRNODE ptr

	dim as EXPRNODE ptr l = any

	l = exprNewSYM( sym )
	if( memberdtype <> FB_DATATYPE_INVALID andalso symbIsCArray(sym) andalso _
	    ((sym->stats and FB_SYMBSTATS_VTABLE) = 0) ) then
		dim as EXPRNODE ptr byte_index = NULL
		var indexed = exprTryCArrayIndex(sym, byte_index, ofs, memberdtype, membersubtype, TRUE)
		if( indexed <> NULL ) then
			exprFreeTree(l)
			return exprNewCAddress( indexed)
		end if
	end if
	if( (memberdtype <> FB_DATATYPE_INVALID) andalso _
	    (typeIsPtr(l->dtype) = FALSE) andalso (symbIsCArray(sym) = FALSE) ) then
		var member = exprTryCMember(l, ofs, memberdtype, membersubtype)
		if( member <> NULL ) then return exprNewCAddress( member)
	end if

	'' Add '&' for things that aren't pointers already
	if( (symbIsImport( sym ) or symbIsCArray( sym ) or _
	     symbIsProc( sym ) or symbIsLabel( sym )) = FALSE ) then
		l = exprNewCAddress( l )
	end if

	'' Add on the byte offset, if any
	if( ofs <> 0 ) then
		'' Cast to ubyte ptr to work around C's pointer arithmetic
		l = exprNewCAST( hCBytePointerType(l->dtype), NULL, l )
		l = exprNewBOP( AST_OP_ADD, l, exprNewIMMi( ofs ) )
	end if

	function = l
end function

private function hIsScalarLoadType _
	( _
		byval dtype as integer _
	) as integer

	select case as const( typeGetClass( dtype ) )
	case FB_DATACLASS_INTEGER, FB_DATACLASS_FPOINT
		function = TRUE
	case else
		function = FALSE
	end select

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

#include once "backend/c/c-alignment.bi"

private function hSymBaseMayBeUnaligned _
	( _
		byval sym as FBSYMBOL ptr _
	) as integer

	if( sym = NULL ) then
		return FALSE
	end if

	'' BYREF symbols are emitted as pointers.  The pointer can come from a
	'' packed field, so scalar accesses through it cannot assume natural
	'' alignment even when the byte offset itself looks aligned.
	if( symbIsRef( sym ) ) then
		return TRUE
	end if

	if( symbGetClass( sym ) = FB_SYMBCLASS_PARAM ) then
		select case symbGetParamMode( sym )
		case FB_PARAMMODE_BYREF, FB_PARAMMODE_BYDESC
			return TRUE
		case else
			'' Other parameter modes use the type alignment checks below.
			exit select
		end select
	end if

	if( symbGetSubType( sym ) = NULL ) then
		return FALSE
	end if

	'' Constructors and methods receive their UDT through a hidden pointer whose
	'' encoded datatype is not necessarily STRUCT at this backend stage.  The
	'' subtype symbol remains authoritative.
	if( symbGetClass( symbGetSubType( sym ) ) <> FB_SYMBCLASS_STRUCT ) then
		return FALSE
	end if

	if( symbGetUDTAlign( symbGetSubType( sym ) ) = 0 ) then
		return FALSE
	end if

	'' The UDT's recorded natural alignment already reflects FIELD=N packing,
	'' so comparing against it can hide a reduced alignment.  Treat any
	'' explicitly field-aligned UDT conservatively; memcpy-based scalar access
	'' remains correct when N happens to satisfy the scalar's natural alignment.
	function = TRUE
end function

private function hVregBaseMayBeUnaligned _
	( _
		byval vreg as IRVREG ptr _
	) as integer

	if( vreg = NULL ) then
		return FALSE
	end if

	if( vreg->sym = NULL ) then
		return hVregBaseMayBeUnaligned( vreg->vidx )
	end if

	function = hSymBaseMayBeUnaligned( vreg->sym )
end function

private function hExprBaseMayBeUnaligned _
	( _
		byval n as EXPRNODE ptr _
	) as integer

	if( n = NULL ) then
		return FALSE
	end if

	select case as const n->class
	case EXPRCLASS_SYM
		function = hSymBaseMayBeUnaligned( n->sym )

	case EXPRCLASS_CAST, EXPRCLASS_UOP, EXPRCLASS_MEMBER, EXPRCLASS_INDEX, EXPRCLASS_ARRAYADDR
		function = hExprBaseMayBeUnaligned( n->l )

	case EXPRCLASS_BOP
		function = hExprBaseMayBeUnaligned( n->l ) or _
		           hExprBaseMayBeUnaligned( n->r )

	case else
		function = FALSE
	end select
end function

private function hAddrNeedsUnalignedAccess _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval addr as EXPRNODE ptr, _
		byval base_may_be_unaligned as integer = FALSE _
	) as integer

	dim as integer align = any

	if( hIsScalarLoadType( dtype ) = FALSE ) then
		return FALSE
	end if

	align = typeCalcNaturalAlign( dtype, subtype )
	if( align <= 1 ) then
		return FALSE
	end if

	'' A byte view carries no natural scalar alignment promise. Copy the
	'' same scalar width rather than converting that address to a more
	'' strictly aligned pointer. The existing copy owner evaluates it once.
	var byte_view = addr
	do
		if( typeIsPtr(byte_view->dtype) ) then
			select case typeGetDtAndPtrOnly(typeDeref(byte_view->dtype))
			case FB_DATATYPE_BYTE, FB_DATATYPE_UBYTE, FB_DATATYPE_CHAR
				return TRUE
			end select
		end if
		if( byte_view->class <> EXPRCLASS_CAST ) then exit do
		if( typeIsPtr(byte_view->dtype) = FALSE ) then exit do
		if( typeIsPtr(byte_view->l->dtype) = FALSE ) then exit do
		byte_view = byte_view->l
	loop

	if( base_may_be_unaligned or hExprBaseMayBeUnaligned( addr ) ) then
		return TRUE
	end if

	function = (hExprConstOffsetMod( addr, align ) <> 0)
end function

private function exprNewUnalignedLoad _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval addr as EXPRNODE ptr _
	) as EXPRNODE ptr

	dim as string typename
	dim as string addrtext
	dim as string s

	typename = hEmitType( typeUnsetIsConst( dtype ), subtype, TRUE )
	var tempname = hCUnalignedTempName()
	var member = exprTryCMember( addr, 0, dtype, subtype, TRUE )
	if( member <> NULL ) then addr = exprNewCAddress( member )
	addrtext = exprFlush( hCUnalignedAddress(addr) )

	'' Packed fields can be reached through byte offsets that are not aligned
	'' for their declared type.  memcpy lets the C compiler choose an access
	'' sequence that is valid on targets where unaligned scalar loads fault.
	s = "__extension__ ({ "
	s += typename + " " + tempname + "; "
	s += "__builtin_memcpy( &" + tempname + ", (void const*)("
	s += addrtext
	s += "), sizeof( " + tempname + " ) ); "
	s += tempname + "; })"

	function = exprNewTEXT( dtype, subtype, s )

end function

private function exprNewVREG _
	( _
		byval vreg as IRVREG ptr, _
		byval is_lvalue as integer = FALSE, _
		byval write_value as integer = TRUE _
	) as EXPRNODE ptr

	dim as EXPRNODE ptr l = any
	dim as integer dtype = any, have_offset = any
	dim as EXPRNODE ptr byte_index = NULL
	dim as integer accessdtype = vreg->dtype
	if( is_lvalue and write_value ) then
		'' The AST has already checked writes, including initialization of
		'' CONST variables. Their C storage is mutable during initialization;
		'' remove only the stored value's const, keeping pointee qualifiers.
		accessdtype = typeUnsetIsConst( accessdtype )
	end if

	select case as const( vreg->typ )
	case IR_VREGTYPE_VAR, IR_VREGTYPE_IDX, IR_VREGTYPE_PTR
		if( vreg->sym = NULL ) then
			'' No symbol attached, but vidx instead, unless the
			'' address was given as a constant,
			'' e.g. in derefs like *cptr(byte ptr, 0),
			'' then there is neither a symbol nor vidx,
			'' but just the "offset".
			''    *(vregtype*)offset
			''    *(vregtype*)vidx
			''    *(vregtype*)((uint8*)vidx + offset)

			if( vreg->vidx ) then
				'' recursion
				l = exprNewVREG( vreg->vidx )
				var member = exprTryCMember(l, vreg->ofs, accessdtype, vreg->subtype, _
					FALSE, (is_lvalue = FALSE), (is_lvalue and write_value))
				if( member <> NULL ) then
					l = member
					exit select
				end if

				if( vreg->ofs <> 0 ) then
					'' Cast to ubyte ptr to work around C's pointer arithmetic
					l = exprNewCAST( hCBytePointerType(l->dtype), NULL, l )
					l = exprNewBOP( AST_OP_ADD, l, exprNewIMMi( vreg->ofs ) )
				end if
			else
				l = exprNewIMMi( vreg->ofs )
			end if

			if( is_lvalue = FALSE andalso hIsScalarLoadType(vreg->dtype) ) then
				hCReadAddress(l)
			end if

			if( (is_lvalue = FALSE) andalso _
			    hAddrNeedsUnalignedAccess( vreg->dtype, vreg->subtype, l, hVregBaseMayBeUnaligned( vreg ) ) ) then
				l = exprNewUnalignedLoad( vreg->dtype, vreg->subtype, l )
			else
				l = exprNewCAST( hCAccessPointerType(accessdtype, is_lvalue), vreg->subtype, l )
				l = exprNewUOP( AST_OP_DEREF, l )
			end if
			exit select
		end if

		assert( symbIsProc( vreg->sym ) = FALSE )  '' should be an IR_VREGTYPE_OFS
		assert( symbIsLabel( vreg->sym ) = FALSE ) '' should be handled in _emitAddr()

		'' memory accesses - stack vars, arrays, UDT fields, ptr derefs
		''
		'' - offsets are byte offsets as calculated by the AST
		'' - vreg's dtype can be different from symbol's dtype,
		''   e.g. UDT var + field access, or due to type casting.
		'' - vregs can be structs/strings here in the C backend
		'' - C doesn't allow direct casting to/from structs, but we can
		''   do a deref/addrof trick like *(vregtype*)&udtvar instead.
		'' - no float <-> int conversions should be done here, so be
		''   careful with vregdtype=integer while sym=floatvar etc.,
		''   the work-around (again) is the deref/addrof trick.
		''
		'' simple var accesses:
		''        sym
		''        (vregtype)sym
		'' ptr derefs:
		''        *(vregtype*)sym
		''        *(vregtype*)((uint8*)sym + offset)
		'' array accesses (idx):
		''        *(vregtype*)((uint8*)sym + vidx + offset)
		'' field accesses:
		''        *(vregtype*)&sym
		''        *(vregtype*)((uint8*)&sym + offset)
		'' addrof var:
		''        (vregtype)((uint8*)&sym + offset)

		have_offset = ((vreg->ofs <> 0) or (vreg->vidx <> NULL))

		'' Check whether to do plain access or deref/addrof trick
		'' - any offset? use trick, to allow doing +offset
		'' - symbol is an array in the C code? (arrays, fixlen strings...)
		''   cannot just do (elementtype)carray, it must always be
		''   *(elementtype*)carray to access the memory in these cases.
		var is_c_array = symbIsCArray( vreg->sym )
		var do_deref = have_offset or is_c_array

		l = exprNewSYM( vreg->sym )
		if( is_c_array ) then
			if( vreg->vidx <> NULL ) then byte_index = exprNewVREG(vreg->vidx)
			var indexed = exprTryCArrayIndex(vreg->sym, byte_index, vreg->ofs, accessdtype, _
				vreg->subtype, FALSE, (is_lvalue = FALSE))
			if( indexed <> NULL ) then
				exprFreeTree(l)
				l = indexed
				exit select
			end if
		end if
		if( (is_c_array = FALSE) andalso (vreg->vidx = NULL) andalso _
		    ((typeIsPtr(l->dtype) = FALSE) or (vreg->typ = IR_VREGTYPE_PTR)) ) then
			var member = exprTryCMember(l, vreg->ofs, accessdtype, vreg->subtype, _
				FALSE, (is_lvalue = FALSE), (is_lvalue and write_value))
			if( member <> NULL ) then
				l = member
				exit select
			end if
		end if

		dim as integer symdtype = l->dtype
		dim as FBSYMBOL ptr symsubtype = l->subtype

		'' Different types?
		if( (vreg->dtype <> symdtype) or (vreg->subtype <> symsubtype) ) then
			'' a) float <-> int: access raw bytes instead of converting
			'' b) struct <-> any other: ensure valid C syntax

			'' different data classes?
			do_deref or= (typeGetClass( vreg->dtype ) <> typeGetClass( symdtype ))

			'' any structs involved? (note: FBSTRINGs are structs in the C code too!)
			select case( typeGet( vreg->dtype ) )
			case FB_DATATYPE_STRING, FB_DATATYPE_USTRING
				do_deref = TRUE
			case FB_DATATYPE_STRUCT
				'' don't deref if it is a va_list[1] type, unless we going to anyway
				do_deref or= not symbIsValistStructArray( symbGetFullType( vreg->sym ), symbGetSubType( vreg->sym ) )
			case else
				select case( typeGet( symdtype ) )
				case FB_DATATYPE_STRING, FB_DATATYPE_USTRING
					do_deref = TRUE
				case FB_DATATYPE_STRUCT
					'' don't deref if it is a va_list[1] type, unless we going to anyway
					do_deref or= not symbIsValistStructArray( symbGetFullType( vreg->sym ), symbGetSubType( vreg->sym ) )
				end select
			end select
		end if

		if( do_deref = FALSE ) then
			'' Plain access is enough
			exit select
		end if

		'' Deref/addrof trick

		'' Add '&' if symbol isn't emitted as pointer already
		if( is_c_array = FALSE ) then
			l = exprNewCAddress( l )
		end if
		if( have_offset ) then
			if( is_c_array ) then
				'' Cast to uintptr_t to work around gcc outside-of-array-bounds
				'' warnings if we are casting from FBSTRING array to pointer.
				'' fbc uses a virtual pointer for the array's @(0,..) index
				'' location; technically this is undefined behaviour in C and
				'' is impossible to cast away even when using pointer-only casts
				'' in the same expression.  Some compilers warn when setting a
				'' pointer for the array's virtual index location.
				''
				'' Emscripten's wasm backend needs this too.  If the generated C
				'' keeps the base as a byte pointer, LLVM may lower the constant
				'' array diff into the wasm load offset and leave a negative
				'' dynamic index in front of it.  That can trap even though the
				'' final address is inside the array object.
				''
				'' To fix this for compliant C code, should fix the design
				'' and rewrite the array descriptor to contain only the
				'' offset value from actual memory pointer and compute the
				'' array access fully on each array element access.
				l = exprNewCAST( FB_DATATYPE_UINT, NULL, l )
			else
				'' Cast to ubyte ptr to work around C's pointer arithmetic
				l = exprNewCAST( hCBytePointerType(l->dtype), NULL, l )
			end if
			if( vreg->vidx <> NULL ) then
				if( byte_index = NULL ) then byte_index = exprNewVREG(vreg->vidx)
				l = exprNewBOP( AST_OP_ADD, l, byte_index )
			end if
			if( vreg->ofs <> 0 ) then
				l = exprNewBOP( AST_OP_ADD, l, exprNewIMMi( vreg->ofs ) )
			end if
		end if

		if( is_lvalue = FALSE andalso hIsScalarLoadType(vreg->dtype) ) then
			hCReadAddress(l)
		end if

		if( (is_lvalue = FALSE) andalso _
		    hAddrNeedsUnalignedAccess( vreg->dtype, vreg->subtype, l, hVregBaseMayBeUnaligned( vreg ) ) ) then
			l = exprNewUnalignedLoad( vreg->dtype, vreg->subtype, l )
		else
			'' cast to vregdtype*
			l = exprNewCAST( hCAccessPointerType(accessdtype, is_lvalue), vreg->subtype, l )

			'' deref to get vregdtype
			l = exprNewUOP( AST_OP_DEREF, l )
		end if

	case IR_VREGTYPE_OFS
		'' Accessing a global, including string literals and function
		'' symbols (used when taking address of functions).
		if( typeIsPtr(vreg->dtype) ) then
			l = exprNewOFFSET( vreg->sym, vreg->ofs, typeDeref(vreg->dtype), vreg->subtype )
		else
			l = exprNewOFFSET( vreg->sym, vreg->ofs )
		end if

	case IR_VREGTYPE_IMM
		'' An immediate -- a constant value
		'' The integer literal can be emitted as 32bit or 64bit,
		'' signed or unsigned, and afterwards it should be cast to the
		'' vreg's type for cases like
		''    "cptr(any ptr, 0)"
		'' where the constant has some pointer type, and we'd like to
		'' avoid gcc warnings about pointers...

		dtype = vreg->dtype
		if( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
			l = exprNewIMMf( vreg->value.f, dtype )
		elseif( dtype = FB_DATATYPE_BOOLEAN ) then
			l = exprNewIMMi( iif( vreg->value.i, 1, 0 ) )
		else
			l = exprNewIMMi( vreg->value.i, dtype )
		end if

	case IR_VREGTYPE_REG
		'' Access to existing vreg (e.g. BOP result)
		l = exprLookup( vreg->reg )
		if( l = NULL ) then
			'' Accessing a previous vreg a second time
			'' This currently should only happen with -exx pointer
			'' or array checking function calls, where the AST is
			'' reusing the function result vreg. Since the vreg is
			'' a call result, the C backend will have emitted a
			'' temp var, allowing this reuse to work.
			l = exprNewTEXT( vreg->dtype, vreg->subtype, "fb_c_vreg_" + str( vreg->reg ) )
		end if

	case else
		errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
		'' Keep error recovery traversable after rejecting an unknown IR kind.
		return exprNewIMMi( 0 )

	end select

	if( is_lvalue = FALSE ) then
		if( vreg->typ = IR_VREGTYPE_REG ) then
			l = exprNewValueCAST( vreg->dtype, vreg->subtype, l )
		else
			dtype = vreg->dtype
			if( vreg->typ = IR_VREGTYPE_VAR andalso vreg->ofs = 0 andalso vreg->vidx = NULL ) then
				dtype = hCReadOnlyLocalType(vreg->sym, dtype)
			end if
			l = exprNewCAST( dtype, vreg->subtype, l )
		end if
	end if

	if( is_lvalue = FALSE andalso typeIsPtr(l->dtype) andalso l->class = EXPRCLASS_MEMBER ) then
		'' Natural record fields retain their ABI declarations. Restore the
		'' BASIC pointee contract when loading a pointer value from a field.
		var value = exprNew(EXPRCLASS_CAST, l->dtype, l->subtype)
		value->l = l
		l = value
	end if
	function = l
end function

private function exprNewVREGAddr _
	( _
		byval vreg as IRVREG ptr _
	) as EXPRNODE ptr

	dim as EXPRNODE ptr l = any
	dim as EXPRNODE ptr byte_index = NULL

	select case as const( vreg->typ )
	case IR_VREGTYPE_VAR, IR_VREGTYPE_IDX, IR_VREGTYPE_PTR
		if( vreg->sym = NULL ) then
			if( vreg->vidx ) then
				l = exprNewVREG( vreg->vidx )
				var member = exprTryCMember(l, vreg->ofs, vreg->dtype, vreg->subtype)
				if( member <> NULL ) then return exprNewCAddress( member)

				if( vreg->ofs <> 0 ) then
					l = exprNewCAST( hCBytePointerType(l->dtype), NULL, l )
					l = exprNewBOP( AST_OP_ADD, l, exprNewIMMi( vreg->ofs ) )
				end if
			else
				l = exprNewIMMi( vreg->ofs )
			end if

			function = l
			exit function
		end if

		var is_c_array = symbIsCArray( vreg->sym )
		var have_offset = ((vreg->ofs <> 0) or (vreg->vidx <> NULL))

		l = exprNewSYM( vreg->sym )
		if( is_c_array ) then
			if( vreg->vidx <> NULL ) then byte_index = exprNewVREG(vreg->vidx)
			var indexed = exprTryCArrayIndex(vreg->sym, byte_index, vreg->ofs, vreg->dtype, vreg->subtype, TRUE)
			if( indexed <> NULL ) then
				exprFreeTree(l)
				return exprNewCAddress( indexed)
			end if
		end if
		if( (is_c_array = FALSE) andalso (vreg->vidx = NULL) andalso _
		    ((typeIsPtr(l->dtype) = FALSE) or (vreg->typ = IR_VREGTYPE_PTR)) ) then
			var member = exprTryCMember(l, vreg->ofs, vreg->dtype, vreg->subtype)
			if( member <> NULL ) then return exprNewCAddress( member)
		end if

		if( is_c_array = FALSE ) then
			l = exprNewCAddress( l )
		end if

		if( have_offset ) then
			if( is_c_array ) then
				l = exprNewCAST( FB_DATATYPE_UINT, NULL, l )
			else
				l = exprNewCAST( hCBytePointerType(l->dtype), NULL, l )
			end if
			if( vreg->vidx <> NULL ) then
				if( byte_index = NULL ) then byte_index = exprNewVREG(vreg->vidx)
				l = exprNewBOP( AST_OP_ADD, l, byte_index )
			end if
			if( vreg->ofs <> 0 ) then
				l = exprNewBOP( AST_OP_ADD, l, exprNewIMMi( vreg->ofs ) )
			end if
		end if

	case IR_VREGTYPE_OFS
		l = exprNewOFFSET( vreg->sym, vreg->ofs )

	case else
		errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
		l = exprNewIMMi( 0 )
	end select

	function = l
end function

private function hGetUnalignedLvalueAddr _
	( _
		byval vreg as IRVREG ptr, _
		byval lvalue as EXPRNODE ptr _
	) as EXPRNODE ptr

	if( lvalue = NULL ) then
		return NULL
	end if

	if( hCMemberViewIsAggregate(lvalue) ) then
		lvalue->member_view = C_MEMBER_VIEW_STORAGE
		return exprNewUOP(AST_OP_ADDROF, lvalue)
	end if

	if( lvalue->class <> EXPRCLASS_UOP ) then
		return NULL
	end if

	if( lvalue->op <> AST_OP_DEREF ) then
		return NULL
	end if

	if( hAddrNeedsUnalignedAccess( vreg->dtype, vreg->subtype, lvalue->l, hVregBaseMayBeUnaligned( vreg ) ) ) then
		'' Transfer the owned address to the store; the dereference wrapper
		'' is no longer part of the emitted expression.
		var addr = lvalue->l
		exprFreeNode(lvalue)
		function = addr
	else
		function = NULL
	end if
end function

private sub hEmitUnalignedStore _
	( _
		byval vreg as IRVREG ptr, _
		byval addr as EXPRNODE ptr, _
		byval r as EXPRNODE ptr _
	)

	dim as string typename
	dim as string addrtext
	dim as string valuetext
	dim as string s

	'' The temporary binding is writable, but pointer-target qualifiers are
	'' part of its value type and must agree with the converted initializer.
	typename = hEmitType( typeUnsetIsConst( vreg->dtype ), vreg->subtype, TRUE )
	var tempname = hCUnalignedTempName()
	var member = exprTryCMember( addr, 0, vreg->dtype, vreg->subtype, TRUE )
	if( member <> NULL ) then addr = exprNewCAddress( member )
	addrtext = exprFlush( hCUnalignedAddress(addr) )
	valuetext = exprFlush( r )

	s = "__extension__ ({ "
	s += typename + " " + tempname + " = "
	s += valuetext
	s += "; __builtin_memcpy( (void*)("
	s += addrtext
	s += "), &" + tempname + ", sizeof( " + tempname + " ) ); });"

	hWriteLine( s )
end sub

private sub _emitLabel( byval label as FBSYMBOL ptr )
	'' Only when inside normal procedures
	'' (NAKED procedures don't increase the indentation)
	if( sectionInsideProc( ) ) then
		var labeltext = hGetCId( label ) + ":;"
		hWriteLine( labeltext )
		if( label = ctx.procentrylabel ) then
			'' Keep any preceding #line directive; only the label is optional.
			ctx.procentrysection = ctx.section
			ctx.procentrybytes = ctx.sections(ctx.section).indent + len( labeltext ) + len( NEWLINE )
			ctx.procentryoffset = ctx.sections(ctx.section).bufferlen - ctx.procentrybytes
		end if
	end if
end sub

'' store an expression into a vreg
private sub exprSTORE _
	( _
		byval vr as IRVREG ptr, _
		byval r as EXPRNODE ptr, _
		byval has_sidefx as integer = FALSE _
	)

	static as string ln, tempvar
	dim as EXPRNODE ptr l = NULL
	dim as EXPRNODE ptr addr = NULL
	dim as integer store_dtype = vr->dtype
	if( vr->typ = IR_VREGTYPE_VAR andalso vr->ofs = 0 andalso vr->vidx = NULL ) then
		store_dtype = hCReadOnlyLocalType(vr->sym, store_dtype)
	end if

	if( vr->typ = IR_VREGTYPE_VAR andalso hCScalarStoreUnusedValue(vr->sym) andalso _
	    (vr->ofs = 0) andalso (vr->vidx = NULL) andalso _
	    (clngint(typeGetSize(vr->dtype)) = symbGetRealSize(vr->sym)) ) then
		'' An unread scalar destination does not own the evaluation. In
		'' particular, floating conversions still use the original helpers
		'' and retain their rounding and floating-environment behaviour.
		r = exprNewCAST( vr->dtype, vr->subtype, r )
		var value = exprFlush(r)
		if( hCScalarStoreValueContext(vr->sym) ) then value = hCEvaluatedScalar(vr->dtype, value)
		hWriteLine( "(void)(" + value + ");" )
		exit sub
	end if

	if( vr->typ = IR_VREGTYPE_VAR andalso hCAutoDescUnused(vr->sym) ) then
		'' Only pure, unread descriptor initialization can reach this path.
		exprFreeTree(r)
		exit sub
	end if

	if( irIsREG( vr ) ) then
		if( has_sidefx ) then
			'' Expressions (REG) with side-effects (i.e. CALLs)
			'' should be emitted immediately in-place, that's what
			'' the AST expects, like with the ASM backend.
			''  a) due to the side-effects
			''  b) because sometimes it leaves the vreg dangling
			''     and relies only on the side-effects, e.g. when
			''     calling functions that return their UDT result
			''     through a hidden parameter. The CALL expression
			''     must be emitted, but the result vreg won't ever
			''     be accessed.
			''
			'' -> Create a temp var and use that as the new vreg
			'' expression, instead of the original expr itself:
			''    type tempvar = expr;
			'' (no cast needed, the assignment has the same effect)
			tempvar = "fb_c_vreg_" + str( vr->reg )

			ln = hEmitType( typeUnsetIsConst( vr->dtype ), vr->subtype, TRUE )
			ln += " " + tempvar
			if( typeIsPtr( vr->dtype ) orelse _
			    (symbIsValistStructArray( vr->dtype, vr->subtype ) = FALSE) ) then
				'' These temporaries hold a value only after this call executes.
				'' Separate storage from evaluation so a branch past the call
				'' does not jump over a C initializer. hEmitStructWithFields()
				'' makes field bindings writable, so aggregate assignment is
				'' valid too, without discarding pointed-to const qualifiers.
				'' Keep initializers for builtin va_list array types, which C
				'' does not allow us to assign. Skipped calls invent no value.
				hWriteDeclaration( ln + ";" )
				ln = tempvar
			end if
			ln += " = "
			ln += exprFlush( r )
			ln += ";"

			hWriteLine( ln )

			r = exprNewTEXT( vr->dtype, vr->subtype, tempvar )
		else
			r = exprNewValueCAST( vr->dtype, vr->subtype, r )
		end if

		'' Put the expression on hold, it'll be used in the following
		'' access to that vreg, instead of being emitted right here
		'' as a #define or temp var.
		exprCache( vr->reg, r )
	else
		'' Store into existing vreg (assign to var/deref, i.e. lvalue)
		''    vreg = (vregtype)r;
		'' FB allows noconv casts (no data class/size change) on the
		'' lhs, but C does not, the rhs should be casted here instead,
		'' although it probably doesn't matter much either way.
		'' 1st to the desired vreg type
		r = exprNewCAST( store_dtype, vr->subtype, r )

		l = exprNewVREG( vr, TRUE )
		if( store_dtype <> vr->dtype ) then l->dtype = store_dtype
		addr = hGetUnalignedLvalueAddr( vr, l )
		if( addr <> NULL ) then
			hEmitUnalignedStore( vr, addr, r )
			exit sub
		end if

		if( typeIsPtr( l->dtype ) or typeIsPtr( r->dtype ) or _
		    (typeGetDtAndPtrOnly(l->dtype) = FB_DATATYPE_CHAR) ) then
			'' Match the physical C lvalue after the BASIC value conversion.
			'' A char store preserves byte bits even on signed-char targets.
			r = exprNewCAST( l->dtype, l->subtype, r )
		end if

		ln = exprFlush( l )
		ln += " = "
		ln += exprFlush( r )
		ln += ";"

		hWriteLine( ln )
	end if

end sub

private sub _emitBop _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval v2 as IRVREG ptr, _
		byval vr as IRVREG ptr, _
		byval label as FBSYMBOL ptr, _
		byval options as IR_EMITOPT _
	)

	dim as EXPRNODE ptr l = any, r = any

	l = exprNewVREG( v1 )
	r = exprNewVREG( v2 )

	'' Conditional branch?
	if( label ) then
		assert( vr = NULL )
		static as string s
		s = "if( "
		if( (options and IR_EMITOPT_REL_DOINVERSE) <> 0 ) then
			s += "!("
		end if
		s += exprFlush( exprNewBOP( op, l, r ) )
		if( (options and IR_EMITOPT_REL_DOINVERSE) <> 0 ) then
			s += ")"
		end if
		s += ") goto "
		s += hGetCId( label )
		s += ";"
		hWriteLine( s )
		exit sub
	end if

	if( vr = NULL ) then
		vr = v1
	end if

	select case as const( op )
	case AST_OP_EQ, AST_OP_NE, AST_OP_GT, AST_OP_LT, AST_OP_GE, AST_OP_LE
		l = exprNewBOP( op, l, r )

		if( (options and IR_EMITOPT_REL_DOINVERSE) <> 0 ) then
			l = exprNewUOP( AST_OP_BOOLNOT, l )
		end if

		'' comparisons returning a boolean produce 0/1,
		'' comparisons returning an integer produce 0/-1.
		if( vr->dtype <> FB_DATATYPE_BOOLEAN ) then
			l = exprNewUOP( AST_OP_NEG, l )
		end if

	case AST_OP_ADD, AST_OP_SUB
		if( typeIsPtr(v1->dtype) andalso (typeIsPtr(v2->dtype) = FALSE) ) then
			if( hCScalePointerOffset(l->dtype, l->subtype, r) ) then
				l = exprNewBOP(op, l, r)
				exit select
			end if
		end if
		'' Other byte displacements keep their original scale and qualifiers.
		if( typeIsPtr( v1->dtype ) ) then
			l = exprNewCAST( hCBytePointerType(l->dtype), NULL, l )
		end if
		if( typeIsPtr( v2->dtype ) ) then
			r = exprNewCAST( hCBytePointerType(r->dtype), NULL, r )
		end if
		l = exprNewBOP( op, l, r )

	case AST_OP_DIV
		'' Ensure '/' means floating point divide by casting to double
		'' For AST_OP_INTDIV this is not needed, since the AST will already
		'' cast both operands to integer before doing the intdiv.
		l = exprNewCAST( FB_DATATYPE_DOUBLE, NULL, l )
		r = exprNewCAST( FB_DATATYPE_DOUBLE, NULL, r )
		l = exprNewBOP( op, l, r )

	case AST_OP_MUL, AST_OP_INTDIV, AST_OP_MOD, _
	     AST_OP_AND, AST_OP_OR, AST_OP_XOR
		l = exprNewBOP( op, l, r )

	case AST_OP_SHL, AST_OP_SHR
		'' Mask the shift amount to ensure it's valid, avoiding C UB
		r = exprNewBOP( AST_OP_AND, r, exprNewIMMi( typeGetBits( l->dtype ) - 1 ) )

		l = exprNewBOP( op, l, r )

	case AST_OP_EQV
		l = exprNewBOP( AST_OP_XOR, l, r )
		if( vr->dtype = FB_DATATYPE_BOOLEAN ) then
			'' vr = (v1 xor v2) xor 1
			l = exprNewBOP( AST_OP_XOR, l, exprNewIMMi( 1 ) )
		else
			'' vr = not (v1 xor v2)
			l = exprNewUOP( AST_OP_NOT, l )
		end if

	case AST_OP_IMP
		if( vr->dtype = FB_DATATYPE_BOOLEAN ) then
			'' vr = (v1 xor 1) or v2
			l = exprNewBOP( AST_OP_XOR, l, exprNewIMMi( 1 ) )
		else
			'' vr = (not v1) or v2
			l = exprNewUOP( AST_OP_NOT, l )
		end if
		l = exprNewBOP( AST_OP_OR, l, r )

	end select

	exprSTORE( vr, l )
end sub

private sub _emitUop _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval vr as IRVREG ptr _
	)

	if( vr = NULL ) then
		vr = v1
	end if

	var expr = exprNewVREG( v1 )

	'' boolean NOT?
	if( (op = AST_OP_NOT) and (vr->dtype = FB_DATATYPE_BOOLEAN) ) then
		'' booleans store 0/1, and a boolean NOT is supposed to produce
		'' the inverse 1/0 boolean. Thus it can't be implemented as
		'' bitwise NOT.
		'' C's logical NOT produces the required inverse 1/0 value.
		expr = exprNewUOP( AST_OP_BOOLNOT, expr )
	else
		expr = exprNewUOP( op, expr )
	end if

	exprSTORE( vr, expr )

end sub

'' v1 = cast( <v1's dtype>, v2 )
private sub _emitConvert( byval v1 as IRVREG ptr, byval v2 as IRVREG ptr )
	var r = exprNewVREG( v2 )
	'' A read through a pointer can carry const qualification even when the
	'' value being converted is a Boolean. Ignore qualifiers here, while
	'' retaining pointer levels so a Boolean pointer is never treated as one.
	var result_is_boolean = (typeGetDtAndPtrOnly(v1->dtype) = FB_DATATYPE_BOOLEAN)
	var input_is_boolean = (typeGetDtAndPtrOnly(r->dtype) = FB_DATATYPE_BOOLEAN)

	'' float/int := bool?
	if( (result_is_boolean = FALSE) and input_is_boolean ) then
		'' 0/-1 := 0/1
		if( r->class = EXPRCLASS_IMM ) then
			r->val.i = (r->val.i <> 0)
			r->dtype = FB_DATATYPE_LONG
		else
			r = exprNewUOP( AST_OP_NEG, r )
		end if

	'' bool := float/int?
	elseif( result_is_boolean and (input_is_boolean = FALSE) ) then
		'' 0/1 := zero/non-zero
		if( r->class = EXPRCLASS_IMM ) then
			'' (FB's <> 0 produces 0/-1 so we have to negate to get 0/1)
			r->val.i = - iif( typeGetClass( r->dtype ) = FB_DATACLASS_FPOINT, _
			                  r->val.f <> 0, r->val.i <> 0 )
			r->dtype = FB_DATATYPE_LONG
		else
			'' (expr != 0) (C's != produces the 0/1 we want)
			r = exprNewBOP( AST_OP_NE, r, exprNewIMMi( 0 ) )
		end if

	'' int := float? Needs special treatment to achieve FB's rounding behaviour
	elseif( (typeGetClass( v1->dtype ) = FB_DATACLASS_INTEGER) and _
	        (typeGetClass( r->dtype ) = FB_DATACLASS_FPOINT) ) then

		'' ((type)fb_F2I( r ))
		''
		'' If converting to integer <= int32: use fb_*2I()
		'' If converting to uint32 or int64: use fb_*2L()
		''   (uint32: need to avoid truncation to int32)
		'' If converting to uint64: use fb_*2UL()
		''   (need to avoid truncation to int64)

		dim s as string, tempdtype as integer
		select case( typeGetSizeType( v1->dtype ) )
		case is <= FB_SIZETYPE_INT32
			if( r->dtype = FB_DATATYPE_SINGLE ) then
				s = "fb_F2I" : ctx.usedbuiltins or= BUILTIN_F2I
			else
				s = "fb_D2I" : ctx.usedbuiltins or= BUILTIN_D2I
			end if
			tempdtype = FB_DATATYPE_LONG
		case is <= FB_SIZETYPE_INT64
			if( r->dtype = FB_DATATYPE_SINGLE ) then
				s = "fb_F2L" : ctx.usedbuiltins or= BUILTIN_F2L
			else
				s = "fb_D2L" : ctx.usedbuiltins or= BUILTIN_D2L
			end if
			tempdtype = FB_DATATYPE_LONGINT
		case FB_SIZETYPE_INT128
			if( r->dtype = FB_DATATYPE_SINGLE ) then
				s = "fb_F2I128" : ctx.usedbuiltins or= BUILTIN_F2I128
			else
				s = "fb_D2I128" : ctx.usedbuiltins or= BUILTIN_D2I128
			end if
			tempdtype = FB_DATATYPE_INTEGER128
		case FB_SIZETYPE_UINT128
			if( r->dtype = FB_DATATYPE_SINGLE ) then
				s = "fb_F2U128" : ctx.usedbuiltins or= BUILTIN_F2U128
			else
				s = "fb_D2U128" : ctx.usedbuiltins or= BUILTIN_D2U128
			end if
			tempdtype = FB_DATATYPE_UINTEGER128
		case else
			if( r->dtype = FB_DATATYPE_SINGLE ) then
				s = "fb_F2UL" : ctx.usedbuiltins or= BUILTIN_F2UL
			else
				s = "fb_D2UL" : ctx.usedbuiltins or= BUILTIN_D2UL
			end if
			tempdtype = FB_DATATYPE_ULONGINT
		end select
		s += "( " + exprFlush( r ) + " )"
		r = exprNewTEXT( tempdtype, NULL, s )
	end if

	exprSTORE( v1, r )
end sub

private sub _emitStore( byval v1 as IRVREG ptr, byval v2 as IRVREG ptr )
	exprSTORE( v1, exprNewVREG( v2 ) )
end sub

private sub _emitSpillRegs( )
	/' do nothing '/
end sub

private sub _emitLoad( byval v1 as IRVREG ptr )
	/' do nothing '/
end sub

private sub _emitLoadRes( byval v1 as IRVREG ptr, byval vr as IRVREG ptr )
	_emitStore( vr, v1 )
	if( hUsesCLongDoubleCABI( ctx.currproc ) andalso _
	    hIsCLongDoubleType( vr->dtype, vr->subtype ) ) then
		var identifier = "fb_c_clongdouble_return_" + str( symbUniqueId( ) )
		var type_name = hEmitType( typeUnsetIsConst( vr->dtype ), vr->subtype, TRUE )
		hWriteLine( type_name + " " + identifier + " = " + _
		            exprFlush( exprNewVREG( vr ) ) + ";" )
		hWriteLine( "return fb_LongDoubleToNative( (const void*)&" + _
		            identifier + ", " + str( vr->subtype->lgt ) + "u );" )
	else
		hWriteLine( "return " + exprFlush( exprNewVREG( vr ) ) + ";" )
	end if
end sub

private sub _emitAddr _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval vr as IRVREG ptr _
	)

	dim as EXPRNODE ptr l = NULL

	select case( op )
	case AST_OP_ADDROF
		'' Taking address of label?
		if( (v1->typ = IR_VREGTYPE_VAR) and (v1->sym <> NULL) ) then
			if( symbIsLabel( v1->sym ) ) then
				''
				'' special case used by FB error handling code
				''
				'' The VAR vreg's dtype for the label access
				'' is useless because 1) the AST is inconsistently
				'' using integer or byte and 2) labels cannot be
				'' casted anyways.
				''
				'' The only thing that matters is the dtype of the
				'' result vreg (the type of the ADDROF expression).
				''
				l = exprNewSYM( v1->sym )
				l = exprNewCAST( vr->dtype, vr->subtype, l )
				exit select
			end if
		end if

		'' Taking an address requires storage, but does not grant write access.
		l = exprNewCAddress( exprNewVREG( v1, TRUE, FALSE /' address only '/ ) )

	case AST_OP_DEREF
		'' Note: The deref is already done in the vreg itself; as in
		'' the ASM backend, no explicit deref operation is needed.
		l = exprNewVREG( v1 )

	end select

	exprSTORE( vr, l )
end sub

#include once "backend/c/c-array-callbacks.bi"
#include once "backend/c/c-gfx-callbacks.bi"

private function hCAllocationArgument( byval value as EXPRNODE ptr ) as EXPRNODE ptr
	'' FB accepts const object pointers for DEALLOCATE and REALLOCATE.
	'' Their C entry points require void*. C11 6.2.5p28 guarantees identical
	'' representations for void* and const void*. Copy the pointer objects;
	'' do not access the allocation, including when its value is NULL.
	if( typeIsPtr(value->dtype) = FALSE ) then return exprNewCAST(typeAddrOf(FB_DATATYPE_VOID), NULL, value)
	if( typeIsConstAt(value->dtype, 1) = FALSE ) then return exprNewCAST(typeAddrOf(FB_DATATYPE_VOID), NULL, value)
	if( typeGetPtrCnt(value->dtype) = 1 andalso typeGetDtOnly(value->dtype) = FB_DATATYPE_FUNCTION ) then
		'' Function addresses are not allocation handles. Keep their existing
		'' conversion diagnostic instead of applying an object-pointer bridge.
		return exprNewCAST(typeAddrOf(FB_DATATYPE_VOID), NULL, value)
	end if
	var source_name = hCUnalignedTempName()
	var result_name = hCUnalignedTempName()
	var text = "__extension__ ({ const void* " + source_name + " = "
	text += exprFlush(exprNewCAST(typeAddrOf(typeSetIsConst(FB_DATATYPE_VOID)), NULL, value))
	text += "; void* " + result_name + "; __builtin_memcpy( &" + result_name + ", &" + source_name
	text += ", sizeof(" + result_name + ") ); " + result_name + "; })"
	return exprNewTEXT(typeAddrOf(FB_DATATYPE_VOID), NULL, text)
end function

private sub hDoCall _
	( _
		byref s as string, _
		byval proc as FBSYMBOL ptr, _
		byval indirect as integer, _
		byval bytestopop as integer, _
		byval vr as IRVREG ptr, _
		byval level as integer _
	)

	dim as CREADONLYPROC ptr readonly_variant = NULL
	if( indirect = FALSE ) then
		readonly_variant = hCReadOnlyCallee(proc)
		if( readonly_variant <> NULL ) then
			s = *readonly_variant->identifier
			if( proc <> ctx.currproc ) then readonly_variant->external_call = TRUE
		end if
	end if
	dim as IRCALLARG ptr arg = any
	dim as integer dtype = any
	dim as FBSYMBOL ptr subtype = any
	dim as integer param_index = 0
	dim as DZSTRING args
	DZstrZero( args )
	dim as integer semantic_result_dtype = FB_DATATYPE_INVALID
	dim as FBSYMBOL ptr semantic_result_subtype = NULL
	if( vr <> NULL ) then
		semantic_result_dtype = vr->dtype
		semantic_result_subtype = vr->subtype
	end if
	dim as longint semantic_call = fbSemanticModelCallAbiBegin(proc, "c", indirect, vr <> NULL, _
		semantic_result_dtype, semantic_result_subtype, "", "", level, bytestopop)

	dim as CGFXBLENDER ptr gfx_adapter = NULL
	dim as string gfx_callback, gfx_context
	dim as integer gfx_userdata_index = -1

	'' Flush argument list
	s += "( "
	arg = listGetTail( @irhl.callargs )
	while( arg andalso (arg->level = level) )
		dim as IRCALLARG ptr prev = listGetPrev( arg )
		dim as integer semantic_input_dtype = arg->vr->dtype
		dim as FBSYMBOL ptr semantic_input_subtype = arg->vr->subtype
		dim as string semantic_kind = "unmapped-value"
		if( arg->param <> NULL ) then
			semantic_kind = "value"
			if( arg->param->param.mode = FB_PARAMMODE_VARARG ) then semantic_kind = "variadic-value"
		end if

		dim as integer is_longdouble_vararg = _
			arg->param andalso _
			(arg->param->param.mode = FB_PARAMMODE_VARARG) andalso _
			hIsCLongDoubleType( semantic_input_dtype, semantic_input_subtype )
		dim as integer is_longdouble_cabi_argument = FALSE
		var expr = exprNewVREG( arg->vr, is_longdouble_vararg )

		'' param will be NULL for hidden struct result arg, since
		'' no corresponding PARAM exists.
		if( arg->param andalso (arg->param->param.mode <> FB_PARAMMODE_VARARG)  ) then
			'' Cast arg to param's type to prevent gcc warning.
			'' (this will be done by astNewARG() already, except for
			'' BYREF AS ANY params, where the exact type will only
			'' be known later, or never)
			symbGetRealParamDtype( arg->param, dtype, subtype )
			var param_proc = symbGetParent( arg->param )
			var gfx_blender = hGfxBlenderType(param_proc, param_index)
			if( gfx_blender <> NULL ) then
				expr = hGfxBlenderArgument(expr, gfx_blender, gfx_adapter, gfx_callback, gfx_context)
				if( gfx_adapter <> NULL ) then gfx_userdata_index = param_index + 1
				semantic_kind = "graphics-blender"
			elseif( hIsArrayCallbackParam( param_proc, param_index ) ) then
				expr = hArrayCallbackArgument( expr )
				semantic_kind = "array-callback"
			else
				dim as integer semantic_original_dtype = dtype
				dim as FBSYMBOL ptr semantic_original_subtype = subtype
				var allocation_argument = FALSE
				if( param_proc <> NULL ) then
					var mangled = *symbGetMangledName( param_proc )
					allocation_argument = hEmitMutableLibcPtrParam(mangled, param_index)
					hAdjustCParamType( mangled, param_index, dtype, subtype )
				end if
				if( (dtype <> semantic_original_dtype) or (subtype <> semantic_original_subtype) ) then semantic_kind = "libc-adjusted"
				if( hCReadOnlyParam(readonly_variant, arg->param) ) then
					dtype = typeAddrOf(typeSetIsConst(typeDeref(dtype)))
					semantic_kind = "readonly-implementation"
				end if
				is_longdouble_cabi_argument = _
					hUsesCLongDoubleCABI( proc ) andalso _
					hIsCLongDoubleType( dtype, subtype )
				'' A fixed ABI still needs an operand for an unobserved parameter.
				'' A pure literal address can be replaced without evaluating a
				'' mutable cast or changing argument evaluation elsewhere.
				if( indirect = FALSE andalso hCUnobservedLiteral(arg->param, expr) ) then
					exprFreeTree(expr)
					expr = exprNewIMMi(0)
					'' Value selection retains the existing fixed-argument ABI kind.
				end if
				if( allocation_argument ) then
					expr = hCAllocationArgument(expr)
				else
					expr = exprNewCAST( dtype, subtype, expr )
				end if
			end if
		end if
		'' A hidden result argument is absent from the declared parameter list.
		if( arg->param <> NULL ) then param_index += 1

		dim as integer semantic_selected_dtype = expr->dtype
		dim as FBSYMBOL ptr semantic_selected_subtype = expr->subtype
		if( semantic_kind = "array-callback" ) then semantic_selected_dtype = FB_DATATYPE_INVALID
		dim as string semantic_value
		if( is_longdouble_vararg ) then
			var source_address = exprNewCAddress( expr )
			semantic_value = "fb_LongDoubleToNative((const void*)" + _
				exprFlush( source_address ) + ", " + _
				str( semantic_input_subtype->lgt ) + "u)"
			semantic_kind = "variadic-clongdouble-native"
		elseif( is_longdouble_cabi_argument ) then
			semantic_value = hCLongDoubleToNativeExpression( _
				exprFlush( expr ), subtype )
			semantic_kind = "fixed-clongdouble-native"
			semantic_selected_dtype = FB_DATATYPE_INVALID
			semantic_selected_subtype = NULL
		else
			semantic_value = exprFlush( expr )
			if( gfx_adapter <> NULL andalso param_index - 1 = gfx_userdata_index ) then
				hWriteLine("struct " + hGfxContextType(gfx_adapter) + " " + gfx_context + _
					" = { " + gfx_callback + ", " + semantic_value + " };")
				semantic_value = "&" + gfx_context
				semantic_kind = "graphics-blender-context"
				semantic_selected_dtype = FB_DATATYPE_INVALID
				semantic_selected_subtype = NULL
				gfx_adapter = NULL
			end if
		end if
		fbSemanticModelCallAbiArgument(semantic_call, arg->param, 0, 1, semantic_kind, _
			semantic_input_dtype, semantic_input_subtype, semantic_selected_dtype, semantic_selected_subtype, "", "", semantic_value)
		DZstrConcatAssign( args, semantic_value )

		listDelNode( @irhl.callargs, arg )

		if( prev ) then
			if( prev->level = level ) then
				DZstrConcatAssign( args, ", " )
			end if
		end if

		arg = prev
	wend
	if( args.data <> NULL ) then
		s += *args.data
	end if
	DZstrAllocate( args, 0 )
	s += " )"
	fbSemanticModelCallAbiEnd(semantic_call, s)
	if( (vr <> NULL) andalso hUsesCLongDoubleCABI( proc ) andalso _
	    hIsCLongDoubleType( vr->dtype, vr->subtype ) ) then
		s = hCLongDoubleFromNativeExpression( s, vr->subtype )
	end if

	if( vr = NULL ) then
		s += ";"
		hWriteLine( s )
	else
		exprSTORE( vr, exprNewTEXT( vr->dtype, vr->subtype, s ), TRUE )
	end if

end sub

private sub _emitCall _
	( _
		byval proc as FBSYMBOL ptr, _
		byval bytestopop as integer, _
		byval vr as IRVREG ptr, _
		byval level as integer _
	)

	static as string s

	hCLeafReference( proc )
	s = hGetCId( proc )
	hDoCall( s, proc, FALSE, bytestopop, vr, level )

end sub

private sub _emitCallPtr _
	( _
		byval proc as FBSYMBOL ptr, _
		byval v1 as IRVREG ptr, _
		byval vr as IRVREG ptr, _
		byval bytestopop as integer, _
		byval level as integer _
	)

	static as string s

	s = "(" + exprFlush( exprNewVREG( v1 ) ) + ")"
	hDoCall( s, proc, TRUE, bytestopop, vr, level )

end sub

private sub hEmitComputedGotoAddress( )
	if( ctx.computedgotoaddress or (not sectionInsideProc( )) ) then
		exit sub
	end if

	if( ((env.clopt.backend = FB_BACKEND_CLANG) or _
	     (env.clopt.backend = FB_BACKEND_GCC) or _
	     (env.clopt.target = FB_COMPTARGET_DARWIN)) and _
	    (env.clopt.target <> FB_COMPTARGET_JS) ) then
		'' Clang requires an address-of-label expression in a function with
		'' a computed goto, including ordinary ERROR statements. If no real
		'' address has been emitted, refer to the existing procedure entry
		'' label. An artificial label here would let
		'' Clang's control-flow analysis jump past a temporary's initialization
		'' into its use by the following indirect jump.
		'' Casting the address to void needs no unused temporary variable.
		'' See https://bugs.llvm.org/show_bug.cgi?id=18658
		''
		'' Emscripten cannot lower blockaddress expressions, so the JavaScript
		'' target must continue to omit this workaround.
		assert( ctx.procentrylabel <> NULL )
		hWriteLine( "#if defined(__clang__)", TRUE )
		hWriteLine( "(void)&&" + hGetCId( ctx.procentrylabel ) + ";" )
		hWriteLine( "#endif", TRUE )
		ctx.computedgotoentry = TRUE
		ctx.computedgotoaddress = TRUE
	end if
end sub

private sub _emitJumpPtr( byval v1 as IRVREG ptr )
	var jumptarget = exprFlush( exprNewVREG( v1 ), TRUE )
	ctx.computedgotodispatch = TRUE
	if( ctx.computedgotoopaque ) then hEmitComputedGotoAddress( )
	'' The procedure-end pass has seen every real label address. Its local
	'' macro selects the transfer form without buffering or evaluating this
	'' expression again, including when the destination appears later.
	hWriteLine( "fb_c_indirect_jump(" + jumptarget + ");" )
end sub

private sub _emitBranch( byval op as integer, byval label as FBSYMBOL ptr )
	assert( op = AST_OP_JMP )
	hWriteLine( "goto " + hGetCId( label ) + ";" )
end sub

private sub _emitJmpTb _
	( _
		byval v1 as IRVREG ptr, _
		byval tbsym as FBSYMBOL ptr, _
		byval values as ulongint ptr, _
		byval labels as FBSYMBOL ptr ptr, _
		byval labelcount as integer, _
		byval deflabel as FBSYMBOL ptr, _
		byval bias as ulongint, _
		byval span as ulongint _
	)

	dim as string temp
	dim as EXPRNODE ptr l = any

	var dtype = v1->dtype
	assert( (dtype = FB_DATATYPE_UINT) or (dtype = FB_DATATYPE_ULONGINT) )

	'' SELECT CASE AS CONST always uses a temp var, no need to worry about side effects
	assert( v1->typ = IR_VREGTYPE_VAR )
	temp = exprFlush( exprNewVREG( v1 ) )

	if( labelcount <= 0 ) then
		'' The selector is already evaluated. Consume its temporary before
		'' branching; a use after the unconditional jump would be dead code.
		hWriteLine( "(void)" + temp + ";", TRUE )
		hWriteLine( "goto " + hGetCId( deflabel ) + ";", TRUE )
		exit sub
	end if

	'' Preserve the unique-ID allocation used by the old table declaration.
	'' Later source/model names must not depend on the C dispatch spelling.
	symbUniqueId( )

	'' Give the C compiler explicit destinations. A computed goto admits
	'' every address-taken label in the procedure to its control-flow graph,
	'' including labels beyond BASIC initializations in other SELECT blocks.
	'' SWITCH retains unsigned bias wrapping and lets the target compiler
	'' choose a jump table or comparisons without exporting label addresses.
	l = exprNewTEXT( dtype, NULL, temp )
	if( bias <> 0 ) then
		l = exprNewBOP( AST_OP_SUB, l, exprNewIMMi( bias, dtype ) )
	end if
	var selector = exprFlush( l )

	'' Retain the original out-of-span branch before dispatching valid keys.
	l = exprNewTEXT( dtype, NULL, selector )
	l = exprNewBOP( AST_OP_GT, l, exprNewIMMi( span, dtype ) )
	hWriteLine( "if( " + exprFlush( l ) + " ) goto " + hGetCId( deflabel ) + ";", TRUE )
	hWriteLine( "switch( " + selector + " ) {", TRUE )
	sectionIndent( )
	for i as integer = 0 to labelcount - 1
		assert( values[i] <= span )
		l = exprNewIMMi( values[i], dtype )
		hWriteLine( "case " + exprFlush( l ) + ": goto " + hGetCId( labels[i] ) + ";", TRUE )
	next
	hWriteLine( "default: goto " + hGetCId( deflabel ) + ";", TRUE )
	sectionUnindent( )
	hWriteLine( "}", TRUE )
end sub

private sub _emitMem _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval v2 as IRVREG ptr, _
		byval bytes as longint, _
		byval fillchar as integer _
	)

	'' These operations promise byte access, not alignment of the pointed-to
	'' BASIC type. Clang otherwise checks/assumes record alignment even for
	'' a builtin memcpy destination. Preserve pointee constness in byte views.
	var destination = exprNewVREG(v1)
	destination = exprNewCAST(hCBytePointerType(destination->dtype), NULL, destination)
	select case op
	case AST_OP_MEMFILL
		hWriteLine("__builtin_memset( " + exprFlush(destination) + ", " + str(fillchar) + ", " + exprFlush( exprNewVREG( v2 ) ) + " );" )
	case AST_OP_MEMMOVE
		var source = exprNewVREG(v2)
		'' memcpy never writes its source. A recovered member can expose
		'' constness that was absent from the intermediate IR pointer type.
		source = exprNewCAST(hCBytePointerType(source->dtype, TRUE), NULL, source)
		hWriteLine("__builtin_memcpy( " + exprFlush(destination) + ", " + exprFlush(source) + ", " + str( cunsg( bytes ) ) + " );" )
	end select

end sub

private sub _emitMacro _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval v2 as IRVREG ptr, _
		byval vr as IRVREG ptr _
	)

	dim as EXPRNODE ptr l = any, r = any

	select case op
	case AST_OP_VA_START
		l = exprNewVREG( v1, TRUE /' mutable va_list '/ )
		r = exprNewVREG( v2 )
		hWriteLine( exprFlush( exprNewMACRO( op, FB_DATATYPE_INVALID, NULL, l, r ) ) + ";" )

	case AST_OP_VA_END
		l = exprNewVREG( v1, TRUE /' mutable va_list '/ )
		hWriteLine( exprFlush( exprNewMACRO( op, FB_DATATYPE_INVALID, NULL, l, NULL ) ) + ";" )

	case AST_OP_VA_ARG
		l = exprNewVREG( v1, TRUE /' mutable va_list '/ )
		l = exprNewMACRO( op, vr->dtype, vr->subtype, l, NULL )
		'' Struct va_arg results are rvalues in C. Store one into a temporary so
		'' aggregate-copy emission can take its address safely.
		exprSTORE( vr, l, typeGetDtAndPtrOnly( vr->dtype ) = FB_DATATYPE_STRUCT )

	case AST_OP_VA_COPY
		l = exprNewVREG( v1, TRUE /' destination va_list '/ )
		r = exprNewVREG( v2, TRUE /' source va_list object '/ )
		hWriteLine( exprFlush( exprNewMACRO( op, FB_DATATYPE_INVALID, NULL, l, r ) ) + ";" )

	end select

end sub

private sub _emitDECL( byval sym as FBSYMBOL ptr )
	dim as FBSYMBOL ptr array = any

	assert( symbIsLocal( sym ) )

	'' Emit locals/statics locally, except statics with dtor - those are
	'' handled in _procAllocStaticVars(), including their dynamic array
	'' descriptors (if any).
	if( hIsStaticWithDtor( sym ) ) then
		exit sub
	end if

	'' Check whether it's a dynamic array descriptor with a back link to
	'' the corresponding array that needs to be checked instead...
	'' (the descriptor needs to be handled like the array)
	assert( symbIsVar( sym ) )
	array = sym->var_.desc.array
	if( array ) then
		if( hIsStaticWithDtor( array ) ) then
			exit sub
		end if
	end if

	hMaybeEmitLocalVar( sym )
end sub

'':::::
private sub _emitDBG _
	( _
		byval op as integer, _
		byval proc as FBSYMBOL ptr, _
		byval lnum as integer, _
		ByVal filename As zstring ptr _
	)

	if( op = AST_OP_DBG_LINEINI ) then
		ctx.linenum = lnum
		if( filename <> NULL ) then
			hUpdateCurrentFileName( filename )
		end if
	end if

end sub

private sub _emitComment( byval text as zstring ptr )
	dim as integer length = any
	static as string s

	s = *text
	s = trim( s )

	length = len( s )
	if( length > 0 ) then
		if( s[length-1] = asc( "\" ) ) then
			s += "not_an_escape"
		end if
		hWriteLine( "// " + s, TRUE )
	end if
end sub

''
'' -asm intel: FB asm blocks are expected to be in Intel format as usual;
''             we have to convert them to the GCC format here.
''
'' Some x86-specific examples:
''
''    myLabel:
''    asm
''        mov eax, [myVar]
''        mov eax, offset myFunction
''        jmp myLabel
''    end asm
''
'' =>
''
''    myLabel:
''    asm("mov eax, [%0]\n"
''        : "+m" (MYVAR$0)            // output contraints
''        :                           // input contraints
''        : "cc", "memory", "eax", "ebx", "ecx", "edx", "esp", "edi", "esi");  // clobbered flags/memory/registers
''    asm("mov eax, offset myFunction\n" : : : /*clobber list*/);
''    asm goto("jmp %l0" : /*no outputs allowed with goto*/ : : /*clobber list*/ : myLabel);
''
''  * references to variables are replaced by %N place-holders, and we add
''    memory constraints (both output/input, using "+m", because we don't know
''    what the ASM code does).
''    This lets gcc replace the variable references by the proper stack offset
''    expression (i.e. esp+N or ebp-N).
''
''    Actually gcc inserts something like <DWORD PTR [ebp-N]>, not just <ebp-N>,
''    corresponding to the referenced variable's data type. Thus, FB inline ASM
''    like this:
''        dword ptr [a]
''    is effectively turned into
''        dword ptr [dword ptr [a]]
''
''    But it's ok, because the GNU assembler apparently uses the outer-most
''    size specifier only, which means the user's size specifier overrides the
''    one inserted by gcc.
''        dim x as long
''        asm
''            div dword ptr [dword ptr [x]]
''            div byte ptr [dword ptr [x]]
''        end asm
''    compiled into an .o with -gen gas, and then disassembled with objdump -M intel -d:
''        div DWORD PTR [ebp-0x4]
''        div BYTE PTR [ebp-0x4]
''
''  * Functions can be emitted as-is (using the external/mangled name),
''    the assembler will recognize them already.
''
''  * references to labels are replaced by %lN place-holders, and the special
''    "asm goto" syntax must be used.
''
''  * gcc's optimizations require us to specify what flags/memory/registers are
''    overwritten/changed by the ASM code. Since we don't know what the ASM code
''    does, we add all flags/memory/registers to the clobbered list...
''
'' -asm att: FB asm blocks are expected to be in the GCC format already,
''           i.e. quoted and including constraints if needed.
''
private function hFindAsmSymbolInSeenList( _
	byref labellist as TLIST, _
	byval labelsym as FBSYMBOL ptr _
) as long
	dim as FBSYMBOL ptr ptr symnode = listGetHead( @labellist )
	dim as long index = -1, curindex = 0
	do
		if symnode = NULL then exit do
		if *symnode = labelsym then
			index = curindex
			exit do
		end if
		symnode = listGetNext( symnode )
		curindex += 1
	loop
	return index
end function

private sub hCopyQuotedAsmText _
	( _
		byref result as DZSTRING, _
		byref asmcode as string, _
		byref i as integer _
	)
	dim as integer quotemark = asmcode[i - 1]
	dim as integer length = len( asmcode )
	DZstrConcatAssignC( result, quotemark )
	i += 1
	while( i <= length )
		dim as integer ch = asmcode[i - 1]
		DZstrConcatAssignC( result, ch )
		i += 1
		if( (ch = asc( "\" )) andalso (i <= length) ) then
			DZstrConcatAssignC( result, asmcode[i - 1] )
			i += 1
		elseif( ch = quotemark ) then
			exit while
		end if
	wend
end sub

private function hMoveAsmSegmentPrefixes( byref asmcode as string ) as string
	dim as DZSTRING result
	dim as integer i = 1
	dim as integer length = len( asmcode )
	DZstrZero( result )

	'' Clang needs segment prefixes outside address brackets, as in gs:[address].
	'' Keep quoted assembler data intact, including escaped quotes in .ascii strings.
	while( i <= length )
		dim as integer ch = asmcode[i - 1]
		if( (ch = asc( QUOTE )) or (ch = asc( "'" )) ) then
			hCopyQuotedAsmText( result, asmcode, i )
			continue while
		elseif( ch = asc( "[" ) ) then
			dim as integer j = i + 1
			'' Intel operands permit spaces and ASCII horizontal tabs (9).
			while( (j <= length) andalso _
			       ((asmcode[j - 1] = asc( " " )) orelse (asmcode[j - 1] = 9)) )
				j += 1
			wend
			if( j + 1 <= length ) then
				dim as string segment_name = lcase( mid( asmcode, j, 2 ) )
				select case( segment_name )
				case "cs", "ds", "es", "fs", "gs", "ss"
					dim as integer k = j + 2
					while( (k <= length) andalso _
					       ((asmcode[k - 1] = asc( " " )) orelse (asmcode[k - 1] = 9)) )
						k += 1
					wend
					if( (k <= length) andalso (asmcode[k - 1] = asc( ":" )) ) then
						DZstrConcatAssign( result, mid( asmcode, j, 2 ) + ":[" )
						i = k + 1
						continue while
					end if
				end select
			end if
		end if

		DZstrConcatAssignC( result, ch )
		i += 1
	wend

	if( result.data <> NULL ) then
		function = *result.data
	else
		function = ""
	end if
	DZstrAllocate( result, 0 )
end function

private function hStripSimpleAsmPlaceholderBrackets( byref asmcode as string ) as string
	dim as DZSTRING result
	dim as integer i = 1
	dim as integer length = len( asmcode )
	DZstrZero( result )

	while( i <= length )
		dim as integer matched = FALSE
		dim as integer ch = asmcode[i - 1]
		if( (ch = asc( QUOTE )) or (ch = asc( "'" )) ) then
			hCopyQuotedAsmText( result, asmcode, i )
			continue while
		end if

		if( ch = asc( "[" ) ) then
			dim as integer j = i + 1
			'' Spaces and ASCII horizontal tabs (9) do not change a %N operand.
			while( (j <= length) andalso _
			       ((asmcode[j - 1] = asc( " " )) orelse (asmcode[j - 1] = 9)) )
				j += 1
			wend
			if( (j <= length) andalso (asmcode[j - 1] = asc( "%" )) ) then
				dim as integer first = j
				j += 1
				dim as integer digits = j

				while( j <= length )
					ch = asmcode[j - 1]
					if( (ch < asc( "0" )) orelse (ch > asc( "9" )) ) then
						exit while
					end if
					j += 1
				wend
				if( j > digits ) then
					dim as integer afterdigits = j
					while( (j <= length) andalso _
					       ((asmcode[j - 1] = asc( " " )) orelse (asmcode[j - 1] = 9)) )
						j += 1
					wend
					if( (j <= length) andalso (asmcode[j - 1] = asc( "]" )) ) then
						DZstrConcatAssign( result, mid( asmcode, first, afterdigits - first ) )
						i = j + 1
						matched = TRUE
					end if
				end if
			end if
		end if

		if( matched = FALSE ) then
			DZstrConcatAssignC( result, asmcode[i - 1] )
			i += 1
		end if
	wend

	if( result.data <> NULL ) then
		function = *result.data
	else
		function = ""
	end if
	DZstrAllocate( result, 0 )
end function

private sub _emitAsmLine( byval asmtokenhead as ASTASMTOK ptr )
	c_leaf_opaque = TRUE
	'' 1st pass to count some stats (no emitting yet)
	dim as integer uses_label, labelindex, labelindexbase
	dim as TLIST seenlabellist, seenvarlist
	listInit( @seenlabellist, 8, sizeof( FBSYMBOL ptr ) )
	listInit( @seenvarlist, 8, sizeof( FBSYMBOL ptr ) )
	var n = asmtokenhead
	while( n )
		if( n->type = AST_ASMTOK_SYMB ) then
			select case( n->sym->class )
			case FB_SYMBCLASS_LABEL
				'' Determine whether to use <asm()> or <asm goto()>
				uses_label = TRUE
			case FB_SYMBCLASS_VAR
				'' Find first valid index into the label list;
				'' it starts behind the constraints.
				'' We'll emit 1 input operand per variable (and no outputs).
				if( hFindAsmSymbolInSeenList(seenvarlist, n->sym) < 0 ) then
					*cptr(FBSYMBOL ptr ptr, listNewNode(@seenvarlist)) = n->sym
					labelindex += 1
				end if
			end select
		end if
		n = n->next
	wend

	labelindexbase = labelindex
	listEnd( @seenvarlist )
	listInit( @seenvarlist, 8, sizeof(FBSYMBOL ptr) )
	dim as string ln = "__asm__"

	'' Only when inside normal procedures
	'' (NAKED procedures don't increase the indentation)
	if( sectionInsideProc( ) ) then
		ln += " __volatile__"
	end if

	if( uses_label ) then
		ln += " goto"
	end if

	ln += "( "

	dim asmcode as string
	dim as DZSTRING asmbuffer, outputbuffer, inputbuffer, labelbuffer
	DZstrZero( asmbuffer )
	DZstrZero( outputbuffer )
	DZstrZero( inputbuffer )
	DZstrZero( labelbuffer )

	'' 2nd pass - emitting
	dim as integer operandindex
	dim as integer clang_lea = FALSE, asmlinebegin = TRUE

	dim as string outputconstraints, inputconstraints, labellist
	n = asmtokenhead
	while( n )

		select case( n->type )
		case AST_ASMTOK_TEXT
			dim as string asmtext = *n->text
			if( asmtext = NEWLINE ) then
				asmlinebegin = TRUE
			elseif( len(trim(asmtext)) > 0 andalso asmlinebegin ) then
				clang_lea = (lcase(asmtext) = "lea")
				asmlinebegin = FALSE
			end if
			if( irUsesClangCCompiler() and _
			    (env.clopt.asmsyntax = FB_ASMSYNTAX_INTEL) ) then
				'' LLVM accepts lower or upper case operand qualifiers, but not
				'' mixed forms such as Ptr. Keep literals and symbol text intact.
				select case( lcase(asmtext) )
				case "ptr", "offset", "byte", "word", "dword", "qword", _
				     "tbyte", "fword", "xmmword", "ymmword", "zmmword"
					asmtext = lcase( asmtext )
				end select
			end if
			'' GNU as accepts OFFSET on LEA's memory operand. Clang's Intel
			'' parser rejects it; LEA already computes the address. Keep OFFSET
			'' on other instructions, where it selects an immediate operand.
			if( clang_lea andalso (asmtext = "offset") ) then
				n = n->next
				continue while
			end if
			'' -asm intel: ASM keywords, string literals, constants, etc.
			'' -asm att: String literals containing the ASM, tokens for the constraints lists, etc.
			'' Symbol references in the asm strings for -asm att must use the ASM name already,
			'' since we don't parse the string literal content.
			if( sectionInsideProc( ) and env.clopt.asmsyntax = FB_ASMSYNTAX_INTEL ) then
				'' Raw Intel text has no C operands. Escape literal percent signs
				'' before adding the generated %N and %= placeholders separately.
				asmtext = hReplace( asmtext, "%", "%%" )
			end if
			DZstrConcatAssign( asmbuffer, asmtext )

		case AST_ASMTOK_SYMB
			if( env.clopt.asmsyntax = FB_ASMSYNTAX_INTEL ) then
				if( sectionInsideProc( ) ) then
					select case( n->sym->class )
					case FB_SYMBCLASS_VAR
						'' Referencing a variable: insert %N place-holder...
						'' (N refers to N'th operand written in the whole
						'' asm [goto]([outputs...] inputs... [labels...]) statement)
						var seenindex = hFindAsmSymbolInSeenList( seenvarlist, n->sym )
						if( seenindex >= 0 ) then
							DZstrConcatAssign( asmbuffer, "%" & seenindex )
							n = n->next
							continue while
						end if
						*cptr(FBSYMBOL ptr ptr, listNewNode(@seenvarlist)) = n->sym
						DZstrConcatAssign( asmbuffer, "%" & operandindex )
						operandindex += 1

						'' ... and add the symbol to a constraint list
						if( uses_label ) then
							'' read-only input operand constraint: "m" (symbol)
							'' "asm goto" doesn't allow output constraints, so we can only use
							'' input constraints. Hopefully there are no jump instructions that
							'' modify their memory operand...
							if( inputbuffer.len > 0 ) then
								DZstrConcatAssign( inputbuffer, ", " )
							end if
							DZstrConcatAssign( inputbuffer, """m"" (" + hGetCId( n->sym ) + ")" )
						else
							'' read+write output operand constraint: "+m" (symbol)
							if( outputbuffer.len > 0 ) then
								DZstrConcatAssign( outputbuffer, ", " )
							end if
							DZstrConcatAssign( outputbuffer, """+m"" (" + hGetCId( n->sym ) + ")" )
						end if

					case FB_SYMBCLASS_LABEL
						'' Referencing a label: insert %lN place-holder
						'' (N refers to N'th operand written in whole
						'' asm goto(outputs... inputs... labels...) statement)

						'' GCC only allows a label to be in the label list once
						'' no matter how many times it appears in the code, so we keep track
						'' of what we've seen to make sure we only emit labels into the list once
						'' and subsequent access use the previous index
						dim as integer labelnum = any
						dim as FBSYMBOL ptr labelsym = n->sym
						dim as long seenlabelindex = hFindAsmSymbolInSeenList( seenlabellist, labelsym )
						if seenlabelindex <> -1 then
							labelnum = seenlabelindex + labelindexbase
						else
							labelnum = labelindex
							labelindex += 1
							*CPtr(FBSYMBOL ptr ptr, listNewNode( @seenlabellist ) ) = labelsym
							if( labelbuffer.len > 0 ) then
								DZstrConcatAssign( labelbuffer, ", " )
							end if
							DZstrConcatAssign( labelbuffer, hGetCId( labelsym ) )
						end if

						DZstrConcatAssign( asmbuffer, "%l" & labelnum )

					case else
						'' Referencing a procedure: no gcc constraints needed;
						'' instead emit the symbol directly with its ASM name.
						DZstrConcatAssign( asmbuffer, hGetMangledNameForASM( n->sym, TRUE ) )
					end select
				else
					'' Inside NAKED procedure: Currently emitted as pure inline asm,
					'' so constraints are (hopefully!?) not needed.
					DZstrConcatAssign( asmbuffer, hGetMangledNameForASM( n->sym, TRUE ) )
				end if
			else
				if( sectionInsideProc( ) ) then
					'' Normal AT&T inline asm uses C expressions in its
					'' constraints, so these references need C names.
					DZstrConcatAssign( asmbuffer, hGetCId( n->sym ) )
				else
					'' NAKED procedures are emitted as file-scope asm strings.
					'' Direct references there need the platform assembler
					'' name, including any required leading underscore. Keep
					'' the name inside a C string literal so adjacent ASM text
					'' tokens remain valid C.
					DZstrConcatAssign( asmbuffer, _
						QUOTE + hGetMangledNameForASM( n->sym, TRUE ) + QUOTE )
				end if
			end if

		end select

		n = n->next
	wend
	if( asmbuffer.data <> NULL ) then asmcode = *asmbuffer.data
	if( outputbuffer.data <> NULL ) then outputconstraints = *outputbuffer.data
	if( inputbuffer.data <> NULL ) then inputconstraints = *inputbuffer.data
	if( labelbuffer.data <> NULL ) then labellist = *labelbuffer.data
	DZstrAllocate( asmbuffer, 0 )
	DZstrAllocate( outputbuffer, 0 )
	DZstrAllocate( inputbuffer, 0 )
	DZstrAllocate( labelbuffer, 0 )
	listEnd( @seenlabellist )
	listEnd( @seenvarlist )

	if( env.clopt.asmsyntax = FB_ASMSYNTAX_INTEL ) then
		if( sectionInsideProc( ) ) then asmcode = hAsmLocalizeNames( asmcode )
		if( irUsesClangCCompiler() ) then
			'' Normalize the segment first: a C memory operand in gs:[%0]
			'' already supplies its own brackets and becomes gs:%0 below.
			asmcode = hMoveAsmSegmentPrefixes( asmcode )
			asmcode = hStripSimpleAsmPlaceholderBrackets( asmcode )
		end if

		''
		'' Escape double quotes, backslashes, etc. in the asm text,
		'' for example to emit .ascii directives correctly:
		''
		'' .bas:
		''    asm
		''        .ascii $"zzz\""\0"
		''    end asm
		''
		'' .c (-gen gcc -asm intel):
		''    __asm__ __volatile__(
		''        ".ascii \x22zzz\x5C\x22\x5C" "0\x22"
		''        :  :  : "cc", "memory", ...
		''    );
		''
		'' .asm:
		''    .ascii "zzz\"\0"
		''
		hBuildStrLit( ln, strptr( asmcode ), len( asmcode ) + 1, 0 )

		'' Only when inside normal procedures
		'' (NAKED procedures don't increase the indentation)
		if( sectionInsideProc( ) ) then
			ln += " : " + outputconstraints
			ln += " : " + inputconstraints

			'' We don't know what registers etc. will be trashed,
			'' so assume everything... except for rsp/esp - gcc requires a valid
			'' stack to preserve registers and if the asm code clobbers esp/rsp
			'' then there is no way to get it back after esp/rsp changes to
			'' something else.  User is always responsible for handling the stack
			'' registers.
			''
			ln += " : ""cc"", ""memory"""

			hAppendProcAsmClobbers( ln )

			if( uses_label ) then
				ln += " : " + labellist
			end if
		end if
	else
		''
		'' Pass asm text through as-is, assuming it is in the gcc format
		''
		'' .bas:
		''    asm
		''        "incl %0\n" : "+m" (a) : :
		''    end asm
		''
		'' .c (-gen gcc -asm att):
		''    __asm__ __volatile__( "incl %0\n" : "+m" (A$0) : : );
		''
		'' .asm:
		''    incl -16(%ebp)
		''
		ln += asmcode
	end if

	ln += " );"

	hWriteLine( ln )
end sub

private sub hVarIniDiscardScopes( )
	while( ctx.variniscopes <> NULL )
		var initializer_scope = ctx.variniscopes
		ctx.variniscopes = initializer_scope->previous
		deallocate( initializer_scope )
	wend
	ctx.variniscopelevel = 0
end sub

private sub hVarIniFail( byval reason as const zstring ptr )
	if( ctx.varinifailed = FALSE andalso errGetCount( ) = 0 ) then
		errReportEx( FB_ERRMSG_INTERNAL, reason )
	end if
	ctx.varinifailed = TRUE
end sub

private sub _emitVarIniBegin( byval sym as FBSYMBOL ptr )
	hVarIniDiscardScopes( )
	ctx.varini = ""
	ctx.varinifailed = FALSE
	ctx.varinisym = sym
	ctx.vtablemembers = ""
	ctx.vtableelements = 0
end sub

private sub _emitVarIniEnd( byval sym as FBSYMBOL ptr )
	if( ctx.variniscopes <> NULL ) then
		'' Parser recovery can abandon an initializer after a source error.
		if( errGetCount( ) = 0 ) then
			hVarIniFail( "unclosed C aggregate initializer" )
		else
			ctx.varinifailed = TRUE
		end if
	end if
	if( ctx.varinifailed = FALSE ) then hEmitVarDecl( FALSE, sym, ctx.varini )
	hVarIniDiscardScopes( )
	ctx.varini = ""
	ctx.varinisym = NULL
	ctx.vtablemembers = ""
	ctx.vtableelements = 0
end sub

private function hVarIniFieldPrefix( ) as string
	var initializer_scope = ctx.variniscopes
	if( initializer_scope = NULL ) then return ""
	if( initializer_scope->is_array or initializer_scope->udt = NULL ) then return ""

	'' C promotes anonymous members, but positional values still need the
	'' anonymous aggregate's braces. Name the BASIC member directly instead.
	var fld = symbUdtGetFirstField( initializer_scope->udt )
	var emitted = initializer_scope->emitted_elements
	while( fld <> NULL )
		if( symbIsDynamic( fld ) = FALSE ) then
			if( emitted = 0 ) then return "." + hGetCFieldId( fld ) + " = "
			emitted -= 1
		end if
		fld = symbUdtGetNextInitableField( fld )
	wend
	hVarIniFail( "too many C aggregate initializer fields" )
	return ""
end function

private sub hVarIniSeparator( )
	if( ctx.variniscopes <> NULL ) then
		ctx.varini += ", "
		ctx.variniscopes->emitted_elements += 1
	end if
end sub

private function hIsVtableInitializer( ) as integer
	if( ctx.varinisym = NULL ) then return FALSE
	return ((ctx.varinisym->stats and FB_SYMBSTATS_VTABLE) <> 0)
end function

private sub hAppendVtableSlot( byval dtype as integer, byval subtype as FBSYMBOL ptr )
	ctx.vtablemembers += hEmitType( dtype, subtype ) + " slot_" + str(ctx.vtableelements) + "; "
	ctx.vtableelements += 1
end sub

private sub _emitVarIniI( byval sym as FBSYMBOL ptr, byval value as longint )
	if( hIsVtableInitializer( ) ) then
		'' The ABI header and abstract slots use native null object pointers.
		if( value <> 0 ) then
			hVarIniFail( "nonzero integer in C virtual table" )
			exit sub
		end if
		hAppendVtableSlot( typeAddrOf(FB_DATATYPE_VOID), NULL )
	end if
	var dtype = symbGetType( sym ) or typeGetConstMask( symbGetFullType( sym ) )

	'' AST stores boolean true as -1, but we emit it as 1 for gcc compatibility
	if( (dtype = FB_DATATYPE_BOOLEAN) and (value <> 0) ) then
		value = 1
	end if

	if( (value = 0) andalso typeIsPtr( dtype ) andalso _
	    (symbIsRef( sym ) = FALSE) ) then
		'' C integer zero is a null pointer constant. Avoid a typed cast here,
		'' because it can erase a C typedef alias from the emitted pointer type.
		ctx.varini += hVarIniFieldPrefix( ) + "0"
		hVarIniSeparator( )
		exit sub
	end if

	var l = exprNewIMMi( value, dtype )
	if( symbIsRef( sym ) ) then dtype = typeAddrOf( dtype )
	l = exprNewCAST( dtype, sym->subtype, l )

	ctx.varini += hVarIniFieldPrefix( ) + exprFlush( l )
	hVarIniSeparator( )
end sub

private function hEmitInt128Word( byval value as longint ) as string
	var result = hex( culngint( value ) )
	if( len( result ) < 16 ) then result = string( 16 - len( result ), "0" ) + result
	function = result
end function

private sub _emitVarIni128( byval sym as FBSYMBOL ptr, byval low as longint, byval high as longint )
	dim as string value
	if( fbGetBits( ) = 32 ) then
		'' This is a static initializer, not an aggregate-valued expression.
		'' Braces keep file-scope and nested two-word values constant in C11.
		value = "{ 0x" + hEmitInt128Word( low ) + _
			"ull, 0x" + hEmitInt128Word( high ) + "ull }"
	else
		'' Initializers must use the same native types as declarations.
		var unsignedtype = *dtypeName(FB_DATATYPE_UINTEGER128)
		value = "((" + unsignedtype + ")0x" + hEmitInt128Word( high ) + _
			"ull << 64) | (" + unsignedtype + ")0x" + hEmitInt128Word( low ) + "ull"
		if( typeIsSigned( symbGetType( sym ) ) ) then
			value = "((" + *dtypeName(FB_DATATYPE_INTEGER128) + ")(" + value + "))"
		end if
	end if
	ctx.varini += hVarIniFieldPrefix( ) + value
	hVarIniSeparator( )
end sub

private sub _emitVarIniF( byval sym as FBSYMBOL ptr, byval value as double )
	var dtype = symbGetType( sym )
	var l = exprNewIMMf( value, dtype )
	if( symbIsRef( sym ) ) then
		dtype = typeAddrOf( dtype )
	end if
	l = exprNewCAST( dtype, sym->subtype, l )
	ctx.varini += hVarIniFieldPrefix( ) + exprFlush( l )
	hVarIniSeparator( )
end sub

private sub _emitVarIniOfs _
	( _
		byval sym as FBSYMBOL ptr, _
		byval rhs as FBSYMBOL ptr, _
		byval ofs as longint _
	)

	if( hIsVtableInitializer( ) ) then
		if( ofs <> 0 ) then
			hVarIniFail( "nonzero address offset in C virtual table" )
			exit sub
		end if
		if( symbIsProc(rhs) ) then
			var signature = symbAddProcPtrFromFunction( rhs )
			hAppendVtableSlot( typeAddrOf(FB_DATATYPE_FUNCTION), signature )
		else
			hAppendVtableSlot( typeAddrOf(symbGetFullType(rhs)), symbGetSubtype(rhs) )
		end if
		ctx.varini += exprFlush( exprNewOFFSET(rhs, 0) )
		hVarIniSeparator( )
		exit sub
	end if

	dim as EXPRNODE ptr l = any

	l = exprNewOFFSET( rhs, ofs )

	var dtype = symbGetType( sym ) or typeGetConstMask( symbGetFullType( sym ) )
	if( symbIsRef( sym ) ) then
		dtype = typeAddrOf( dtype )
	end if
	l = exprNewCAST( dtype, sym->subtype, l )

	ctx.varini += hVarIniFieldPrefix( ) + exprFlush( l )
	hVarIniSeparator( )
end sub

private sub _emitVarIniStr _
	( _
		byval varlength as longint, _    '' without null terminator
		byval literal as zstring ptr, _
		byval litlength as longint, _     '' without null terminator
		byval noterm as integer _
	)

	'' Simple fixed-length string initialized from string literal
	'' "..."
	ctx.varini += hVarIniFieldPrefix( )

	'' String literal too long? (GCC would show a warning)
	if( litlength > varlength ) then
		'' Cut off; may be empty afterwards
		litlength = varlength
	end if

	if( noterm ) then
		hBuildStrLit( ctx.varini, hUnescape( literal ), litlength + 1, varlength + 1 )
	else
		hBuildStrLit( ctx.varini, hUnescape( literal ), litlength + 1, 0 )
	end if

	hVarIniSeparator( )

end sub

private sub hAppendWstrVarIniCodeUnit _
	( _
		byref initializer as DZSTRING, _
		byval codeunit as uinteger, _
		byval wcharsize as integer, _
		byval use_numeric as integer _
	)

	if( use_numeric ) then
		DZstrConcatAssign( initializer, hEmitInt( env.target.wchar, codeunit ) )
	else
		'' On targets with 1-byte wstring data, C wide strings/chars are too
		'' wide, so don't use them.
		if( wcharsize = 1 ) then
			DZstrConcatAssign( initializer, "'" )
		else
			DZstrConcatAssign( initializer, "L'" )
		end if

		if( hCharNeedsEscaping( codeunit, asc( "'" ) ) ) then
			DZstrConcatAssign( initializer, $"\x" + hex( codeunit, wcharsize * 2 ) )
		else
			DZstrConcatAssignC( initializer, codeunit )
		end if

		DZstrConcatAssign( initializer, "'" )
	end if
end sub

private sub _emitVarIniWstr _
	( _
		byval varlength as longint, _  '' without null terminator
		byval literal as wstring ptr, _
		byval litlength as longint _   '' without null terminator
	)

	dim as uinteger ch = any
	dim as integer wcharsize = any
	dim as integer use_numeric = any
	dim as wstring ptr src = any, src_end = any
	dim as DZSTRING initializer

	DZstrZero( initializer )
	ctx.varini += hVarIniFieldPrefix( )
	DZstrAssign( initializer, strptr( ctx.varini ) )

	'' In Linux GCC, wchar_t and thus L"..." expressions use signed int,
	'' but FB uses unsigned integers. But GCC will show an error when doing
	''    unsigned int mywstring[] = L"foo"
	'' so we must emit it as
	''    unsigned int mywstring[] = { L'f', L'o', L'o' }

	DZstrConcatAssign( initializer, "{ " )
	wcharsize = fbGetTargetWcharSize( )
	use_numeric = hUseNumericWstrLit( )

	'' String literal too long?
	if( litlength > varlength ) then
		'' Cut off; may be empty afterwards
		litlength = varlength
	end if

	src = literal
	src_end = literal + len( *literal )
	'' Keep the internal encoding until the reader consumes it once.
	'' Fixed buffers count target code units, including both surrogate halves.
	dim as longint emitted_units = 0
	while( emitted_units < litlength ) andalso (src < src_end)

		if( emitted_units > 0 ) then
			DZstrConcatAssign( initializer, ", " )
		end if

		ch = hReadWstrChar( src, src_end )

		if( (wcharsize = 2) and (ch >= &h10000ul) and (ch <= &h10FFFFul) ) then
			ch -= &h10000ul
			hAppendWstrVarIniCodeUnit( initializer, (ch shr 10) + &hD800ul, _
			                               wcharsize, use_numeric )
			emitted_units += 1
			if( emitted_units >= litlength ) then exit while
			DZstrConcatAssign( initializer, ", " )
			hAppendWstrVarIniCodeUnit( initializer, (ch and &h3FFul) + &hDC00ul, _
			                               wcharsize, use_numeric )
		else
			hAppendWstrVarIniCodeUnit( initializer, ch, wcharsize, use_numeric )
		end if
		emitted_units += 1
	wend

	'' ISO C needs an element even when the literal has no data code units.
	if( emitted_units = 0 ) then DZstrConcatAssign( initializer, "0" )

	DZstrConcatAssign( initializer, " }" )
	ctx.varini = *initializer.data
	DZstrAllocate( initializer, 0 )

	hVarIniSeparator( )

end sub

private sub _emitVarIniPad( byval bytes as longint, byval fillchar as integer )
	'' Nothing to do -- we're using {...} for structs and each array
	'' dimension, and gcc will zero-initialize any uninitialized elements,
	'' aswell as add padding between fields etc. where needed.
end sub

private sub _emitVarIniScopeBegin( byval sym as FBSYMBOL ptr, byval is_array as integer )
	if( ctx.varinifailed ) then exit sub
	var prefix = hVarIniFieldPrefix( )
	if( ctx.varinifailed ) then exit sub

	'' TYPEINI can mark the outer array scope as a record. Each dimension
	'' must be entered before a later scope initializes the element's fields.
	if( sym <> NULL andalso (symbIsVar( sym ) or symbIsField( sym )) ) then
		if( is_array = FALSE andalso symbGetArrayDimensions( sym ) > 0 ) then
			var dimensions = 0
			var parent = ctx.variniscopes
			while( parent <> NULL )
				if( parent->is_array andalso parent->sym = sym ) then dimensions += 1
				parent = parent->previous
			wend
			if( dimensions < symbGetArrayDimensions( sym ) ) then is_array = TRUE
		end if
	end if

	dim as CVARINISCOPE ptr initializer_scope = callocate( sizeof( CVARINISCOPE ) )
	if( initializer_scope = NULL ) then
		hVarIniFail( "out of memory generating C aggregate initializer" )
		exit sub
	end if
	initializer_scope->previous = ctx.variniscopes
	initializer_scope->sym = sym
	initializer_scope->is_array = is_array
	if( is_array = FALSE andalso sym <> NULL ) then
		if( symbIsStruct( sym ) ) then
			initializer_scope->udt = sym
		elseif( symbGetType( sym ) = FB_DATATYPE_STRUCT ) then
			initializer_scope->udt = symbGetSubtype( sym )
		end if
	end if
	ctx.variniscopes = initializer_scope
	ctx.variniscopelevel += 1

	ctx.varini += prefix + "{ "

end sub

private function hVarIniArrayDimensions( byval sym as FBSYMBOL ptr ) as integer
	'' The C declarator includes fixed strings' extra character-array dimension.
	var dimensions = hEmitArrayDecl( sym )
	var count = 0
	var position = instr( dimensions, "[" )
	while( position > 0 )
		count += 1
		position = instr( position + 1, dimensions, "[" )
	wend
	return count
end function

private function hVarIniZeroField _
	( _
		byval fld as FBSYMBOL ptr, _
		byval array_element as integer = FALSE _
	) as string

	'' An array of pointers still needs array braces. Include fixed strings'
	'' extra C dimension, then initialize one complete element at each level.
	if( array_element = FALSE ) then
		var dimensions = hVarIniArrayDimensions( fld )
		if( dimensions > 0 ) then
			var value = hVarIniZeroField( fld, TRUE )
			for i as integer = 1 to dimensions
				value = "{ " + value + " }"
			next
			return value
		end if
	end if

	var dtype = symbGetType( fld )
	if( symbIsRef( fld ) or typeIsPtr( dtype ) ) then return "0"
	if( symbIsBuiltinVaListType( dtype, symbGetSubtype( fld ) ) ) then
		select case symbGetValistType( dtype, symbGetSubtype( fld ) )
		case FB_CVA_LIST_BUILTIN_POINTER, FB_CVA_LIST_BUILTIN_VOID_POINTER
			return "0"
		case else
			return "{ 0 }"
		end select
	end if
	select case dtype
	case FB_DATATYPE_STRUCT
		var udt = symbGetSubtype( fld )
		if( udt = NULL ) then
			hVarIniFail( "missing C aggregate member type" )
			return "0"
		end if
		var value = ""
		var member = symbUdtGetFirstField( udt )
		while( member <> NULL )
			if( symbIsDynamic( member ) = FALSE ) then
				if( len( value ) > 0 ) then value += ", "
				'' Designators also name promoted anonymous struct/union fields.
				value += "." + hGetCFieldId( member ) + " = " + hVarIniZeroField( member )
			end if
			member = symbUdtGetNextInitableField( member )
		wend
		if( len( value ) = 0 ) then value = "0"
		return "{ " + value + " }"
	case FB_DATATYPE_STRING
		'' The C string descriptor contains data, length and allocated size.
		return "{ 0, 0, 0 }"
	case FB_DATATYPE_INTEGER128, FB_DATATYPE_UINTEGER128
		'' The 32-bit C representation stores low and high 64-bit words.
		if( fbGetBits( ) = 32 ) then return "{ 0, 0 }"
	end select
	return "0"
end function

private sub _emitVarIniScopeEnd( )
	if( ctx.varinifailed ) then exit sub
	var initializer_scope = ctx.variniscopes
	if( initializer_scope = NULL ) then
		hVarIniFail( "unmatched C aggregate initializer end" )
		exit sub
	end if

	'' BASIC leaves omitted members zeroed. Spell those defaults explicitly
	'' instead of relying on a partial positional C initializer. Layout padding
	'' remains the C compiler's responsibility; it is not an extra member.
	if( initializer_scope->udt <> NULL ) then
		var fld = symbUdtGetFirstField( initializer_scope->udt )
		var emitted = initializer_scope->emitted_elements
		while( fld <> NULL )
			if( symbIsDynamic( fld ) = FALSE ) then
				if( emitted > 0 ) then
					emitted -= 1
				else
					ctx.varini += "." + hGetCFieldId( fld ) + " = " + hVarIniZeroField( fld )
					hVarIniSeparator( )
				end if
			end if
			fld = symbUdtGetNextInitableField( fld )
		wend
	elseif( initializer_scope->emitted_elements = 0 ) then
		'' Synthesized descriptors can contain an empty array of dimension
		'' records. Initialize one complete element, with any dimensions that
		'' have not yet been entered, rather than eliding its record braces.
		var value = "0"
		if( initializer_scope->sym <> NULL ) then
			value = hVarIniZeroField( initializer_scope->sym, TRUE )
			var dimensions = hVarIniArrayDimensions( initializer_scope->sym )
			var parent = initializer_scope
			while( parent <> NULL )
				if( parent->is_array andalso parent->sym = initializer_scope->sym ) then dimensions -= 1
				parent = parent->previous
			wend
			if( initializer_scope->is_array = FALSE ) then dimensions -= 1
			for i as integer = 1 to dimensions
				value = "{ " + value + " }"
			next
		end if
		ctx.varini += value
		hVarIniSeparator( )
	end if

	'' Trim separator at the end, to make the output look a bit more clean
	'' (this isn't needed though, since the extra comma is allowed in C)
	if( right( ctx.varini, 2 ) = ", " ) then
		'' fbc doesn't optimize this very well and on long strings it becomes expensive
		'' it's built in to the run-time, so revert to normal expression
		'' if we don't have fb_leftself defined yet.
		#ifndef fb_leftself
			ctx.varini = left( ctx.varini, len( ctx.varini ) - 2 )
		#else
			fb_leftself( ctx.varini, len( ctx.varini ) - 2 )
		#endif
	end if

	ctx.varini += " }"
	ctx.variniscopes = initializer_scope->previous
	deallocate( initializer_scope )
	ctx.variniscopelevel -= 1
	hVarIniSeparator( )
end sub

private sub _emitFbctinfBegin( )
	hWriteLine( "", TRUE )

	'' static         - should not be a public symbol
	'' const          - read-only
	'' char[]         - a string
	'' used attribute - prevent removal due to optimizations
	'' section attribute - This global must be put into a custom .fbctinf
	''                     section, as done by the ASM backend.
	ctx.fbctinf = "static const char "
	if (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DARWIN) then
		'' Must specify a segment name (can use any name)
		ctx.fbctinf += "__attribute__((used, section(""__DATA," + FB_INFOSEC_NAME + """))) "
	else
		ctx.fbctinf += "__attribute__((used, section(""." + FB_INFOSEC_NAME + """))) "
	end if
	ctx.fbctinf += "fb_c_metadata[]"
	var object_name = "__fbctinf"
	if( env.underscoreprefix ) then object_name = "_" + object_name
	hAppendCObjectAlias( ctx.fbctinf, object_name )
	ctx.fbctinf += " = """
end sub

private sub _emitFbctinfString( byval s as const zstring ptr )
	ctx.fbctinf += *s + $"\0"
end sub

private sub _emitFbctinfEnd( )
	'' Cut off unnecessary \0 at the end; gcc will add it automatically,
	'' since it's a string literal...
	if( right( ctx.fbctinf, 2 ) = $"\0" ) then
		ctx.fbctinf = left( ctx.fbctinf, len( ctx.fbctinf ) - 2 )
	end if
	ctx.fbctinf += """;"
	hWriteLine( ctx.fbctinf, TRUE )
end sub

private sub _emitProcBegin _
	( _
		byval proc as FBSYMBOL ptr, _
		byval initlabel as FBSYMBOL ptr _
	)

	ctx.currproc = proc
	ctx.computedgotoopaque = symbIsNaked( proc )
	if( not symbIsNaked( proc ) ) then
		if( astProcMayReturn( astGetProc( ) ) = FALSE ) then
			symbSetProcIsNoReturn( proc )
		end if
	end if
	hClearCLocalReferences( )
	hashInit(@c_local_refs, 256, TRUE) '' Fixed buckets; chains admit larger local sets.
	c_local_refs_complete = ((symbIsNaked(proc) = FALSE) and (env.clopt.debuginfo = FALSE))
	hMarkReferencedLocals( astGetProc()->l )
	if( hCComparisonsBegin(astGetProc()->l) ) then
		'' Recompute storage uses after comparisons lose their value operands.
		hClearCLocalReferences()
		hashInit(@c_local_refs, 256, TRUE)
		hMarkReferencedLocals(astGetProc()->l)
	end if
	var locals_changed = hCScalarStoresBegin(proc, astGetProc()->l)
	locals_changed or= hCAutoDescBegin(astGetProc()->l)
	if( locals_changed ) then
		'' Rebuild the storage index after removing unobserved stores.
		hClearCLocalReferences()
		hashInit(@c_local_refs, 256, TRUE)
		hMarkReferencedLocals(astGetProc()->l)
	end if
	hCStaticArrayInitializers(proc)
	hAsmBeginNames( proc )
	hCReadOnlyLocalsBegin(astGetProc()->l)
	hCUnobservedBegin(proc, astGetProc()->l)
	ctx.computedgotoaddress = FALSE
	ctx.computedgotoentry = FALSE
	ctx.computedgotodispatch = FALSE
	ctx.procentrylabel = initlabel
	ctx.procentrysection = -1
	ctx.procentryoffset = 0
	ctx.procentrybytes = 0

	hUpdateCurrentFileName( symbGetProcIncFile( proc ) )

	irhlEmitProcBegin( )
	hCLeafBegin( proc )

	dim as string mangled

	assert( listGetHead( @ctx.exprcache ) = NULL )
	assert( listGetHead( @ctx.exprnodes ) = NULL )

	hWriteLine( "", TRUE )

	if( env.clopt.debuginfo ) then
		_emitDBG( AST_OP_DBG_LINEINI, proc, proc->proc.ext->dbg.iniline )
	end if

	'' NAKED procedure? Use inline asm, since gcc doesn't support
	'' __attribute__((naked)) on x86
	if( symbIsNaked( proc ) ) then
		mangled = hGetMangledNameForASM( proc, TRUE )
		hWriteLine( "__asm__( "".text"" );" )
		hWriteLine( "__asm__( "".globl " + mangled + """ );" )
		hWriteLine( "__asm__( """ + mangled + ":"" );" )
		exit sub
	end if

	sectionBegin( )

	'' If an asm("mangledname") alias is needed, emit an extra prototype
	'' right above the procedure body, because asm() is only allowed on
	'' prototypes.
	if( hNeedAlias( proc ) ) then
		hWriteLine( hEmitProcHeader( proc, EMITPROC_ISPROTO ) + ";" )
	end if

	var original_header = hEmitProcHeader(proc, 0)
	var readonly_variant = hCReadOnlyBegin(proc, original_header)
	if( readonly_variant = NULL ) then
		hWriteLine(original_header)
	else
		hWriteLine(hEmitProcHeader(proc, 0, readonly_variant))
	end if

	hWriteLine( "{" )
	sectionIndent( )
	sectionBegin( )

	'' CLongDouble is stored as a byte-backed UDT in FreeBASIC, while the
	'' CDecl boundary receives native C long double values.
	dim as FBSYMBOL ptr param = symbGetProcLastParam( proc )
	do while( param <> NULL )
		if( (symbGetParamMode( param ) <> FB_PARAMMODE_VARARG) ) then
			dim as integer dtype = any
			dim as FBSYMBOL ptr subtype = any
			symbGetRealParamDtype( param, dtype, subtype )
			if( hUsesCLongDoubleCABI( proc ) andalso _
			    hIsCLongDoubleType( dtype, subtype ) ) then
				var local_identifier = hGetCId( symbGetParamVar( param ) )
				var native_identifier = "fb_c_native_" + local_identifier
				var type_name = hEmitType( typeUnsetIsConst( dtype ), subtype, TRUE )
				hWriteLine( type_name + " " + local_identifier + ";" )
				hWriteLine( "__builtin_memset( (void*)&" + local_identifier + _
				            ", 0, sizeof( " + local_identifier + " ) );" )
				hWriteLine( "__builtin_memcpy( (void*)&" + local_identifier + _
				            ", (const void*)&" + native_identifier + ", " + _
				            "(sizeof( " + local_identifier + " ) < sizeof( " + _
				            native_identifier + " ) ? sizeof( " + local_identifier + _
				            " ) : sizeof( " + native_identifier + " )) );" )
			end if
		end if
		param = symbGetProcPrevParam( proc, param )
	loop

end sub

private sub hDropUnusedProcEntry( byval label as FBSYMBOL ptr )
	if( ctx.procentrybytes = 0 ) then exit sub
	if( symbGetIsAccessed( label ) ) then exit sub

	'' Keep the original entry position until all indirect jumps are lowered.
	'' An unused entry can then be removed in place, before this procedure's
	'' section is appended to its parent, without another buffer allocation.
	dim as SECTIONENTRY ptr e = @ctx.sections(ctx.section)
	if( (ctx.procentrysection <> ctx.section) orelse e->old orelse _
	    (ctx.procentryoffset > e->bufferlen) orelse _
	    (ctx.procentrybytes > e->bufferlen - ctx.procentryoffset) ) then
		errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
		exit sub
	end if

	var tailbytes = e->bufferlen - ctx.procentryoffset - ctx.procentrybytes
	if( ctx.computedgotoentry ) then
		'' The synthetic entry address is a Clang workaround. GCC needs
		'' neither that address nor its otherwise unused label. Real entry
		'' references were retained by the ACCESSED check above.
		dim as string prefix = "#if defined(__clang__)" + NEWLINE
		dim as string suffix = "#endif" + NEWLINE
		dim as string extra = prefix + suffix
		var previouslen = e->bufferlen
		sectionAppend( e, strptr(extra), len(extra) )
		if( e->bufferlen - previouslen <> len(extra) ) then exit sub
		memmove( e->buffer + ctx.procentryoffset + ctx.procentrybytes + len(extra), _
		         e->buffer + ctx.procentryoffset + ctx.procentrybytes, tailbytes )
		memmove( e->buffer + ctx.procentryoffset + len(prefix), _
		         e->buffer + ctx.procentryoffset, ctx.procentrybytes )
		memcpy( e->buffer + ctx.procentryoffset, strptr(prefix), len(prefix) )
		memcpy( e->buffer + ctx.procentryoffset + len(prefix) + ctx.procentrybytes, _
		        strptr(suffix), len(suffix) )
		ctx.procentrybytes = 0
		exit sub
	end if
	if( tailbytes > 0 ) then
		memmove( e->buffer + ctx.procentryoffset, _
		         e->buffer + ctx.procentryoffset + ctx.procentrybytes, tailbytes )
	end if
	e->bufferlen -= ctx.procentrybytes
	ctx.procentrybytes = 0
end sub

private sub _emitProcEnd _
	( _
		byval proc as FBSYMBOL ptr, _
		byval initlabel as FBSYMBOL ptr, _
		byval exitlabel as FBSYMBOL ptr _
	)

	hAsmEndNames( )
	dim as string mangled
	dim as EXPRCACHENODE ptr cachenode = any

	ctx.procentrylabel = NULL

	'' NAKED procedure? Use inline asm, since gcc doesn't support
	'' __attribute__((naked)) on x86
	if( symbIsNaked( proc ) ) then
		'' Emit .size like ASM backend, for Linux
		if( env.clopt.target = FB_COMPTARGET_LINUX ) then
			mangled = hGetMangledNameForASM( proc, TRUE )
			hWriteLine( "__asm__( "".size " + mangled + ", .-" + mangled + """ );", TRUE )
		end if
		exit sub
	end if

	hDropUnusedProcEntry( initlabel )
	if( ctx.computedgotodispatch ) then
		var section = sectionGosub( 2 )
		if( ctx.computedgotoaddress or ctx.computedgotoopaque ) then
			'' The slot belongs to this activation, including recursive calls.
			'' Its header declaration also covers jumps from nested scopes.
			hWriteLine( "void* fb_c_jump_target = 0;" )
			hWriteLine( "#define fb_c_indirect_jump(value) do { fb_c_jump_target = (value); goto fb_c_jump_dispatch; } while(0)", TRUE )
		else
			'' A GNU indirect jump can only target a label in this C function.
			'' With no label addresses or opaque text, error handling must either
			'' terminate or unwind through an owned runtime frame. A returned
			'' target cannot name a valid continuation here. Stop instead of
			'' jumping into another activation with the wrong stack and locals.
			'' Keep the legacy label ABI whenever real local targets exist.
			hWriteLine( "#define fb_c_indirect_jump(value) do { (void)(value); __builtin_abort(); } while(0)", TRUE )
		end if
		sectionReturn( section )
		if( ctx.computedgotoaddress or ctx.computedgotoopaque ) then
			'' Preserve normal fallthrough without executing the dispatch block.
			hWriteLine( "goto fb_c_jump_dispatch_end;" )
			hWriteLine( "fb_c_jump_dispatch:;" )
			hWriteLine( "goto *fb_c_jump_target;" )
			hWriteLine( "fb_c_jump_dispatch_end:;" )
		end if
	end if
	sectionEnd( )

	sectionUnindent( )
	hWriteLine( "}" )
	if( ctx.computedgotodispatch ) then hWriteLine( "#undef fb_c_indirect_jump", TRUE )

	sectionEnd( )
	hCReadOnlyEnd(proc)
	hCLeafEnd( )
	hCAutoDescClear()
	hCScalarStoresClear()
	hCReadOnlyLocalsClear()
	if( (env.clopt.target = FB_COMPTARGET_NETBSD) and _
	    (proc->stats and FB_SYMBSTATS_GLOBALDTOR) ) then
		'' NetBSD walks .fini_array forwards, including in shared libraries.
		'' Reverse the BASIC priority in the section name so destructors run
		'' in the opposite order to constructors. The default priority is
		'' the highest value, as it is for the C compiler's destructor.
		const maximum_priority = 65535
		dim as integer priority = symbGetProcPriority( proc )
		if( priority = 0 ) then priority = maximum_priority
		dim as string section_priority = right( "00000" + str( maximum_priority - priority ), 5 )
		dim as string proc_name = hGetCId( proc )
		dim as string semantic_registration = "static void (*fb_c_fini_" + proc_name + ")(void) " + _
			"__attribute__((section("".fini_array." + section_priority + """), used)) = " + proc_name + ";"
		hWriteLine(semantic_registration, TRUE)
		fbSemanticModelStartupEmission(proc, "destructor", "c", "array-entry", section_priority, _
			".fini_array." + section_priority, semantic_registration)
	end if

	'' Forget any left-over expression nodes (unused function results)
	do
		cachenode = listGetHead( @ctx.exprcache )
		if( cachenode = NULL ) then
			exit do
		end if
		exprFreeTree( cachenode->expr )
		listDelNode( @ctx.exprcache, cachenode )
	loop
	assert( listGetHead( @ctx.exprcache ) = NULL )
	assert( listGetHead( @ctx.exprnodes ) = NULL )

	'' Static local descriptors are finalized after PROCEND. Keep their
	'' borrowed reference keys until the next procedure begins.
	irhlEmitProcEnd( )

end sub

private sub _emitScopeBegin( byval s as FBSYMBOL ptr )
	sectionBegin( )
	hWriteLine( "{", TRUE )
	sectionIndent( )
	sectionBegin( )
end sub

private sub _emitScopeEnd( byval s as FBSYMBOL ptr )
	sectionEnd( )
	sectionUnindent( )
	hWriteLine( "}", TRUE )
	sectionEnd( )
end sub

''::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

dim shared as IR_VTBL irhlc_vtbl = _
( _
	@_init, _
	@_end, _
	@_emitBegin, _
	@_emitEnd, _
	@_getOptionValue, _
	@_supportsOp, _
	@_procBegin, _
	@_procEnd, _
	NULL, _
	NULL, _
	NULL, _
	@_scopeBegin, _
	@_scopeEnd, _
	@_procAllocStaticVars, _
	@_emitConvert, _
	@_emitLabel, _
	@_emitLabel, _
	NULL, _
	@_emitProcBegin, _
	@_emitProcEnd, _
	@irhlEmitPushArg, _
	@_emitAsmLine, _
	@_emitComment, _
	@_emitBop, _
	@_emitUop, _
	@_emitStore, _
	@_emitSpillRegs, _
	@_emitLoad, _
	@_emitLoadRes, _
	NULL, _
	@_emitAddr, _
	@_emitCall, _
	@_emitCallPtr, _
	NULL, _
	@_emitJumpPtr, _
	@_emitBranch, _
	@_emitJmpTb, _
	@_emitMem, _
	@_emitMacro, _
	@_emitScopeBegin, _
	@_emitScopeEnd, _
	@_emitDECL, _
	@_emitDBG, _
	@_emitVarIniBegin, _
	@_emitVarIniEnd, _
	@_emitVarIniI, _
	@_emitVarIni128, _
	@_emitVarIniF, _
	@_emitVarIniOfs, _
	@_emitVarIniStr, _
	@_emitVarIniWstr, _
	@_emitVarIniPad, _
	@_emitVarIniScopeBegin, _
	@_emitVarIniScopeEnd, _
	@_emitFbctinfBegin, _
	@_emitFbctinfString, _
	@_emitFbctinfEnd, _
	@irhlAllocVreg, _
	@irhlAllocVrImm, _
	@irhlAllocVrImmF, _
	@irhlAllocVrVar, _
	@irhlAllocVrIdx, _
	@irhlAllocVrPtr, _
	@irhlAllocVrOfs, _
	@_setVregDataType, _
	NULL, _
	NULL, _
	NULL, _
	NULL _
)

'' end of backend/c/ir-hlc.bas
