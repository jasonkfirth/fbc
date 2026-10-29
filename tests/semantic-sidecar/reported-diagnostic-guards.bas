'' Project: FreeBASIC semantic diagnostic tests
'' File: reported-diagnostic-guards.bas
'' Purpose: Exercise native reporter acceptance without a parser or driver.
'' Responsibilities: Parameter order, filtering, limits and invocation lifetime.
'' This file intentionally does NOT replace the native reporting decisions.

#include once "core/fbint.bi"
#include once "parser/parser.bi"
#include once "lexer/lex.bi"
#include once "diagnostics/error.bi"
#include once "tooling/semantic-diagnostics.bi"
#include once "support/strings/hlp-str.bi"

dim shared as FBENV env
dim shared as PARSERCTX parser
dim shared as LEX_CTX lex

dim shared as integer policies, reports, frames, initialized, native_count, promoted
dim shared as integer selected_warning_level

dim shared as integer parameters(0 to 3)
dim shared as string spellings(0 to 3)

'' Formatting dependencies are inert. The production list/hash allocation and
'' reporter code are linked separately; policy and parameter callbacks capture
'' the reporter's selected state before its console formatting proceeds.
function fbGetLangOptions( byval language as FB_LANG ) as FB_LANG_OPT
	return 0
end function
function fbGetLangName( byval language as FB_LANG ) as string
	return "test"
end function
function lexGetToken( byval flags as LEXCHECK ) as integer
	return FB_TK_EOF
end function
function lexGetText( ) as zstring ptr
	return @""
end function
function lexPeekCurrentLine( byref position as string, byval do_trim as integer ) as string
	position = ""
	return ""
end function
function symbKeywordGetText( byval token as integer ) as const zstring ptr
	return @"keyword"
end function
function symbGetMangledName( byval sym as FBSYMBOL ptr ) as zstring ptr
	return @"selected"
end function
function symbProcPtrToStr( byval proc as FBSYMBOL ptr ) as string
	return "selected-pointer"
end function
function symbMethodToStr( byval proc as FBSYMBOL ptr ) as string
	return "selected-method"
end function
sub ZstrAssign( byval destination as zstring ptr ptr, byval source as const zstring ptr )
	if( *destination <> NULL ) then deallocate(*destination)
	*destination = NULL
	if( source = NULL ) then exit sub
	*destination = allocate(len(*source) + 1)
	if( *destination = NULL ) then end 30
	**destination = *source
end sub

sub fbSemanticDiagnosticsPolicy( byval active as integer, byval options as integer, _
	byval show_source as integer, byval error_count as integer, byval warning_error as integer, byval warning_level as integer )
	policies += 1
	frames = 0
	initialized = active
	native_count = error_count
	promoted = warning_error
	selected_warning_level = warning_level
end sub
sub fbSemanticDiagnosticsParameter( byval proc as FBSYMBOL ptr, byval token as integer, _
	byval parameter as integer, byval spelling as const zstring ptr )
	if( frames > ubound(parameters) ) then end 31
	parameters(frames) = parameter
	spellings(frames) = ""
	if( spelling <> NULL ) then spellings(frames) = *spelling
	frames += 1
end sub
sub fbSemanticModelDiagnostic( byref severity as const string, byval code as integer, _
	byval line_number as integer, byval message as const zstring ptr, _
	byval detail as const zstring ptr, byval custom_text as const zstring ptr )
	reports += 1
	if( policies <> reports ) then end 32
end sub

errPreInit( )
env.clopt.warninglevel = 0
env.clopt.maxerrors = 2
errReportWarnEx(FB_WARNINGMSG_NOFUNCTIONRESULT, @"invocation", 0)
if( reports <> 1 or initialized or frames ) then end 1
if( selected_warning_level <> 1 ) then end 10

errInit( )
errPushParamLocation(NULL, FB_TK_ID, 1, @"outer%argument")
errPushParamLocation(NULL, FB_TK_ID, 2, @"inner argument")
errReportEx(FB_ERRMSG_TYPEMISMATCH, NULL)
if( reports <> 2 or initialized = FALSE or frames <> 2 or native_count <> 0 ) then end 2
if( selected_warning_level <> -1 ) then end 11
if( parameters(0) <> 1 or parameters(1) <> 2 ) then end 3
if( spellings(0) <> "outer%argument" or spellings(1) <> "inner argument" ) then end 4
errPopParamLocation( )
errPopParamLocation( )

'' A second report at the same statement is filtered before observation.
errReportEx(FB_ERRMSG_TYPEMISMATCH, NULL)
if( reports <> 2 or policies <> 2 ) then end 5
parser.stmt.cnt += 1
errReportEx(FB_ERRMSG_TYPEMISMATCH, NULL)
if( reports <> 4 or frames or native_count <> 2 ) then end 6
errReportWarnEx(FB_WARNINGMSG_NOFUNCTIONRESULT, NULL, 0)
errReportEx(FB_ERRMSG_TYPEMISMATCH, NULL, 10)
if( reports <> 4 or policies <> 4 ) then end 7
errEnd( )

errPreInit( )
env.clopt.warninglevel = FB_WARNINGMSGS_HIGHEST_LEVEL + 1
errReportWarnEx(FB_WARNINGMSG_NOFUNCTIONRESULT, NULL, 0)
errReportWarnEx(0, NULL, 0)
errReportWarnEx(FB_WARNINGMSGS, NULL, 0)
if( reports <> 4 ) then end 8
env.clopt.warninglevel = 0
env.clopt.pdcheckopt or= FB_PDCHECK_ERROR
errReportWarnEx(FB_WARNINGMSG_NOFUNCTIONRESULT, @"after teardown", 0)
if( reports <> 5 or initialized or frames or promoted = FALSE ) then end 9
if( selected_warning_level <> 1 ) then end 12
print "REPORTED_DIAGNOSTIC_GUARDS_PASS"

'' end of reported-diagnostic-guards.bas
