/*
 * Project: FreeBASIC C emitter regression tests
 * File: c-error-terminal-shim.h
 * Purpose: Observe the terminal guard without generating a host crash dump.
 * Responsibilities: Declare the test replacement for the abort builtin.
 * This file intentionally does NOT change production compiler output.
 */
#ifndef FBC_TEST_C_ERROR_TERMINAL_SHIM_H
#define FBC_TEST_C_ERROR_TERMINAL_SHIM_H
_Noreturn void fb_test_invalid_transfer( void );
#endif
/* end of c-error-terminal-shim.h */
