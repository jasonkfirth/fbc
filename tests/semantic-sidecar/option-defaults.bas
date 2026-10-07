'' Project: FreeBASIC semantic sidecar tests
'' File: option-defaults.bas
'' Purpose: Observe consumed option values and independent opening-token sites.
'' Responsibilities: Includes, expansions, repeats, branches and remapped lines.
'' This file intentionally does NOT contain: invented option expression nodes.
#lang "fblite"
#include once "option-defaults.bi"
option byval
option dynamic
option static
option explicit
option private
option escape
option nokeyword sleep
#define one_directive option base 1
#define value_two 2
#macro two_directive
    option base 2
#endmacro
one_directive
dim one_values(4) as long
if lbound(one_values) <> 1 then end 1
option base value_two
dim two_values(4) as long
if lbound(two_values) <> 2 then end 2
two_directive
option base 1: option base 2
dim colon_values(4) as long
if lbound(colon_values) <> 2 then end 3
#if 0
option base 4
#endif
#if 1
option base 3
#endif
dim active_values(4) as long
if lbound(active_values) <> 3 then end 4
#line 500 "option-logical-name.bas"
option base 5
dim remapped_values(6) as long
if lbound(remapped_values) <> 5 then end 5
'' The existing numeric-token text conversion consumes 1.5 as 1 and 0.5 as 0.
option base 1.5
dim fraction_values(4) as long
if lbound(fraction_values) <> clng("1.5") then end 6
option base 0.5
dim zero_values(4) as long
if lbound(zero_values) <> clng("0.5") then end 7
end 0
'' end of option-defaults.bas
