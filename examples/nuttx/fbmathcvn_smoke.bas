''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbmathcvn_smoke.bas
''
'' Purpose:
''
''     Prove that the NuttX smoke runtime can use the normal rtlib
''     CV numeric bit-conversion helpers.
''
'' Responsibilities:
''
''     - exercise CVDFROMLONGINT
''     - exercise CVSFROML
''     - exercise CVLFROMS
''     - exercise CVLONGINTFROMD
''
'' This file intentionally does NOT contain:
''
''     - string-based CV/MK tests
''     - floating point formatting checks
''     - board-specific hardware behavior
''

dim as longint double_bits = &h3ff0000000000000ll
dim as integer single_bits = &h3f800000
dim as single single_value = 1.0
dim as double double_value = 1.0

if fb_CVDFROMLONGINT(double_bits) <> 1.0 then
    end 20
end if

if fb_CVSFROML(single_bits) <> 1.0 then
    end 21
end if

if fb_CVLFROMS(single_value) <> single_bits then
    end 22
end if

if fb_CVLONGINTFROMD(double_value) <> double_bits then
    end 23
end if

print "FB_NUTTX_MATH_CVN_SMOKE_OK"
end 0

'' end of fbmathcvn_smoke.bas
