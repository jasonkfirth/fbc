'' Project: FreeBASIC compiler - semantic configuration contexts
'' File: tooling/semantic-context.bas
'' Purpose: Preserve effective compiler configuration and its committed changes.
'' Responsibilities: Observe option snapshots and attach facts to their context.
'' This file intentionally does NOT contain: option parsing or policy decisions.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-constructs.bi"
#include once "crt/mem.bi"

'' Compiler modules execute serially. These snapshots contain only scalar
'' compiler fields and are reset for each parse attempt. They never change
'' options or call preprocessor callbacks. Byte comparison avoids formatting
'' every option for each token when the effective configuration is unchanged.
dim shared as integer semantic_context_valid
dim shared as longint semantic_context_id
dim shared as FBCMMLINEOPT semantic_context_compiler
dim shared as FBOPTION semantic_context_defaults
dim shared as FB_LANG_CTX semantic_context_language
dim shared as integer semantic_context_remkeyword_active

sub fbSemanticModelResetContext( )
	semantic_context_valid = FALSE
	semantic_context_id = 0
	'' The native keyword table installs REM in every dialect at module start.
	'' OPTION NOKEYWORD subsequently removes that actual keyword symbol.
	semantic_context_remkeyword_active = TRUE
end sub

sub fbSemanticModelKeywordRemoved( byval tokenid as integer )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	if( tokenid <> FB_TK_REM ) then exit sub
	'' Hash removal does not change FBOPTION. Invalidate the cached snapshot
	'' explicitly, without looking up names through the lexer's chain scratch.
	semantic_context_remkeyword_active = FALSE
	semantic_context_valid = FALSE
end sub

private sub hOption(byref domain as const string, byref key as const string, byval value as longint)
	fbSemanticModelAppendDetail("OPT" + TABCHAR + fbSemanticModelNumber(semantic_context_id) + _
		TABCHAR + domain + TABCHAR + key + TABCHAR + fbSemanticModelNumber(value))
end sub

function fbSemanticModelCurrentContext( ) as longint
	if( fbSemanticModelFullEnabled( ) = FALSE ) then return 0
	if( semantic_context_valid ) then
		if( (memcmp(@semantic_context_compiler, @env.clopt, sizeof(FBCMMLINEOPT)) = 0) and _
			(memcmp(@semantic_context_defaults, @env.opt, sizeof(FBOPTION)) = 0) and _
			(memcmp(@semantic_context_language, @env.lang, sizeof(FB_LANG_CTX)) = 0) ) then
			return semantic_context_id
		end if
	end if
	semantic_context_compiler = env.clopt
	semantic_context_defaults = env.opt
	semantic_context_language = env.lang
	semantic_context_id = fbSemanticModelNextDetailIdentity( )
	semantic_context_valid = TRUE
	fbSemanticModelAppendDetail("CTX" + TABCHAR + fbSemanticModelNumber(semantic_context_id) + _
		TABCHAR + fbSemanticModelNumber(fbSemanticModelModuleIdentity( )))
	hOption("compiler", "outtype", env.clopt.outtype)
	hOption("compiler", "pponly", env.clopt.pponly)
	hOption("compiler", "backend", env.clopt.backend)
	hOption("compiler", "target", env.clopt.target)
	hOption("compiler", "cputype", env.clopt.cputype)
	hOption("compiler", "fputype", env.clopt.fputype)
	hOption("compiler", "fpmode", env.clopt.fpmode)
	hOption("compiler", "vectorize", env.clopt.vectorize)
	hOption("compiler", "optlevel", env.clopt.optlevel)
	hOption("compiler", "asmsyntax", env.clopt.asmsyntax)
	hOption("compiler", "lang", env.clopt.lang)
	hOption("compiler", "forcelang", env.clopt.forcelang)
	hOption("compiler", "debug", env.clopt.debug)
	hOption("compiler", "debuginfo", env.clopt.debuginfo)
	hOption("compiler", "assertions", env.clopt.assertions)
	hOption("compiler", "errorcheck", env.clopt.errorcheck)
	hOption("compiler", "resumeerr", env.clopt.resumeerr)
	hOption("compiler", "extraerrchk", env.clopt.extraerrchk)
	hOption("compiler", "errlocation", env.clopt.errlocation)
	hOption("compiler", "arrayboundchk", env.clopt.arrayboundchk)
	hOption("compiler", "arraydimschk", env.clopt.arraydimschk)
	hOption("compiler", "nullptrchk", env.clopt.nullptrchk)
	hOption("compiler", "unwindinfo", env.clopt.unwindinfo)
	hOption("compiler", "profile", env.clopt.profile)
	hOption("compiler", "warninglevel", env.clopt.warninglevel)
	hOption("compiler", "showerror", env.clopt.showerror)
	hOption("compiler", "maxerrors", env.clopt.maxerrors)
	hOption("compiler", "pdcheckopt", env.clopt.pdcheckopt)
	hOption("compiler", "gosubsetjmp", env.clopt.gosubsetjmp)
	hOption("compiler", "valistasptr", env.clopt.valistasptr)
	hOption("compiler", "nothiscall", env.clopt.nothiscall)
	hOption("compiler", "nofastcall", env.clopt.nofastcall)
	hOption("compiler", "fbrt", env.clopt.fbrt)
	hOption("compiler", "export", env.clopt.export)
	hOption("compiler", "msbitfields", env.clopt.msbitfields)
	hOption("compiler", "multithreaded", env.clopt.multithreaded)
	hOption("compiler", "fbgfx", env.clopt.fbgfx)
	hOption("compiler", "fbsfx", env.clopt.fbsfx)
	hOption("compiler", "pic", env.clopt.pic)
	hOption("compiler", "stacksize", env.clopt.stacksize)
	hOption("compiler", "objinfo", env.clopt.objinfo)
	hOption("compiler", "showincludes", env.clopt.showincludes)
	hOption("compiler", "modeview", env.clopt.modeview)
	hOption("compiler", "nocmdline", env.clopt.nocmdline)
	hOption("compiler", "returninflts", env.clopt.returninflts)
	hOption("compiler", "nobuiltins", env.clopt.nobuiltins)
	hOption("compiler", "optabstract", env.clopt.optabstract)
	hOption("language-default", "base", env.opt.base)
	hOption("language-default", "parammode", env.opt.parammode)
	hOption("language-default", "explicit", env.opt.explicit)
	hOption("language-default", "procpublic", env.opt.procpublic)
	hOption("language-default", "procprofile", env.opt.procprofile)
	hOption("language-default", "escapestr", env.opt.escapestr)
	hOption("language-default", "dynamic", env.opt.dynamic)
	hOption("language-default", "gosub", env.opt.gosub)
	hOption("language-policy", "opt", env.lang.opt)
	hOption("language-policy", "integerkeyworddtype", env.lang.integerkeyworddtype)
	hOption("language-policy", "remkeyword-active", abs(semantic_context_remkeyword_active <> FALSE))
	hOption("language-policy", "int15literaldtype", env.lang.int15literaldtype)
	hOption("language-policy", "int16literaldtype", env.lang.int16literaldtype)
	hOption("language-policy", "int31literaldtype", env.lang.int31literaldtype)
	hOption("language-policy", "int32literaldtype", env.lang.int32literaldtype)
	hOption("language-policy", "int63literaldtype", env.lang.int63literaldtype)
	hOption("language-policy", "int64literaldtype", env.lang.int64literaldtype)
	hOption("language-policy", "floatliteraldtype", env.lang.floatliteraldtype)
	return semantic_context_id
end function

sub fbSemanticModelOptionDeclaration( byval is_base as integer, byval value as longint )
	if( fbSemanticModelFullEnabled( ) = FALSE ) then exit sub
	dim as longint statement = fbSemanticModelCurrentStatement( )
	if( statement = 0 ) then
		fbSemanticModelFailAt("semantic-context.bas:118")
		exit sub
	end if
	dim as longint context = fbSemanticModelCurrentContext( )
	if( context = 0 ) then
		fbSemanticModelFailAt("semantic-context.bas:123")
		exit sub
	end if
	'' ST retains the entry configuration, before OPTION changes the default.
	'' These optional OPT keys identify the accepted occurrence in its committed
	'' context. Repeated identical directives must not collapse into one event.
	'' The statement identity is unique within a model; no new grammar or AST
	'' expression is invented for this parser-consumed scalar setting.
	'' option-statement: 1 means the BASE route; 0 means another OPTION route.
	'' A receipt for every parsed OPTION makes BASE membership complete, even
	'' when a repeated directive leaves the effective context unchanged.
	hOption("language-default", "option-statement-" + fbSemanticModelNumber(statement), iif(is_base, 1, 0))
	if( is_base ) then
		hOption("language-default", "base-statement-" + fbSemanticModelNumber(statement), value)
	end if
end sub

sub fbSemanticModelBindContext(byref domain as const string, byval identity as longint)
	if( identity <= 0 ) then exit sub
	dim as longint contextid = fbSemanticModelCurrentContext( )
	if( contextid = 0 ) then exit sub
	fbSemanticModelAppendDetail("USE" + TABCHAR + domain + TABCHAR + _
		fbSemanticModelNumber(identity) + TABCHAR + fbSemanticModelNumber(contextid))
end sub

'' end of tooling/semantic-context.bas
