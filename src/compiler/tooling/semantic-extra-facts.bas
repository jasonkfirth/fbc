'' Project: FreeBASIC compiler semantic observations
'' File: tooling/semantic-extra-facts.bas
'' Purpose: Keep optional source capture out of unrelated compiler exports.
'' Responsibilities: Validate requests before mutation and retain their bit mask.
'' This file intentionally does NOT contain: capture buffers or compilation behavior changes.

#include once "core/fbint.bi"
#include once "tooling/semantic-extra-facts.bi"

'' Requests belong to the single-threaded compiler invocation. Module reset
'' clears observations, while these options persist for every input module.
private dim shared as uinteger requested_facts

function fbSemanticExtraFactsEnabled(byval wanted as uinteger) as integer
	return (requested_facts and wanted) = wanted
end function

function fbSemanticExtraFactsConfigure(byref request as const string) as integer
	'' There are four short names. Bound even rejected command-line input and
	'' commit only after validating the complete list, including empty fields.
	if( (len(request) = 0) or (len(request) > 256) ) then return FALSE
	dim as string text = lcase(trim(request))
	if( text = "none" ) then
		requested_facts = FB_SEMEXTRA_NONE
		return TRUE
	end if
	dim as uinteger pending = requested_facts
	dim as integer first = 1
	do
		dim as integer ending = instr(first, text, ",")
		if( ending = 0 ) then ending = len(text) + 1
		dim as string feature_name = trim(mid(text, first, ending - first))
		select case feature_name
		case "hints": pending or= FB_SEMEXTRA_HINTS
		case "declarators": pending or= FB_SEMEXTRA_DECLARATORS
		case "keywords": pending or= FB_SEMEXTRA_KEYWORDS
		case "macro-access": pending or= FB_SEMEXTRA_MACRO_ACCESS
		case "all": pending or= FB_SEMEXTRA_ALL
		case else: return FALSE
		end select
		if( ending > len(text) ) then exit do
		first = ending + 1
	loop
	requested_facts = pending
	return TRUE
end function

'' end of tooling/semantic-extra-facts.bas
