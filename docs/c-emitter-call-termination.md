<!--
Project FreeBASIC C-emitter qualification
File c-emitter-call-termination.md
Purpose Record C2FB terminating-call contracts and the whoami warning repair.
Responsibilities Bind source semantics, native outcomes and the current census.
Allocator-return analysis and other warning classes have their own checks.
-->

# Call termination, November 17

C2FB already recognized terminating C statements during its flow analysis.
The generated BASIC now carries that contract through the existing `Error 5`
fallback used for C unreachable statements. The fallback follows the call,
so a conforming terminating provider transfers control first.

GNU `error` and `error_at_line` return for status zero and terminate for nonzero
status. Their declarations therefore need a conditional call-site contract.
The recognizer checks the variadic void signature and named-parameter count.
Explicit nonreturning declarations retain their contract. Ordinary source
definitions, callback parameters, function-pointer objects, nonvoid functions
and fixed-arity functions retain their returning behavior. [GNU error reporting](https://sourceware.org/glibc/manual/latest/html_node/Error-Messages.html)

The complete `whoami` owner receives one generated terminal statement after
its nonzero-status error branch. The other source bytes match the preceding
qualified module. Its original main body, fetched from the Coreutils v9.12
tag, also receives the terminal statement through normal C preprocessing and
translation. The caller follows the documented GNU API contract. The repair
preserves that information across the C-to-BASIC boundary. [Coreutils v9.12 whoami](https://github.com/coreutils/coreutils/blob/v9.12/src/whoami.c)

## Native evidence

The updated converter is
`build/c-emitter-forward-views-20261105/c2fb-call-termination-v1-20261117.exe`,
SHA256 `5f4444ac156381ea41c55b52f9d24dc4e87d3c6248efb3f0f594bb2bf87c709b`.
Both FreeBASIC C producers emit identical complete converter C. Its GCC build
reports zero warnings. Clang records 158 padding and two startup observations.
Both native converter images produce the same primary regression BASIC.

Eight primary emitted-C captures cover both converter revisions, both
FreeBASIC C producers and Windows/Linux x86-64. Each producer pair is identical,
so one native build qualifies both identities. GCC and Clang at O0/O2 execute
16 distinct native contexts and 336 original-C comparisons. Zero, positive,
negative and variable statuses retain their outputs and exit codes. Nullable
input exercises both the returning and terminating branches.

Sixteen additional provider-return controls deliberately return from a
nonzero-status error call. The original translation continues to its saved
return value. The repaired translation reaches the real runtime's terminal
fallback and exits with code 5. This control uses the ordinary scalar-return
fixture and keeps pointer access out of the returned-provider path.

Four complete `whoami` captures cover both revisions and both C producers.
Native GCC/Clang O0/O2 execution gives eight contexts and 288 comparisons
against the original main body. Five user IDs, including the 32-bit unsigned
maximum, and five names cover empty, spaced, UTF-8 and percent-containing
values. Missing records, lookup errors and failed identity acquisition also
match. The observer validates callback arguments and uses the SDK's actual
`passwd` record. GNU libc supplies error reporting and termination.

Together the accepted matrices pass 624 comparison pairs and 16 terminal
controls, accounting for 1,264 native program invocations. Both platforms link
their real FreeBASIC runtime. The Linux worker reconstructs the qualified
non-MT runtime serially, with GCC zero and matching archive/startup hashes.
An additional direct public-CLI smoke supplies 42 comparison pairs.

Primary emitted modules are warning-clean under both native tools. The old
complete `whoami` module records one GCC O2 null-dereference observation. Its
repaired module records zero GCC warnings at O0 and O2. Clang retains one
reserved-identifier and one missing-noreturn observation per complete module.
Reference/provider diagnostics remain enabled and stored separately in the
native command receipts.

The normal converter regression checks ten functions and five override
controls. All five override translations match their original output exactly.
The registered GNU error regression also asserts the zero/nonzero distinction.
Strict source-only lint checks the shared statement include and refreshed
owner with zero errors. Both source cuts retain the same two generated-line
length advisories.

## Current census and reproduction

Coreutils reaches GCC 154 / Clang 1,778 across 498 qualified profiles. One
owning profile is refreshed, with 497 carried source cuts. The original
complete C identity matches the current compiler's preceding owning capture,
and fresh production emission matches both qualified repaired C captures.
The disjoint production GCC total reaches 170, comprising Coreutils 154,
retained compiler/corpus 14 and Templari 2. SQLite and the four additional
E-drive project queues retain GCC zero. All 129 reviewed nonliteral
observations retain their diagnostic dispositions.

The `careadlinkat` return-address observation still requires its recorded
allocator, growth and callback execution checks. Its source has an explicit
allocate-and-copy branch for the private stack buffer. Other GCC classes,
broader Clang work, native platform replay and full converter integration
retain their existing follow-up.

The port patch is `call-termination-port-v1-20261117.patch`. Five files cover
shared statement lowering, the refreshed owner and test registration/checks.
It applies to the exact preceding private source cut, passes reverse checking
and reproduces the current file bytes. Port publication remains pending.

The public converter runner is
`fb_port/tools/c2fb/tests/test_call_termination.py`, with `--converter`,
`--scratch-root` and `--receipt`. The native runner is
`build_scripts/test-c2fb-call-termination.py`. It accepts `--module-c`,
`--reference-c`, `--cc`, `--native-flags-json`, `--runtime-dir`,
`--optimization`, `--revision`, `--scratch-root` and `--receipt`.
Its native provider source is `tests/compiler-support/c2fb-call-termination.c`.

Receipts under `build/c-emitter-forward-views-20261105` include
`call-termination-build-v1-20261117.json.zlib`,
`call-termination-whoami-v1-20261117.json.zlib`,
`call-termination-fixtures-v1-20261117.json.zlib`,
`call-termination-windows-v1-20261117.json.zlib`,
`call-termination-linux-v1-20261117.json.zlib`,
`call-termination-census-v1-20261117.json.zlib`,
`call-termination-lint-v2-20261117.json.zlib`,
`call-termination-delivery-v1-20261117.json.zlib` and
`call-termination-qualification-v1-20261117.json.zlib`.

The converter build peaks at 34,044,735 owned bytes. The native Windows worker
records 149,806 bytes and Linux records 11,198,335 bytes. Temporary build
directories are removed. The ten protected installed or working products
retain their recorded identities.

<!-- end of c-emitter-call-termination.md -->
