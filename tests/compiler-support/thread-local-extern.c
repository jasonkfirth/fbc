/*
 * Project: FreeBASIC compiler backend regression tests
 * File: thread-local-extern.c
 * Purpose: Provide an external C thread-local variable for ABI tests.
 * Responsibilities: Verify each C thread-local instance starts at its initializer.
 * This file intentionally does NOT contain: FreeBASIC thread management or test output.
 */
_Thread_local unsigned int externalThreadLocalValue = 7;
_Thread_local unsigned int externalThreadLocalValues[2] = {3, 4};

/* end of thread-local-extern.c */
