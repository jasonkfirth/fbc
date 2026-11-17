<!--
Project: FreeBASIC runtime-library verification
File: c-emitter-library-crt-constants.md
Purpose: Record CRT constant ownership and native Linux source qualification.
Responsibilities: Bind source repairs, native values and platform-port refusals.
-->

# CRT constant ownership

The translated runtime's `crt_extra/stdlib.bi` supplied Linux `RAND_MAX` even
when the SDK had already defined it. The fallback now requires an absent SDK
definition. Its value and existing wrapper source remain intact.

The permanent BASIC fixture captures the SDK constant and exports its selected
value to an independent native C observer. Linux also checks the legacy fallback
after deliberately removing the SDK macro. Eight Windows and eight Linux value
executions pass across both C producers and both native compilers. The original
header fails duplicate-definition controls through GCC and GAS64 emission.
Strict source-only lint passes the header and BASIC fixture.

All 940 accepted Windows parent profiles that opened the header are reemitted.
Every C output matches its qualified parent exactly. The corresponding native
diagnostics retain their original identities. The immutable parent census and
installed products remain available with their recorded hashes.

## Linux source checks

Actual Make-selected target graphs identify fifty-six pending Linux x86-64
runtime source units. The original header permits one emission. The repaired
header permits nine. All nine compile with native GCC 14.2 and Clang 19.1.7
under the original unsuppressed diagnostic flags, with zero warnings.

The remaining forty-seven profiles fail in both GCC and GAS64 frontends with
the same first diagnostic. Twenty-seven require the port's missing `termios`
definition. Other failures include missing platform bindings, undeclared CRT
interfaces, incomplete declarations and source syntax. These remain explicit
platform-port failures. The complete Linux runtime port remains unfinished.

Receipts in `build/c-emitter-forward-views-20261105` include
`fbrtlib-crt-constant-contract-v1`, `fbrtlib-crt-guard-readers-v1`,
`fbrtlib-linux-pending-v2`, `fbrtlib-linux-native-result-v1` and
`fbrtlib-linux-frontend-controls-v1`, all dated November 14.
The native guest uses one reusable C/object/log slot. Its library checks produce
objects. The independent constant observer supplies the value executions.

<!-- end of c-emitter-library-crt-constants.md -->
