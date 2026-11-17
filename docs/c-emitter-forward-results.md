<!--
Project: FreeBASIC C backend qualification
File: c-emitter-forward-results.md
Purpose: Explain completed pointer-result types and private TLS C identifiers.
Responsibilities: Record source ownership, native acceptance and value checks.
Compiler selection and complete corpus counts are tracked in current status.
-->

# Forward pointer results and private TLS names

## Completed result types

A function body can be parsed before a forward type alias is resolved. Its
result virtual register can retain that original type while the function's
declaration already describes the completed pointer type. The old C return
expression converted the result back to `void *`. Clang reported two implicit
conversions in the retained forward-signature test.

The same defect affects nested pointers. A valid BASIC `T PTR PTR` result
became a C `void **` expression returned through a completed `T **` declaration.
Both native compilers reject the regression fixture under the aggressive
GNU23 profiles. The emitter now converts these unresolved result expressions
to the procedure's completed pointer type at the return site.

`tests/compiler-support/c-forward-pointer-results.bas` checks null, non-null,
qualified and nested pointer results. Thirty-two actual executions pass across
both Windows widths, both C producers, both native tools, GNU11/GNU23 and O0/O2.
Native warnings and BASIC warnings are zero. The public backend suite includes
ordinary and optimized cases, with the Clang implicit-conversion diagnostic
enabled as an error. Strict compiler-backed lint accepts the fixture.

## Private TLS identifiers

Module-private TLS storage can use an encoded C identifier. The native compiler
derives its internal TLS descriptor name from that identifier. The emitter
therefore encodes private dollar-mangled names and lets the declaration own its
private storage identity. Public and external TLS retain their ABI-name policy.

The existing threaded fixture supplies 32 actual executions with zero native
warnings. A single-module public keyword-alias control adds 32 executions and
four exact preceding C identities. Its GNU11 builds retain the existing unused
parameter diagnostic. Sixteen further provider/consumer executions cover
standard-keyword and reserved external names. Sixteen exact source-module C
identities establish the same inputs for mixed old/new compiler cuts. Native
reserved-identifier diagnostics remain recorded for the fixed external names.

The retained corpus contains twelve dollar-identifier observations in the
private TLS class. The backend suite's Clang threaded case now treats that
diagnostic as an error. The existing AROS public-dollar descriptor control
continues to check its required external spelling.

## Qualification

The experimental combined compiler reaches a 224-unit GCC-clean fixed point.
Its four exact C identities bind the pointer-result checks to the combined
repair. The subsequent canonical-source compiler also reaches a 224-unit
GCC-clean fixed point. All 703 fbctests profiles retain zero GCC warnings, while
Clang observations fall from 779 to 771. The expanded complete GCC/Clang suite
passes in 430.701 seconds. Twenty-eight exact C identities bind the canonical
compiler to the 112 preceding value executions. Sixteen additional executions
pass through its selected development path. Full-suite and broad-count results are recorded in
[current status](c-emitter-current-status.md).

Evidence uses the existing `build/c-emitter-forward-views-20261105` directory.
Receipts include `return-pointer-values-v1-20261113.json.zlib`,
`return-pointer-combined-binding-v1-20261113.json.zlib`,
`private-tls-values-v3-20261113.json.zlib`,
`private-tls-public-controls-v1-20261113.json.zlib` and the
`return-private-tls-canonical-` build and qualification receipts.

Lexical strict lint reports zero findings in the two changed compiler files.
The full IR semantic-lint pass was stopped after reaching 2.2 GB of working
memory and producing a 228 MB temporary export. Its semantic result remains
unverified. Compiler self-builds and native value matrices provide their own
completed evidence.

<!-- end of c-emitter-forward-results.md -->
