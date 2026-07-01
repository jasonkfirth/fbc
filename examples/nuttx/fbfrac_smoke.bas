''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbfrac_smoke.bas
''
'' Purpose:
''
''     Prove that the NuttX smoke runtime can use the normal FreeBASIC
''     FRAC() helper implementation.
''
'' Responsibilities:
''
''     - exercise FRAC() for positive and negative single values
''     - exercise FRAC() for positive and negative double values
''     - keep the result visible in QEMU logs for the device lab
''
'' This file intentionally does NOT contain:
''
''     - broad floating point formatting coverage
''     - random-number or trigonometry coverage
''     - board-specific hardware behavior
''

dim as single single_positive = csng(3.75)
dim as single single_negative = csng(-3.75)
dim as double double_positive = cdbl(8.125)
dim as double double_negative = cdbl(-8.125)
dim as single single_positive_frac = frac(single_positive)
dim as single single_negative_frac = frac(single_negative)
dim as double double_positive_frac = frac(double_positive)
dim as double double_negative_frac = frac(double_negative)

print "frac single positive ="; single_positive_frac
print "frac single negative ="; single_negative_frac
print "frac double positive ="; double_positive_frac
print "frac double negative ="; double_negative_frac

if single_positive_frac < csng(0.7499) or _
    single_positive_frac > csng(0.7501) then
    end 10
end if

if single_negative_frac < csng(-0.7501) or _
    single_negative_frac > csng(-0.7499) then
    end 11
end if

if double_positive_frac < cdbl(0.124999) or _
    double_positive_frac > cdbl(0.125001) then
    end 12
end if

if double_negative_frac < cdbl(-0.125001) or _
    double_negative_frac > cdbl(-0.124999) then
    end 13
end if

print "FB_NUTTX_FRAC_SMOKE_OK"

end 0

'' end of fbfrac_smoke.bas
