/* FreeBASIC GMP native qualification
 * File: gmp-native.c
 * Purpose: Provide C layouts and native limb arithmetic results.
 * Responsibilities: Upper limb bits, carry returns and caller-owned output.
 * This file contains no substitute big-number arithmetic implementation.
 */
#include <gmp.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    mpz_t number;
    mp_limb_t input[2] = {0, 1}, output[2], high, carry;
    char text[64];
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(mp_limb_t), sizeof(mp_limb_signed_t), sizeof(mp_bitcnt_t),
           sizeof(mp_size_t), sizeof(mp_exp_t), sizeof(__mpz_struct),
           sizeof(__mpq_struct), sizeof(__mpf_struct),
           offsetof(__mpz_struct, _mp_d), offsetof(__mpf_struct, _mp_d));
    if (GMP_NAIL_BITS || GMP_LIMB_BITS != 8 * sizeof(mp_limb_t)) return 1;
    high = (mp_limb_t)1 << (GMP_LIMB_BITS - 1);
    carry = mpn_add_1(output, input, 2, high);
    if (carry || output[0] != high || output[1] != 1) return 2;
    input[0] = 1; input[1] = 0;
    carry = mpn_rshift(output, input, 2, 1);
    if (carry != high || output[0] || output[1]) return 3;
    mpz_init(number);
    if (mpz_set_str(number, "18446744073709551615", 10)) {
        mpz_clear(number);
        return 4;
    }
    if (mpz_getlimbn(number, 0) != (mp_limb_t)-1 ||
        !mpz_get_str(text, 10, number) || strcmp(text, "18446744073709551615")) {
        mpz_clear(number);
        return 5;
    }
    mpz_clear(number);
    printf("GMP_NATIVE=PASS limb_bits=%d high=%llu carry=%llu\n",
           GMP_LIMB_BITS, (unsigned long long)high, (unsigned long long)carry);
    return 0;
}
/* end of gmp-native.c */
