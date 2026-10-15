'' TEST_MODE : COMPILE_ONLY
'' Project: FreeBASIC compiler lexer tests
'' File: nondecimal-literal-boundaries.bas
'' Purpose: Verify radix scanning across narrow and wide accumulator transitions.
'' Responsibilities: Check zero, 32-bit promotion and 64-bit unsigned boundaries.
'' This file intentionally does NOT implement number scanning or runtime I/O.

const unsigned_max as ulongint = not culngint(0)

'' Hexadecimal promotes on its ninth significant digit.
#assert &h0ULL = 0
#assert &hffffffffULL = 4294967295ULL
#assert &h100000000ULL = 4294967296ULL
#assert &hffffffffffffffffULL = unsigned_max

'' Octal promotion depends on both length and the leading digit.
#assert &o0ULL = 0
#assert &o37777777777ULL = 4294967295ULL
#assert &o40000000000ULL = 4294967296ULL
#assert &o100000000000ULL = 8589934592ULL
#assert &o1777777777777777777777ULL = unsigned_max

'' Binary reaches the wide accumulator after 32 significant digits.
#assert &b0ULL = 0
#assert &b11111111111111111111111111111111ULL = 4294967295ULL
#assert &b100000000000000000000000000000000ULL = 4294967296ULL
#assert &b1111111111111111111111111111111111111111111111111111111111111111ULL = unsigned_max

'' end of nondecimal-literal-boundaries.bas
