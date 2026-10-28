'' Project: FreeBASIC compiler - LLVM IR backend
'' -----------------------------------------
''
'' File: backend/llvm/ir-llvm.bas
''
'' Purpose:
''
''     Emit LLVM IR with explicit types, SSA values, and FreeBASIC ABI layouts.
''
'' Responsibilities:
''
''     - serialize typed IR, data layouts, and control flow as LLVM IR
''     - maintain SSA identities, declarations, and intrinsic lowering
''
'' This file intentionally does NOT contain:
''
''     - BASIC grammar or invocation of llc and the linker
''

''
'' Project: FreeBASIC Compiler
'' File: ir-llvm.bas
''
'' Purpose:
''     Emit LLVM IR from the compiler's intermediate representation.
''
'' Responsibilities:
''     - translate compiler operations into LLVM declarations and instructions
''     - buffer ordered declaration, procedure, and debug output sections
''
'' This file intentionally does NOT contain:
''     - source parsing or semantic-model export
''
'' Resource ownership:
''     The backend context owns its output buffers. _end() releases them after
''     module emission, and each local dynamic string is reset by its caller.
''
'' IR interface for emitting LLVM IR to output file
''
'' For comparison, see
''
''    - LLVM IR language reference:
''          http://llvm.org/docs/LangRef.html
''
''    - clang's LLVM IR output (useful to see what LLVM IR code it will produce
''      for certain C constructs):
''          $ clang test.c -S -emit-llvm -o test.ll
''
''    - llc (compiles LLVM IR to native ASM):
''          $ llc -O2 test.ll -o test.asm
''
'' LLVM IR instructions inside procedures look like this:
''
''    %var = alloca i32          ; dim var as int32 ptr = alloca( 4 )
''    store i32 0, i32* %var     ; *var = 0
''  loop:
''    %temp0 = load i32* %var                ; temp0 = *var
''    %temp1 = add i32 %temp0, 1             ; temp1 = temp0 + 1
''    store i32 %temp1, i32* %var            ; *var = temp1
''    %temp2 = load i32* %var                ; temp2 = *var
''    %cond = icmp lt i32 %temp2, 10         ; condition = (temp2 < 10)
''    br i1 %cond, label %loop, label %exit  ; if condition then goto loop else goto exit
''  exit:
''
'' - Operations must be in SSA form, there are no self-ops. Operations that
''   don't return void can be assigned to a %name which can be referenced in
''   following operations. The result values can only be stored into memory
''   by separate/explicit store ops.
''
'' - Operations without name implicitly use the %N naming scheme: %1, %2, %3 ...
''   For fbc it seems better to emit proper names though and not rely on the
''   implicit position-based names, because the IR vreg allocation order does
''   not match the order of emitted operations.
''
'' - Labels begin basic blocks, certain operations (ret, br, ...) end them.
''   Basic blocks without a name/label are given a default name/label similar
''   to the default naming for operations.
''
'' - Labels are not allowed to appear consecutively (a basic block can only
''   have one name), and labels are not allowed in the middle of basic blocks
''   (only after an end operation like ret or br).
''   Both situations can happen in FB code easily (empty scope blocks, GOTO...),
''   so _emitLabel() needs to work around that by inserting no-ops or branches.
''   (a more complex solution would be to remove duplicate labels from the AST,
''    and redirect all uses of the removed label to the label that was kept)
''
'' - Operand types are always emitted explicitly; they are not guessed or
''   automatically derived from the actual operand.
''
'' - All types must match exactly, or llc will complain.
''   Since the AST does not always call irSetVregDataType() or irEmitConvert(),
''   the operations emitting ensures to emit casts if needed.
''
'' - Local variables are allocated from stack using "alloca",
''   the returned value is a pointer to the memory.
''
'' - Procedure parameters are passed as values, not pointers, so if the
''   function wants to take the address of a parameter,
''   it has to alloca a stack variable to hold the parameter value.
''   (that's what clang does)
''
'' - LLVM has some built-in functions (llvm.sin.f32, llvm.sin.f64,
''   llvm.memset.p0i8.i32, etc.) that we can use to implement various operations
''   instead of calling RTL functions (although, LLVM may implement them by
''   calling RTL functions if no optimization possible). Despite being intrinsic
''   these need to be declared globally and called just like normal procedures.
''   It seems best to handle them from here though instead of through the rtl
''   modules (e.g. rtlMathUop() and rtlMathBop()) because, for example, some use
''   i1 parameters (no corresponding FB type), and they're not really external
''   functions anyways (rtl reserved for functions from libc/libfb).
''
'' - Global constructors/destructors must be added to the llvm.global_ctors or
''   llvm.global_dtors arrays (each element = priority + function pointer).
''   There can be only one declaration of either per module, so all ctors/dtors
''   must be emitted into one of the two lists.
''
'' - Procedures can either be declared (if they're extern) or defined (with a
''   body) but not both.
''
'' - va_*() macros: Same issues as with the C backend
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
#include once "symbols/symb.bi"
#include once "support/containers/flist.bi"
#include once "support/containers/hash.bi"
#include once "lexer/lex.bi"
#include once "backend/ir-private.bi"
#include once "crt/mem.bi"

declare function hGetMagicStructNumber( byval sym as FBSYMBOL ptr ) as integer

enum
	SECTION_HEAD  '' global declarations
	SECTION_BODY  '' procedure bodies
	SECTION_FOOT  '' debugging meta data
end enum

enum
	BUILTIN_MEMSET = 0
	BUILTIN_MEMMOVE
	BUILTIN_MEMSET64
	BUILTIN_MEMMOVE64
	BUILTIN_SINF
	BUILTIN_SIN
	BUILTIN_COSF
	BUILTIN_COS
	BUILTIN_EXPF
	BUILTIN_EXP
	BUILTIN_LOGF
	BUILTIN_LOG
	BUILTIN_SQRTF
	BUILTIN_SQRT
	BUILTIN_FLOORF
	BUILTIN_FLOOR
	BUILTIN_ABSF
	BUILTIN_ABS
	BUILTIN_NEARBYINTF
	BUILTIN_NEARBYINT
	BUILTIN_VA_START
	BUILTIN_VA_END
	BUILTIN_VA_COPY
	BUILTIN__COUNT
end enum

type BUILTIN
	decl as const zstring ptr
	used as integer
end type

'' LLVM output sections are flushed in declaration, body, and debug order.
'' Geometric buffers keep large modules from copying prior output per line.
const LLVM_OUTPUT_BUFFER_INITIAL_CAPACITY = 8192

type IRLLVMOUTPUTBUFFER
	buffer          as ubyte ptr
	bufferlen       as uinteger
	buffercapacity  as uinteger
end type

dim shared as BUILTIN builtins(0 to BUILTIN__COUNT-1) => _
{ _
	(@"declare void @llvm.memset.p0i8.i32(i8*, i8, i32, i32, i1) nounwind"), _
	(@"declare void @llvm.memmove.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1) nounwind"), _
	(@"declare void @llvm.memset.p0i8.i64(i8*, i8, i64, i32, i1) nounwind"), _
	(@"declare void @llvm.memmove.p0i8.p0i8.i64(i8*, i8*, i64, i32, i1) nounwind"), _
	(@"declare float  @llvm.sin.f32(float ) nounwind"), _
	(@"declare double @llvm.sin.f64(double) nounwind"), _
	(@"declare float  @llvm.cos.f32(float ) nounwind"), _
	(@"declare double @llvm.cos.f64(double) nounwind"), _
	(@"declare float  @llvm.exp.f32(float ) nounwind"), _
	(@"declare double @llvm.exp.f64(double) nounwind"), _
	(@"declare float  @llvm.log.f32(float ) nounwind"), _
	(@"declare double @llvm.log.f64(double) nounwind"), _
	(@"declare float  @llvm.sqrt.f32(float ) nounwind"), _
	(@"declare double @llvm.sqrt.f64(double) nounwind"), _
	(@"declare float  @llvm.floor.f32(float ) nounwind"), _
	(@"declare double @llvm.floor.f64(double) nounwind"), _
	(@"declare float  @llvm.fabs.f32(float ) nounwind"), _
	(@"declare double @llvm.fabs.f64(double) nounwind"), _
	(@"declare float  @llvm.nearbyint.f32(float ) nounwind"), _
	(@"declare double @llvm.nearbyint.f64(double) nounwind"), _
	(@"declare void @llvm.va_start.p0i8(i8*) nounwind"), _
	(@"declare void @llvm.va_end.p0i8(i8*) nounwind"), _
	(@"declare void @llvm.va_copy.p0i8.p0i8(i8*, i8*) nounwind") _
}

type IRLLVMVARINISCOPE
	is_array as byte
	is_packed as byte
	has_padding as byte
	openlength as byte
	contentstart as integer
	sym as FBSYMBOL ptr
	expected_elements as longint
	emitted_elements as longint
	struct_offset as longint
	elementtype as string
end type

const MAXVARINISCOPES = 128

type IRLLVMCONTEXT
	indent              as integer  '' current indentation used by hWriteLine()
	linenum             as integer

	varini              as string
	variniscopelevel    as integer
	variniscopes(0 to MAXVARINISCOPES-1) as IRLLVMVARINISCOPE

	ctors               as string
	dtors               as string
	ctorcount           as integer
	dtorcount           as integer
	definedprocs        as TSTRSET
	declaredprocs       as TSTRSET
	udtnames            as TSTRSET
	udtnamecount        as integer
	localnames          as TSTRSET
	localnamecount      as integer
	addressedlabels     as TSTRSET

	fbctinf             as string
	fbctinf_len         as integer

	section             as integer  '' current section to write to
	head_txt            as IRLLVMOUTPUTBUFFER
	body_txt            as IRLLVMOUTPUTBUFFER
	foot_txt            as IRLLVMOUTPUTBUFFER
	proc_allocas        as IRLLVMOUTPUTBUFFER
	proc_body_offset    as uinteger
	proc_entry_offset   as uinteger
	proc_labels         as string
	proc_has_indirectbr as byte
	current_proc        as FBSYMBOL ptr
	inproc              as byte
end type

declare function hEmitType _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr _
	) as string
declare function hInteger128StorageAlignment _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr _
	) as integer
declare sub hEmitStruct( byval s as FBSYMBOL ptr )
declare sub _emitDBG _
	( _
		byval op as integer, _
		byval proc as FBSYMBOL ptr, _
		byval lnum as integer, _
		ByVal filename As zstring ptr = 0 _
	)
declare function hVregToStr( byval vreg as IRVREG ptr ) as string
declare function hLoadCLongDoubleNativeValue _
	( byval value as IRVREG ptr, byval subtype as FBSYMBOL ptr ) as IRVREG ptr
declare sub hEmitConvert( byval v1 as IRVREG ptr, byval v2 as IRVREG ptr )
declare sub hEmitStore( byval v1 as IRVREG ptr, byval v2 as IRVREG ptr )
declare sub hLoadVreg( byval v as IRVREG ptr )
declare sub hEmitBop _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval v2 as IRVREG ptr, _
		byval vr as IRVREG ptr, _
		byval label as FBSYMBOL ptr, _
		byval options as IR_EMITOPT _
	)

'' globals
dim shared as IRLLVMCONTEXT ctx

'' same order as FB_DATATYPE
dim shared as const zstring ptr dtypeName(0 to FB_DATATYPES-1) = _
{ _
	@"i8"       , _ '' void
	@"i8"       , _ '' boolean
	@"i8"       , _ '' byte
	@"i8"       , _ '' ubyte
	@"i8"       , _ '' char
	@"i16"      , _ '' short
	@"i16"      , _ '' ushort
	NULL        , _ '' wchar
	NULL        , _ '' integer
	NULL        , _ '' uinteger
	NULL        , _ '' enum
	@"i32"      , _ '' long
	@"i32"      , _ '' ulong
	@"i64"      , _ '' longint
	@"i64"      , _ '' ulongint
	@"i128"     , _ '' integer128
	@"i128"     , _ '' uinteger128
	@"float"    , _ '' single
	@"double"   , _ '' double
	@"%FBSTRING", _ '' string
	@"i8"       , _ '' fix-len string
	@"%struct.va_list", _ '' va_list - not tested, it can be different for every platform
	NULL        , _ '' struct
	NULL        , _ '' namespace
	NULL        , _ '' function
	@"i8"       , _ '' fwdref (needed for any un-resolved fwdrefs)
	NULL        , _ '' pointer
	NULL        , _ '' xmmword
	@"%FBSTRING"   _ '' ustring
}

private sub _init( )
	irhlInit( )

	irSetOption( IR_OPT_FPUIMMEDIATES or IR_OPT_MISSINGOPS )

	'' IR_OPT_CPUSELFBOPS disabled, to prevent AST from producing self-ops.
	'' LLVM does not have self ops, and implementing them manually here would be
	'' unnecessarily complex, especially in cases like:
	''    a = noconvcast(a) op b   (self-bop with type-casted destination vreg)
	'' because _setVregDataType() generates code to represent the cast, and the
	'' resulting REG can't be used as store destination.

	if( fbIs64bit( ) ) then
		dtypeName(FB_DATATYPE_INTEGER) = dtypeName(FB_DATATYPE_LONGINT)
		dtypeName(FB_DATATYPE_UINT   ) = dtypeName(FB_DATATYPE_ULONGINT)
	else
		dtypeName(FB_DATATYPE_INTEGER) = dtypeName(FB_DATATYPE_LONG)
		dtypeName(FB_DATATYPE_UINT   ) = dtypeName(FB_DATATYPE_ULONG)
	end if
end sub

'' LLVM lowering consults the module triple for target-specific va_list ABI
'' handling. Keep it in the IR so va_start knows which register-save layout
'' belongs to the selected FreeBASIC target.
private function hLLVMTargetTriple( ) as string
	dim as integer cpu_family = fbGetCpuFamily( )
	dim as string arch = *fbGetFbcArch( )
	dim as string vendor = "unknown"
	dim as string os_id

	select case cpu_family
	case FB_CPUFAMILY_X86
		if( env.clopt.cputype >= FB_CPUTYPE_686 ) then
			arch = "i686"
		else
			arch = "i386"
		end if
	case FB_CPUFAMILY_X86_64
		arch = "x86_64"
	case FB_CPUFAMILY_ARM
		select case env.clopt.cputype
		case FB_CPUTYPE_ARMV4: arch = "armv4t"
		case FB_CPUTYPE_ARMV5TE: arch = "armv5te"
		case FB_CPUTYPE_ARMV6, FB_CPUTYPE_ARMV6_FP: arch = "armv6"
		case FB_CPUTYPE_ARMV7A, FB_CPUTYPE_ARMV7A_FP: arch = "armv7"
		end select
	case FB_CPUFAMILY_AARCH64
		arch = "aarch64"
	case FB_CPUFAMILY_PPC
		arch = "powerpc"
	case FB_CPUFAMILY_PPC64
		arch = "powerpc64"
	case FB_CPUFAMILY_PPC64LE
		arch = "powerpc64le"
	case FB_CPUFAMILY_RISCV32
		arch = "riscv32"
	case FB_CPUFAMILY_RISCV64
		arch = "riscv64"
	case FB_CPUFAMILY_S390X
		arch = "s390x"
	case FB_CPUFAMILY_LOONGARCH64
		arch = "loongarch64"
	case FB_CPUFAMILY_MIPS32
		arch = "mips"
	case FB_CPUFAMILY_MIPS32EL
		arch = "mipsel"
	case FB_CPUFAMILY_MIPS64
		arch = "mips64"
	case FB_CPUFAMILY_MIPS64EL
		arch = "mips64el"
	case FB_CPUFAMILY_ASMJS
		arch = "wasm32"
	end select

	select case env.clopt.target
	case FB_COMPTARGET_LINUX
		os_id = "linux-gnu"
	case FB_COMPTARGET_ANDROID
		os_id = "linux-android"
	case FB_COMPTARGET_WIN32
		vendor = "w64"
		os_id = "windows-gnu"
	case FB_COMPTARGET_WINCE
		vendor = "unknown"
		os_id = "windows-msvc"
	case FB_COMPTARGET_CYGWIN
		vendor = "pc"
		os_id = "cygwin"
	case FB_COMPTARGET_DARWIN
		vendor = "apple"
		os_id = "darwin"
	case FB_COMPTARGET_FREEBSD
		os_id = "freebsd"
	case FB_COMPTARGET_DRAGONFLY
		os_id = "dragonfly"
	case FB_COMPTARGET_OPENBSD
		os_id = "openbsd"
	case FB_COMPTARGET_NETBSD
		os_id = "netbsd"
	case FB_COMPTARGET_HAIKU
		os_id = "haiku"
	case FB_COMPTARGET_SOLARIS
		os_id = "solaris"
	case FB_COMPTARGET_ILLUMOS
		os_id = "illumos"
	case FB_COMPTARGET_RISCOS
		os_id = "riscos"
	case FB_COMPTARGET_AROS
		os_id = "aros"
	case FB_COMPTARGET_AMIGA
		os_id = "amigaos"
	case FB_COMPTARGET_WII
		os_id = "none"
	case FB_COMPTARGET_JS
		os_id = "unknown"
	case FB_COMPTARGET_DOS
		vendor = "pc"
		os_id = "msdosdjgpp"
	case FB_COMPTARGET_XBOX
		vendor = "pc"
		os_id = "xbox"
	case else
		os_id = "unknown"
	end select

	return arch + "-" + vendor + "-" + os_id
end function

private sub _end( )
	irhlEnd( )

	if( ctx.head_txt.buffer <> NULL ) then
		deallocate( ctx.head_txt.buffer )
		ctx.head_txt.buffer = NULL
	end if
	ctx.head_txt.bufferlen = 0
	ctx.head_txt.buffercapacity = 0

	if( ctx.body_txt.buffer <> NULL ) then
		deallocate( ctx.body_txt.buffer )
		ctx.body_txt.buffer = NULL
	end if
	ctx.body_txt.bufferlen = 0
	ctx.body_txt.buffercapacity = 0

	if( ctx.foot_txt.buffer <> NULL ) then
		deallocate( ctx.foot_txt.buffer )
		ctx.foot_txt.buffer = NULL
	end if
	ctx.foot_txt.bufferlen = 0
	ctx.foot_txt.buffercapacity = 0

	if( ctx.proc_allocas.buffer <> NULL ) then
		deallocate( ctx.proc_allocas.buffer )
		ctx.proc_allocas.buffer = NULL
	end if
	ctx.proc_allocas.bufferlen = 0
	ctx.proc_allocas.buffercapacity = 0
end sub

private sub hAppendOutputBuffer _
	( _
		byval output_buffer as IRLLVMOUTPUTBUFFER ptr, _
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

	'' Unsigned wrap would otherwise allow an undersized allocation and copy.
	required = output_buffer->bufferlen + bytes
	if( required < output_buffer->bufferlen ) then
		error( 4 )
		exit sub
	end if

	if( required > output_buffer->buffercapacity ) then
		newcapacity = output_buffer->buffercapacity
		if( newcapacity = 0 ) then
			newcapacity = LLVM_OUTPUT_BUFFER_INITIAL_CAPACITY
		end if

		do while( newcapacity < required )
			doubledcapacity = newcapacity shl 1

			'' Once doubling would wrap, use the exact final request.
			if( doubledcapacity <= newcapacity ) then
				newcapacity = required
				exit do
			end if

			newcapacity = doubledcapacity
		loop

		'' Keep the old allocation until reallocation has succeeded.
		newbuffer = reallocate( output_buffer->buffer, newcapacity )
		if( newbuffer = NULL ) then
			error( 4 )
			exit sub
		end if

		output_buffer->buffer = newbuffer
		output_buffer->buffercapacity = newcapacity
	end if

	memcpy( output_buffer->buffer + output_buffer->bufferlen, src, bytes )
	output_buffer->bufferlen = required
end sub

private sub hWriteLine( byref ln as string )
	dim as IRLLVMOUTPUTBUFFER ptr output_buffer = any

	if( (ctx.indent > 0) andalso (len( ln ) > 0) ) then
		ln = string( ctx.indent, TABCHAR ) + ln
	end if

	ln += NEWLINE

	'' The full output is written in section order by _emitEnd().
	select case as const( ctx.section )
	case SECTION_HEAD
		output_buffer = @ctx.head_txt
	case SECTION_BODY
		output_buffer = @ctx.body_txt
	case SECTION_FOOT
		output_buffer = @ctx.foot_txt
	case else
		exit sub
	end select

	hAppendOutputBuffer( output_buffer, strptr( ln ), len( ln ) )
end sub

private sub hInternalCommand( byref message as string )
	hWriteLine( "; " + message )
end sub

private sub hAstCommand( byref message as string )
	hWriteLine( "" )
	hInternalCommand( message )
end sub

private sub hWriteLabel( byval id as zstring ptr )
	ctx.indent -= 1
	hWriteLine( *id + ":" )
	ctx.indent += 1
end sub

enum LLVM_BUILTIN
	LLVM_BUILTIN_NONE
	LLVM_BUILTIN_LIBC
	LLVM_BUILTIN_FFS
	LLVM_BUILTIN_CLZ
	LLVM_BUILTIN_CTZ
	LLVM_BUILTIN_CLRSB
	LLVM_BUILTIN_POPCOUNT
	LLVM_BUILTIN_PARITY
	LLVM_BUILTIN_BSWAP
	LLVM_BUILTIN_OVERFLOW
	LLVM_BUILTIN_OBJECT_SIZE
	LLVM_BUILTIN_DYNAMIC_OBJECT_SIZE
	LLVM_BUILTIN_EXPECT
	LLVM_BUILTIN_EXPECT_PROBABILITY
	LLVM_BUILTIN_PREFETCH
	LLVM_BUILTIN_TRAP
	LLVM_BUILTIN_UNREACHABLE
end enum

private function hGetBuiltin( byval proc as FBSYMBOL ptr ) as LLVM_BUILTIN
	if( symbGetIsParsed( proc ) ) then return LLVM_BUILTIN_NONE
	if( proc->id.alias = NULL ) then return LLVM_BUILTIN_NONE
	if( symbGetProcMode( proc ) <> FB_FUNCMODE_CDECL ) then return LLVM_BUILTIN_NONE
	'' Match the C alias before target decoration. Windows LLVM names may
	'' contain a quoted escape byte and the x86 leading underscore.
	var pname = *proc->id.alias
	if( left( pname, 10 ) <> "__builtin_" ) then return LLVM_BUILTIN_NONE
	if( len( irGetBuiltinLibcName( proc ) ) > 0 ) then return LLVM_BUILTIN_LIBC
	select case( mid( pname, 11 ) )
	case "ffs", "ffsl", "ffsll": return LLVM_BUILTIN_FFS
	case "clz", "clzl", "clzll": return LLVM_BUILTIN_CLZ
	case "ctz", "ctzl", "ctzll": return LLVM_BUILTIN_CTZ
	case "clrsb", "clrsbl", "clrsbll": return LLVM_BUILTIN_CLRSB
	case "popcount", "popcountl", "popcountll": return LLVM_BUILTIN_POPCOUNT
	case "parity", "parityl", "parityll": return LLVM_BUILTIN_PARITY
	case "bswap16", "bswap32", "bswap64": return LLVM_BUILTIN_BSWAP
	case "sadd_overflow", "saddl_overflow", "saddll_overflow", _
	     "uadd_overflow", "uaddl_overflow", "uaddll_overflow", _
	     "ssub_overflow", "ssubl_overflow", "ssubll_overflow", _
	     "usub_overflow", "usubl_overflow", "usubll_overflow", _
	     "smul_overflow", "smull_overflow", "smulll_overflow", _
	     "umul_overflow", "umull_overflow", "umulll_overflow"
		return LLVM_BUILTIN_OVERFLOW
	case "object_size": return LLVM_BUILTIN_OBJECT_SIZE
	case "dynamic_object_size": return LLVM_BUILTIN_DYNAMIC_OBJECT_SIZE
	case "expect": return LLVM_BUILTIN_EXPECT
	case "expect_with_probability": return LLVM_BUILTIN_EXPECT_PROBABILITY
	case "prefetch": return LLVM_BUILTIN_PREFETCH
	case "trap": return LLVM_BUILTIN_TRAP
	case "unreachable": return LLVM_BUILTIN_UNREACHABLE
	end select
	return LLVM_BUILTIN_NONE
end function

private function hObjectSymbolName( byref pname as const string ) as string
	if( left(pname, 1) <> "@" ) then
		fbSemanticLinkFail( )
		return ""
	end if
	'' This is the emitter's typed global identifier, not source text.
	'' LLVM's byte-one prefix suppresses target mangling; its @ marker
	'' and identifier quotes do not belong to the object symbol. Without
	'' the escape, LLVM supplies the target's ordinary C symbol prefix.
	dim as string object_name = mid(pname, 2)
	if( left(object_name, 1) = """" ) then
		if( (len(object_name) < 2) or (right(object_name, 1) <> """") ) then
			fbSemanticLinkFail( )
			return ""
		else
			object_name = mid(object_name, 2, len(object_name) - 2)
			'' Quoted LLVM identifiers encode a byte as backslash plus
			'' exactly two hexadecimal digits. Decode that native IR
			'' representation once, before applying its mangling escape.
			dim as string decoded
			dim as integer position = 1
			do while( position <= len(object_name) )
				if( object_name[position - 1] = asc("\") ) then
					if( position + 2 > len(object_name) ) then
						fbSemanticLinkFail( )
						return ""
					end if
					dim as integer value = 0
					for digit as integer = position + 1 to position + 2
						dim as integer code = asc(object_name, digit)
						select case code
						case asc("0") to asc("9"): code -= asc("0")
						case asc("A") to asc("F"): code = code - asc("A") + 10
						case asc("a") to asc("f"): code = code - asc("a") + 10
						case else
							fbSemanticLinkFail( )
							return ""
						end select
						value = value * 16 + code
					next
					decoded += chr(value)
					position += 3
				else
					decoded += chr(object_name[position - 1])
					position += 1
				end if
			loop
			object_name = decoded
		end if
	end if
	if( left(object_name, 1) = chr(1) ) then
		object_name = mid(object_name, 2)
	elseif( env.underscoreprefix ) then
		object_name = "_" + object_name
	end if
	return object_name
end function

private function hEmitProcName( byval proc as FBSYMBOL ptr ) as string
	var pname = *symbGetMangledName( proc )
	var builtin = hGetBuiltin( proc )
	if( builtin = LLVM_BUILTIN_LIBC ) then
		'' These builtins have the libc ABI, including their return values.
		'' The selected libc name is shared by calls and procedure addresses.
		pname = "@" + irGetBuiltinLibcName( proc )
	end if
	fbSemanticModelEmissionName(proc, "llvm", pname)
	if( fbSemanticLinkProcedureObjectNeeded(proc) or fbSemanticModelObjectNameNeeded(proc) ) then
		if( symbIsPrivate(proc) and (symbIsExport(proc) = FALSE) ) then
			fbSemanticModelEmissionName(proc, "object", "", "unavailable", "llvm-private")
		elseif( (builtin = LLVM_BUILTIN_NONE) or (builtin = LLVM_BUILTIN_LIBC) ) then
			var object_name = hObjectSymbolName(pname)
			fbSemanticModelEmissionName(proc, "object", object_name)
			fbSemanticLinkProcedureObject(proc, object_name)
		else
			fbSemanticModelEmissionName(proc, "object", "", "unavailable", "backend-intrinsic")
		end if
	end if
	return pname
end function

private sub hCheckBuiltinAddress( byval proc as FBSYMBOL ptr )
	var builtin = hGetBuiltin( proc )
	if( (builtin <> LLVM_BUILTIN_NONE) and (builtin <> LLVM_BUILTIN_LIBC) ) then
		errReport( FB_ERRMSG_INVALIDDATATYPES, , ": builtin must be called directly: " + *symbGetName( proc ) )
	end if
end sub

private function hSymbolKey( byval sym as FBSYMBOL ptr ) as string
	'' Hash identities use the compiler host's pointer width. Widen only after
	'' converting to the pointer-sized unsigned type: a direct pointer-to-64-bit
	'' cast is rejected when this emitter is built for a 32-bit host.
	return hex( culngint( cuint( sym ) ), 16 )
end function

declare function hGlobalVarLinkage( byval sym as FBSYMBOL ptr, byval has_initializer as integer ) as string

private sub hObserveVarName( byval sym as FBSYMBOL ptr, byref identifier as const string )
	fbSemanticModelEmissionName(sym, "llvm", identifier)
	if( fbSemanticModelObjectNameNeeded(sym) = FALSE ) then exit sub
	if( left(identifier, 1) <> "@" ) then
		fbSemanticModelEmissionName(sym, "object", "", "none", "automatic-storage")
	elseif( symbIsThreadLocal(sym) ) then
		fbSemanticModelEmissionName(sym, "object", "", "unavailable", "backend-tls")
	elseif( symbGetIsLiteral(sym) or (hGlobalVarLinkage(sym, FALSE) = "private") ) then
		'' LLVM's assembler printer chooses private-label prefixes. Its IR
		'' identifier remains exact, but is not itself a known object symbol.
		fbSemanticModelEmissionName(sym, "object", "", "unavailable", "llvm-private")
	else
		var object_name = hObjectSymbolName(identifier)
		fbSemanticModelEmissionName(sym, "object", object_name)
	end if
end sub

private function hEmitVarName( byval sym as FBSYMBOL ptr ) as string
	if( symbIsProc( sym ) ) then
		hCheckBuiltinAddress( sym )
		return hEmitProcName( sym )
	end if
	var id = *symbGetMangledName( sym )
	if( symbIsVar( sym ) = FALSE or left( id, 1 ) <> "%" ) then
		hObserveVarName(sym, id)
		return id
	end if

	'' LLVM has one local name scope for an entire procedure. BASIC can
	'' declare another variable with the same name inside a nested scope.
	var key = hSymbolKey( sym )
	var item = cptr( TSTRSETITEM ptr, hashLookup( @ctx.localnames.hash, strptr( key ) ) )
	if( item = NULL ) then
		ctx.localnamecount += 1
		strsetAdd( @ctx.localnames, key, ctx.localnamecount )
		id += "." + str( ctx.localnamecount )
	else
		id += "." + str( item->userdata )
	end if
	hObserveVarName(sym, id)
	return id
end function

private function hSymName( byval sym as FBSYMBOL ptr ) as string
	if( sym->id.alias ) then
		function = *sym->id.alias
	else
		function = *symbGetName( sym )
	end if
end function

private function vregPretty( byval v as IRVREG ptr ) as string
	dim s as string

	select case( v->typ )
	case IR_VREGTYPE_IMM
		if( typeGetClass( v->dtype ) = FB_DATACLASS_FPOINT ) then
			s = str( v->value.f )
		else
			s = str( v->value.i )
		end if

	case IR_VREGTYPE_REG
		if( v->sym ) then
			s = hSymName( v->sym )
		else
			s = "vr" & v->reg
		end if

	case else
		if( v->sym ) then
			s = hSymName( v->sym )
		end if
	end select

	if( v->vidx ) then
		if( len( s ) > 0 ) then
			s += "+"
		end if
		s += vregPretty( v->vidx )
	end if
	if( v->ofs ) then
		s += "+" & v->ofs
	end if
	if( v->mult ) then
		s += "*" & v->mult
	end if

	's += " " + typeDumpToStr( v->dtype, v->subtype )

	function = s
end function

private function hEmitParamName( byval sym as FBSYMBOL ptr ) as string
	function = hEmitVarName( sym ) + "$"
end function

private function hEmitProcCallConv( byval proc as FBSYMBOL ptr ) as string
	'' Calling convention
	'' - default if none specified is Cdecl as in C
	'' - must be given on the declaration, on the body,
	''   and on each CALL instruction
	''
	'' Note: Pascal is like Stdcall (callee cleans up stack), except that
	'' arguments are pushed left-to-right (same order as written in code,
	'' not reversed like Cdecl/Stdcall).
	'' The symbGetProc*Param() macros take care of changing the order when
	'' cycling through parameters of Pascal functions. Together with Stdcall
	'' this results in a double-reverse resulting in the proper ABI.
	''
	'' For non-x86, don't emit any calling convention at all, it would just
	'' be ignored anyways (for x86_64 and ARM it seems that way at least).

	if( fbGetCpuFamily( ) <> FB_CPUFAMILY_X86 ) then
		exit function
	end if

	select case as const( symbGetProcMode( proc ) )
	case FB_FUNCMODE_CDECL
		function = ""
	case FB_FUNCMODE_STDCALL, FB_FUNCMODE_STDCALL_MS, FB_FUNCMODE_PASCAL
		function = "x86_stdcallcc "
	case FB_FUNCMODE_THISCALL
		function = "x86_thiscallcc "
	case FB_FUNCMODE_FASTCALL
		function = "x86_fastcallcc "
	case else
		errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
		function = ""
	end select
end function

#include once "backend/llvm/ir-llvm-abi.bi"

private function hEmitProcHeader _
	( _
		byval proc as FBSYMBOL ptr, _
		byval is_proto as integer, _
		byval is_type as integer, _
		byval emit_semantic as integer = TRUE _
	) as string

	dim as string ln
	dim as integer dtype = any
	dim as FBSYMBOL ptr subtype = any
	dim as integer i386_int128_result = any
	dim as integer cabi_clongdouble_result = _
		hUsesCLongDoubleCABI( proc ) andalso _
		hIsCLongDoubleType( symbGetProcRealType( proc ), _
		                     symbGetProcRealSubtype( proc ) )
	dim as string semantic_result, semantic_convention, semantic_role

	assert( symbIsProc( proc ) )

	'' LLVM attaches calling conventions to declarations and calls, not
	'' function types. A procedure pointer retains its BASIC convention in
	'' the symbol, which hEmitProcCallConv supplies at the indirect call.
	if( is_type = FALSE ) then semantic_convention = hEmitProcCallConv( proc )
	ln += semantic_convention

	i386_int128_result = hIsI386Integer128Result( proc )

	'' The i386 C ABI returns aligned 16-byte aggregates through an sret pointer.
	'' Keep the source scalar type in the body, but expose that target ABI here.
	if( i386_int128_result ) then
		semantic_result = "void"
	elseif( cabi_clongdouble_result ) then
		semantic_result = hGetCLongDoubleLLVMType( symbGetProcRealSubtype( proc ) )
		if( semantic_result = "" ) then
			errReport( FB_ERRMSG_INVALIDDATATYPES, , _
			           ": CLongDouble procedure results are not supported by the LLVM backend for this target" )
			semantic_result = hEmitType( typeGetDtAndPtrOnly( symbGetProcRealType( proc ) ), _
								symbGetProcRealSubtype( proc ) )
		end if
	else
		'' Function result type (is 'void' for subs)
		semantic_result = hEmitType( typeGetDtAndPtrOnly( symbGetProcRealType( proc ) ), _
					symbGetProcRealSubtype( proc ) )
	end if
	ln += semantic_result
	semantic_role = iif(is_type, "pointer-type", iif(is_proto, "declaration", "definition"))
	dim as longint semantic_signature = 0
	if( emit_semantic ) then
		semantic_signature = fbSemanticModelBackendAbiBegin( _
			proc, "llvm", semantic_role, semantic_result, semantic_convention )
	end if

	ln += " "

	if( is_type = FALSE ) then
		'' @id
		ln += hEmitProcName( proc )
	end if

	'' Parameter list
	ln += "( "

	'' If returning a struct, there's an extra parameter
	dim as FBSYMBOL ptr hidden = NULL
	if( symbProcReturnsOnStack( proc ) andalso (cabi_clongdouble_result = FALSE) ) then
		dim as integer semantic_hidden_dtype
		dim as FBSYMBOL ptr semantic_hidden_subtype
		dim as string semantic_hidden, semantic_hidden_attributes
		if( i386_int128_result ) then
			if( is_proto ) then
				semantic_hidden_dtype = typeAddrOf( symbGetFullType( proc ) )
				semantic_hidden_subtype = NULL
			else
				hidden = proc->proc.ext->res
				semantic_hidden_dtype = typeAddrOf( symbGetType( hidden ) )
				semantic_hidden_subtype = symbGetSubtype( hidden )
			end if
			semantic_hidden = hEmitType(semantic_hidden_dtype, semantic_hidden_subtype)
			ln += semantic_hidden
			if( is_type = FALSE ) then
				semantic_hidden_attributes = " sret(i128) align " + str( LLVM_I386_INT128_ABI_ALIGNMENT )
				ln += semantic_hidden_attributes
			end if
			if( is_proto = FALSE ) then
				ln += " " + hEmitParamName( hidden )
			end if
		elseif( is_proto ) then
			hidden = symbGetSubType( proc )
			semantic_hidden_dtype = typeAddrOf( symbGetType( hidden ) )
			semantic_hidden_subtype = hidden
			semantic_hidden = hEmitType(semantic_hidden_dtype, semantic_hidden_subtype)
			ln += semantic_hidden
		else
			hidden = proc->proc.ext->res
			semantic_hidden_dtype = typeAddrOf( symbGetType( hidden ) )
			semantic_hidden_subtype = symbGetSubtype( hidden )
			semantic_hidden = hEmitType(semantic_hidden_dtype, semantic_hidden_subtype)
			ln += semantic_hidden
			ln += " " + hEmitParamName( hidden )
		end if
		if( emit_semantic ) then
			fbSemanticModelBackendAbiParameter(semantic_signature, NULL, 0, 1, "hidden-result", semantic_hidden, _
				semantic_hidden_dtype, semantic_hidden_subtype, semantic_hidden_attributes)
		end if

		if( symbGetProcParams( proc ) > 0 ) then
			ln += ", "
		end if
	end if

	dim as DZSTRING params
	DZstrZero( params )
	var param = symbGetProcLastParam( proc )
	while( param )
		if( symbGetParamMode( param ) = FB_PARAMMODE_VARARG ) then
			DZstrConcatAssign( params, "..." )
			if( emit_semantic ) then _
				fbSemanticModelBackendAbiParameter(semantic_signature, param, 0, 0, "variadic", "...")
		else
			symbGetRealParamDtype( param, dtype, subtype )
			dim as string semantic_kind = "value", semantic_parameter, semantic_attributes
			dim as FBSYMBOL ptr paramvar = NULL
			if( (symbGetParamMode( param ) = FB_PARAMMODE_BYVAL) andalso _
			    (symbGetValistType( dtype, subtype ) = FB_CVA_LIST_BUILTIN_C_STD) ) then
				'' C's array typedef decays to a pointer at this boundary.
				dtype = typeAddrOf( dtype )
				semantic_kind = "va-list-decay"
			end if
			dim as integer split_i386_int128 = _
				hIsI386Integer128Param( dtype, symbGetParamMode( param ) )
			if( split_i386_int128 ) then
				DZstrConcatAssign( params, "i64" )
				if( emit_semantic ) then _
					fbSemanticModelBackendAbiParameter(semantic_signature, param, 0, 2, "i386-int128-low", "i64", FB_DATATYPE_ULONGINT)
				if( is_proto = FALSE ) then
					paramvar = symbGetParamVar( param )
					if( paramvar <> NULL ) then
						DZstrConcatAssign( params, " " + hEmitParamName( paramvar ) + ".low" )
					end if
				end if
				DZstrConcatAssign( params, ", i64" )
				if( emit_semantic ) then _
					fbSemanticModelBackendAbiParameter(semantic_signature, param, 1, 2, "i386-int128-high", "i64", FB_DATATYPE_ULONGINT)
				if( is_proto = FALSE ) then
					if( paramvar <> NULL ) then
						DZstrConcatAssign( params, " " + hEmitParamName( paramvar ) + ".high" )
					end if
				end if
			elseif( hUsesCLongDoubleCABI( proc ) andalso _
			        hIsCLongDoubleType( dtype, subtype ) ) then
				semantic_parameter = hGetCLongDoubleLLVMType( subtype )
				if( semantic_parameter = "" ) then
					errReport( FB_ERRMSG_INVALIDDATATYPES, , _
					           ": CLongDouble procedure parameters are not supported by the LLVM backend for this target" )
					semantic_parameter = hEmitType( dtype, subtype )
				end if
				DZstrConcatAssign( params, semantic_parameter )
				if( emit_semantic ) then _
					fbSemanticModelBackendAbiParameter(semantic_signature, param, 0, 1, _
						"fixed-clongdouble-native", semantic_parameter, dtype, subtype)
			elseif( hIsMemoryByval( dtype, subtype, symbGetParamMode(param) ) ) then
				semantic_parameter = hEmitType(typeAddrOf(dtype), subtype)
				DZstrConcatAssign( params, semantic_parameter )
				if( is_type = FALSE ) then
					semantic_attributes = hMemoryByvalAttribute( subtype )
					DZstrConcatAssign( params, semantic_attributes )
				end if
				if( emit_semantic ) then _
					fbSemanticModelBackendAbiParameter(semantic_signature, param, 0, 1, "memory-byval", semantic_parameter, _
						typeAddrOf(dtype), subtype, semantic_attributes)
			else
				semantic_parameter = hEmitType(dtype, subtype)
				DZstrConcatAssign( params, semantic_parameter )
				if( emit_semantic ) then _
					fbSemanticModelBackendAbiParameter(semantic_signature, param, 0, 1, semantic_kind, semantic_parameter, dtype, subtype)
			end if

			if( (is_proto = FALSE) andalso (split_i386_int128 = FALSE) ) then
				'' Proc body? Emit the mangled name of the param var
				'' (the param itself isn't mangled). NAKED procedures have
				'' no parameter variables; their assembly reads the ABI registers.
				paramvar = symbGetParamVar( param )
				if( paramvar <> NULL ) then
					DZstrConcatAssign( params, " " + hEmitParamName( paramvar ) )
				end if
			end if
		end if

		param = symbGetProcPrevParam( proc, param )
		if( param ) then
			DZstrConcatAssign( params, ", " )
		end if
	wend

	if( params.data <> NULL ) then
		ln += *params.data
	end if
	DZstrAllocate( params, 0 )

	ln += " )"

	if( is_type = FALSE ) then
		'' The LLVM backend has no exception-unwind model. Marking procedures
		'' nounwind records that contract and allows LLVM to optimize calls.
		ln += " nounwind"

		if( proc->pattrib and FB_PROCATTRIB_NAKED ) then
			ln += " naked"
		elseif( (is_proto = FALSE) andalso (ast.proc.curr <> NULL) ) then
			'' PUSH and CALL in user assembly can overwrite a leaf procedure's
			'' red zone. Keep asm operands in a frame whose addresses stay valid
			'' while the user temporarily changes RSP. Other procedures keep the
			'' normal LLVM frame policy.
			var node = ast.proc.curr->l
			while( node <> NULL )
				if( node->class = AST_NODECLASS_ASM ) then
					ln += " noredzone ""frame-pointer""=""all"""
					exit while
				end if
				node = node->next
			wend
		end if
	end if

	if( emit_semantic ) then fbSemanticModelBackendAbiEnd( semantic_signature, ln )
	function = ln
end function

private function hGetUDTName( byval sym as FBSYMBOL ptr ) as string
	'' LLVM type names are module-local. A generated descriptor can have an
	'' unstable source alias, so assign each symbol a stable module ordinal.
	var key = hSymbolKey( sym )
	var item = cptr( TSTRSETITEM ptr, hashLookup( @ctx.udtnames.hash, strptr( key ) ) )
	if( item = NULL ) then
		ctx.udtnamecount += 1
		strsetAdd( @ctx.udtnames, key, ctx.udtnamecount )
		return "%T_" + str( ctx.udtnamecount )
	end if
	return "%T_" + str( item->userdata )
end function

private sub hAssignUDTName( byval sym as FBSYMBOL ptr )
	'' Temporary descriptors may reuse a freed symbol address. Their new
	'' definitions still need fresh LLVM type names in this module.
	var key = hSymbolKey( sym )
	var item = cptr( TSTRSETITEM ptr, hashLookup( @ctx.udtnames.hash, strptr( key ) ) )
	ctx.udtnamecount += 1
	if( item ) then
		item->userdata = ctx.udtnamecount
	else
		strsetAdd( @ctx.udtnames, key, ctx.udtnamecount )
	end if
end sub

private sub hEmitUDT( byval s as FBSYMBOL ptr )
	if( s = NULL ) then
		return
	end if

	if( symbGetIsEmitted( s ) ) then
		return
	end if

	var oldsection = ctx.section
	'' LLVM named type definitions belong at module scope, including types
	'' first encountered while emitting a procedure body.
	ctx.section = SECTION_HEAD

	select case as const( symbGetClass( s ) )
	case FB_SYMBCLASS_ENUM
		hAssignUDTName( s )
		symbSetIsEmitted( s )
		'' no subtype, to avoid infinite recursion
		hWriteLine( hGetUDTName( s ) + " = type " + hEmitType( FB_DATATYPE_ENUM, NULL ) )

	case FB_SYMBCLASS_STRUCT
		hEmitStruct( s )

	end select

	ctx.section = oldsection
end sub

private sub hBuildStrLit _
	( _
		byref ln as string, _
		byval wantedlength as integer, _ '' including null terminator
		byval z as zstring ptr, _
		byval length as integer, _       '' ditto
		byval padchar as integer = 0 _
	)

	dim as integer ch = any
	dim as DZSTRING buffer

	DZstrZero( buffer )
	DZstrAssign( buffer, strptr( ln ) )

	'' Convert the string to LLVM IR format
	'' (assuming internal escape sequences have already been solved out
	'' using hUnescape())
	''
	'' clang turns
	''    "a\0\\\n"
	'' into
	''    [5 x i8] c"a\00\5C\0A\00", align 1
	''
	'' \0 doesn't work, it must be two digits as in \00.

	'' String literal too long?
	if( length > wantedlength ) then
		'' Cut off; may be empty afterwards
		length = wantedlength
	end if

	for i as integer = 0 to length - 1
		ch = (*z)[i]
		'' chars like a-zA-Z0-9 can be emitted literally,
		'' but special chars (including '\') should be encoded in hex
		if( hCharNeedsEscaping( ch, asc( """" ) ) ) then
			'' emit in \XX escape form
			DZstrConcatAssign( buffer, $"\" + hex( ch, 2 ) )
		else
			'' emit as-is
			DZstrConcatAssignC( buffer, ch )
		end if
	next

	'' FIXSTR uses space padding; ZSTRING and literals use zeroes.
	while( length < wantedlength )
		if( padchar = 0 ) then
			DZstrConcatAssign( buffer, $"\00" )
		else
			DZstrConcatAssignC( buffer, padchar )
		end if
		length += 1
	wend

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
		byval wantedlength as integer, _  '' including null terminator
		byval w as wstring ptr, _
		byval length as integer _         '' ditto
	)

	dim as uinteger ch = any, wcharsize = any
	dim as DZSTRING buffer
	const WCHAR_BYTE_1_SHIFT = 8
	const WCHAR_BYTE_2_SHIFT = 16
	const WCHAR_BYTE_3_SHIFT = 24

	DZstrZero( buffer )
	DZstrAssign( buffer, strptr( ln ) )

	'' (ditto)
	''
	'' clang turns
	''    L"a\0\\\n"
	'' into
	''    [20 x i8] c"a\00\00\00\00\00\00\00\5C\00\00\00\0A\00\00\00\00\00\00\00", align 4
	'' (with Linux 4-byte wchar_t)

	wcharsize = typeGetSize( env.target.wchar )

	'' String literal too long?
	if( length > wantedlength ) then
		'' Cut off; may be empty afterwards
		length = wantedlength
	end if

	for i as integer = 0 to length - 1
		ch = (*w)[i]
		'' (ditto)
		if( hCharNeedsEscaping( ch, asc( """" ) ) ) then
			if( wcharsize >= 1 ) then
				DZstrConcatAssign( buffer, $"\" + hex( (ch       ) and &hFF, 2 ) )
			end if
			if( wcharsize >= 2 ) then
				DZstrConcatAssign( buffer, $"\" + hex( (ch shr WCHAR_BYTE_1_SHIFT) and &hFF, 2 ) )
			end if
			if( wcharsize >= 4 ) then
				DZstrConcatAssign( buffer, $"\" + hex( (ch shr WCHAR_BYTE_2_SHIFT) and &hFF, 2 ) )
				DZstrConcatAssign( buffer, $"\" + hex( (ch shr WCHAR_BYTE_3_SHIFT) and &hFF, 2 ) )
			end if
		else
			DZstrConcatAssignC( buffer, ch )
			'' Pad up to wchar_t size
			for j as integer = 2 to wcharsize
				DZstrConcatAssign( buffer, $"\00" )
			next
		end if
	next

	'' Pad with zeroes if string literal too short
	while( length < wantedlength )
		'' Pad up to wchar_t size
		for j as integer = 1 to wcharsize
			DZstrConcatAssign( buffer, $"\00" )
		next
		length += 1
	wend

	if( buffer.data <> NULL ) then
		ln = *buffer.data
	else
		ln = ""
	end if
	DZstrAllocate( buffer, 0 )
end sub

private function hEmitStrLitType( byval length as integer ) as string
	function = "[" + str( length ) + " x i8]"
end function

private function hEmitSymType( byval sym as FBSYMBOL ptr ) as string
	dim s as string

	var dtype = symbGetType( sym )
	if( symbIsRef( sym ) ) then
		s = hEmitType( typeAddrOf( dtype ), sym->subtype )
	else
		select case( dtype )
		case FB_DATATYPE_FIXSTR, FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR
			s = hEmitStrLitType( sym->lgt )
		case else
			s = hEmitType( dtype, sym->subtype )
		end select
	end if

	'' Fake dynamic-array symbols are filtered before type emission; their
	'' companion descriptor symbols carry the actual structure type.
	assert( symbGetIsDynamic( sym ) = FALSE )

	select case( symbGetClass( sym ) )
	case FB_SYMBCLASS_VAR, FB_SYMBCLASS_FIELD
		'' Fixed-size array vars/fields
		''    (0 to 9) as long            =>   [10 x i32]
		''    (0 to 9, 0 to 19) as long   =>   [10 x [20 x i32]]
		for i as integer = symbGetArrayDimensions( sym ) - 1 to 0 step -1
			var elements = symbArrayUbound( sym, i ) - symbArrayLbound( sym, i ) + 1
			s = "[" & elements & " x " + s + "]"
		next
	end select

	function = s
end function

private function hGlobalVarLinkage( byval sym as FBSYMBOL ptr, byval has_initializer as integer ) as string
	'' LLVM globals need the same visibility as the C backend's declarations.
	'' DATA descriptors and module-local STATIC/SHARED variables stay local.
	if( symbIsDataDesc( sym ) or symbIsPrivate( sym ) or _
	    (symbIsStatic( sym ) andalso _
	     ((symbGetAttrib( sym ) and _
	       (FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_PUBLIC or FB_SYMBATTRIB_EXTERN)) = 0)) ) then
		return "private"
	end if
	if( symbIsCommon( sym ) andalso (has_initializer = FALSE) ) then
		return "common"
	end if
	return ""
end function

private sub hEmitVariable( byval sym as FBSYMBOL ptr )
	dim as string ln
	dim as integer is_global = any, length = any
	dim as integer storage_alignment = any

	'' literal?
	if( symbGetIsLiteral( sym ) ) then
		if( symbGetIsAccessed( sym ) = FALSE ) then
			exit sub
		end if

		select case( symbGetType( sym ) )
		case FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR
			'' string literals are emitted as global char arrays,
			'' this also means a bitcast to char pointer is needed
			'' on every use of the global symbol.
			ln = hEmitVarName( sym ) + " = "
			ln += "private constant "
			ln += hEmitSymType( sym )
			ln += " c"""
			'' Literal storage can include padding beyond the decoded text.
			'' The shared unescape buffer initializes only decoded characters
			'' and their terminator; the builder must supply the remaining zeroes.
			dim as integer decodedlength
			if( symbGetType( sym ) = FB_DATATYPE_WCHAR ) then
				length = symbGetWstrLength( sym ) + 1
				var decoded = hUnescapeW( symbGetVarLitTextW( sym ), decodedlength )
				hBuildWstrLit( ln, length, decoded, decodedlength + 1 )
			else
				length = symbGetStrLength( sym ) + 1
				var decoded = hUnescape( symbGetVarLitText( sym ), decodedlength )
				hBuildStrLit( ln, length, decoded, decodedlength + 1 )
			end if
			ln += """"
			if( symbGetType( sym ) = FB_DATATYPE_WCHAR ) then
				'' Wide literals are byte arrays in LLVM, but rtlib reads them
				'' through FB_WCHAR pointers and requires WCHAR alignment.
				ln += ", align " & typeGetSize( FB_DATATYPE_WCHAR )
			end if
			hWriteLine( ln )
		case else
			'' float constants are handled as "literals",
			'' at least under the ASM backend
			exit select
		end select

		exit sub
	end if

	'' initialized? only if not local or local and static
	if( (symbGetTypeIniTree( sym ) <> NULL) and (symbIsLocal( sym ) = FALSE or symbIsStatic( sym )) ) then
		'' never referenced?
		if( symbIsLocal( sym ) = FALSE ) then
			if( symbGetIsAccessed( sym ) = FALSE ) then
				'' not public?
				if( symbIsPublic( sym ) = FALSE ) then
					exit sub
				end if
			end if
		end if

		irhlFlushStaticInitializer( sym )
		exit sub
	end if

	'' dynamic? only the array descriptor is emitted
	if( symbGetIsDynamic( sym ) ) then
		exit sub
	end if

	if( symbIsExtern( sym ) ) then
		'' An EXTERN is a declaration. Defining it here creates duplicate
		'' symbols when several modules use the same BASIC or runtime global.
		if( symbGetIsAccessed( sym ) ) then
			ln = hEmitVarName( sym ) + " = external"
			if( symbIsThreadLocal( sym ) ) then ln += " thread_local"
			ln += " global " + hEmitSymType( sym )
			storage_alignment = hInteger128StorageAlignment( _
				symbGetType( sym ), symbGetSubtype( sym ) )
			if( storage_alignment > 0 ) then _
				ln += ", align " + str( storage_alignment )
			hWriteLine( ln )
		end if
		exit sub
	end if

	is_global = symbGetAttrib( sym ) and _
			(FB_SYMBATTRIB_COMMON or FB_SYMBATTRIB_PUBLIC or _
			FB_SYMBATTRIB_EXTERN or FB_SYMBATTRIB_STATIC or _
			FB_SYMBATTRIB_SHARED)

	'' Global var:
	''    @sym = global <type> <initvalue>
	'' Stack var:
	''    %sym = alloca <type>
	ln = hEmitVarName( sym )
	ln += " = "
	if( is_global ) then
		var linkage = hGlobalVarLinkage( sym, FALSE )
		if( len(linkage) > 0 ) then ln += linkage + " "
		if( symbIsThreadLocal( sym ) ) then ln += "thread_local "
		ln += "global"
	else
		ln += "alloca"
	end if
	ln += " " + hEmitSymType( sym )
	if( is_global ) then
		'' Globals without initializer are zeroed in FB
		ln += " zeroinitializer"
	end if
	storage_alignment = hInteger128StorageAlignment( _
		symbGetType( sym ), symbGetSubtype( sym ) )
	if( storage_alignment > 0 ) then
		ln += ", align " + str( storage_alignment )
	elseif( symbGetType( sym ) = FB_DATATYPE_WCHAR ) then
		ln += ", align " & typeGetSize( FB_DATATYPE_WCHAR )
	end if
	if( is_global or (ctx.inproc = FALSE) ) then
		hWriteLine( ln )
	else
		'' A branch may skip a source-level declaration. Keep every local
		'' alloca in the entry block so it dominates all later uses.
		ln = string( ctx.indent, TABCHAR ) + ln + NEWLINE
		hAppendOutputBuffer( @ctx.proc_allocas, strptr( ln ), len( ln ) )
	end if
end sub

private sub hMaybeEmitGlobalVar( byval sym as FBSYMBOL ptr )
	'' Skip DATA descriptor arrays here, they're handled by irForEachDataStmt()
	if( symbIsDataDesc( sym ) = FALSE ) then
		hEmitVariable( sym )
	end if
end sub

private sub hMaybeEmitProcProto( byval s as FBSYMBOL ptr )
	if( symbGetIsFuncPtr( s ) or (not symbGetIsAccessed( s )) ) then
		exit sub
	end if

	if( symbGetMangledName( s ) = NULL ) then
		exit sub
	end if

	'' Separate DECLARE and DEFINE symbols can share one linker name. A
	'' definition in this module makes the prototype unnecessary.
	if( hashLookup( @ctx.definedprocs.hash, symbGetMangledName( s ) ) <> NULL ) then
		exit sub
	end if

	'' Only declare functions that won't be defined (don't have a body),
	'' llc doesn't seem to allow DECLARE+DEFINE with the same id
	if( symbGetIsParsed( s ) ) then
		exit sub
	end if
	var builtin = hGetBuiltin( s )
	if( (builtin <> LLVM_BUILTIN_NONE) and (builtin <> LLVM_BUILTIN_LIBC) ) then
		'' Intrinsics have LLVM signatures, sometimes including i1 or structs.
		'' Their declarations are emitted by the builtin call lowering instead.
		exit sub
	end if
	var procname = hEmitProcName( s )
	if( hashLookup( @ctx.definedprocs.hash, strptr( procname ) ) <> NULL ) then
		exit sub
	end if
	if( hashLookup( @ctx.declaredprocs.hash, strptr( procname ) ) <> NULL ) then
		exit sub
	end if
	strsetAdd( @ctx.declaredprocs, procname, 0 )

	var oldsection = ctx.section
	ctx.section = SECTION_HEAD
	hWriteLine( "declare " + hEmitProcHeader( s, TRUE, FALSE ) )
	ctx.section = oldsection
end sub

private function hUDTHasInteger128Value( byval s as FBSYMBOL ptr ) as integer
	var fld = symbUdtGetFirstField( s )
	while( fld )
		if( symbIsDynamic( fld ) = FALSE ) then
			var dtype = symbGetType( fld )
			if( typeIsInteger128Scalar( dtype ) ) then return TRUE
			if( (typeGetPtrCnt( dtype ) = 0) andalso _
			    (typeGetDtOnly( dtype ) = FB_DATATYPE_STRUCT) andalso _
			    (symbGetSubtype( fld ) <> NULL) andalso _
			    hUDTHasInteger128Value( symbGetSubtype( fld ) ) ) then
				return TRUE
			end if
		end if
		fld = symbUdtGetNextField( fld )
	wend
	return FALSE
end function

private function hUDTUsesPackedLayout( byval s as FBSYMBOL ptr ) as integer
	'' Some i386 LLVM targets give i128 less than FreeBASIC's 16-byte alignment.
	'' Preserve the compiler-computed member offsets instead of relying on that
	'' target data layout for aggregates that contain 128-bit values.
	return ((s->udt.align > 0) and (s->udt.align < 8)) or _
	       (hUsesI386Integer128ABI( ) andalso hUDTHasInteger128Value( s ))
end function

private function hUDTHasExplicitPadding( byval s as FBSYMBOL ptr ) as integer
	return hUDTUsesPackedLayout( s ) or _
		symbGetUDTIsUnion( s ) or symbGetUDTHasAnonUnion( s )
end function

private function hInteger128StorageAlignment _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr _
	) as integer

	if( typeIsInteger128Scalar( dtype ) ) then return LLVM_INT128_STORAGE_ALIGNMENT
	if( hUsesI386Integer128ABI( ) andalso _
	    (typeGetPtrCnt( dtype ) = 0) andalso _
	    (typeGetDtOnly( dtype ) = FB_DATATYPE_STRUCT) andalso _
	    (subtype <> NULL) andalso hUDTHasInteger128Value( subtype ) ) then
		return typeCalcNaturalAlign( dtype, subtype )
	end if
	return 0
end function

private function hUDTAlignmentType( byval s as FBSYMBOL ptr ) as string
	if( (s->udt.align > 0) and (s->udt.align < 8) ) then return ""
	if( symbGetUDTIsUnion( s ) or symbGetUDTHasAnonUnion( s ) ) then
		'' Only the first union member occupies LLVM storage. An empty
		'' array preserves the alignment required by the other members.
		return "[0 x i" & (s->udt.natalign * 8) & "]"
	end if
	return ""
end function

private sub hEmitStruct( byval s as FBSYMBOL ptr )
	dim as FBSYMBOL ptr fld = any

	''
	'' Already emitting this UDT currently? This means there is a circular
	'' dependency between this UDT and one (or multiple) other UDT(s).
	'' Note: LLVM IR doesn't seem to require explicit declaration of
	'' forward references, clang for example generates code like:
	''
	''    %struct.T = type { %struct.T* }
	''    %struct.XX = type { %struct.YY* }
	''    %struct.YY = type { %struct.XX }
	''
	'' On top of that, it seems to be possible to forward reference
	'' structures even directly and not by pointer:
	''
	''    %struct.XX = type { %struct.T }
	''    %struct.T = type { %struct.T* }
	''
	'' ... as long as the type will be fully declared before its first use
	'' in a function/variable declaration etc. This makes UDT emitting
	'' pretty easy compared to the C backend.
	''
	if( symbGetIsBeingEmitted( s ) ) then
		return
	end if

	hAssignUDTName( s )
	symbSetIsBeingEmitted( s )

	'' Check every field for non-emitted subtypes
	fld = symbUdtGetFirstField( s )
	while( fld )
		hEmitUDT( symbGetSubtype( fld ) )
		fld = symbUdtGetNextField( fld )
	wend

	'' Was it emitted in the mean time? (maybe one of the fields did that)
	if( symbGetIsEmitted( s ) ) then
		return
	end if

	'' We'll emit it now.
	symbSetIsEmitted( s )

	dim as string ln

	'' UDT name
	if( symbGetName( s ) ) then
		ln += hGetUDTName( s )
	else
		ln += "%" + *symbUniqueId( )
	end if

	'' LLVM's natural field alignment can differ from the FBC layout, including
	'' on i386 i128 fields. Use explicit gaps where needed to preserve offsets.
	var packed = hUDTUsesPackedLayout( s )
	var has_padding = hUDTHasExplicitPadding( s )

	ln += " = type "
	if( packed ) then ln += "<"
	ln += "{ "

	'' Write out the elements
	dim as DZSTRING fields
	DZstrZero( fields )
	var fieldcount = 0
	var struct_offset = 0LL
	fld = symbUdtGetFirstField( s )
	while( fld )

		'' Don't emit fake dynamic array fields
		if( symbIsDynamic( fld ) = FALSE ) then
			if( has_padding andalso fld->ofs > struct_offset ) then
				if( fieldcount > 0 ) then DZstrConcatAssign( fields, ", " )
				DZstrConcatAssign( fields, "[" & (fld->ofs - struct_offset) & " x i8]" )
				fieldcount += 1
			end if
			if( fieldcount > 0 ) then
				DZstrConcatAssign( fields, ", " )
			end if
			DZstrConcatAssign( fields, hEmitSymType( fld ) )
			fieldcount += 1
			struct_offset = fld->ofs + symbGetRealSize( fld )
		end if

		'' Union aliases share storage; their first initializable member
		'' describes that storage, with any remaining bytes added as padding.
		fld = symbUdtGetNextInitableField( fld )
	wend
	if( has_padding andalso s->lgt > struct_offset ) then
		if( fieldcount > 0 ) then DZstrConcatAssign( fields, ", " )
		DZstrConcatAssign( fields, "[" & (s->lgt - struct_offset) & " x i8]" )
		fieldcount += 1
	end if
	var alignmenttype = hUDTAlignmentType( s )
	if( len( alignmenttype ) > 0 ) then
		if( fieldcount > 0 ) then DZstrConcatAssign( fields, ", " )
		DZstrConcatAssign( fields, alignmenttype )
	end if

	if( fields.data <> NULL ) then
		ln += *fields.data
	end if
	DZstrAllocate( fields, 0 )

	'' Close UDT body
	ln += " }"
	if( packed ) then ln += ">"

	hWriteLine( ln )

	symbResetIsBeingEmitted( s )
end sub

private sub hEmitCtorDtorArrayElement _
	( _
		byval proc as FBSYMBOL ptr, _
		byref s as string, _
		byref kind as const string _
	)
	dim as integer priority = symbGetProcPriority( proc )

	if( len( s ) > 0 ) then
		s += ", "
	end if
	dim as integer semantic_start = len(s)

	'' LLVM's ctor/dtor entries include an optional associated data pointer.
	'' Keep it null for FreeBASIC module initialization procedures.
	'' FreeBASIC uses zero for the default priority, while LLVM treats zero
	'' as the earliest priority. The default must run after rtlib startup.
	if( priority = 0 ) then
		priority = 65535
	end if
	s += "{ i32, void ()*, i8* } { i32 "
	s += str( priority )
	s += ", void ()* "
	s += *symbGetMangledName( proc )
	s += ", i8* null"
	s += " }"
	fbSemanticModelStartupEmission(proc, kind, "llvm", "array-entry", str(priority), _
		iif(kind = "constructor", "llvm.global_ctors", "llvm.global_dtors"), mid(s, semantic_start + 1))

end sub

private sub hAddGlobalCtorDtor( byval proc as FBSYMBOL ptr )
	if( symbGetIsFuncPtr( proc ) ) then
		exit sub
	end if

	if( symbGetIsGlobalCtor( proc ) ) then
		ctx.ctorcount += 1
		hEmitCtorDtorArrayElement( proc, ctx.ctors, "constructor" )
	elseif( symbGetIsGlobalDtor( proc ) ) then
		ctx.dtorcount += 1
		hEmitCtorDtorArrayElement( proc, ctx.dtors, "destructor" )
	end if
end sub

private function _emitBegin( ) as integer
	if( hFileExists( env.outf.name ) ) then
		if( kill( env.outf.name ) <> 0 ) then
			return FALSE
		end if
	end if

	env.outf.num = freefile
	if( open( env.outf.name, for binary, access read write, as #env.outf.num ) <> 0 ) then
		return FALSE
	end if

	ctx.indent = 0
	ctx.ctors = ""
	ctx.dtors = ""
	ctx.ctorcount = 0
	ctx.dtorcount = 0
	ctx.head_txt.bufferlen = 0
	ctx.body_txt.bufferlen = 0
	ctx.foot_txt.bufferlen = 0
	ctx.proc_allocas.bufferlen = 0
	ctx.current_proc = NULL
	ctx.inproc = FALSE
	ctx.linenum = 0
	ctx.section = SECTION_HEAD
	hWriteLine( "target triple = """ + hLLVMTargetTriple( ) + """" )
	strsetInit( @ctx.definedprocs, 64 )
	strsetInit( @ctx.declaredprocs, 64 )
	strsetInit( @ctx.udtnames, 64 )
	ctx.udtnamecount = 0
	strsetInit( @ctx.localnames, 64 )
	strsetInit( @ctx.addressedlabels, 16 )
	ctx.localnamecount = 0

	for i as integer = 0 to BUILTIN__COUNT-1
		builtins(i).used = FALSE
	next

	if( env.clopt.debuginfo ) then
		_emitDBG( AST_OP_DBG_LINEINI, NULL, 0 )
	end if

	if( fbIs64bit( ) ) then
		hWriteLine( "%FBSTRING = type { i8*, i64, i64 }" )
	else
		hWriteLine( "%FBSTRING = type { i8*, i32, i32 }" )
	end if

	ctx.section = SECTION_BODY

	function = TRUE
end function

private sub _emitEnd( )
	dim as integer output_error = FALSE

	'' Append global declarations to the header section.
	'' This must be done during _emitEnd() instead of _emitBegin() because
	'' _emitBegin() is called even before any input code is parsed.
	ctx.section = SECTION_HEAD

	for i as integer = 0 to BUILTIN__COUNT-1
		if( builtins(i).used ) then
			dim as string builtin_decl = *builtins(i).decl
			hWriteLine( builtin_decl )
		end if
	next

	'' Emit proc decls first (because of function pointer initializers referencing procs)
	hWriteLine( "" )
	symbForEachGlobal( FB_SYMBCLASS_PROC, @hMaybeEmitProcProto )

	'' Then the variables
	hWriteLine( "" )
	symbForEachGlobal( FB_SYMBCLASS_VAR, @hMaybeEmitGlobalVar )

	'' DATA array initializers can reference globals by taking their address,
	'' so they must be emitted after the other global declarations.
	irForEachDataStmt( @hEmitVariable )

	'' Global arrays for the ctors/dtors actually emitted in this module.
	if( ctx.ctorcount > 0 ) then
		hWriteLine( "@llvm.global_ctors = appending global [" & ctx.ctorcount & " x { i32, void ()*, i8* }] [" + ctx.ctors + "]" )
	end if
	if( ctx.dtorcount > 0 ) then
		hWriteLine( "@llvm.global_dtors = appending global [" & ctx.dtorcount & " x { i32, void ()*, i8* }] [" + ctx.dtors + "]" )
	end if

	ctx.section = SECTION_FOOT

	' flush all sections to file
	if( ctx.head_txt.bufferlen > 0 ) then
		if( put( #env.outf.num, , *ctx.head_txt.buffer, ctx.head_txt.bufferlen ) <> 0 ) then
			output_error = TRUE
		end if
	end if
	if( ctx.body_txt.bufferlen > 0 ) then
		if( put( #env.outf.num, , *ctx.body_txt.buffer, ctx.body_txt.bufferlen ) <> 0 ) then
			output_error = TRUE
		end if
	end if
	if( ctx.foot_txt.bufferlen > 0 ) then
		if( put( #env.outf.num, , *ctx.foot_txt.buffer, ctx.foot_txt.bufferlen ) <> 0 ) then
			output_error = TRUE
		end if
	end if

	if( close( #env.outf.num ) <> 0 ) then
		output_error = TRUE
	end if
	strsetEnd( @ctx.definedprocs )
	strsetEnd( @ctx.declaredprocs )
	strsetEnd( @ctx.udtnames )
	strsetEnd( @ctx.localnames )
	strsetEnd( @ctx.addressedlabels )

	env.outf.num = 0
	if( output_error ) then
		errReportEx( FB_ERRMSG_FILEACCESSERROR, env.outf.name, -1 )
	end if
end sub

private function _getOptionValue( byval opt as IR_OPTIONVALUE ) as integer
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

	'' These aren't available as llvm.*() built-ins:
	select case as const( op )
	case AST_OP_SGN, AST_OP_FIX, AST_OP_FRAC, _
	     AST_OP_ASIN, AST_OP_ACOS, AST_OP_TAN, AST_OP_ATAN, _
	     AST_OP_RSQRT, AST_OP_RCP
		function = FALSE

	case AST_OP_ABS
		'' There is @llvm.fabs.* for floats, but no @llvm.abs.* for integers
		function = (typeGetClass( dtype ) = FB_DATACLASS_FPOINT)

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

private sub _procAllocArg( byval proc as FBSYMBOL ptr, byval sym as FBSYMBOL ptr )
	dim as string ln

	hAstCommand( "paramvar " + hSymName( sym ) )

	''
	'' Load the parameter values into local stack vars, to support taking
	'' the address of the parameters on stack.
	''
	'' This means there are two symbols per parameter:
	''    - the parameter value in the procedure header
	''    - the alloca operation representing the stack var
	'' they must use different names to avoid collision.
	''

	dim dtype as integer
	dim subtype as FBSYMBOL ptr
	symbGetRealType( sym, dtype, subtype )
	if( symbIsParamVarByval( sym ) andalso _
	    (symbGetValistType( dtype, subtype ) = FB_CVA_LIST_BUILTIN_C_STD) ) then
		dtype = typeAddrOf( dtype )
	end if
	if( symbIsParamVarByval( sym ) andalso _
	    hUsesCLongDoubleCABI( proc ) andalso _
	    hIsCLongDoubleType( dtype, subtype ) ) then
		var native_type = hGetCLongDoubleLLVMType( subtype )
		if( native_type = "" ) then
			errReport( FB_ERRMSG_INVALIDDATATYPES, , _
			           ": CLongDouble procedure parameters are not supported by the LLVM backend for this target" )
			exit sub
		end if
		var aggregate_type = hEmitType( dtype, subtype )
		ln = hEmitVarName( sym ) + " = alloca " + aggregate_type
		hWriteLine( ln )
		hWriteLine( "store " + aggregate_type + " zeroinitializer, " + _
		            aggregate_type + "* " + hEmitVarName( sym ) + ", align 1" )
		var native_pointer = irhlAllocVreg( typeAddrOf( FB_DATATYPE_BYTE ), NULL )
		hWriteLine( hVregToStr( native_pointer ) + " = bitcast " + aggregate_type + _
		            "* " + hEmitVarName( sym ) + " to " + native_type + "*" )
		hWriteLine( "store " + native_type + " " + hEmitParamName( sym ) + ", " + _
		            native_type + "* " + hVregToStr( native_pointer ) + ", align 1" )
		exit sub
	end if
	dim as integer split_i386_int128 = _
		symbIsParamVarByval( sym ) andalso _
		hIsI386Integer128Param( dtype, FB_PARAMMODE_BYVAL )

	'' %myparam = alloca type
	ln = hEmitVarName( sym ) + " = alloca "
	ln += hEmitType( dtype, subtype )
	var storage_alignment = hInteger128StorageAlignment( dtype, subtype )
	if( storage_alignment > 0 ) then ln += ", align " + str( storage_alignment )
	hWriteLine( ln )

	'' store type %myparam$, type* %myparam
	ln = "store "
	if( split_i386_int128 ) then
		dim as string low_name = hEmitParamName( sym ) + ".low"
		dim as string high_name = hEmitParamName( sym ) + ".high"
		dim as string incoming = hJoinI386Integer128Words( low_name, high_name )
		ln += hEmitType( dtype, subtype ) + " " + incoming
	elseif( symbIsParamVarByval( sym ) andalso _
	    hIsMemoryByval( dtype, subtype, FB_PARAMMODE_BYVAL ) ) then
		var incoming = irhlAllocVreg( dtype, subtype )
		hWriteLine( hVregToStr(incoming) + " = load " + hEmitType(dtype, subtype) + _
		    ", " + hEmitType(typeAddrOf(dtype), subtype) + " " + hEmitParamName(sym) )
		ln += hEmitType( dtype, subtype ) + " " + hVregToStr( incoming )
	else
		ln += hEmitType( dtype, subtype ) + " " + hEmitParamName( sym )
	end if
	ln += ", "
	ln += hEmitType( typeAddrOf( dtype ), subtype ) + " " + hEmitVarName( sym )
	hWriteLine( ln )
end sub

private sub _procAllocLocal( byval proc as FBSYMBOL ptr, byval sym as FBSYMBOL ptr )
	hAstCommand( "localvar " + hSymName( sym ) )
	hEmitVariable( sym )
end sub

private sub _scopeBegin( byval s as FBSYMBOL ptr )
end sub

private sub _scopeEnd( byval s as FBSYMBOL ptr )
end sub

private sub _procAllocStaticVars( byval head_sym as FBSYMBOL ptr )
	var oldsection = ctx.section
	ctx.section = SECTION_HEAD

	while( head_sym )
		select case as const( symbGetClass( head_sym ) )
		case FB_SYMBCLASS_SCOPE
			_procAllocStaticVars( symbGetScopeSymbTbHead( head_sym ) )
		case FB_SYMBCLASS_VAR
			if( symbIsStatic( head_sym ) ) then
				hEmitVariable( head_sym )
			end if
		end select
		head_sym = symbGetNext( head_sym )
	wend

	ctx.section = oldsection
end sub

private sub _setVregDataType _
	( _
		byval v as IRVREG ptr, _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr _
	)

	dim as IRVREG ptr temp0 = any

	if( (v->dtype <> dtype) or (v->subtype <> subtype) ) then
		if( (v->typ = IR_VREGTYPE_REG) andalso (v->sym <> NULL) andalso _
		    (symbIsVar( v->sym ) or symbIsField( v->sym )) andalso _
		    symbIsTemp( v->sym ) ) then
			dim symdtype as integer
			dim symsubtype as FBSYMBOL ptr
			symbGetRealType( v->sym, symdtype, symsubtype )
			if( typeIsPtr( symdtype ) andalso _
			    (v->dtype = typeAddrOf( symdtype )) andalso _
			    (v->subtype = symsubtype) andalso _
			    (hEmitType( dtype, subtype ) = hEmitType( symdtype, symsubtype )) ) then
				'' Symbol backed registers can name a pointer variable's
				'' alloca. Convert its stored value, not the alloca address.
				var loaded = irhlAllocVreg( symdtype, symsubtype )
				hWriteLine( hVregToStr( loaded ) + " = load " + _
				    hEmitType( symdtype, symsubtype ) + ", " + _
				    hEmitType( typeAddrOf( symdtype ), symsubtype ) + " " + _
				    hVregToStr( v ) )
				*v = *loaded
			end if
		end if
		if( (v->typ = IR_VREGTYPE_VAR) andalso _
		    (typeGetClass( v->dtype ) = FB_DATACLASS_INTEGER) andalso _
		    (typeGetClass( dtype ) = FB_DATACLASS_INTEGER) andalso _
		    (typeGetSize( v->dtype ) = typeGetSize( dtype )) ) then
			'' An equal width cast of an assignable variable keeps its
			'' address. Loading it here would lose the assignment target.
			v->dtype = dtype
			v->subtype = subtype
			exit sub
		end if
		temp0 = irhlAllocVreg( dtype, subtype )
		hEmitConvert( temp0, v )
		*v = *temp0
	end if

end sub

private sub hAddAddressComponents _
	( _
		byval v as IRVREG ptr, _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval ofs as longint, _
		byval mult as integer, _
		byval vidx as IRVREG ptr _
	)

	'' voffset = ptrtoint l
	var voffset = irhlAllocVreg( FB_DATATYPE_INTEGER, NULL )
	hEmitConvert( voffset, v )

	if( (vidx <> NULL) andalso (mult <> 0) ) then
		hLoadVreg( vidx )
		_setVregDataType( vidx, FB_DATATYPE_INTEGER, NULL )

		if( mult <> 1 ) then
			var vscale = irhlAllocVrImm( FB_DATATYPE_INTEGER, NULL, mult )
			var vscaled = irhlAllocVreg( FB_DATATYPE_INTEGER, NULL )
			hEmitBop( AST_OP_MUL, vidx, vscale, vscaled, NULL, IR_EMITOPT_NONE )
			vidx = vscaled
		end if

		var vindexed = irhlAllocVreg( FB_DATATYPE_INTEGER, NULL )
		hEmitBop( AST_OP_ADD, voffset, vidx, vindexed, NULL, IR_EMITOPT_NONE )
		voffset = vindexed
	end if

	if( ofs <> 0 ) then
		'' voffset += <offset>
		'' (implemented as normal BOP, not self-BOP, because we
		'' can't do self-BOPs on REGs)
		var vimmoffset = irhlAllocVrImm( FB_DATATYPE_INTEGER, NULL, ofs )
		var vnewoffset = irhlAllocVreg( FB_DATATYPE_INTEGER, NULL )
		hEmitBop( AST_OP_ADD, voffset, vimmoffset, vnewoffset, NULL, IR_EMITOPT_NONE )
		voffset = vnewoffset
	end if

	'' voffset = inttoptr voffset
	_setVregDataType( voffset, dtype, subtype )

	*v = *voffset
end sub

private sub hPrepareAddress( byval v as IRVREG ptr )
	assert( (v->typ = IR_VREGTYPE_VAR) or _
		(v->typ = IR_VREGTYPE_OFS) or _
		(v->typ = IR_VREGTYPE_IDX) or _
		(v->typ = IR_VREGTYPE_PTR) )

	'' VAR - local var access
	'' OFS - global symbol access
	'' IDX - local array indexing
	'' PTR - derefs
	''
	'' In LLVM, references to local/global vars are pointers (addresses)
	'' implicitly, so we turn such vregs into pointers (without having to do
	'' addrof operations), for use with LLVM's load/store operations, which
	'' take addresses, not the memory itself.
	''
	'' If there is an offset or index, it must be added on top of the
	'' base address.

	var addrdtype = v->dtype
	var addrsubtype = v->subtype
	var ofs = v->ofs
	var mult = v->mult
	var vidx = v->vidx
	var sym = v->sym

	'' OFS vregs already have pointer type, but others don't
	if( v->typ = IR_VREGTYPE_OFS ) then
		assert( typeIsPtr( addrdtype ) )
	else
		addrdtype = typeAddrOf( addrdtype )
	end if

	if( sym = NULL ) then
		if( vidx ) then
			'' A dynamic array's data address can still be in its
			'' descriptor variable here. Read the value before using it
			'' as the indexed element's base address.
			if( irIsREG( vidx ) = FALSE ) then
				hLoadVreg( vidx )
			end if
			'' For PTR and symbol-less IDX vregs, vidx already holds the
			'' complete base address. It is not an offset to add again.
			*v = *vidx
			_setVregDataType( v, addrdtype, addrsubtype )
			vidx = NULL
		else
			'' A vreg without a symbol or index stores an absolute address in ofs,
			'' as in *CPTR( integer ptr, 0 ). Convert that address to a pointer
			'' value instead of dereferencing a missing base register.
			*v = *irhlAllocVrImm( FB_DATATYPE_INTEGER, NULL, ofs )
			_setVregDataType( v, addrdtype, addrsubtype )
			ofs = 0
		end if
	else
		v->typ = IR_VREGTYPE_REG
		v->reg = INVALID
		''v->sym = NULL  '' leaving this for use by hVregToStr()
		v->ofs = 0
		v->vidx = NULL

		if( symbIsVar( sym ) or symbIsField( sym ) ) then
			symbGetRealType( sym, v->dtype, v->subtype )

			'' vreg is the address of the memory allocated for the sym
			v->dtype = typeAddrOf( v->dtype )

			'' May need to cast from symbol's type to vreg's type (e.g. for field accesses)
			_setVregDataType( v, addrdtype, addrsubtype )
		else
			'' Procedure and label symbols name addresses directly. They
			'' have no variable-storage layout for symbGetRealType() to read.
			v->dtype = addrdtype
			v->subtype = addrsubtype
		end if
	end if

	if( (vidx <> NULL) or (ofs <> 0) ) then
		hAddAddressComponents( v, addrdtype, addrsubtype, ofs, mult, vidx )
	end if
end sub

private sub hLoadVreg( byval v as IRVREG ptr )
	'' LLVM instructions take registers or immediates (including offsets,
	'' i.e. addresses of globals/procedures),
	'' anything else must be loaded into a register first.
	'' (register in LLVM just means a <%N = insn ...> temporary value)

	select case( v->typ )
	case IR_VREGTYPE_REG, IR_VREGTYPE_IMM
		'' Ok as-is, no loading needed
		exit select

	case IR_VREGTYPE_OFS
		'' global symbol address, no loading needed
		'' (not even an explicit addrof is needed, since symbol
		'' references are pointers implicitly)
		''
		'' with offset:
		''    %0 = ptrtoint foo* @global to i32
		''    %1 = add i32 %0, i32 <offset>
		''    %2 = inttoptr i32 %1 to foo*
		''
		'' without offset:
		'' (no "loading" necessary, handled purely in hVregToStr())
		''    @global
		hPrepareAddress( v )

	case else
		'' memory accesses: stack/global vars, arrays, ptr derefs
		'' Get the address and then load the value stored there.
		hPrepareAddress( v )
		assert( typeIsPtr( v->dtype ) )
		var temp0 = irhlAllocVreg( typeDeref( v->dtype ), v->subtype )
		var s = hVregToStr( temp0 ) + " = load "
		s += hEmitType( typeDeref( v->dtype ), v->subtype ) + ", "
		s += hEmitType( v->dtype, v->subtype ) + " "
		s += hVregToStr( v )
		hWriteLine( s )
		*v = *temp0
	end select
end sub

private function hEmitType _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr _
	) as string

	dim as string s
	dim as integer ptrcount = any

	ptrcount = typeGetPtrCnt( dtype )
	dtype = typeGetDtOnly( dtype )

	select case as const( dtype )
	case FB_DATATYPE_VOID
		'' "void*" isn't allowed in LLVM IR, "i8*" must be used instead,
		'' that's why FB_DATATYPE_VOID is mapped to "i8" in the above
		'' table. "void" can only be used for subs.
		if( ptrcount = 0 ) then
			s = "void"
		else
			s = *dtypeName(FB_DATATYPE_VOID)
		end if

	case FB_DATATYPE_STRUCT, FB_DATATYPE_ENUM
		if( subtype ) then
			hEmitUDT( subtype )
			s = hGetUdtName( subtype )
		elseif( dtype = FB_DATATYPE_ENUM ) then
			s = *dtypeName(typeGetRemapType( dtype ))
		else
			s = *dtypeName(FB_DATATYPE_VOID)
		end if

	case FB_DATATYPE_VA_LIST
		'' The va_list mangle type names the underlying ABI struct here.
		'' LLVM needs the emitted struct type, not the C typedef name.
		if( subtype ) then
			hEmitUDT( subtype )
			s = hGetUdtName( subtype )
		else
			s = *dtypeName(FB_DATATYPE_VOID)
		end if

	case FB_DATATYPE_FUNCTION
		assert( ptrcount > 0 )
		ptrcount -= 1
		s = hEmitProcHeader( subtype, TRUE, TRUE ) + "*"

	case FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR
		'' Emit ubyte instead of char,
		'' and ubyte/ushort/uinteger instead of wchar_t
		s = *dtypeName(typeGetRemapType( dtype ))

	case FB_DATATYPE_FIXSTR
		'' Ditto (but typeGetRemapType() returns FB_DATATYPE_FIXSTR,
		'' so do it manually)
		s = *dtypeName(FB_DATATYPE_UBYTE)

	case else
		s = *dtypeName(dtype)
	end select

	if( ptrcount > 0 ) then
		s += string( ptrcount, "*" )
	end if

	function = s
end function

private function hEmitPointerConstant _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval value as longint _
	) as string

	assert( typeIsPtr( dtype ) )
	if( value = 0 ) then
		return "null"
	end if

	return "inttoptr (" + hEmitType( FB_DATATYPE_UINT, NULL ) + " " + str( value ) + _
		" to " + hEmitType( dtype, subtype ) + ")"
end function

private function hEmitInt _
	( _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval value as integer _
	) as string

	dim as string s
	if( typeIsPtr( dtype ) ) then
		return hEmitPointerConstant( dtype, subtype, value )
	end if
	if( typeGetDtAndPtrOnly( dtype ) = FB_DATATYPE_BOOLEAN ) then
		'' Boolean constants use the same 0/1 storage as C's _Bool.
		return iif( value, "1", "0" )
	end if

	select case( dtype )
	case FB_DATATYPE_INTEGER, FB_DATATYPE_UINT, _
	     FB_DATATYPE_LONG, FB_DATATYPE_ULONG, _
	     FB_DATATYPE_ENUM, FB_DATATYPE_INTEGER128, FB_DATATYPE_UINTEGER128
		'' It seems like llc doesn't care whether we emit -1 or
		'' 4294967295, it's the bit pattern that matters.
		s = str( value )

	case else
		'' Cast the i32 constant to a narrower integer type.
		'' <castop> (i32 <n> to <type>)
		s = "trunc "
		s += "("
		s += hEmitType( FB_DATATYPE_INTEGER, NULL ) + " " + str( value )
		s += " to " + hEmitType( dtype, subtype )
		s += ")"
	end select

	function = s
end function

private function hEmitLong( byval value as longint ) as string
	function = str( value )
end function

private function hEmitFloat( byval value as double, byval dtype as integer ) as string
	'' Single/double float constants can be emitted as decimals or
	'' as raw bytes in 0x hex notation with 16 digits (even singles must
	'' be emitted as doubles, i.e. 16 hex digits, according to the LangRef).
	'' We always use the raw hex form, that avoids any rounding issues
	'' or errors with the decimals...
	if( typeGetSize( dtype ) = 4 ) then
		'' LLVM accepts a hex float operand only when the double bit pattern
		'' represents the exact single-precision value it will store.
		value = csng( value )
	end if
	function = "0x" + hex( *cptr( ulongint ptr, @value ), 16 )
end function

private function hIsFixLenStr( byval sym as FBSYMBOL ptr ) as integer
	if( symbIsVar( sym ) ) then
		select case( symbGetType( sym ) )
		case FB_DATATYPE_FIXSTR, FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR
			return TRUE
		end select
	end if
	return FALSE
end function

private function hVregToStr( byval v as IRVREG ptr ) as string
	select case as const( v->typ )
	case IR_VREGTYPE_IMM
		if( typeGetClass( v->dtype ) = FB_DATACLASS_FPOINT ) then
			return hEmitFloat( v->value.f, v->dtype )
		end if
		if( typeIsPtr( v->dtype ) ) then
			return hEmitPointerConstant( v->dtype, v->subtype, v->value.i )
		end if
		if( typeGetSize( v->dtype ) = 8 ) then
			return hEmitLong( v->value.i )
		end if
		return hEmitInt( v->dtype, v->subtype, v->value.i )

	case IR_VREGTYPE_REG
		'' Normally REGs will have their reg field set, but if
		'' hPrepareAddress() left the sym intact, we're supposed to use
		'' that instead.
		if( v->sym = NULL ) then
			return "%vr" + str( v->reg )
		end if
	end select

	var sym = v->sym
	if( (sym <> NULL) andalso symbIsLabel( sym ) ) then
		'' BASIC error handlers and RESUME use addresses of labels in the
		'' current procedure. LLVM represents those with blockaddress().
		assert( ctx.current_proc <> NULL )
		var labelname = *symbGetMangledName( sym )
		var key = *symbGetMangledName( ctx.current_proc ) + ":" + labelname
		if( hashLookup( @ctx.addressedlabels.hash, strptr( key ) ) = NULL ) then
			strsetAdd( @ctx.addressedlabels, key, 0 )
			if( len( ctx.proc_labels ) > 0 ) then ctx.proc_labels += ", "
			ctx.proc_labels += "label %" + labelname
		end if
		return "blockaddress(" + *symbGetMangledName( ctx.current_proc ) + _
		    ", %" + labelname + ")"
	end if

	'' If accessing global fixed-length strings (including string literals)
	'' we have to add an inline type cast, because for example the symbol
	'' reference will be "[10 x i8]*", but we want to have "i8*" instead.
	if( hIsFixLenStr( sym ) ) then
		var s = "bitcast ("
		s += hEmitSymType( sym ) + "* "
		s += hEmitVarName( sym )
		s += " to "
		s += hEmitType( typeAddrOf( symbGetType( sym ) ), NULL )
		s += ")"
		if( symbIsLocal( sym ) andalso (not symbIsStatic( sym )) ) then
			'' A constant-expression bitcast cannot use a function-local alloca.
			var casted = irhlAllocVreg( typeAddrOf( symbGetType( sym ) ), NULL )
			hWriteLine( hVregToStr( casted ) + " = bitcast " + _
				hEmitSymType( sym ) + "* " + hEmitVarName( sym ) + _
				" to " + hEmitType( casted->dtype, casted->subtype ) )
			return hVregToStr( casted )
		end if
		return s
	end if

	return hEmitVarName( sym )
end function

private sub _emitLabel( byval label as FBSYMBOL ptr )
	hAstCommand( "label " + hSymName( label ) )

	'' end current basic block
	hWriteLine( "br label %" + *symbGetMangledName( label ) )

	'' and start the next one
	hWriteLabel( symbGetMangledName( label ) )
end sub

private function hGetBopCode _
	( _
		byval op as integer, _
		byval dtype as integer _
	) as const zstring ptr

	select case as const( op )
	case AST_OP_ADD
		if( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
			function = @"fadd"
		else
			function = @"add"
		end if
	case AST_OP_SUB
		if( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
			function = @"fsub"
		else
			function = @"sub"
		end if
	case AST_OP_MUL
		if( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
			function = @"fmul"
		else
			function = @"mul"
		end if
	case AST_OP_DIV
		function = @"fdiv"
	case AST_OP_INTDIV
		function = iif( typeIsSigned( dtype ), cptr(const zstring ptr, @"sdiv"), cptr(const zstring ptr, @"udiv") )
	case AST_OP_MOD
		if( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
			function = @"frem"
		else
			function = iif( typeIsSigned( dtype ), cptr(const zstring ptr, @"srem"), cptr(const zstring ptr, @"urem") )
		end if
	case AST_OP_SHL
		function = @"shl"
	case AST_OP_SHR
		function = iif( typeIsSigned( dtype ), cptr(const zstring ptr, @"ashr"), cptr(const zstring ptr, @"lshr") )
	case AST_OP_AND
		function = @"and"
	case AST_OP_OR
		function = @"or"
	case AST_OP_XOR
		function = @"xor"
	case AST_OP_EQ
		if( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
			function = @"fcmp oeq"
		else
			function = @"icmp eq"
		end if
	case AST_OP_NE
		if( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
			'' FB follows IEEE comparison rules: a NaN is not equal to every
			'' value, including itself. LLVM's unordered-not-equal predicate
			'' preserves that behaviour.
			function = @"fcmp une"
		else
			function = @"icmp ne"
		end if
	case AST_OP_GT
		if( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
			function = @"fcmp ogt"
		else
			function = iif( typeIsSigned( dtype ), cptr(const zstring ptr, @"icmp sgt"), cptr(const zstring ptr, @"icmp ugt") )
		end if
	case AST_OP_LT
		if( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
			function = @"fcmp olt"
		else
			function = iif( typeIsSigned( dtype ), cptr(const zstring ptr, @"icmp slt"), cptr(const zstring ptr, @"icmp ult") )
		end if
	case AST_OP_GE
		if( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
			function = @"fcmp oge"
		else
			function = iif( typeIsSigned( dtype ), cptr(const zstring ptr, @"icmp sge"), cptr(const zstring ptr, @"icmp uge") )
		end if
	case AST_OP_LE
		if( typeGetClass( dtype ) = FB_DATACLASS_FPOINT ) then
			function = @"fcmp ole"
		else
			function = iif( typeIsSigned( dtype ), cptr(const zstring ptr, @"icmp sle"), cptr(const zstring ptr, @"icmp ule") )
		end if
	case AST_OP_EQV, AST_OP_IMP
		'' hEmitBop() lowers these operations to XOR/OR instructions before
		'' reaching this opcode lookup.
		errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
		function = @"xor"

	end select

end function

private sub hLoadOperandsAndWriteBop _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval v2 as IRVREG ptr, _
		byval vr as IRVREG ptr, _
		byval dtype as integer, _
		byval subtype as FBSYMBOL ptr, _
		byval options as IR_EMITOPT _
	)

	hLoadVreg( v1 )
	hLoadVreg( v2 )
	_setVregDataType( v1, dtype, subtype )
	_setVregDataType( v2, dtype, subtype )

	'' Inverting an ordered floating comparison by swapping its predicate
	'' loses the unordered (NaN) case. Invert the i1 result instead.
	var invert_float = ((options and IR_EMITOPT_REL_DOINVERSE) <> 0) andalso _
	                   (typeGetClass( dtype ) = FB_DATACLASS_FPOINT)
	if( ((options and IR_EMITOPT_REL_DOINVERSE) <> 0) andalso (invert_float = FALSE) ) then
		op = astGetInverseLogOp( op )
	end if

	var resultname = hVregToStr( vr )
	if( invert_float ) then
		var comparison = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
		resultname = hVregToStr( comparison )
	end if
	var ln = resultname
	ln += " = "
	ln += *hGetBopCode( op, dtype )
	ln += " "
	ln += hEmitType( dtype, subtype )
	ln += " "
	ln += hVregToStr( v1 )
	ln += ", "
	ln += hVregToStr( v2 )
	hWriteLine( ln )
	if( invert_float ) then
		hWriteLine( hVregToStr( vr ) + " = xor i1 " + resultname + ", true" )
	end if

end sub

private sub hEmitBop _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval v2 as IRVREG ptr, _
		byval vr as IRVREG ptr, _
		byval label as FBSYMBOL ptr, _
		byval options as IR_EMITOPT _
	)

	'' Conditional branch?
	if( label ) then
		assert( vr = NULL )
		vr = irhlAllocVreg( FB_DATATYPE_INTEGER, NULL )

		'' condition = comparison expression
		hLoadOperandsAndWriteBop( op, v1, v2, vr, v1->dtype, v1->subtype, options )

		'' The conditional branch in LLVM always needs both
		'' true and false labels, to keep the proper basic
		'' block semantics up.
		'' true label = the label given through the BOP,
		'' false label = the code right behind the branch

		'' branch condition, truelabel, falselabel
		var falselabel = *symbUniqueLabel( )
		var ln = "br i1 " + hVregToStr( vr )
		ln += ", "
		ln += "label %" + *symbGetMangledName( label )
		ln += ", "
		ln += "label %" + falselabel
		hWriteLine( ln )

		'' falselabel:
		hWriteLabel( falselabel )
		exit sub
	end if

	var isself = FALSE
	dim v1orig as IRVREG

	if( vr = NULL ) then
		'' v1 bop= b2
		''
		'' Self-BOP - have to allocate a vr manually and store that
		'' into v1 later, because LLVM IR doesn't have self-BOPs.
		''
		'' Also, we have to preserve the "storable" version of v1 - as
		'' a BOP operand it may loaded, turning it into a REG,
		'' but for the store we need the original VAR etc.
		isself = TRUE
		vr = irhlAllocVreg( v1->dtype, v1->subtype )
		v1orig = *v1
	end if

	select case as const( op )
	case AST_OP_EQ, AST_OP_NE, AST_OP_GT, AST_OP_LT, AST_OP_GE, AST_OP_LE
		'' A comparison's result is INTEGER, but LLVM must compare values
		'' in the operands' type (especially for SINGLE and DOUBLE).
		hLoadOperandsAndWriteBop( op, v1, v2, vr, v1->dtype, v1->subtype, options )

	case AST_OP_ADD, AST_OP_SUB
		if( typeIsPtr( vr->dtype ) ) then
			'' LLVM integer arithmetic cannot operate on pointers. Work in a
			'' pointer-sized integer, then convert the address back to a pointer.
			hLoadVreg( v1 )
			hLoadVreg( v2 )
			_setVregDataType( v1, FB_DATATYPE_UINT, NULL )
			_setVregDataType( v2, FB_DATATYPE_UINT, NULL )
			var address = irhlAllocVreg( FB_DATATYPE_UINT, NULL )
			hEmitBop( op, v1, v2, address, NULL, options )
			hEmitConvert( vr, address )
		else
			hLoadOperandsAndWriteBop( op, v1, v2, vr, vr->dtype, vr->subtype, options )
		end if

	case AST_OP_AND, AST_OP_OR, AST_OP_XOR
		if( (typeGetDtAndPtrOnly( vr->dtype ) = FB_DATATYPE_BOOLEAN) andalso _
		    (typeGetDtAndPtrOnly( v1->dtype ) <> FB_DATATYPE_BOOLEAN) andalso _
		    (typeGetDtAndPtrOnly( v2->dtype ) <> FB_DATATYPE_BOOLEAN) ) then
			'' Bitfield extraction uses integer masks before its result is
			'' converted to BOOLEAN. Applying the mask to two truth values
			'' would make every nonzero source bit look like bit zero.
			var masked = irhlAllocVreg( v1->dtype, v1->subtype )
			hLoadOperandsAndWriteBop( op, v1, v2, masked, _
			    v1->dtype, v1->subtype, options )
			hEmitConvert( vr, masked )
		else
			hLoadOperandsAndWriteBop( op, v1, v2, vr, vr->dtype, vr->subtype, options )
		end if

	case AST_OP_EQV
		'' LLVM has no EQV instruction. EQV is NOT (v1 XOR v2).
		var vtemp = irhlAllocVreg( vr->dtype, vr->subtype )
		hEmitBop( AST_OP_XOR, v1, v2, vtemp, NULL, IR_EMITOPT_NONE )

		'' Boolean values are stored as 0/1, so invert their low bit. Other
		'' integer types use an all-bits-set mask for the bitwise NOT.
		var inverse = irhlAllocVrImm( FB_DATATYPE_INTEGER, NULL, iif( vr->dtype = FB_DATATYPE_BOOLEAN, 1, -1 ) )
		hEmitBop( AST_OP_XOR, vtemp, inverse, vr, NULL, IR_EMITOPT_NONE )

	case AST_OP_IMP
		'' LLVM has no IMP instruction. IMP is (NOT v1) OR v2.
		var vtemp = irhlAllocVreg( vr->dtype, vr->subtype )
		var inverse = irhlAllocVrImm( FB_DATATYPE_INTEGER, NULL, iif( vr->dtype = FB_DATATYPE_BOOLEAN, 1, -1 ) )
		hEmitBop( AST_OP_XOR, v1, inverse, vtemp, NULL, IR_EMITOPT_NONE )
		hEmitBop( AST_OP_OR, vtemp, v2, vr, NULL, IR_EMITOPT_NONE )

	case else
		hLoadOperandsAndWriteBop( op, v1, v2, vr, vr->dtype, vr->subtype, options )
	end select

	'' LLVM comparison ops return i1. BOOLEAN stores 0/1, while wider
	'' FreeBASIC comparison results use 0/-1.
	if( astOpIsRelational( op ) ) then
		var vtemp = irhlAllocVreg( vr->dtype, vr->subtype )
		var ln = hVregToStr( vtemp )
		if( typeGetDtAndPtrOnly( vr->dtype ) = FB_DATATYPE_BOOLEAN ) then
			ln += " = zext "
		else
			ln += " = sext "
		end if
		ln += "i1 " + hVregToStr( vr )
		ln += " to "
		ln += hEmitType( vr->dtype, vr->subtype )
		hWriteLine( ln )
		*vr = *vtemp
	end if

	'' store self-BOP result
	if( isself ) then
		hEmitStore( @v1orig, vr )
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

	var bopdump = vregPretty( v1 ) + " " + astDumpOpToStr( op ) + " " + vregPretty( v2 )
	if( label ) then
		hAstCommand( "branchbop " + bopdump )
	elseif( vr = NULL ) then
		hAstCommand( "selfbop " + bopdump )
	else
		hAstCommand( "bop " + bopdump )
	end if

	hEmitBop( op, v1, v2, vr, label, options )

end sub

private sub hBuiltInUop _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval vr as IRVREG ptr _
	)

	dim as string ln

	hLoadVreg( v1 )

	ln = hVregToStr( vr ) + " = call "

	if( v1->dtype = FB_DATATYPE_SINGLE ) then
		ln += "float @llvm."
		select case( op )
		case AST_OP_ABS : builtins(BUILTIN_ABSF).used = TRUE : ln += "fabs"
		case AST_OP_SIN : builtins(BUILTIN_SINF).used = TRUE : ln += "sin"
		case AST_OP_COS : builtins(BUILTIN_COSF).used = TRUE : ln += "cos"
		case AST_OP_EXP : builtins(BUILTIN_EXPF).used = TRUE : ln += "exp"
		case AST_OP_LOG : builtins(BUILTIN_LOGF).used = TRUE : ln += "log"
		case AST_OP_SQRT : builtins(BUILTIN_SQRTF).used = TRUE : ln += "sqrt"
		case AST_OP_FLOOR : builtins(BUILTIN_FLOORF).used = TRUE : ln += "floor"
		case else
			errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
			exit sub
		end select
		ln += ".f32(float "
	else
		assert( v1->dtype = FB_DATATYPE_DOUBLE )
		ln += "double @llvm."
		select case( op )
		case AST_OP_ABS : builtins(BUILTIN_ABS).used = TRUE : ln += "fabs"
		case AST_OP_SIN : builtins(BUILTIN_SIN).used = TRUE : ln += "sin"
		case AST_OP_COS : builtins(BUILTIN_COS).used = TRUE : ln += "cos"
		case AST_OP_EXP : builtins(BUILTIN_EXP).used = TRUE : ln += "exp"
		case AST_OP_LOG : builtins(BUILTIN_LOG).used = TRUE : ln += "log"
		case AST_OP_SQRT : builtins(BUILTIN_SQRT).used = TRUE : ln += "sqrt"
		case AST_OP_FLOOR : builtins(BUILTIN_FLOOR).used = TRUE : ln += "floor"
		case else
			errReportEx( FB_ERRMSG_INTERNAL, __FUNCTION__ )
			exit sub
		end select
		ln += ".f64(double "
	end if

	ln += hVregToStr( v1 ) + ")"
	hWriteLine( ln )

end sub

private sub _emitUop _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval vr as IRVREG ptr _
	)

	var uopdump = astDumpOpToStr( op ) + " " + vregPretty( v1 )
	if( vr = NULL ) then
		hAstCommand( "selfuop " + uopdump )
	else
		hAstCommand( "uop " + uopdump )
	end if

	'' LLVM IR doesn't have unary operations, corresponding BOPs are
	'' supposed to be used instead. However there are built-in functions
	'' for sin() & co.
	select case( op )
	case AST_OP_NEG
		'' vr = 0 - v1

		dim v1orig as IRVREG

		var isself = FALSE
		if( vr = NULL ) then
			'' Self-UOP
			''
			'' Need to allocate a result REG manually and then store
			'' that into v1 later.
			''
			'' Unfortunately we can't let hEmitBop() handle this,
			'' because in case of a self-BOP it expects the lhs to
			'' be the variable, while here it's the rhs. So we need
			'' to do it manually, by always using a non-self-BOP.
			''
			'' Also, just like hEmitBop(), we have to preserve the
			'' "storable" version of v1, because hEmitBop() may
			'' overwrite it with the loaded version.
			isself = TRUE
			vr = irhlAllocVreg( v1->dtype, v1->subtype )
			v1orig = *v1
		end if

		if( typeGetClass( v1->dtype ) = FB_DATACLASS_FPOINT ) then
			'' fneg flips the sign bit, including for NaNs and signed zero.
			'' Subtracting from zero does not preserve those semantics.
			hLoadVreg( v1 )
			hWriteLine( hVregToStr( vr ) + " = fneg " + _
				hEmitType( v1->dtype, v1->subtype ) + " " + hVregToStr( v1 ) )
		else
			var zero = irhlAllocVrImm( FB_DATATYPE_INTEGER, NULL, 0 )
			hEmitBop( AST_OP_SUB, zero, v1, vr, NULL, IR_EMITOPT_NONE )
		end if

		if( isself ) then
			hEmitStore( @v1orig, vr )
		end if

	case AST_OP_NOT
		'' vr = v1 xor -1

		'' Just pass on as BOP. Works even for self-UOPs, as v1 will be
		'' the lhs of the self-BOP as expected by hEmitBop().
		var minusone = irhlAllocVrImm( FB_DATATYPE_INTEGER, NULL, -1 )
		hEmitBop( AST_OP_XOR, v1, minusone, vr, NULL, IR_EMITOPT_NONE )

	case else
		hBuiltInUop( op, v1, vr )
	end select

end sub

private function hGetConvOpCode( byval ldtype as integer, byval rdtype as integer ) as const zstring ptr
	if( typeGetClass( ldtype ) = FB_DATACLASS_FPOINT ) then
		if( typeGetClass( rdtype ) = FB_DATACLASS_FPOINT ) then

			'' same size? i.e. due to const -> non-const, then no convert
			if( typeGetSize( ldtype ) = typeGetSize( rdtype ) ) then
				return NULL
			end if

			'' float to float (i.e. single <-> double)
			return iif( typeGetSize( ldtype ) < typeGetSize( rdtype ), cptr(const zstring ptr, @"fptrunc"), cptr(const zstring ptr, @"fpext") )
		end if

		'' int to float
		return iif( typeIsSigned( rdtype ), cptr(const zstring ptr, @"sitofp"), cptr(const zstring ptr, @"uitofp") )
	end if

	if( typeGetClass( rdtype ) = FB_DATACLASS_FPOINT ) then
		'' float to int (rounding was taken care of above)
		return iif( typeIsSigned( ldtype ), cptr(const zstring ptr, @"fptosi"), cptr(const zstring ptr, @"fptoui") )
	end if

	'' int to int

	if( typeIsPtr( ldtype ) ) then
		if( typeIsPtr( rdtype ) ) then
			'' both are pointers, just convert the type
			'' (bitcast only changes the compile-time type, not any bits)
			return @"bitcast"
		end if
		return @"inttoptr"
	elseif( typeIsPtr( rdtype ) ) then
		return @"ptrtoint"
	end if

	'' same size ints?
	if( typeGetSize( ldtype ) = typeGetSize( rdtype ) ) then
		if( typeGetSizeType( ldtype ) = typeGetSizeType( rdtype ) ) then
			'' Do nothing for 32bit Long <-> 32bit Integer, etc.
			return NULL
		end if

		'' signed <-> unsigned
		return @"bitcast"
	end if

	if( typeGetSize( ldtype ) < typeGetSize( rdtype ) ) then
		return @"trunc"
	end if

	return iif( typeIsSigned( rdtype ), cptr(const zstring ptr, @"sext"), cptr(const zstring ptr, @"zext") )
end function

private sub hEmitConvert( byval v1 as IRVREG ptr, byval v2 as IRVREG ptr )
	if( (typeGetDtAndPtrOnly( v1->dtype ) = FB_DATATYPE_BOOLEAN) andalso _
	    (typeGetDtAndPtrOnly( v2->dtype ) <> FB_DATATYPE_BOOLEAN) ) then
		'' CBOOL produces the canonical 0 or 1 value. Truncating a nonzero
		'' integer to i8 would preserve its low byte instead (for example 255).
		hLoadVreg( v2 )
		var nonzero = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
		var ln = hVregToStr( nonzero )
		if( typeGetClass( v2->dtype ) = FB_DATACLASS_FPOINT ) then
			ln += " = fcmp une "
		else
			ln += " = icmp ne "
		end if
		ln += hEmitType( v2->dtype, v2->subtype ) + " " + hVregToStr( v2 )
		if( typeGetClass( v2->dtype ) = FB_DATACLASS_FPOINT ) then
			ln += ", 0.0"
		elseif( typeIsPtr( v2->dtype ) ) then
			ln += ", null"
		else
			ln += ", 0"
		end if
		hWriteLine( ln )
		var normalized = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
		hWriteLine( hVregToStr( normalized ) + " = zext i1 " + _
		    hVregToStr( nonzero ) + " to i8" )
		if( irIsREG( v1 ) ) then
			*v1 = *normalized
		else
			hEmitStore( v1, normalized )
		end if
		exit sub
	end if
	if( typeGetDtAndPtrOnly( v2->dtype ) = FB_DATATYPE_BOOLEAN ) then
		'' A stored TRUE is 1, but numeric casts of a FreeBASIC BOOLEAN
		'' produce -1. Sign extend its truth bit before the conversion.
		hLoadVreg( v2 )
		var truth = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
		hWriteLine( hVregToStr( truth ) + " = icmp ne i8 " + _
		    hVregToStr( v2 ) + ", 0" )
		dim result as IRVREG ptr
		if( irIsREG( v1 ) ) then
			result = v1
		else
			result = irhlAllocVreg( v1->dtype, v1->subtype )
		end if
		if( typeIsPtr( v1->dtype ) ) then
			var integer_result = irhlAllocVreg( FB_DATATYPE_INTEGER, NULL )
			hWriteLine( hVregToStr( integer_result ) + " = sext i1 " + _
			    hVregToStr( truth ) + " to " + _
			    hEmitType( FB_DATATYPE_INTEGER, NULL ) )
			hWriteLine( hVregToStr( result ) + " = inttoptr " + _
			    hEmitType( FB_DATATYPE_INTEGER, NULL ) + " " + _
			    hVregToStr( integer_result ) + " to " + _
			    hEmitType( v1->dtype, v1->subtype ) )
		elseif( typeGetClass( v1->dtype ) = FB_DATACLASS_FPOINT ) then
			hWriteLine( hVregToStr( result ) + " = sitofp i1 " + _
			    hVregToStr( truth ) + " to " + _
			    hEmitType( v1->dtype, v1->subtype ) )
		else
			hWriteLine( hVregToStr( result ) + " = sext i1 " + _
			    hVregToStr( truth ) + " to " + _
			    hEmitType( v1->dtype, v1->subtype ) )
		end if
		if( irIsREG( v1 ) = FALSE ) then
			hEmitStore( v1, result )
		end if
		exit sub
	end if

	if( (typeGet( v2->dtype ) = FB_DATATYPE_STRUCT) andalso _
	    (irIsREG( v2 ) = FALSE) andalso _
	    ((v1->dtype <> v2->dtype) or (v1->subtype <> v2->subtype)) ) then
		'' An inherited field can still carry its containing UDT's memory
		'' vreg. Load the requested field type through that address.
		dim scalar as IRVREG = *v2
		scalar.dtype = v1->dtype
		scalar.subtype = v1->subtype
		hLoadVreg( @scalar )
		if( irIsREG( v1 ) ) then
			*v1 = scalar
		else
			hEmitStore( v1, @scalar )
		end if
		exit sub
	end if

	'' Converting float to int? Needs special treatment to achieve FB's rounding behaviour,
	'' because LLVM's fptosi/fptoui just truncate.
	if( (typeGetClass( v2->dtype ) = FB_DATACLASS_FPOINT) and _
	    (typeGetClass( v1->dtype ) = FB_DATACLASS_INTEGER) ) then
		'' Round v2 by calling llvm.nearbyint() and then using the result
		'' as the new v2. This rounding does float to float, then we can feed
		'' that into fptosi/fptoui to get the [u]int.
		var v0 = irhlAllocVreg( v2->dtype, v2->subtype )
		hLoadVreg( v2 )

		var ln = hVregToStr( v0 ) + " = call "
		if( v2->dtype = FB_DATATYPE_SINGLE ) then
			builtins(BUILTIN_NEARBYINTF).used = TRUE
			ln += "float @llvm.nearbyint.f32(float "
		else
			assert( v2->dtype = FB_DATATYPE_DOUBLE )
			builtins(BUILTIN_NEARBYINT).used = TRUE
			ln += "double @llvm.nearbyint.f64(double "
		end if
		ln += hVregToStr( v2 ) + ")"
		hWriteLine( ln )

		*v2 = *v0
	end if

	assert( (v1->dtype <> v2->dtype) or (v1->subtype <> v2->subtype) )

	var op = hGetConvOpCode( v1->dtype, v2->dtype )
	if( op = NULL ) then
		'' Do nothing for 32bit Long <-> 32bit Integer, etc.
		*v1 = *v2
		exit sub
	end if

	dim v0 as IRVREG ptr
	if( irIsREG( v1 ) ) then
		v0 = v1
	else
		v0 = irhlAllocVreg( v1->dtype, v1->subtype )
	end if

	hLoadVreg( v2 )

	var ln = hVregToStr( v0 ) + " = " + *op + " "
	ln += hEmitType( v2->dtype, v2->subtype )
	ln += " " + hVregToStr( v2 ) + " to "
	ln += hEmitType( v1->dtype, v1->subtype )
	hWriteLine( ln )

	if( irIsREG( v1 ) = FALSE ) then
		hEmitStore( v1, v0 )
	end if
end sub

private sub _emitConvert( byval v1 as IRVREG ptr, byval v2 as IRVREG ptr )
	hAstCommand( "conv " + vregPretty( v2 ) + " => " + vregPretty( v1 ) )
	hEmitConvert( v1, v2 )
end sub

private sub hEmitStore( byval l as IRVREG ptr, byval r as IRVREG ptr )
	hLoadVreg( r )
	_setVregDataType( r, l->dtype, l->subtype )

	hPrepareAddress( l )

	var ln = "store "
	ln += hEmitType( typeDeref( l->dtype ), l->subtype ) + " "
	var rhs = hVregToStr( r )
	'' LLVM uses the 'null' token for a pointer constant.  An integer zero
	'' is not a valid pointer operand even after the vreg type is changed.
	if( typeIsPtr( typeDeref( l->dtype ) ) andalso _
		irIsIMM( r ) andalso (r->value.i = 0) ) then
		rhs = "null"
	end if
	ln += rhs + ", "
	ln += hEmitType( l->dtype, l->subtype ) + " "
	ln += hVregToStr( l )
	hWriteLine( ln )
end sub

private sub _emitStore( byval l as IRVREG ptr, byval r as IRVREG ptr )
	hAstCommand( "store " + vregPretty( l ) + " := " + vregPretty( r ) )
	hEmitStore( l, r )
end sub

private sub _emitSpillRegs( )
	/' do nothing '/
end sub

private sub _emitLoad( byval v1 as IRVREG ptr )
	/' do nothing '/
end sub

private sub _emitLoadRes _
	( _
		byval v1 as IRVREG ptr, _
		byval vr as IRVREG ptr _
	)

	hAstCommand( "loadres " + vregPretty( v1 ) )
	if( hUsesCLongDoubleCABI( ctx.current_proc ) andalso _
	    hIsCLongDoubleType( vr->dtype, vr->subtype ) ) then
		var native_type = hGetCLongDoubleLLVMType( vr->subtype )
		if( native_type = "" ) then
			errReport( FB_ERRMSG_INVALIDDATATYPES, , _
			           ": CLongDouble procedure results are not supported by the LLVM backend for this target" )
			exit sub
		end if
		var native_value = hLoadCLongDoubleNativeValue( v1, vr->subtype )
		hWriteLine( "ret " + native_type + " " + hVregToStr( native_value ) )
		exit sub
	end if

	hLoadVreg( v1 )
	_setVregDataType( v1, vr->dtype, vr->subtype )

	hWriteLine( "ret " + hEmitType( vr->dtype, vr->subtype ) + " " + hVregToStr( v1 ) )

end sub

private sub _emitAddr _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval vr as IRVREG ptr _
	)

	select case( op )
	case AST_OP_ADDROF
		hAstCommand( "addrof " + vregPretty( v1 ) )

		'' There is no address-of operator in LLVM, because it only
		'' uses addresses to access memory, i.e. everything is a
		'' pointer already.
		''
		'' If a different type is wanted we can do a bitcast,
		'' but without loading the vreg, and if it's the same type
		'' the expression can be re-used as-is.
		if( irIsREG( v1 ) ) then
			'' An aggregate expression may already be an SSA value. Put it
			'' in addressable storage before taking its address.
			var slot = irhlAllocVreg( typeAddrOf( v1->dtype ), v1->subtype )
			var valuetype = hEmitType( v1->dtype, v1->subtype )
			var allocline = string( ctx.indent, TABCHAR ) + hVregToStr( slot ) + _
			    " = alloca " + valuetype + NEWLINE
			hAppendOutputBuffer( @ctx.proc_allocas, strptr( allocline ), len( allocline ) )
			hWriteLine( "store " + valuetype + " " + hVregToStr( v1 ) + ", " + _
			    valuetype + "* " + hVregToStr( slot ) )
			*v1 = *slot
		else
			hPrepareAddress( v1 )
		end if
		_setVregDataType( v1, vr->dtype, vr->subtype )

	case AST_OP_DEREF
		hAstCommand( "deref " + vregPretty( v1 ) )
		hLoadVreg( v1 )

	end select

	assert( irIsREG( vr ) and irIsREG( v1 ) )
	*vr = *v1

end sub

#include once "backend/llvm/ir-llvm-memory.bi"

private function hLoadCLongDoubleNativeValue _
	( byval value as IRVREG ptr, byval subtype as FBSYMBOL ptr ) as IRVREG ptr

	var native_type = hGetCLongDoubleLLVMType( subtype )
	if( native_type = "" ) then
		errReport( FB_ERRMSG_INVALIDDATATYPES, , _
		           ": CLongDouble fixed arguments are not supported by the LLVM backend for this target" )
		return value
	end if

	var aggregate_type = hEmitType( FB_DATATYPE_STRUCT, subtype )
	dim as IRVREG ptr aggregate_pointer = any
	if( value->typ = IR_VREGTYPE_REG ) then
		hLoadVreg( value )
		aggregate_pointer = irhlAllocVreg( typeAddrOf( FB_DATATYPE_STRUCT ), subtype )
		var alloca_line = string( ctx.indent, TABCHAR ) + _
			hVregToStr( aggregate_pointer ) + " = alloca " + aggregate_type + NEWLINE
		hAppendOutputBuffer( @ctx.proc_allocas, strptr( alloca_line ), len( alloca_line ) )
		hWriteLine( "store " + aggregate_type + " " + hVregToStr( value ) + ", " + _
		            aggregate_type + "* " + hVregToStr( aggregate_pointer ) + ", align 1" )
	else
		hPrepareAddress( value )
		aggregate_pointer = irhlAllocVreg( typeAddrOf( FB_DATATYPE_STRUCT ), subtype )
		hWriteLine( hVregToStr( aggregate_pointer ) + " = bitcast " + _
		            hEmitType( value->dtype, subtype ) + " " + hVregToStr( value ) + _
		            " to " + aggregate_type + "*" )
	end if

	var native_pointer = irhlAllocVreg( typeAddrOf( FB_DATATYPE_BYTE ), NULL )
	hWriteLine( hVregToStr( native_pointer ) + " = bitcast " + aggregate_type + _
	            "* " + hVregToStr( aggregate_pointer ) + " to " + native_type + "*" )
	var native_value = irhlAllocVreg( FB_DATATYPE_DOUBLE, NULL )
	hWriteLine( hVregToStr( native_value ) + " = load " + native_type + ", " + _
	            native_type + "* " + hVregToStr( native_pointer ) + ", align 1" )
	return native_value
end function

private function hStoreCLongDoubleNativeValue _
	( byval native_value as IRVREG ptr, byval subtype as FBSYMBOL ptr ) as IRVREG ptr

	var native_type = hGetCLongDoubleLLVMType( subtype )
	if( native_type = "" ) then
		errReport( FB_ERRMSG_INVALIDDATATYPES, , _
		           ": CLongDouble fixed results are not supported by the LLVM backend for this target" )
		return native_value
	end if

	var aggregate_type = hEmitType( FB_DATATYPE_STRUCT, subtype )
	var aggregate_pointer = irhlAllocVreg( typeAddrOf( FB_DATATYPE_STRUCT ), subtype )
	var alloca_line = string( ctx.indent, TABCHAR ) + _
		hVregToStr( aggregate_pointer ) + " = alloca " + aggregate_type + NEWLINE
	hAppendOutputBuffer( @ctx.proc_allocas, strptr( alloca_line ), len( alloca_line ) )
	hWriteLine( "store " + aggregate_type + " zeroinitializer, " + _
	            aggregate_type + "* " + hVregToStr( aggregate_pointer ) + ", align 1" )
	var native_pointer = irhlAllocVreg( typeAddrOf( FB_DATATYPE_BYTE ), NULL )
	hWriteLine( hVregToStr( native_pointer ) + " = bitcast " + aggregate_type + _
	            "* " + hVregToStr( aggregate_pointer ) + " to " + native_type + "*" )
	hWriteLine( "store " + native_type + " " + hVregToStr( native_value ) + ", " + _
	            native_type + "* " + hVregToStr( native_pointer ) + ", align 1" )
	var aggregate_value = irhlAllocVreg( FB_DATATYPE_STRUCT, subtype )
	hWriteLine( hVregToStr( aggregate_value ) + " = load " + aggregate_type + ", " + _
	            aggregate_type + "* " + hVregToStr( aggregate_pointer ) + ", align 1" )
	return aggregate_value
end function

private sub hPrepareValistAddress( byval v as IRVREG ptr )
	if( v->typ = IR_VREGTYPE_REG ) then
		assert( typeIsPtr( v->dtype ) )
		exit sub
	end if

	if( (v->sym <> NULL) andalso symbIsParamVarByval( v->sym ) andalso _
	    (symbGetValistType( v->dtype, v->subtype ) = FB_CVA_LIST_BUILTIN_C_STD) ) then
		'' A byval cva_list parameter is already a pointer to the array's
		'' first element. Its local symbol holds that pointer value.
		var subtype = v->subtype
		hPrepareAddress( v )
		_setVregDataType( v, typeAddrOf( typeAddrOf( FB_DATATYPE_STRUCT ) ), subtype )
		var loaded = irhlAllocVreg( typeAddrOf( FB_DATATYPE_STRUCT ), subtype )
		hWriteLine( hVregToStr( loaded ) + " = load " + _
		    hEmitType( loaded->dtype, subtype ) + ", " + _
		    hEmitType( v->dtype, subtype ) + " " + hVregToStr( v ) )
		*v = *loaded
	else
		hPrepareAddress( v )
	end if
end sub

'' Save a GP/SSE eightbyte in an addressable buffer before loading the UDT.
private function hLoadCallArg( byval arg as IRCALLARG ptr ) as IRVREG ptr
	dim varg as IRVREG ptr = arg->vr
	dim as integer arg_mode = FB_PARAMMODE_VARARG
	hInternalCommand( "arg " + vregPretty( varg ) )

	'' Emit param's dtype (to match the declaration), not the arg's
	dim dtype as integer
	dim subtype as FBSYMBOL ptr
	'' A vararg marker has VOID dtype, but each variadic operand must
	'' carry its own actual type in the LLVM call instruction.
	if( arg->param andalso (symbGetParamMode( arg->param ) <> FB_PARAMMODE_VARARG) ) then
		symbGetRealParamDtype( arg->param, dtype, subtype )
		arg_mode = symbGetParamMode( arg->param )
		if( (symbGetParamMode( arg->param ) = FB_PARAMMODE_BYVAL) andalso _
		    (symbGetValistType( dtype, subtype ) = FB_CVA_LIST_BUILTIN_C_STD) ) then
			dtype = typeAddrOf( dtype )
		end if
	else
		dtype = varg->dtype
		subtype = varg->subtype
	end if
	if( arg->param andalso (arg_mode <> FB_PARAMMODE_VARARG) andalso _
	    hUsesCLongDoubleCABI( symbGetParent( arg->param ) ) andalso _
	    hIsCLongDoubleType( dtype, subtype ) ) then
		return hLoadCLongDoubleNativeValue( varg, subtype )
	end if
	if( arg->param andalso hIsMemoryByval( dtype, subtype, symbGetParamMode(arg->param) ) ) then
		return hCopyMemoryByval( varg, dtype, subtype )
	end if
	if( hIsSystemVVarargAggregate( dtype, subtype, arg_mode ) ) then
		return hCopyMemoryByval( varg, dtype, subtype )
	end if
	if( hIsMicrosoftVarargAggregate( dtype, subtype, arg_mode ) ) then
		return hCopyMemoryByval( varg, dtype, subtype )
	end if
	if( (arg_mode = FB_PARAMMODE_VARARG) andalso _
	    hIsCLongDoubleType( dtype, subtype ) ) then
		return hCopyMemoryByval( varg, dtype, subtype )
	end if
	'' An address REG may still name a pointer temporary's alloca.
	'' When the parameter wants the pointer value, load it instead of
	'' casting the alloca's address to the parameter pointer type.
	if( (varg->typ = IR_VREGTYPE_REG) andalso (varg->sym <> NULL) andalso _
	    (symbIsVar( varg->sym ) or symbIsField( varg->sym )) ) then
		dim symdtype as integer
		dim symsubtype as FBSYMBOL ptr
		symbGetRealType( varg->sym, symdtype, symsubtype )
		if( typeIsPtr( symdtype ) andalso _
		    (hEmitType( dtype, subtype ) = hEmitType( symdtype, symsubtype )) andalso _
		    (varg->dtype = typeAddrOf( symdtype )) ) then
			var loaded = irhlAllocVreg( symdtype, symsubtype )
			hWriteLine( hVregToStr( loaded ) + " = load " + _
			    hEmitType( symdtype, symsubtype ) + ", " + _
			    hEmitType( varg->dtype, varg->subtype ) + " " + _
			    hVregToStr( varg ) )
			varg = loaded
		end if
	end if
	if( arg->param andalso _
	    (symbGetParamMode( arg->param ) = FB_PARAMMODE_BYVAL) andalso _
	    (symbGetValistType( symbGetFullType( arg->param ), _
	                         symbGetSubtype( arg->param ) ) = FB_CVA_LIST_BUILTIN_C_STD) andalso _
	    (typeIsPtr( varg->dtype ) = FALSE) ) then
		hPrepareValistAddress( varg )
	else
		hLoadVreg( varg )
	end if

	'' Convert arg to param's dtype if needed
	_setVregDataType( varg, dtype, subtype )
	return varg
end function

#include once "backend/llvm/ir-llvm-builtins.bi"

private sub hDoCall _
	( _
		byval pname as zstring ptr, _
		byval proc as FBSYMBOL ptr, _
		byval indirect as integer, _
		byval bytestopop as integer, _
		byval vr as IRVREG ptr, _
		byval level as integer _
	)

	dim as string ln
	dim as IRCALLARG ptr arg = any, prev = any
	dim as IRVREG ptr varg = any, v0 = vr
	dim as DZSTRING args
	dim as integer i386_int128_result = any
	dim as integer is_clongdouble_cabi_result = FALSE
	dim as integer semantic_demanded = (vr <> NULL)
	dim as integer semantic_result_dtype = FB_DATATYPE_VOID
	dim as FBSYMBOL ptr semantic_result_subtype = NULL
	dim as string semantic_result, semantic_convention
	dim as integer is_variadic = FALSE
	dim as integer is_systemv_call = (fbGetCpuFamily( ) = FB_CPUFAMILY_X86_64) andalso _
	                               ((env.target.options and FB_TARGETOPT_UNIX) <> 0)
	dim as integer systemv_integer_registers_used = 0
	dim as integer systemv_float_registers_used = 0
	dim as FBSYMBOL ptr tail_param = symbGetProcTailParam( proc )
	DZstrZero( args )

	assert( symbIsProc( proc ) )
	if( tail_param <> NULL ) then
		is_variadic = (symbGetParamMode( tail_param ) = FB_PARAMMODE_VARARG)
	end if
	i386_int128_result = hIsI386Integer128Result( proc )
	if( i386_int128_result ) then
		'' The caller-owned slot is the LLVM result for this target ABI.
		vr = NULL
	end if

	if( vr = NULL ) then
		'' Result discarded? Not allowed in LLVM, so assign to a
		'' temporary result vreg that will be unused.
		if( (symbGetType( proc ) <> FB_DATATYPE_VOID) andalso _
		    (i386_int128_result = FALSE) ) then
			vr = irhlAllocVreg( typeGetDtAndPtrOnly( symbGetProcRealType( proc ) ), _
						symbGetProcRealSubtype( proc ) )
		end if
	end if
	is_clongdouble_cabi_result = (vr <> NULL) andalso _
		hUsesCLongDoubleCABI( proc ) andalso _
		hIsCLongDoubleType( vr->dtype, vr->subtype )

	'' The LLVM call instruction must include the full function signature,
	'' and it has to match the declaration. We have to emit this based on
	'' the proc/param symbols - can't rely on the args, because our AST can
	'' have args with different dtype than the params.

	if( vr ) then
		if( is_clongdouble_cabi_result ) then
			v0 = irhlAllocVreg( FB_DATATYPE_DOUBLE, NULL )
		elseif( irIsREG( vr ) ) then
			v0 = vr
		else
			v0 = irhlAllocVreg( vr->dtype, vr->subtype )
		end if

		ln = hVregToStr( v0 ) + " = call "
		semantic_convention = hEmitProcCallConv( proc )
		ln += semantic_convention
		if( is_clongdouble_cabi_result ) then
			semantic_result = hGetCLongDoubleLLVMType( vr->subtype )
			if( semantic_result = "" ) then
				errReport( FB_ERRMSG_INVALIDDATATYPES, , _
				           ": CLongDouble call results are not supported by the LLVM backend for this target" )
				semantic_result = hEmitType( v0->dtype, v0->subtype )
			end if
		else
			semantic_result = hEmitType( v0->dtype, v0->subtype )
		end if
		if( is_variadic ) then
			ln += hEmitProcHeader( proc, TRUE, TRUE, FALSE ) + " "
		else
			ln += semantic_result + " "
		end if
		semantic_result_dtype = v0->dtype
		semantic_result_subtype = v0->subtype
		if( is_clongdouble_cabi_result ) then
			semantic_result_dtype = vr->dtype
			semantic_result_subtype = vr->subtype
		end if
	else
		semantic_convention = hEmitProcCallConv( proc )
		semantic_result = "void"
		ln = "call " + semantic_convention
		if( is_variadic ) then
			ln += hEmitProcHeader( proc, TRUE, TRUE, FALSE ) + " "
		else
			ln += "void "
		end if
	end if
	dim as longint semantic_call = fbSemanticModelCallAbiBegin(proc, "llvm", indirect, semantic_demanded, _
		semantic_result_dtype, semantic_result_subtype, semantic_result, semantic_convention, level, bytestopop)

	ln += *pname + "( "

	'' args
	arg = listGetTail( @irhl.callargs )
	while( arg andalso (arg->level = level) )
		prev = listGetPrev( arg )
		dim as integer semantic_input_dtype = arg->vr->dtype
		dim as FBSYMBOL ptr semantic_input_subtype = arg->vr->subtype
		dim as string semantic_kind, semantic_type, semantic_attributes, semantic_value
		dim as integer arg_mode = FB_PARAMMODE_VARARG
		dim as integer is_variadic_operand = is_variadic
		dim as integer is_systemv_aggregate_vararg = FALSE
		dim as integer is_microsoft_aggregate_vararg = FALSE
		dim as integer is_clongdouble_vararg = FALSE
		if( arg->param ) then
			arg_mode = symbGetParamMode( arg->param )
			is_variadic_operand = (arg_mode = FB_PARAMMODE_VARARG)
		end if
		if( is_systemv_call andalso is_variadic_operand ) then
			is_systemv_aggregate_vararg = hIsSystemVVarargAggregate( _
				semantic_input_dtype, semantic_input_subtype, FB_PARAMMODE_VARARG )
		end if
		if( is_variadic_operand ) then
			is_microsoft_aggregate_vararg = hIsMicrosoftVarargAggregate( _
				semantic_input_dtype, semantic_input_subtype, FB_PARAMMODE_VARARG )
			is_clongdouble_vararg = hIsCLongDoubleType( _
				semantic_input_dtype, semantic_input_subtype )
		end if
		if( is_systemv_call andalso (is_systemv_aggregate_vararg = FALSE) andalso _
		    (is_clongdouble_vararg = FALSE) ) then
			dim as integer call_dtype = semantic_input_dtype
			dim as FBSYMBOL ptr call_subtype = semantic_input_subtype
			dim as integer argument_integer_registers = 0
			dim as integer argument_float_registers = 0
			if( arg->param andalso (arg_mode <> FB_PARAMMODE_VARARG) ) then
				symbGetRealParamDtype( arg->param, call_dtype, call_subtype )
			end if
			if( (arg_mode = FB_PARAMMODE_VARARG) orElse _
			    (hUsesCLongDoubleCABI( proc ) = FALSE) orElse _
			    (hIsCLongDoubleType( call_dtype, call_subtype ) = FALSE) ) then
				hGetSystemVArgumentRegisterCounts( call_dtype, call_subtype, _
					arg_mode, argument_integer_registers, argument_float_registers )
			end if
			systemv_integer_registers_used += argument_integer_registers
			systemv_float_registers_used += argument_float_registers
		end if

		varg = hLoadCallArg( arg )
		if( arg->param = NULL andalso i386_int128_result ) then
			semantic_type = hEmitType( varg->dtype, varg->subtype )
			DZstrConcatAssign( args, semantic_type )
			semantic_attributes = " sret(i128) align " + str( LLVM_I386_INT128_ABI_ALIGNMENT )
			DZstrConcatAssign( args, semantic_attributes )
			semantic_value = hVregToStr( varg )
			DZstrConcatAssign( args, " " + semantic_value )
			fbSemanticModelCallAbiArgument(semantic_call, NULL, 0, 1, "hidden-result", _
				semantic_input_dtype, semantic_input_subtype, varg->dtype, varg->subtype, _
				semantic_type, semantic_attributes, semantic_value)
		elseif( arg->param andalso _
		        (symbGetParamMode(arg->param) <> FB_PARAMMODE_VARARG) ) then
			dim dtype as integer, subtype as FBSYMBOL ptr
			symbGetRealParamDtype( arg->param, dtype, subtype )
			if( hIsI386Integer128Param( dtype, symbGetParamMode(arg->param) ) ) then
				dim as string low_name, high_name
				hSplitI386Integer128( varg, low_name, high_name )
				DZstrConcatAssign( args, "i64 " + low_name + ", i64 " + high_name )
				fbSemanticModelCallAbiArgument(semantic_call, arg->param, 0, 2, "i386-int128-low", _
					semantic_input_dtype, semantic_input_subtype, FB_DATATYPE_ULONGINT, NULL, "i64", "", low_name)
				fbSemanticModelCallAbiArgument(semantic_call, arg->param, 1, 2, "i386-int128-high", _
					semantic_input_dtype, semantic_input_subtype, FB_DATATYPE_ULONGINT, NULL, "i64", "", high_name)
			elseif( hUsesCLongDoubleCABI( proc ) andalso _
			        hIsCLongDoubleType( dtype, subtype ) ) then
				semantic_type = hGetCLongDoubleLLVMType( subtype )
				if( semantic_type = "" ) then
					errReport( FB_ERRMSG_INVALIDDATATYPES, , _
					           ": CLongDouble fixed arguments are not supported by the LLVM backend for this target" )
					semantic_type = hEmitType( varg->dtype, varg->subtype )
				end if
				semantic_value = hVregToStr( varg )
				DZstrConcatAssign( args, semantic_type + " " + semantic_value )
				fbSemanticModelCallAbiArgument(semantic_call, arg->param, 0, 1, _
					"fixed-clongdouble-native", semantic_input_dtype, semantic_input_subtype, _
					FB_DATATYPE_DOUBLE, NULL, semantic_type, "", semantic_value)
			elseif( hIsMemoryByval( dtype, subtype, symbGetParamMode(arg->param) ) ) then
				semantic_type = hEmitType( varg->dtype, varg->subtype )
				DZstrConcatAssign( args, semantic_type )
				semantic_attributes = hMemoryByvalAttribute( subtype )
				DZstrConcatAssign( args, semantic_attributes )
				semantic_value = hVregToStr( varg )
				DZstrConcatAssign( args, " " + semantic_value )
				fbSemanticModelCallAbiArgument(semantic_call, arg->param, 0, 1, "memory-byval", _
					semantic_input_dtype, semantic_input_subtype, varg->dtype, varg->subtype, _
					semantic_type, semantic_attributes, semantic_value)
			else
				semantic_type = hEmitType( varg->dtype, varg->subtype )
				DZstrConcatAssign( args, semantic_type )
				semantic_value = hVregToStr( varg )
				DZstrConcatAssign( args, " " + semantic_value )
				semantic_kind = "value"
				if( (symbGetParamMode(arg->param) = FB_PARAMMODE_BYVAL) andalso _
					(symbGetValistType(dtype, subtype) = FB_CVA_LIST_BUILTIN_C_STD) ) then semantic_kind = "va-list-decay"
				fbSemanticModelCallAbiArgument(semantic_call, arg->param, 0, 1, semantic_kind, _
					semantic_input_dtype, semantic_input_subtype, varg->dtype, varg->subtype, _
					semantic_type, "", semantic_value)
			end if
		elseif( is_clongdouble_vararg ) then
			semantic_type = hGetCLongDoubleLLVMType( semantic_input_subtype )
			if( semantic_type = "" ) then
				errReport( FB_ERRMSG_INVALIDDATATYPES, , _
					": CLongDouble varargs are not supported by the LLVM backend for this target" )
				semantic_type = hEmitType( varg->dtype, varg->subtype )
				semantic_value = hVregToStr( varg )
				DZstrConcatAssign( args, semantic_type + " " + semantic_value )
				fbSemanticModelCallAbiArgument(semantic_call, arg->param, 0, 1, _
					"unsupported-clongdouble-vararg", semantic_input_dtype, _
					semantic_input_subtype, varg->dtype, varg->subtype, _
					semantic_type, "", semantic_value)
			else
				dim as IRVREG ptr native_pointer = irhlAllocVreg( _
					typeAddrOf( FB_DATATYPE_BYTE ), NULL )
				dim as IRVREG ptr native_value = irhlAllocVreg( _
					FB_DATATYPE_DOUBLE, NULL )
				hWriteLine( hVregToStr( native_pointer ) + " = bitcast " + _
					hEmitType( varg->dtype, varg->subtype ) + " " + hVregToStr( varg ) + _
					" to " + semantic_type + "*" )
				hWriteLine( hVregToStr( native_value ) + " = load " + semantic_type + ", " + _
					semantic_type + "* " + hVregToStr( native_pointer ) + ", align 1" )
				semantic_value = hVregToStr( native_value )
				DZstrConcatAssign( args, semantic_type + " " + semantic_value )
				fbSemanticModelCallAbiArgument(semantic_call, arg->param, 0, 1, _
					"variadic-clongdouble-native", semantic_input_dtype, _
					semantic_input_subtype, FB_DATATYPE_STRUCT, semantic_input_subtype, _
					semantic_type, "", semantic_value)
			end if
		elseif( is_systemv_aggregate_vararg ) then
			dim as integer integer_register_count = 0
			dim as integer floating_register_count = 0
			dim as integer can_use_registers = hGetSystemVStructRegisterCounts( _
				semantic_input_subtype, integer_register_count, floating_register_count )
			if( can_use_registers ) then
				can_use_registers = (systemv_integer_registers_used + integer_register_count <= 6) andalso _
				                    (systemv_float_registers_used + floating_register_count <= 8)
			end if
			if( can_use_registers ) then
				dim as string aggregate_type = hEmitType( _
					FB_DATATYPE_STRUCT, semantic_input_subtype )
				dim as string buffer_type = "[16 x i8]"
				dim as integer structure_class = hGetMagicStructNumber( semantic_input_subtype )
				dim as integer chunk_count = (semantic_input_subtype->lgt + 7) \ 8
				dim as IRVREG ptr buffer = irhlAllocVreg( _
					typeAddrOf( FB_DATATYPE_BYTE ), NULL )
				dim as IRVREG ptr typed_buffer = irhlAllocVreg( _
					typeAddrOf( FB_DATATYPE_STRUCT ), semantic_input_subtype )
				dim as IRVREG ptr aggregate_value = irhlAllocVreg( _
					FB_DATATYPE_STRUCT, semantic_input_subtype )
				dim as string alloca_line = string( ctx.indent, TABCHAR ) + _
					hVregToStr( buffer ) + " = alloca " + buffer_type + ", align 8" + NEWLINE
				hAppendOutputBuffer( @ctx.proc_allocas, strptr( alloca_line ), len( alloca_line ) )
				hWriteLine( "store " + buffer_type + " zeroinitializer, " + _
					buffer_type + "* " + hVregToStr( buffer ) + ", align 1" )
				hWriteLine( hVregToStr( typed_buffer ) + " = bitcast " + _
					buffer_type + "* " + hVregToStr( buffer ) + " to " + aggregate_type + "*" )
				hWriteLine( hVregToStr( aggregate_value ) + " = load " + aggregate_type + ", " + _
					aggregate_type + "* " + hVregToStr( varg ) )
				hWriteLine( "store " + aggregate_type + " " + hVregToStr( aggregate_value ) + ", " + _
					aggregate_type + "* " + hVregToStr( typed_buffer ) + ", align 1" )
				for chunk as integer = 0 to chunk_count - 1
					dim as integer is_floating_chunk = FALSE
					select case as const structure_class
					case KSTRUCT_X, KSTRUCT_XR
						is_floating_chunk = (chunk = 0)
					case KSTRUCT_RX, KSTRUCT_XX
						is_floating_chunk = (chunk = 1) orElse (structure_class = KSTRUCT_XX)
					end select
					dim as IRVREG ptr byte_pointer = irhlAllocVreg( _
						typeAddrOf( FB_DATATYPE_BYTE ), NULL )
					dim as IRVREG ptr qword_pointer = irhlAllocVreg( _
						typeAddrOf( FB_DATATYPE_ULONGINT ), NULL )
					dim as IRVREG ptr bits = irhlAllocVreg( FB_DATATYPE_ULONGINT, NULL )
					hWriteLine( hVregToStr( byte_pointer ) + " = getelementptr inbounds " + _
						buffer_type + ", " + buffer_type + "* " + hVregToStr( buffer ) + _
						", i64 0, i64 " + str( chunk * 8 ) )
					hWriteLine( hVregToStr( qword_pointer ) + " = bitcast i8* " + _
						hVregToStr( byte_pointer ) + " to i64*" )
					hWriteLine( hVregToStr( bits ) + " = load i64, i64* " + _
						hVregToStr( qword_pointer ) + ", align 1" )
					if( chunk > 0 ) then DZstrConcatAssign( args, ", " )
					if( is_floating_chunk ) then
						dim as IRVREG ptr floating_value = irhlAllocVreg( _
							FB_DATATYPE_DOUBLE, NULL )
						hWriteLine( hVregToStr( floating_value ) + " = bitcast i64 " + _
							hVregToStr( bits ) + " to double" )
						semantic_type = "double"
						semantic_value = hVregToStr( floating_value )
						semantic_kind = "variadic-sse-eightbyte"
						fbSemanticModelCallAbiArgument(semantic_call, arg->param, chunk, _
							chunk_count, semantic_kind, semantic_input_dtype, semantic_input_subtype, _
							FB_DATATYPE_DOUBLE, NULL, semantic_type, "", semantic_value)
					else
						semantic_type = "i64"
						semantic_value = hVregToStr( bits )
						semantic_kind = "variadic-gp-eightbyte"
						fbSemanticModelCallAbiArgument(semantic_call, arg->param, chunk, _
							chunk_count, semantic_kind, semantic_input_dtype, semantic_input_subtype, _
							FB_DATATYPE_ULONGINT, NULL, semantic_type, "", semantic_value)
					end if
					DZstrConcatAssign( args, semantic_type + " " + semantic_value )
				next
				systemv_integer_registers_used += integer_register_count
				systemv_float_registers_used += floating_register_count
			else
				semantic_type = hEmitType( varg->dtype, varg->subtype )
				DZstrConcatAssign( args, semantic_type )
				semantic_attributes = hMemoryByvalAttribute( semantic_input_subtype )
				DZstrConcatAssign( args, semantic_attributes )
				semantic_value = hVregToStr( varg )
				DZstrConcatAssign( args, " " + semantic_value )
				fbSemanticModelCallAbiArgument(semantic_call, arg->param, 0, 1, _
					"variadic-memory-byval", semantic_input_dtype, semantic_input_subtype, _
					varg->dtype, varg->subtype, semantic_type, semantic_attributes, semantic_value)
			end if
		elseif( is_microsoft_aggregate_vararg ) then
			dim as integer aggregate_size = semantic_input_subtype->lgt
			if( (aggregate_size = 1) orElse (aggregate_size = 2) orElse _
			    (aggregate_size = 4) orElse (aggregate_size = 8) ) then
				dim as integer integer_dtype = any
				select case aggregate_size
				case 1: integer_dtype = FB_DATATYPE_UBYTE
				case 2: integer_dtype = FB_DATATYPE_USHORT
				case 4: integer_dtype = FB_DATATYPE_ULONG
				case 8: integer_dtype = FB_DATATYPE_ULONGINT
				end select
				dim as string integer_type = "i" + str( aggregate_size * 8 )
				dim as IRVREG ptr integer_pointer = irhlAllocVreg( _
					typeAddrOf( integer_dtype ), NULL )
				dim as IRVREG ptr integer_value = irhlAllocVreg( integer_dtype, NULL )
				hWriteLine( hVregToStr( integer_pointer ) + " = bitcast " + _
					hEmitType( varg->dtype, varg->subtype ) + " " + hVregToStr( varg ) + _
					" to " + integer_type + "*" )
				hWriteLine( hVregToStr( integer_value ) + " = load " + integer_type + _
					", " + integer_type + "* " + hVregToStr( integer_pointer ) + ", align 1" )
				semantic_type = integer_type
				semantic_value = hVregToStr( integer_value )
				DZstrConcatAssign( args, semantic_type + " " + semantic_value )
				fbSemanticModelCallAbiArgument(semantic_call, arg->param, 0, 1, _
					"variadic-windows-aggregate-integer", semantic_input_dtype, _
					semantic_input_subtype, integer_dtype, NULL, semantic_type, "", semantic_value)
			else
				semantic_type = hEmitType( varg->dtype, varg->subtype )
				semantic_value = hVregToStr( varg )
				DZstrConcatAssign( args, semantic_type + " " + semantic_value )
				fbSemanticModelCallAbiArgument(semantic_call, arg->param, 0, 1, _
					"variadic-windows-aggregate-indirect", semantic_input_dtype, _
					semantic_input_subtype, varg->dtype, varg->subtype, _
					semantic_type, "", semantic_value)
			end if
		else
			semantic_type = hEmitType( varg->dtype, varg->subtype )
			DZstrConcatAssign( args, semantic_type )
			semantic_value = hVregToStr( varg )
			DZstrConcatAssign( args, " " + semantic_value )
			semantic_kind = "unmapped-value"
			if( arg->param <> NULL ) then semantic_kind = "variadic-value"
			fbSemanticModelCallAbiArgument(semantic_call, arg->param, 0, 1, semantic_kind, _
				semantic_input_dtype, semantic_input_subtype, varg->dtype, varg->subtype, _
				semantic_type, "", semantic_value)
		end if

		listDelNode( @irhl.callargs, arg )

		if( prev ) then
			if( prev->level = level ) then
				DZstrConcatAssign( args, ", " )
			end if
		end if

		arg = prev
	wend

	if( args.data <> NULL ) then
		ln += *args.data
	end if
	DZstrAllocate( args, 0 )

	ln += " )"
	fbSemanticModelCallAbiEnd(semantic_call, ln)

	hWriteLine( ln )

	if( vr ) then
		if( is_clongdouble_cabi_result ) then
			var aggregate_value = hStoreCLongDoubleNativeValue( v0, vr->subtype )
			if( irIsREG( vr ) ) then
				*vr = *aggregate_value
			else
				hEmitStore( vr, aggregate_value )
			end if
		elseif( irIsREG( vr ) = FALSE ) then
			hEmitStore( vr, v0 )
		end if
	end if
end sub

private sub _emitCall _
	( _
		byval proc as FBSYMBOL ptr, _
		byval bytestopop as integer, _
		byval vr as IRVREG ptr, _
		byval level as integer _
	)

	hAstCommand( "call " + hSymName( proc ) + "()" )
	if( hEmitBuiltinCall( proc, vr, level ) = FALSE ) then
		var pname = hEmitProcName( proc )
		hDoCall( strptr( pname ), proc, FALSE, bytestopop, vr, level )
	else
		fbSemanticModelCallAbiBuiltin(proc, level)
	end if

end sub

private sub _emitCallPtr _
	( _
		byval proc as FBSYMBOL ptr, _
		byval v1 as IRVREG ptr, _
		byval vr as IRVREG ptr, _
		byval bytestopop as integer, _
		byval level as integer _
	)

	assert( v1->dtype = typeAddrOf( FB_DATATYPE_FUNCTION ) )
	assert( proc = v1->subtype )

	hAstCommand( "callptr " + vregPretty( v1 ) )
	hLoadVreg( v1 )
	hDoCall( hVregToStr( v1 ), proc, TRUE, bytestopop, vr, level )

end sub

private sub _emitJumpPtr( byval v1 as IRVREG ptr )
	hAstCommand( "jumpptr " + vregPretty( v1 ) )
	hLoadVreg( v1 )
	_setVregDataType( v1, typeAddrOf( FB_DATATYPE_BYTE ), NULL )

	'' Only address-taken labels can be indirect destinations. Listing ordinary
	'' blocks would invent control-flow edges that violate SSA dominance.
	'' Complete the list at procedure end, after later label addresses are seen.
	ctx.proc_has_indirectbr = TRUE
	hWriteLine( "indirectbr i8* " + hVregToStr( v1 ) + ", [@FB_INDIRECT_DESTINATIONS@]" )
	hWriteLabel( symbUniqueLabel( ) )
end sub

private sub _emitBranch( byval op as integer, byval label as FBSYMBOL ptr )
	hAstCommand( "goto " + hSymName( label ) )
	'' GOTO label
	assert( op = AST_OP_JMP )

	'' The jump ends the current basic block...
	hWriteLine( "br label %" + *symbGetMangledName( label ) )

	'' so, we need to add a dummy label afterwards (starts new basic block)
	hWriteLabel( symbUniqueLabel( ) )
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

	hAstCommand( "jmptb " + vregPretty( v1 ) )

	dim as string ln

	hLoadVreg( v1 )
	var dtype = hEmitType( v1->dtype, v1->subtype )

	ln = "switch "
	ln += dtype + " "
	ln += hVregToStr( v1 ) + ", "
	ln += "label %" + *symbGetMangledName( deflabel ) + " "
	ln += "["
	hWriteLine( ln )

	ctx.indent += 1
	for i as integer = 0 to labelcount - 1
		ln = dtype + " " & (values[i]+bias) & ", "
		ln += "label %" + *symbGetMangledName( labels[i] )
		hWriteLine( ln )
	next
	ctx.indent -= 1

	hWriteLine( "]" )

end sub



private sub hCvaArgStoreEightbyte( _
	byref buffer_name as string, byref buffer_type as string, _
	byval value as IRVREG ptr, byval is_floating as integer, _
	byval destination_offset as integer _
)
	dim as IRVREG ptr byte_pointer = irhlAllocVreg( typeAddrOf( FB_DATATYPE_BYTE ), NULL )
	dim as IRVREG ptr qword_pointer = irhlAllocVreg( typeAddrOf( FB_DATATYPE_ULONGINT ), NULL )
	dim as string bits = hVregToStr( value )

	hWriteLine( hVregToStr( byte_pointer ) + " = getelementptr inbounds " + _
	            buffer_type + ", " + buffer_type + "* " + buffer_name + _
	            ", i64 0, i64 " + str( destination_offset ) )
	if( is_floating ) then
		dim as IRVREG ptr integer_bits = irhlAllocVreg( FB_DATATYPE_ULONGINT, NULL )
		hWriteLine( hVregToStr( integer_bits ) + " = bitcast double " + _
		            bits + " to i64" )
		bits = hVregToStr( integer_bits )
	end if
	hWriteLine( hVregToStr( qword_pointer ) + " = bitcast i8* " + _
	            hVregToStr( byte_pointer ) + " to i64*" )
	hWriteLine( "store i64 " + bits + ", i64* " + _
	            hVregToStr( qword_pointer ) + ", align 1" )
end sub

'' LLVM 21's x86 va_arg lowering crashes on aggregate result types. Consume
'' each System V eightbyte as its GP/SSE scalar class, using overflow_arg_area
'' when the complete aggregate does not fit in the register save area.
private sub hCvaArgSystemVStruct(byval v1 as IRVREG ptr, _
	byval vr as IRVREG ptr)
	dim as integer lgt = symbGetSizeOf( vr->subtype )
	dim as integer stack_alignment = vr->subtype->udt.natalign
	dim as integer slot_size = (lgt + 7) And Not(7)
	dim as integer structure_class = hGetMagicStructNumber( vr->subtype )
	dim as integer integer_register_count = 0
	dim as integer floating_register_count = 0
	dim as integer can_use_registers = FALSE
	dim as integer first_bytes = iif( lgt > 8, 8, lgt )
	dim as integer second_bytes = lgt - first_bytes
	dim as string list_type = hEmitType( FB_DATATYPE_STRUCT, v1->subtype )
	dim as string result_type = hEmitType( vr->dtype, vr->subtype )
	dim as string buffer_type
	dim as string register_label = *symbUniqueLabel( )
	dim as string overflow_label = *symbUniqueLabel( )
	dim as string done_label = *symbUniqueLabel( )
	dim as IRVREG ptr list_slot = irhlAllocVreg( _
		typeAddrOf( FB_DATATYPE_STRUCT ), v1->subtype )
	dim as IRVREG ptr buffer = irhlAllocVreg( FB_DATATYPE_STRUCT, vr->subtype )
	dim as string alloca_line

	if( lgt <= 0 ) then
		errReportEx( FB_ERRMSG_INTERNAL, "LLVM Cva_Arg zero-sized aggregate" )
		exit sub
	end if
	if( stack_alignment < 8 ) then stack_alignment = 8
	if( stack_alignment > 16 ) then stack_alignment = 16
	buffer_type = "[" + str( iif( lgt > 16, lgt, 16 ) ) + " x i8]"

	alloca_line = string( ctx.indent, TABCHAR ) + hVregToStr( list_slot ) + _
	              " = alloca " + list_type + NEWLINE
	hAppendOutputBuffer( @ctx.proc_allocas, strptr( alloca_line ), len( alloca_line ) )
	alloca_line = string( ctx.indent, TABCHAR ) + hVregToStr( buffer ) + _
	              " = alloca " + buffer_type + NEWLINE
	hAppendOutputBuffer( @ctx.proc_allocas, strptr( alloca_line ), len( alloca_line ) )

	dim as IRVREG ptr before = irhlAllocVreg( FB_DATATYPE_STRUCT, v1->subtype )
	hWriteLine( hVregToStr( before ) + " = load " + list_type + ", " + _
	            list_type + "* " + hVregToStr( v1 ) )
	hWriteLine( "store " + list_type + " " + hVregToStr( before ) + ", " + _
	            list_type + "* " + hVregToStr( list_slot ) )
	hWriteLine( "store " + buffer_type + " zeroinitializer, " + _
	            buffer_type + "* " + hVregToStr( buffer ) + ", align 1" )

	if( lgt <= 16 ) then
		select case as const structure_class
		case KSTRUCT_R
			integer_register_count = 1
			can_use_registers = TRUE
		case KSTRUCT_X
			floating_register_count = 1
			can_use_registers = TRUE
		case KSTRUCT_RR
			integer_register_count = 2
			can_use_registers = TRUE
		case KSTRUCT_RX
			integer_register_count = 1
			floating_register_count = 1
			can_use_registers = TRUE
		case KSTRUCT_XR
			integer_register_count = 1
			floating_register_count = 1
			can_use_registers = TRUE
		case KSTRUCT_XX
			floating_register_count = 2
			can_use_registers = TRUE
		end select
	end if

	if( can_use_registers ) then
		dim as IRVREG ptr gp_field = irhlAllocVreg( _
			typeAddrOf( FB_DATATYPE_ULONG ), NULL )
		dim as IRVREG ptr fp_field = irhlAllocVreg( _
			typeAddrOf( FB_DATATYPE_ULONG ), NULL )
		dim as IRVREG ptr gp_offset = irhlAllocVreg( FB_DATATYPE_ULONG, NULL )
		dim as IRVREG ptr fp_offset = irhlAllocVreg( FB_DATATYPE_ULONG, NULL )
		dim as string register_condition
		hWriteLine( hVregToStr( gp_field ) + " = getelementptr inbounds " + _
		            list_type + ", " + list_type + "* " + hVregToStr( list_slot ) + _
		            ", i32 0, i32 0" )
		hWriteLine( hVregToStr( fp_field ) + " = getelementptr inbounds " + _
		            list_type + ", " + list_type + "* " + hVregToStr( list_slot ) + _
		            ", i32 0, i32 1" )
		hWriteLine( hVregToStr( gp_offset ) + " = load i32, i32* " + _
		            hVregToStr( gp_field ) )
		hWriteLine( hVregToStr( fp_offset ) + " = load i32, i32* " + _
		            hVregToStr( fp_field ) )

		if( integer_register_count > 0 andalso floating_register_count > 0 ) then
			dim as IRVREG ptr gp_available = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
			dim as IRVREG ptr fp_available = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
			dim as IRVREG ptr both_available = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
			hWriteLine( hVregToStr( gp_available ) + " = icmp ule i32 " + _
			            hVregToStr( gp_offset ) + ", " + _
			            str( 48 - integer_register_count * 8 ) )
			hWriteLine( hVregToStr( fp_available ) + " = icmp ule i32 " + _
			            hVregToStr( fp_offset ) + ", " + _
			            str( 176 - floating_register_count * 16 ) )
			hWriteLine( hVregToStr( both_available ) + " = and i1 " + _
			            hVregToStr( gp_available ) + ", " + _
			            hVregToStr( fp_available ) )
			register_condition = hVregToStr( both_available )
		elseif( integer_register_count > 0 ) then
			dim as IRVREG ptr gp_available = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
			hWriteLine( hVregToStr( gp_available ) + " = icmp ule i32 " + _
			            hVregToStr( gp_offset ) + ", " + _
			            str( 48 - integer_register_count * 8 ) )
			register_condition = hVregToStr( gp_available )
		else
			dim as IRVREG ptr fp_available = irhlAllocVreg( FB_DATATYPE_BOOLEAN, NULL )
			hWriteLine( hVregToStr( fp_available ) + " = icmp ule i32 " + _
			            hVregToStr( fp_offset ) + ", " + _
			            str( 176 - floating_register_count * 16 ) )
			register_condition = hVregToStr( fp_available )
		end if
		hWriteLine( "br i1 " + register_condition + ", label %" + _
		            register_label + ", label %" + overflow_label )

		hWriteLabel( strptr( register_label ) )
		dim as integer first_is_float = CInt( _
			(structure_class = KSTRUCT_X) orElse _
			(structure_class = KSTRUCT_XR) orElse _
			(structure_class = KSTRUCT_XX))
		dim as IRVREG ptr first_value = irhlAllocVreg( _
			iif( first_is_float, FB_DATATYPE_DOUBLE, FB_DATATYPE_ULONGINT ), NULL )
		if( first_is_float ) then
			hWriteLine( hVregToStr( first_value ) + " = va_arg " + list_type + "* " + _
			            hVregToStr( list_slot ) + ", double" )
		else
			hWriteLine( hVregToStr( first_value ) + " = va_arg " + list_type + "* " + _
			            hVregToStr( list_slot ) + ", i64" )
		end if
		hCvaArgStoreEightbyte( hVregToStr( buffer ), buffer_type, _
		                       first_value, first_is_float, 0 )

		if( second_bytes > 0 ) then
			dim as integer second_is_float = CInt( _
				(structure_class = KSTRUCT_RX) orElse _
				(structure_class = KSTRUCT_XX))
			dim as IRVREG ptr second_value = irhlAllocVreg( _
				iif( second_is_float, FB_DATATYPE_DOUBLE, FB_DATATYPE_ULONGINT ), NULL )
			if( second_is_float ) then
				hWriteLine( hVregToStr( second_value ) + " = va_arg " + list_type + "* " + _
				            hVregToStr( list_slot ) + ", double" )
			else
				hWriteLine( hVregToStr( second_value ) + " = va_arg " + list_type + "* " + _
				            hVregToStr( list_slot ) + ", i64" )
			end if
			hCvaArgStoreEightbyte( hVregToStr( buffer ), buffer_type, _
			                       second_value, second_is_float, 8 )
		end if
		hWriteLine( "br label %" + done_label )
	else
		hWriteLine( "br label %" + overflow_label )
	end if

	hWriteLabel( strptr( overflow_label ) )
	dim as IRVREG ptr overflow_field = irhlAllocVreg( _
		typeAddrOf( typeAddrOf( FB_DATATYPE_BYTE ) ), NULL )
	dim as IRVREG ptr overflow_pointer = irhlAllocVreg( _
		typeAddrOf( FB_DATATYPE_BYTE ), NULL )
	dim as IRVREG ptr overflow_integer = irhlAllocVreg( FB_DATATYPE_ULONGINT, NULL )
	dim as IRVREG ptr aligned_integer = irhlAllocVreg( FB_DATATYPE_ULONGINT, NULL )
	dim as IRVREG ptr aligned_pointer = irhlAllocVreg( _
		typeAddrOf( FB_DATATYPE_BYTE ), NULL )
	dim as IRVREG ptr next_integer = irhlAllocVreg( FB_DATATYPE_ULONGINT, NULL )
	dim as IRVREG ptr next_pointer = irhlAllocVreg( _
		typeAddrOf( FB_DATATYPE_BYTE ), NULL )
	dim as IRVREG ptr source_udt_pointer = irhlAllocVreg( _
		typeAddrOf( FB_DATATYPE_STRUCT ), vr->subtype )
	dim as IRVREG ptr source_value = irhlAllocVreg( FB_DATATYPE_STRUCT, vr->subtype )
	dim as IRVREG ptr buffer_udt_pointer = irhlAllocVreg( _
		typeAddrOf( FB_DATATYPE_STRUCT ), vr->subtype )
	dim as string udt_pointer_type = result_type + "*"
	hWriteLine( hVregToStr( overflow_field ) + " = getelementptr inbounds " + _
	            list_type + ", " + list_type + "* " + hVregToStr( list_slot ) + _
	            ", i32 0, i32 2" )
	hWriteLine( hVregToStr( overflow_pointer ) + " = load i8*, i8** " + _
	            hVregToStr( overflow_field ) )
	hWriteLine( hVregToStr( overflow_integer ) + " = ptrtoint i8* " + _
	            hVregToStr( overflow_pointer ) + " to i64" )
	if( stack_alignment > 1 ) then
		dim as IRVREG ptr aligned_plus = irhlAllocVreg( FB_DATATYPE_ULONGINT, NULL )
		hWriteLine( hVregToStr( aligned_plus ) + " = add i64 " + _
		            hVregToStr( overflow_integer ) + ", " + _
		            str( stack_alignment - 1 ) )
		hWriteLine( hVregToStr( aligned_integer ) + " = and i64 " + _
		            hVregToStr( aligned_plus ) + ", " + _
		            str( -stack_alignment ) )
	else
		hWriteLine( hVregToStr( aligned_integer ) + " = add i64 " + _
		            hVregToStr( overflow_integer ) + ", 0" )
	end if
	hWriteLine( hVregToStr( aligned_pointer ) + " = inttoptr i64 " + _
	            hVregToStr( aligned_integer ) + " to i8*" )
	hWriteLine( hVregToStr( next_integer ) + " = add i64 " + _
	            hVregToStr( aligned_integer ) + ", " + str( slot_size ) )
	hWriteLine( hVregToStr( next_pointer ) + " = inttoptr i64 " + _
	            hVregToStr( next_integer ) + " to i8*" )
	hWriteLine( "store i8* " + hVregToStr( next_pointer ) + ", i8** " + _
	            hVregToStr( overflow_field ) )
	hWriteLine( hVregToStr( source_udt_pointer ) + " = bitcast i8* " + _
	            hVregToStr( aligned_pointer ) + " to " + udt_pointer_type )
	hWriteLine( hVregToStr( source_value ) + " = load " + result_type + ", " + _
	            udt_pointer_type + " " + hVregToStr( source_udt_pointer ) + _
	            ", align 1" )
	hWriteLine( hVregToStr( buffer_udt_pointer ) + " = bitcast " + _
	            buffer_type + "* " + hVregToStr( buffer ) + " to " + _
	            udt_pointer_type )
	hWriteLine( "store " + result_type + " " + hVregToStr( source_value ) + _
	            ", " + udt_pointer_type + " " + _
	            hVregToStr( buffer_udt_pointer ) + ", align 1" )
	if( can_use_registers ) then
		hWriteLine( "br label %" + done_label )
		hWriteLabel( strptr( done_label ) )
	end if

	dim as IRVREG ptr result_pointer = irhlAllocVreg( _
		typeAddrOf( FB_DATATYPE_STRUCT ), vr->subtype )
	hWriteLine( hVregToStr( result_pointer ) + " = bitcast " + buffer_type + "* " + _
	            hVregToStr( buffer ) + " to " + result_type + "*" )
	hWriteLine( hVregToStr( vr ) + " = load " + result_type + ", " + _
	            result_type + "* " + hVregToStr( result_pointer ) + ", align 1" )

	var after = irhlAllocVreg( FB_DATATYPE_STRUCT, v1->subtype )
	hWriteLine( hVregToStr( after ) + " = load " + list_type + ", " + _
	            list_type + "* " + hVregToStr( list_slot ) )
	hWriteLine( "store " + list_type + " " + hVregToStr( after ) + ", " + _
	            list_type + "* " + hVregToStr( v1 ) )
end sub

private sub _emitMacro _
	( _
		byval op as integer, _
		byval v1 as IRVREG ptr, _
		byval v2 as IRVREG ptr, _
		byval vr as IRVREG ptr _
	)
	'' LLVM's va_list is the platform ABI object, not a pointer to the
	'' next stack argument. The System V x86-64 ABI also saves arguments
	'' passed in registers. LLVM's varargs instructions handle both areas.
	hPrepareValistAddress( v1 )

	select case op
	case AST_OP_VA_ARG
		if( (typeGetDtAndPtrOnly( vr->dtype ) = FB_DATATYPE_STRUCT) andalso _
		    (fbGetCpuFamily( ) = FB_CPUFAMILY_X86_64) andalso _
		    ((env.target.options and FB_TARGETOPT_UNIX) <> 0) ) then
			hCvaArgSystemVStruct( v1, vr )
		else
			'' LLVM 21's x86-64 va_arg lowering expects an alloca-backed
			'' va_list. Copy through a local slot so pointer expressions work too.
			var listtype = hEmitType( FB_DATATYPE_STRUCT, v1->subtype )
			var slot = irhlAllocVreg( typeAddrOf( FB_DATATYPE_STRUCT ), v1->subtype )
			var allocline = string( ctx.indent, TABCHAR ) + hVregToStr( slot ) + _
			    " = alloca " + listtype + NEWLINE
			hAppendOutputBuffer( @ctx.proc_allocas, strptr( allocline ), len( allocline ) )
			var before = irhlAllocVreg( FB_DATATYPE_STRUCT, v1->subtype )
			hWriteLine( hVregToStr( before ) + " = load " + listtype + ", " + _
			    listtype + "* " + hVregToStr( v1 ) )
			hWriteLine( "store " + listtype + " " + hVregToStr( before ) + ", " + _
			    listtype + "* " + hVregToStr( slot ) )
			hWriteLine( hVregToStr( vr ) + " = va_arg " + listtype + "* " + _
			    hVregToStr( slot ) + ", " + hEmitType( vr->dtype, vr->subtype ) )
			var after = irhlAllocVreg( FB_DATATYPE_STRUCT, v1->subtype )
			hWriteLine( hVregToStr( after ) + " = load " + listtype + ", " + _
			    listtype + "* " + hVregToStr( slot ) )
			hWriteLine( "store " + listtype + " " + hVregToStr( after ) + ", " + _
			    listtype + "* " + hVregToStr( v1 ) )
		end if

	case AST_OP_VA_START, AST_OP_VA_END, AST_OP_VA_COPY
		var address = irhlAllocVreg( v1->dtype, v1->subtype )
		*address = *v1
		_setVregDataType( address, typeAddrOf( FB_DATATYPE_BYTE ), NULL )

		select case op
		case AST_OP_VA_START
			builtins(BUILTIN_VA_START).used = TRUE
			hWriteLine( "call void @llvm.va_start.p0i8(i8* " + hVregToStr( address ) + ")" )
		case AST_OP_VA_END
			builtins(BUILTIN_VA_END).used = TRUE
			hWriteLine( "call void @llvm.va_end.p0i8(i8* " + hVregToStr( address ) + ")" )
		case AST_OP_VA_COPY
			hPrepareValistAddress( v2 )
			_setVregDataType( v2, typeAddrOf( FB_DATATYPE_BYTE ), NULL )
			builtins(BUILTIN_VA_COPY).used = TRUE
			hWriteLine( "call void @llvm.va_copy.p0i8.p0i8(i8* " + _
			    hVregToStr( address ) + ", i8* " + hVregToStr( v2 ) + ")" )
		end select
	end select
end sub

private sub _emitDECL( byval sym as FBSYMBOL ptr )
end sub

private sub _emitDBG _
	( _
		byval op as integer, _
		byval proc as FBSYMBOL ptr, _
		byval lnum as integer, _
		ByVal filename As zstring ptr _
	)

	if( op = AST_OP_DBG_LINEINI ) Then
		'' LLVM IR has no #line directive. Keep the source location as a
		'' comment until this backend emits LLVM debug metadata.
		if( filename <> NULL ) then
			hWriteLine( "; line " & lnum & " """ & hReplace( filename, "\", $"\\" ) & """" )
		else
			hWriteLine( "; line " & lnum & " """ & hReplace( env.inf.name, "\", $"\\" ) & """" )
		end if
		ctx.linenum = lnum
	end If

end sub

private sub _emitComment( byval text as zstring ptr )
	hWriteLine( "; " + *text )
end sub

#include once "backend/llvm/ir-llvm-asm.bi"

private sub _emitVarIniBegin( byval sym as FBSYMBOL ptr )
	ctx.varini = *symbGetMangledName( sym )
	ctx.varini += " = "
	var linkage = hGlobalVarLinkage( sym, TRUE )
	if( len(linkage) > 0 ) then ctx.varini += linkage + " "
	if( symbIsThreadLocal( sym ) ) then ctx.varini += "thread_local "
	ctx.varini += "global "
	ctx.varini += hEmitSymType( sym )
	ctx.varini += " "
	ctx.variniscopelevel = 0
	ctx.variniscopes(0).is_array = FALSE
end sub

private sub _emitVarIniEnd( byval sym as FBSYMBOL ptr )
	var storage_alignment = hInteger128StorageAlignment( _
		symbGetType( sym ), symbGetSubtype( sym ) )
	if( storage_alignment > 0 ) then _
		ctx.varini += ", align " + str( storage_alignment )
	hWriteLine( ctx.varini )
	ctx.varini = ""
end sub

private sub hVarIniFieldPadding( byval fld as FBSYMBOL ptr )
	if( ctx.variniscopelevel = 0 ) then exit sub
	var iniscope = @ctx.variniscopes(ctx.variniscopelevel)
	if( iniscope->is_array or (iniscope->has_padding = FALSE) ) then exit sub

	'' String initializer callbacks do not receive their field symbol. Find it
	'' using the number of fields already emitted in this struct scope.
	if( fld = NULL ) then
		if( iniscope->sym = NULL orelse iniscope->sym->subtype = NULL ) then exit sub
		fld = symbUdtGetFirstField( iniscope->sym->subtype )
		for i as longint = 1 to iniscope->emitted_elements
			if( fld = NULL ) then exit sub
			fld = symbUdtGetNextInitableField( fld )
		next
	end if
	if( fld = NULL ) then exit sub
	if( symbIsField( fld ) = FALSE ) then exit sub

	var gap = fld->ofs - iniscope->struct_offset
	if( gap > 0 ) then
		ctx.varini += "[" & gap & " x i8] zeroinitializer, "
	end if
	iniscope->struct_offset = fld->ofs + symbGetRealSize( fld )
end sub

'' For struct initializers, we have to emit each field's dtype in front of each
'' field's initializer expression. E.g.:
''    @myudtvar = global %UDT {i32 1, i32 2}
'' but not for array initializers:
''    @myarray = global [2 x i32] [1, 2]
'' and not at all if at toplevel (i.e. neither struct nor array):
''    @myintvar = global i32 1
private sub hVarIniElementType( byval sym as FBSYMBOL ptr )
	if( (ctx.variniscopelevel > 0) andalso (sym <> NULL) ) then
		'' Can't use hEmitSymType() here in case it's the array and
		'' we're just initializing one of its elements
		if( ctx.variniscopes(ctx.variniscopelevel).is_array ) then
			ctx.varini += ctx.variniscopes(ctx.variniscopelevel).elementtype + " "
		else
			hVarIniFieldPadding( sym )
			ctx.varini += hEmitSymType( sym ) + " "
		end if
	end if
end sub

private sub hVarIniSeparator( )
	if( ctx.variniscopelevel > 0 ) then
		ctx.varini += ", "
		ctx.variniscopes(ctx.variniscopelevel).emitted_elements += 1
	end if
end sub

private sub _emitVarIniI( byval sym as FBSYMBOL ptr, byval value as longint )
	hVarIniElementType( sym )
	var dtype = symbGetType( sym )
	if( symbIsRef( sym ) ) then
		dtype = typeAddrOf( dtype )
	end if

	'' AST stores boolean true as -1, but we emit it as 1 for gcc compatibility
	if( (dtype = FB_DATATYPE_BOOLEAN) and (value <> 0) ) then
		value = 1
	end if

	if( typeIsPtr( dtype ) ) then
		ctx.varini += hEmitPointerConstant( dtype, sym->subtype, value )
	elseif( typeGetSize( dtype ) = 8 ) then
		ctx.varini += hEmitLong( value )
	else
		ctx.varini += hEmitInt( dtype, sym->subtype, value )
	end if

	hVarIniSeparator( )
end sub

private function hEmitInt128Decimal _
	( _
		byval low as ulongint, _
		byval high as ulongint _
	) as string
	dim as ulong chunks(0 to 3)
	chunks(0) = culng( low )
	chunks(1) = culng( low shr 32 )
	chunks(2) = culng( high )
	chunks(3) = culng( high shr 32 )

	dim as string result = ""
	do
		dim as ulongint remainder_value = 0
		for i as integer = 3 to 0 step -1
			dim as ulongint current = (remainder_value shl 32) or chunks(i)
			chunks(i) = culng( current \ 10 )
			remainder_value = current mod 10
		next i

		result = chr( cint(remainder_value) + asc("0") ) + result
		if( (chunks(0) or chunks(1) or chunks(2) or chunks(3)) = 0 ) then exit do
	loop

	function = result
end function

private sub _emitVarIni128( byval sym as FBSYMBOL ptr, byval low as longint, byval high as longint )
	hVarIniElementType( sym )
	dim as ulongint source_low = culngint( low )
	dim as ulongint source_high = culngint( high )
	dim as ulongint magnitude_low = source_low
	dim as ulongint magnitude_high = source_high
	dim as integer is_negative = typeIsSigned( symbGetType( sym ) ) andalso (high < 0)
	if( is_negative ) then
		magnitude_low = 0ull - source_low
		magnitude_high = 0ull - source_high - iif( source_low <> 0, 1ull, 0ull )
		ctx.varini += "-"
	end if
	ctx.varini += hEmitInt128Decimal( magnitude_low, magnitude_high )
	hVarIniSeparator( )
end sub

private sub _emitVarIniF( byval sym as FBSYMBOL ptr, byval value as double )
	hVarIniElementType( sym )
	ctx.varini += hEmitFloat( value, symbGetType( sym ) )
	hVarIniSeparator( )
end sub

'' Add inline conversion instruction, to convert the expression in "s" from rhs
'' type to lhs type, if needed
private sub hMaybeAddConv _
	( _
		byref s as string, _
		byval ldtype as integer, _
		byval lsubtype as FBSYMBOL ptr, _
		byref ltype as string, _
		byval rdtype as integer, _
		byval rsubtype as FBSYMBOL ptr, _
		byval rtype as string _
	)

	if( (ldtype = rdtype) and (lsubtype = rsubtype) ) then
		exit sub
	end if

	var op = hGetConvOpCode( ldtype, rdtype )
	if( op = NULL ) then
		exit sub
	end if

	s = *op + " (" + rtype + " " + s + " to " + ltype + ")"

end sub

private sub _emitVarIniOfs _
	( _
		byval sym as FBSYMBOL ptr, _
		byval rhs as FBSYMBOL ptr, _
		byval ofs as longint _
	)

	hVarIniElementType( sym )

	var s = *symbGetMangledName( rhs )
	if( symbIsProc( rhs ) ) then
		hCheckBuiltinAddress( rhs )
		s = hEmitProcName( rhs )
	end if
	var symdtype = symbGetType( sym )
	if( symbIsRef( sym ) ) then
		'' A BYREF variable stores an address, not a value of its source type.
		symdtype = typeAddrOf( symdtype )
	end if
	var symtype = hEmitType( symdtype, sym->subtype )
	var ptrdtype = typeAddrOf( symbGetType( rhs ) )
	var ptrsubtype = rhs->subtype
	if( symbIsProc( rhs ) ) then
		ptrdtype = typeAddrOf( FB_DATATYPE_FUNCTION )
		ptrsubtype = rhs
	end if
	var ptrtype = hEmitType( ptrdtype, ptrsubtype )

	if( ofs <> 0 ) then
		var inttype = hEmitType( FB_DATATYPE_UINT, NULL )

		'' Emit inline LLVM instructions for something like: cptr(cint(@rhs) + ofs)

		'' %0 = ptrtoint foo* @global to i32
		s = "ptrtoint (" + ptrtype + " " + s + " to " + inttype + ")"

		'' %1 = add (i32 %0), i32 <offset>
		s = "add (" + inttype + " " + s + ", " + inttype + " " & ofs & ")"

		'' %2 = inttoptr (i32 %1) to foo*
		s = "inttoptr (" + inttype + " " + s + " to " + ptrtype + ")"
	end if

	hMaybeAddConv( s, symdtype, sym->subtype, symtype, ptrdtype, ptrsubtype, ptrtype )

	ctx.varini += s

	hVarIniSeparator( )
end sub

private sub _emitVarIniStr _
	( _
		byval varlength as longint, _
		byval literal as zstring ptr, _
		byval litlength as longint, _
		byval noterm as integer _
	)

	'' Fixed-length strings have no terminator; CHAR arrays reserve one.
	var wantedlength = varlength + iif( noterm, 0, 1 )
	dim as integer decodedlength
	var decoded = hUnescape( literal, decodedlength )
	if( decodedlength > litlength ) then decodedlength = litlength
	var inputlength = iif( decodedlength > varlength, varlength, _
	    decodedlength + iif( noterm, 0, 1 ) )
	hVarIniFieldPadding( NULL )
	if( ctx.variniscopelevel > 0 ) then
		ctx.varini += hEmitStrLitType( wantedlength ) + " "
	end if
	ctx.varini += "c"""
	hBuildStrLit( ctx.varini, wantedlength, decoded, _
	    inputlength, iif( noterm, asc( " " ), 0 ) )
	ctx.varini += """"
	hVarIniSeparator( )

end sub

private sub _emitVarIniWstr _
	( _
		byval varlength as longint, _
		byval literal as wstring ptr, _
		byval litlength as longint _
	)

	'' WSTRING lengths are in characters; LLVM represents them as bytes.
	var wantedlength = (varlength + 1) * typeGetSize( env.target.wchar )
	hVarIniFieldPadding( NULL )
	if( ctx.variniscopelevel > 0 ) then
		ctx.varini += hEmitStrLitType( wantedlength ) + " "
	end if
	ctx.varini += "c"""
	dim as integer decodedlength
	var decoded = hUnescapeW( literal, decodedlength )
	if( decodedlength > litlength ) then decodedlength = litlength
	var inputlength = iif( decodedlength > varlength, varlength, decodedlength + 1 )
	hBuildWstrLit( ctx.varini, varlength + 1, decoded, inputlength )
	ctx.varini += """"
	hVarIniSeparator( )

end sub

private sub _emitVarIniPad( byval bytes as longint, byval fillchar as integer )
end sub

private sub _emitVarIniScopeBegin( byval sym as FBSYMBOL ptr, byval is_array as integer )
	'' If starting a nested initializer we have to emit its type (it will
	'' be either a struct type or an array type).
	hVarIniElementType( sym )

	'' A TYPEINI scope for an array can be marked as a struct, including
	'' when the array is a field in another struct. Use an LLVM array
	'' initializer until all dimensions have been entered; later scopes
	'' initialize an element's UDT fields.
	if( is_array = FALSE andalso sym andalso (symbGetArrayDimensions( sym ) > 0) ) then
		var array_scopes = 0
		for i as integer = 1 to ctx.variniscopelevel
			if( ctx.variniscopes(i).is_array andalso (ctx.variniscopes(i).sym = sym) ) then
				array_scopes += 1
			end if
		next
		if( array_scopes < symbGetArrayDimensions( sym ) ) then
			is_array = TRUE
		end if
	end if

	dim as integer dimension = 0
	if( is_array ) then
		'' Nested array scopes for the same symbol advance one dimension.
		for i as integer = 1 to ctx.variniscopelevel
			if( ctx.variniscopes(i).is_array andalso (ctx.variniscopes(i).sym = sym) ) then
				dimension += 1
			end if
		next
	end if

	'' Add varini scope to stack
	ctx.variniscopelevel += 1
	if( ctx.variniscopelevel >= MAXVARINISCOPES ) then
		errReport( FB_ERRMSG_INTERNAL, , "global variable/array initializer nesting level too deep (MAXVARINISCOPES=" & MAXVARINISCOPES & ")" )
		ctx.variniscopelevel -= 1
	end if
	ctx.variniscopes(ctx.variniscopelevel).is_array = is_array
	ctx.variniscopes(ctx.variniscopelevel).is_packed = FALSE
	ctx.variniscopes(ctx.variniscopelevel).has_padding = FALSE
	ctx.variniscopes(ctx.variniscopelevel).sym = sym
	ctx.variniscopes(ctx.variniscopelevel).expected_elements = 0
	ctx.variniscopes(ctx.variniscopelevel).emitted_elements = 0
	ctx.variniscopes(ctx.variniscopelevel).struct_offset = 0
	ctx.variniscopes(ctx.variniscopelevel).elementtype = ""

	if( is_array ) then
		if( (sym <> NULL) andalso (dimension < symbGetArrayDimensions( sym )) ) then
			var low = symbArrayLbound( sym, dimension )
			var high = symbArrayUbound( sym, dimension )
			if( (high <> FB_ARRAYDIM_UNKNOWN) andalso (high >= low) ) then
				ctx.variniscopes(ctx.variniscopelevel).expected_elements = high - low + 1
			end if

			select case as const( symbGetType( sym ) )
			case FB_DATATYPE_FIXSTR, FB_DATATYPE_CHAR, FB_DATATYPE_WCHAR
				ctx.variniscopes(ctx.variniscopelevel).elementtype = hEmitStrLitType( sym->lgt )
			case else
				ctx.variniscopes(ctx.variniscopelevel).elementtype = _
					hEmitType( symbGetType( sym ), sym->subtype )
			end select

			for i as integer = symbGetArrayDimensions( sym ) - 1 to dimension + 1 step -1
				var elements = symbArrayUbound( sym, i ) - symbArrayLbound( sym, i ) + 1
				ctx.variniscopes(ctx.variniscopelevel).elementtype = "[" & elements & " x " + _
					ctx.variniscopes(ctx.variniscopelevel).elementtype + "]"
			next
		end if
		ctx.varini += "[ "
		ctx.variniscopes(ctx.variniscopelevel).openlength = 2
	else
		'' A packed LLVM struct requires a packed initializer too.
		if( sym andalso sym->subtype andalso symbIsStruct( sym->subtype ) ) then
			ctx.variniscopes(ctx.variniscopelevel).is_packed = _
				hUDTUsesPackedLayout( sym->subtype )
			ctx.variniscopes(ctx.variniscopelevel).has_padding = _
				hUDTHasExplicitPadding( sym->subtype )
		end if
		if( ctx.variniscopes(ctx.variniscopelevel).is_packed ) then
			ctx.varini += "<{ "
			ctx.variniscopes(ctx.variniscopelevel).openlength = 3
		else
			ctx.varini += "{ "
			ctx.variniscopes(ctx.variniscopelevel).openlength = 2
		end if
	end if
	ctx.variniscopes(ctx.variniscopelevel).contentstart = len( ctx.varini )
end sub

private sub _emitVarIniScopeEnd( )
	if( ctx.variniscopes(ctx.variniscopelevel).is_array andalso _
	    (ctx.variniscopes(ctx.variniscopelevel).emitted_elements > 0) andalso _
	    (errGetCount( ) = 0) ) then
		'' LLVM array constants require an entry for every element. TYPEINI
		'' padding nodes can leave the trailing elements implicit in C only.
		for i as longint = ctx.variniscopes(ctx.variniscopelevel).emitted_elements to _
			ctx.variniscopes(ctx.variniscopelevel).expected_elements - 1
			ctx.varini += ctx.variniscopes(ctx.variniscopelevel).elementtype + _
				" zeroinitializer, "
			next
	elseif( ctx.variniscopes(ctx.variniscopelevel).is_array = FALSE ) then
		var sym = ctx.variniscopes(ctx.variniscopelevel).sym
		if( sym andalso sym->subtype andalso symbIsStruct( sym->subtype ) ) then
			'' LLVM struct constants must include the trailing fields even when
			'' the TYPEINI tree leaves their zero initialization implicit.
			var fld = symbUdtGetFirstField( sym->subtype )
			var emitted = ctx.variniscopes(ctx.variniscopelevel).emitted_elements
			while( fld )
				if( symbIsDynamic( fld ) = FALSE ) then
					if( emitted > 0 ) then
						emitted -= 1
					else
						hVarIniElementType( fld )
						ctx.varini += "zeroinitializer, "
					end if
				end if
				fld = symbUdtGetNextInitableField( fld )
			wend
			if( ctx.variniscopes(ctx.variniscopelevel).has_padding ) then
				var gap = sym->subtype->lgt - _
					ctx.variniscopes(ctx.variniscopelevel).struct_offset
				if( gap > 0 ) then
					ctx.varini += "[" & gap & " x i8] zeroinitializer, "
				end if
			end if
			var alignmenttype = hUDTAlignmentType( sym->subtype )
			if( len( alignmenttype ) > 0 ) then
				ctx.varini += alignmenttype + " zeroinitializer, "
			end if
		end if
	end if

	if( len( ctx.varini ) = ctx.variniscopes(ctx.variniscopelevel).contentstart ) then
		'' LLVM requires an aggregate constant; empty braces are not one.
		ctx.varini = left( ctx.varini, len( ctx.varini ) - _
			ctx.variniscopes(ctx.variniscopelevel).openlength ) + "zeroinitializer"
		ctx.variniscopelevel -= 1
		hVarIniSeparator( )
		exit sub
	end if

	'' Trim separator at the end, to make the output look a bit more clean
	'' (this isn't needed though, since the extra comma is allowed in C)
	if( right( ctx.varini, 2 ) = ", " ) then
		ctx.varini = left( ctx.varini, len( ctx.varini ) - 2 )
	end if

	if( ctx.variniscopes(ctx.variniscopelevel).is_array ) then
		ctx.varini += " ]"
	elseif( ctx.variniscopes(ctx.variniscopelevel).is_packed ) then
		ctx.varini += " }>"
	else
		ctx.varini += " }"
	end if

	'' Pop varini scope from stack
	'' (due to _emitVarIniScopeBegin's MAXVARINISCOPES handling there could
	'' be a count misbalance during error recovery)
	if( ctx.variniscopelevel > 0 ) then
		ctx.variniscopelevel -= 1
	end if

	hVarIniSeparator( )
end sub

private sub _emitFbctinfBegin( )
	hWriteLine( "" )
end sub

private sub _emitFbctinfString( byval s as const zstring ptr )
	ctx.fbctinf += *s + $"\00"
	ctx.fbctinf_len += len( *s ) + 1
end sub

private sub _emitFbctinfEnd( )
	dim as string ln

	'' This is based on the LLVM IR code generated by clang for a:
	'' static const char __attribute__((used, section(".fbctinf"))) __fbctinf[] = "...";

	'' internal  - private
	'' constant  - read-only
	'' section   - This global must be put into a custom .fbctinf section,
	''             as done by the ASM backend.
	ln = "@__fbctinf = internal constant "
	ln += hEmitStrLitType( ctx.fbctinf_len )
	ln += " c""" + ctx.fbctinf + """"
	if (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DARWIN) then
		'' Must specify a segment name (can use any name)
		ln += ", section ""__DATA," + FB_INFOSEC_NAME + """"
	else
		ln += ", section ""." + FB_INFOSEC_NAME + """"
	end if
	hWriteLine( ln )

	'' Append to the special llvm.used symbol to ensure it won't be
	'' optimized out:
	ln = "@llvm.used = appending global [1 x i8*] "
	ln += "["
	ln += "i8* bitcast (" + hEmitStrLitType( ctx.fbctinf_len ) + "* @__fbctinf to i8*)"
	ln += "]"
	ln += ", section ""llvm.metadata"""
	hWriteLine( ln )

	ctx.fbctinf = ""
	ctx.fbctinf_len = 0
end sub

private sub _emitProcBegin _
	( _
		byval proc as FBSYMBOL ptr, _
		byval initlabel as FBSYMBOL ptr _
	)

	irhlEmitProcBegin( )
	hAddGlobalCtorDtor( proc )
	var procname = *symbGetMangledName( proc )
	strsetAdd( @ctx.definedprocs, procname, 0 )
	ctx.proc_body_offset = ctx.body_txt.bufferlen
	ctx.proc_labels = ""
	ctx.proc_has_indirectbr = FALSE

	hWriteLine( "" )

	dim as string ln

	ln += "define "
	if( symbIsExport( proc ) ) then
		ln += "dllexport "
	elseif( symbIsPrivate( proc ) ) then
		ln += "private "
		''ln += "internal "
	end if
	ln += hEmitProcHeader( proc, FALSE, FALSE )

	hWriteLine( ln )

	hWriteLine( "{" )
	ctx.indent += 1
	ctx.proc_entry_offset = ctx.body_txt.bufferlen
	ctx.proc_allocas.bufferlen = 0
	ctx.current_proc = proc
	ctx.inproc = TRUE

end sub

private sub _emitProcEnd _
	( _
		byval proc as FBSYMBOL ptr, _
		byval initlabel as FBSYMBOL ptr, _
		byval exitlabel as FBSYMBOL ptr _
	)

	'' Naked assembly may return directly, but LLVM still needs a terminator
	'' for the basic block that follows the assembly in the IR.
	if( proc->pattrib and FB_PROCATTRIB_NAKED ) then
		hWriteLine( "unreachable" )
	elseif( hIsI386Integer128Result( proc ) ) then
		hWriteLine( "ret void" )
	'' Sub? Add ret manually, the AST doesn't do a LOAD[RES] for this
	elseif( symbGetType( proc ) = FB_DATATYPE_VOID ) then
		hWriteLine( "ret void" )
	end if

	ctx.indent -= 1
	hWriteLine( "}" )
	ctx.current_proc = NULL
	ctx.inproc = FALSE
	if( ctx.proc_has_indirectbr ) then
		'' Label addresses may be emitted after their indirect branches.
		var bodylength = ctx.body_txt.bufferlen - ctx.proc_body_offset
		var body = space( bodylength )
		memcpy( strptr( body ), ctx.body_txt.buffer + ctx.proc_body_offset, bodylength )
		body = hReplace( body, "@FB_INDIRECT_DESTINATIONS@", ctx.proc_labels )
		ctx.body_txt.bufferlen = ctx.proc_body_offset
		hAppendOutputBuffer( @ctx.body_txt, strptr( body ), len( body ) )
	end if

	'' Insert saved allocas directly after the opening brace. They cannot be
	'' emitted at their source location when a GOTO crosses that declaration.
	var bytes = ctx.proc_allocas.bufferlen
	if( bytes > 0 ) then
		var oldlen = ctx.body_txt.bufferlen
		hAppendOutputBuffer( @ctx.body_txt, ctx.proc_allocas.buffer, bytes )
		if( ctx.body_txt.bufferlen = oldlen + bytes ) then
			var tailbytes = oldlen - ctx.proc_entry_offset
			memmove( ctx.body_txt.buffer + ctx.proc_entry_offset + bytes, _
			         ctx.body_txt.buffer + ctx.proc_entry_offset, tailbytes )
			memcpy( ctx.body_txt.buffer + ctx.proc_entry_offset, ctx.proc_allocas.buffer, bytes )
		end if
	end if
	ctx.proc_allocas.bufferlen = 0

	irhlEmitProcEnd( )

end sub

private sub _emitScopeBegin( byval s as FBSYMBOL ptr )
end sub

private sub _emitScopeEnd( byval s as FBSYMBOL ptr )
end sub

static as IR_VTBL irllvm_vtbl = _
( _
	@_init, _
	@_end, _
	@_emitBegin, _
	@_emitEnd, _
	@_getOptionValue, _
	@_supportsOp, _
	@_procBegin, _
	@_procEnd, _
	@_procAllocArg, _
	@_procAllocLocal, _
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

'' end of backend/llvm/ir-llvm.bas
