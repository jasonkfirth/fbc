'' Project: FreeBASIC compiler constant-folding tests
'' File: constant-storage-bits.bas
'' Purpose: Check integer bit patterns retained in the signed AST value slot.
'' Responsibilities: Cover wrapping arithmetic, signed shifts and unsigned results.
'' This file intentionally does NOT compare floating-point folding or diagnostics.
#lang "fb"

const signed_max as longint = &h7fffffffffffffffll
const signed_min as longint = -signed_max - 1ll
const unsigned_max as ulongint = &hffffffffffffffffull
const high_bit as ulongint = &h8000000000000000ull

'' AST constants use one signed 64-bit slot even for unsigned source types.
'' These checks require the full unsigned payload to survive that storage step.
#assert (signed_max + 1ll) = signed_min
#assert (signed_min - 1ll) = signed_max
#assert (signed_min * -1ll) = signed_min
#assert (unsigned_max + 1ull) = 0ull
#assert (0ull - 1ull) = unsigned_max
#assert (unsigned_max * 2ull) = &hfffffffffffffffeull
#assert (unsigned_max \ 2ull) = &h7fffffffffffffffull
#assert (unsigned_max mod 3ull) = 0ull
#assert (unsigned_max mod high_bit) = &h7fffffffffffffffull
#assert (unsigned_max and high_bit) = high_bit
#assert (0ull or high_bit) = high_bit
#assert (unsigned_max xor high_bit) = &h7fffffffffffffffull
#assert (unsigned_max eqv high_bit) = high_bit
#assert (unsigned_max imp high_bit) = high_bit
#assert clngint(unsigned_max) = -1ll

'' These DOUBLE values are exact at the unsigned 64-bit boundary. Their
'' conversions must keep the high bit when stored in the signed AST slot.
#assert culngint(9223372036854775808.0) = high_bit
#assert culngint(18446744073709549568.0) = &hfffffffffffff800ull
#assert cint(csng(16777217.0)) = 16777216

'' Shift counts have already been normalized by astNewBOP(). Left shifts
'' preserve the bit pattern; signed right shifts must retain the sign bit.
#assert (-1ll shl 1) = -2ll
#assert (signed_min shl 1) = 0ll
#assert (1ll shl 63) = signed_min
#assert (signed_min shr 63) = -1ll
#assert (unsigned_max shr 63) = 1ull
#assert (high_bit shl 1) = 0ull

'' Out-of-range counts are diagnosed and masked to the operand width. Both
'' negative signed counts and counts with their high bit set must reach that
'' same normalization before the host executes a shift.
#assert (1ll shl -1) = signed_min
#assert (1ull shl 64) = 1ull
#assert (1ull shl high_bit) = 1ull

print "CONST_STORAGE_BITS_PASS"

'' end of constant-storage-bits.bas
