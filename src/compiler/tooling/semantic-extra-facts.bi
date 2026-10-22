'' Project: FreeBASIC compiler semantic observations
'' File: tooling/semantic-extra-facts.bi
'' Purpose: Name optional source observations requested by analysis clients.
'' Responsibilities: Stable request flags and checked command-line configuration.
'' This file intentionally does NOT contain: capture routines or linter rule selection.

#pragma once

enum FB_SEMANTIC_EXTRA_FACTS
	FB_SEMEXTRA_NONE = 0
	FB_SEMEXTRA_HINTS = 1
	FB_SEMEXTRA_DECLARATORS = 2
	FB_SEMEXTRA_KEYWORDS = 4
	FB_SEMEXTRA_MACRO_ACCESS = 8
	FB_SEMEXTRA_ALL = FB_SEMEXTRA_HINTS or FB_SEMEXTRA_DECLARATORS or _
		FB_SEMEXTRA_KEYWORDS or FB_SEMEXTRA_MACRO_ACCESS
end enum

declare function fbSemanticExtraFactsConfigure(byref request as const string) as integer
declare function fbSemanticExtraFactsEnabled(byval wanted as uinteger) as integer

'' end of tooling/semantic-extra-facts.bi
