'' Project: FreeBASIC compiler - BASIC intrinsic syntax
'' -----------------------------------------
''
'' File: parser/intrinsics/parser-quirk.bas
''
'' Purpose:
''
''     Dispatch intrinsic statements and functions with BASIC-specific syntax.
''
'' Responsibilities:
''
''     - parse the intrinsic statement or function family described below
''     - validate arguments before building AST and runtime calls
''
'' This file intentionally does NOT contain:
''
''     - the runtime implementation of those intrinsics
''

'' quirk statements (ON, OPEN, PRINT, ...) parsing top-level
''
'' chng: sep/2004 written [v1ctor]


#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "tooling/semantic-extra-facts.bi"
#include once "parser/parser.bi"
#include once "ast/ast.bi"
#include once "tooling/semantic-hooks.bi"
#include once "tooling/semantic-intrinsics.bi"

declare sub fbSemanticModelBuiltinFunctionUse(byval sym as FBSYMBOL ptr, byref source as LEX_LOCATION)
declare function fbSemanticModelFullEnabled() as integer
declare function fbSemanticModelMacroProvenanceEnabled() as integer


#macro CHECK_CODEMASK( )
	if( cCompStmtIsAllowed( FB_CMPSTMT_MASK_CODE ) = FALSE ) then
		hSkipStmt( )
		exit function
	end if
#endmacro

'':::::
''QuirkStmt       =   GotoStmt
''                |   ArrayStmt
''                |   PrintStmt
''                |   MidStmt
''                |   DataStmt
''                |   etc .
''
function cQuirkStmt _
	( _
		byval tk as FB_TOKEN = INVALID _
	) as integer

	function = FALSE
	dim as SEMANTIC_INTRINSIC_SITE semantic_dispatch
	if( tk <> INVALID ) then fbSemanticModelIntrinsicBegin(semantic_dispatch)
	dim as string semantic_route
	dim as integer res = FALSE

	if( tk = INVALID ) then
		tk = lexGetToken( )
		fbSemanticModelIntrinsicBegin(semantic_dispatch)

		select case lexGetClass( )
		case FB_TKCLASS_KEYWORD, FB_TKCLASS_QUIRKWD
			'' QB mode?
			if( env.clopt.lang = FB_LANG_QB ) then
				if( lexGetType() <> FB_DATATYPE_INVALID ) then
					return FALSE
				end if
			end if

		case else
			'' PRINT as '?', can't be a keyword..
			if( tk = CHAR_QUESTION ) then
				CHECK_CODEMASK( )
				res = cPrintStmt( tk )
				fbSemanticModelIntrinsicEnd(semantic_dispatch, "statement", "cPrintStmt", tk, res)
				function = res
			end if
			exit function
		end select
	end if

	select case as const tk
	case FB_TK_GOTO, FB_TK_GOSUB, FB_TK_RETURN, FB_TK_RESUME
		CHECK_CODEMASK( )
		semantic_route = "cGotoStmt"
		res = cGotoStmt( tk )

	case FB_TK_PRINT, FB_TK_LPRINT
		CHECK_CODEMASK( )
		semantic_route = "cPrintStmt"
		res = cPrintStmt( tk )

	case FB_TK_RESTORE, FB_TK_READ, FB_TK_DATA
		CHECK_CODEMASK( )
		semantic_route = "cDataStmt"
		res = cDataStmt( tk )

	case FB_TK_ERASE
		CHECK_CODEMASK( )
		semantic_route = "cEraseStmt"
		res = cEraseStmt()

	case FB_TK_SWAP
		CHECK_CODEMASK( )
		semantic_route = "cSwapStmt"
		res = cSwapStmt()

	case FB_TK_LINE
		CHECK_CODEMASK( )
		semantic_route = "cLineInputStmt"
		res = cLineInputStmt( )

	case FB_TK_INPUT
		CHECK_CODEMASK( )
		semantic_route = "cInputStmt"
		res = cInputStmt( )

	case FB_TK_POKE
		CHECK_CODEMASK( )
		semantic_route = "cPokeStmt"
		res = cPokeStmt( )

	case FB_TK_OPEN, FB_TK_CLOSE, FB_TK_SEEK, FB_TK_PUT, FB_TK_GET, _
		 FB_TK_LOCK, FB_TK_UNLOCK, FB_TK_NAME
		CHECK_CODEMASK( )
		semantic_route = "cFileStmt"
		res = cFileStmt( tk )

	case FB_TK_ON
		CHECK_CODEMASK( )
		semantic_route = "cOnStmt"
		res = cOnStmt( )

	case FB_TK_WRITE
		CHECK_CODEMASK( )
		semantic_route = "cWriteStmt"
		res = cWriteStmt()

	case FB_TK_ERROR
		CHECK_CODEMASK( )
		semantic_route = "cErrorStmt"
		res = cErrorStmt()

	case FB_TK_ERR
		CHECK_CODEMASK( )
		semantic_route = "cErrSetStmt"
		res = cErrSetStmt()

	case FB_TK_VIEW
		CHECK_CODEMASK( )
		semantic_route = "cViewStmt"
		res = (cViewStmt(FALSE) <> NULL)

	case FB_TK_MID
		CHECK_CODEMASK( )
		semantic_route = "cMidStmt"
		res = cMidStmt( )

	case FB_TK_LSET, FB_TK_RSET
		CHECK_CODEMASK( )
		semantic_route = "cLRSetStmt"
		res = cLRSetStmt( tk )

	case FB_TK_WIDTH
		CHECK_CODEMASK( )
		semantic_route = "cWidthStmt"
		res = cWidthStmt( FALSE ) <> NULL

	case FB_TK_COLOR
		CHECK_CODEMASK( )
		semantic_route = "cColorStmt"
		res = cColorStmt( FALSE ) <> NULL

	case FB_TK_REM
		'' due the QB quirks..
		if( env.clopt.lang = FB_LANG_QB ) then
			semantic_route = "cComment"
			res = cComment( )
		end if

	case FB_TK_CVA_START, FB_TK_CVA_END, FB_TK_CVA_COPY
		CHECK_CODEMASK( )
		semantic_route = "cVALISTStmt"
		res = cVALISTStmt( tk )

	end select

	fbSemanticModelIntrinsicEnd(semantic_dispatch, "statement", semantic_route, tk, res)

	if( res = FALSE ) then
		res = cSfxStmt( tk )
	end if

	if( res = FALSE ) then
		res = cGfxStmt( tk )
	end if

	function = res

end function

'':::::
''QuirkFunction =   QBFUNCTION ('(' ProcParamList ')')? .
''
function cQuirkFunction(byval sym as FBSYMBOL ptr) as ASTNODE ptr
	dim as ASTNODE ptr funcexpr = NULL
	dim as FB_TOKEN tk = sym->key.id

	''
	'' Allow quirk function names to be used as literals in PP expressions,
	'' if not followed by '(', or '<' (for type<...>(...)).
	''
	'' There are some cases of quirk functions below that can be called
	'' without ()'s (ERR, THREADCALL, SCREEN) but none of them make sense
	'' in PP expressions anyways, as they can't be evaluated at
	'' compile-time, so there's no harm done disallowing them to be used.
	''
	if( fbGetIsPP( ) ) then
		if( (lexGetLookAhead( 1 ) <> CHAR_LPRNT) and _
		    ((tk <> FB_TK_TYPE) or (lexGetLookAhead( 1 ) <> FB_TK_LT)) ) then
			funcexpr = astNewCONSTstr( ucase( *lexGetText( ) ) )
			lexSkipToken( )
			return funcexpr
		end if
	end if


	'' Keep the accepted keyword's own lexer range before parsing operands.
	'' PP literal-name handling above deliberately returns without an invocation.
	dim as LEX_LOCATION semantic_site
	dim as integer semantic_observe = fbSemanticExtraFactsEnabled(FB_SEMEXTRA_BUILTINS) and _
		fbSemanticModelFullEnabled( ) and _
		fbSemanticModelMacroProvenanceEnabled( ) and (lex.ctx->semantic_probe = FALSE)
	if( semantic_observe ) then semantic_site = lexGetCurrentLocation( )

	dim as SEMANTIC_INTRINSIC_SITE semantic_dispatch
	fbSemanticModelIntrinsicBegin(semantic_dispatch)
	dim as string semantic_route

	select case as const tk
	case FB_TK_SERIALIZE, FB_TK_DESERIALIZE
		semantic_route = "cSerializationFunct"
		funcexpr = cSerializationFunct( tk )

	case FB_TK_MKD, FB_TK_MKS, FB_TK_MKI, FB_TK_MKL, _
	     FB_TK_MKSHORT, FB_TK_MKLONGINT
		semantic_route = "cMKXFunct"
		funcexpr = cMKXFunct(tk)

	case FB_TK_CVD, FB_TK_CVS, FB_TK_CVI, FB_TK_CVL, _
	     FB_TK_CVSHORT, FB_TK_CVLONGINT
		semantic_route = "cCVXFunct"
		funcexpr = cCVXFunct(tk)

	case FB_TK_STR, FB_TK_WSTR, FB_TK_USTRING, FB_TK_UCHR, _
	     FB_TK_MID, FB_TK_STRING, FB_TK_WSTRING, _
	     FB_TK_CHR, FB_TK_WCHR, _
	     FB_TK_ASC, FB_TK_INSTR, FB_TK_INSTRREV, _
	     FB_TK_TRIM, FB_TK_RTRIM, FB_TK_LTRIM, _
	     FB_TK_LCASE, FB_TK_UCASE
		semantic_route = "cStringFunct"
		funcexpr = cStringFunct(tk)

	case FB_TK_ABS, FB_TK_SGN, FB_TK_FIX, FB_TK_FRAC, FB_TK_LEN, FB_TK_SIZEOF, _
		 FB_TK_SIN, FB_TK_ASIN, FB_TK_COS, FB_TK_ACOS, FB_TK_TAN, FB_TK_ATN, _
		 FB_TK_SQR, FB_TK_LOG, FB_TK_EXP, FB_TK_ATAN2, FB_TK_INT
		semantic_route = "cMathFunct"
		funcexpr = cMathFunct(tk, FALSE)

	case FB_TK_PEEK
		semantic_route = "cPeekFunct"
		funcexpr = cPeekFunct()

	case FB_TK_LBOUND, FB_TK_UBOUND
		semantic_route = "cArrayFunct"
		funcexpr = cArrayFunct(tk)

	case FB_TK_SEEK, FB_TK_INPUT, FB_TK_WINPUT, FB_TK_OPEN, FB_TK_CLOSE, _
	     FB_TK_GET, FB_TK_PUT, FB_TK_NAME
		semantic_route = "cFileFunct"
		funcexpr = cFileFunct(tk)

	case FB_TK_ERR
		semantic_route = "cErrorFunct"
		funcexpr = cErrorFunct()

	case FB_TK_IIF
		semantic_route = "cStrIdxOrMemberDeref"
		funcexpr = cStrIdxOrMemberDeref( cIIFFunct( ) )
		goto quirk_function_accepted

	case FB_TK_VA_FIRST
		semantic_route = "cVAFunct"
		funcexpr = cVAFunct()

	case FB_TK_CVA_ARG
		semantic_route = "cVALISTFunct"
		funcexpr = cVALISTFunct( tk )

	case FB_TK_CBYTE, FB_TK_CSHORT, FB_TK_CINT, FB_TK_CLNG, FB_TK_CLNGINT, _
	     FB_TK_CUBYTE, FB_TK_CUSHORT, FB_TK_CUINT, FB_TK_CULNG, FB_TK_CULNGINT, _
	     FB_TK_CSNG, FB_TK_CDBL, FB_TK_CSIGN, FB_TK_CUNSG, FB_TK_CBOOL
		semantic_route = "cTypeConvExpr"
		funcexpr = cTypeConvExpr( tk )
		goto quirk_function_accepted

	case FB_TK_TYPE
		semantic_route = "cStrIdxOrMemberDeref"
		funcexpr = cStrIdxOrMemberDeref( cAnonType( ) )
		goto quirk_function_accepted

	case FB_TK_VIEW
		semantic_route = "cViewStmt"
		funcexpr = cViewStmt(TRUE)

	case FB_TK_WIDTH
		semantic_route = "cWidthStmt"
		funcexpr = cWidthStmt( TRUE )

	case FB_TK_COLOR
		semantic_route = "cColorStmt"
		funcexpr = cColorStmt( TRUE )

	case FB_TK_SCREEN, FB_TK_SCREENQB
		semantic_route = "cScreenFunct"
		funcexpr = cScreenFunct()

	case FB_TK_THREADCALL
		semantic_route = "cThreadCallFunc"
		funcexpr = cThreadCallFunc()

	case FB_TK_TCP
		semantic_route = "cFileFunct"
		funcexpr = cFileFunct( tk )

	end select

	fbSemanticModelIntrinsicEnd(semantic_dispatch, "function", semantic_route, tk, funcexpr <> NULL, funcexpr)
	semantic_route = ""

	if( funcexpr = NULL ) then
		funcexpr = cSfxFunct( tk )
	end if

	if( funcexpr = NULL ) then
		funcexpr = cGfxFunct( tk )
	end if

quirk_function_accepted:
	fbSemanticModelIntrinsicEnd(semantic_dispatch, "function", semantic_route, tk, funcexpr <> NULL, funcexpr)
	if( semantic_observe and (funcexpr <> NULL) ) then fbSemanticModelBuiltinFunctionUse(sym, semantic_site)
	function = funcexpr
end function

'' end of parser/intrinsics/parser-quirk.bas
