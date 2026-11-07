<!--
Project: FreeBASIC runtime qualification
File: c-emitter-utf-spans.md
Purpose: Record checked UTF conversion spans and supplementary scalar handling.
Responsibilities: Explain ownership, diagnostic reduction and actual qualification.
This file intentionally does NOT claim foreign platform ABI or whole-campaign completion.
-->

# UTF conversion counts and output ownership

Four shared UTF converter units now use one private checked byte-count helper.
Signed character counts are checked before multiplication or conversion to
size_t; decoder growth preserves its existing eight-unit allocation policy and
checks the representable capacity before addition. Public conversion signatures
and native UTF16/UTF32 code-unit layout remain unchanged.

The same review repairs two valid-text defects. UTF8 U+10000 previously became
zero after subtracting the surrogate base, causing an early terminator test;
the original scalar is now retained for that test. The UTF16-to-32-bit-wide
decoder used a 16-bit intermediate, truncating combined surrogate pairs. Its
intermediate now has the required 32-bit width. Finally, the 32-bit-wide UTF16
encoder no longer reallocates caller-owned output storage. Allocating callers
receive a checked worst-case buffer; supplied buffers retain their ownership
under the API's existing sufficiently-large-output precondition.

| Native profile | GCC before / after | Clang before / after |
| --- | ---: | ---: |
| ST, 522 members | 889 / 837 | 14,535 / 14,487 |
| MT, 522 members | 889 / 837 | 14,542 / 14,494 |
| Startup | 2 / 2 | 24 / 24 |

Sixteen fresh native checks of actual checkout sources remove 104 GCC and 96
Clang observations, with no warning-class increase or diagnostic suppression.
Only these four C units include the private helper. The other 2,074 native
checks inherit exact source/tool/flag identities. Eight GCC objects change;
matched normal libraries retain their 522-member order and export multiset.
Narrowing casts follow known UTF byte/unit ranges rather than hiding unchecked
signed allocation arithmetic. Two unused private byte-count parameters are
removed instead of adding dummy reads.

All 48 before/after GCC/Clang GNU11/GNU23 O0/O2 value runs pass. Sixteen use
actual Win64 headers and 16-bit FB_WCHAR. Thirty-two use explicit 8-/32-bit
scalar fixtures for the unmodified converter bodies, with actual shared Unicode
definitions, encoding enum, ABI bindings and declarations. These fixtures do
not qualify foreign SDK or platform ABI. Before tests observe both expected
supplementary-scalar defects; after tests pass fixed vectors, every Latin-1 byte,
caller-owned buffers, repeated growth and signed count limits.

Win64's existing DISABLE_WCHAR profile conflicts with CRT swprintf declarations,
and its C wchar_t typedef remains 16-bit even with -fno-short-wchar. Those failed
preflights are retained; native 8-/32-bit-wide Windows execution is not claimed.
Eight further actual Win32 definition compiles and four C/C++ client objects
preserve seven C definitions and four public client reference names. Win32
execution remains unverified.

The portable checker passes eight C/C++ executions against both matched normal
archives. Relinking compiler 468f19e499d7 preserves its exact bytes, so the last
console checkpoint's 224 self-host C identities and both backend regression
suite results are inherited. Unchanged compiler tests are not repeated. All
126 runtime graph cases and three guards pass across 1,255 files and 21 groups.
An identifying-header comment omission was corrected; rebuilt GCC objects stay
exact and no value test is repeated for that comment-only change.

The 134,621-byte utf-span-source-update.zip includes all preceding word,
RND, C declaration and console repairs. Extract portable-source.zip, then
string-length-source-update.zip, then utf-span-source-update.zip before the
first build. Separate word/RND/prototype/console archives are not required.
All 612 runtime source identities pass the unchanged delivered verifier, and
the delivered checker bytes match the checker actually executed. Follow
[the portable build](c-emitter-portable-build.md) and run the shipped
verify-runtime-utf-spans.py with --tool-dir after rebuilding.

Another-host rebuilding, full corpus replay from the package, foreign SDK
execution and experimental compiler promotion remain unverified. Main emitted-C
totals remain broad 2,139 GCC / 6,966 Clang and Exampleageddon 3,495 / 8,916 at
their measured cuts. Installed files and unrelated dirty work are unchanged;
no valid example or PNG source changes. Retained storage remains below 160 MiB,
integration below 192 MiB, with reused scratch and at most two heavy workers.
Temporary source namespaces, archives, compiler images and object slots are
retired. The complete warning-cleanup campaign remains active.

Evidence: utf-span screen/native/values/archive/Win32/structure/source-update
receipts in the existing qualification directory, and the component seal and
verifier in build/c-emitter-forward-views-20261105.

<!-- end of c-emitter-utf-spans.md -->
