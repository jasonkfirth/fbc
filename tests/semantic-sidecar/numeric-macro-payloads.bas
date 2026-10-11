' Project: FreeBASIC semantic sidecar tests
' File: numeric-macro-payloads.bas
' Purpose: Observe original numeric input without changing macro execution.
' Responsibilities: Definition, argument, nested, pasted and clipped payloads.
' This file intentionally does NOT call external rounding functions.
#lang "fb"

' Warning 8 controls preserve the existing canonical clipping behavior.
#define NUMBER_FRACTION 0.0000000000000000000000000000000000000000000000000000000000000000000000100000001e71F
#define NUMBER_WHOLE 1.00000000000000000000000000000000000000000000000000000000000000000000000000F
#define NUMBER_ALIAS NUMBER_FRACTION
#define NUMBER_ID(value) value
#define NUMBER_BODY(ignore) 0.0000000000000000000000000000000000000000000000000000000000000000000000100000001e71F
#define NUMBER_PAIR(first, second) ((first) + (second))
#define NUMBER_REPEAT(value) ((value) + (value))
#define NUMBER_JOIN(first, second) first##second
#define NUMBER_EXPONENT (1D+)
#define NUMBER_DOT .0000001

Print Int(2.0) ' payload: direct
Print Int(NUMBER_FRACTION) ' payload: definition
Print Int(NUMBER_WHOLE) ' payload: whole
Print Int(NUMBER_ALIAS) ' payload: alias
Print Int(NUMBER_ID(0.0000000000000000000000000000000000000000000000000000000000000000000000100000001e71F)) ' payload: argument
Print Int(NUMBER_ID(NUMBER_FRACTION)) ' payload: nested
Print Int(NUMBER_ID(   5.0   )) ' payload: trimmed
Print Int(NUMBER_BODY(0)) ' payload: body
Print Int(NUMBER_PAIR(6.0, 7.0)) ' payload: pair
Print Int(NUMBER_REPEAT(8.0)) ' payload: repeated
Print Int(NUMBER_JOIN(1, .0)) ' payload: paste
Print Int(NUMBER_EXPONENT) ' payload: exponent
Print Int(NUMBER_DOT) ' payload: leading-dot
Print Int(NUMBER_ID(.0000002)) ' payload: dot-argument
Print Int(NUMBER_ID(NUMBER_DOT)) ' payload: dot-nested
#define NUMBER_REUSED 9.0
Print Int(NUMBER_REUSED) ' payload: before-undef
#undef NUMBER_REUSED
#define NUMBER_REUSED 10.0
Print Int(NUMBER_REUSED) ' payload: after-undef

' end of numeric-macro-payloads.bas
