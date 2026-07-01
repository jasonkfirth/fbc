''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbsign_smoke.bas
''
'' Purpose:
''
''     Prove that the NuttX smoke runtime can use the normal FreeBASIC
''     SGN() helper implementation.
''
'' Responsibilities:
''
''     - exercise SGN() for integer, longint, single, and double values
''     - keep the result visible in QEMU logs for the device lab
''
'' This file intentionally does NOT contain:
''
''     - graphics, audio, or device-driver checks
''     - broad math-library coverage
''

dim as integer seed = len(command)
dim as integer int_negative = -7 - seed
dim as integer int_zero = seed - seed
dim as integer int_positive = 9 + seed
dim as longint long_negative = clngint(-1234567890123) - seed
dim as longint long_zero = clngint(seed - seed)
dim as longint long_positive = clngint(1234567890123) + seed
dim as single single_negative = csng(-2.5) - csng(seed)
dim as single single_zero = csng(seed - seed)
dim as single single_positive = csng(3.5) + csng(seed)
dim as double double_negative = cdbl(-4.5) - cdbl(seed)
dim as double double_zero = cdbl(seed - seed)
dim as double double_positive = cdbl(5.5) + cdbl(seed)

dim as integer int_total
dim as longint long_total
dim as single single_total
dim as double double_total

int_total = sgn(int_negative) + sgn(int_zero) + sgn(int_positive)
long_total = sgn(long_negative) + sgn(long_zero) + sgn(long_positive)
single_total = sgn(single_negative) + sgn(single_zero) + sgn(single_positive)
double_total = sgn(double_negative) + sgn(double_zero) + sgn(double_positive)

if int_total <> 0 then
    end 10
end if

if long_total <> 0 then
    end 11
end if

if single_total <> 0 then
    end 12
end if

if double_total <> 0 then
    end 13
end if

print "FB_NUTTX_SIGN_SMOKE_OK"

end 0

'' end of fbsign_smoke.bas
