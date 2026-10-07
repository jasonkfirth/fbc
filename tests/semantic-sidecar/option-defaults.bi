'' Project: FreeBASIC semantic sidecar tests
'' File: option-defaults.bi
'' Purpose: Preserve included option occurrences without flattening their sites.
'' Responsibilities: Repeated defaults and a native array-bound control.
'' This file intentionally does NOT contain: an alternate option parser.
#ifndef OPTION_DEFAULTS_BI
#define OPTION_DEFAULTS_BI
option base 1
dim included_values(3) as long
if lbound(included_values) <> 1 then end 101
option base 0
#endif
'' end of option-defaults.bi
