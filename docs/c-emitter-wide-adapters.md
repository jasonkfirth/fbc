<!--
Project: FreeBASIC runtime warning qualification
File: c-emitter-wide-adapters.md
Purpose: Record native character-adapter conversions and their qualification.
Responsibilities: Bind exact runtime sources, warning counts and object identities.
Application builds and compiler promotion retain their existing qualifications.
-->

# Native character classification adapters

The four case-classification and case-mapping helpers in `src/rtlib/fb_unicode.h`
now explicitly convert between runtime character storage and the native library
argument type. `FB_WINT` follows `wint_t` in wide configurations and `int` in the
`DISABLE_WCHAR` configuration. Mapping results convert back to `FB_WCHAR`.

The C library accepts wide characters through `wint_t`, whose signedness can
differ from `wchar_t`. Case mappings return a character or `WEOF`. The wrappers
retain the runtime's existing character and end-marker representations. Narrow
helpers keep the promoted character argument used by the `ctype.h` functions.
These contracts follow the wide-character classification and mapping clauses
of the C draft. [WG14 N3096](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3096.pdf)

## Full Linux runtime comparison

The November 15 runtime source cut rebuilds all 519 Make-selected MT/startup
objects per native tool from an exact 1,328-file packet. GCC records 1,825
warnings and Clang 10,270, with zero build errors. That packet has 103 changed
inputs relative to the earlier packet, including this adapter repair and
concurrent runtime maintenance. These totals qualify the complete captured
source cut. Sixteen live inputs changed again during verification and retain
their explicit source-coherence limit.

The worker uses one serial low-priority native build and a 64 MiB scratch bound,
peaking at 14,746,798 bytes. The `runtime-current-packet-v1-20261115.json.zlib`
and `runtime-current-linux-v1-20261115.json.zlib` receipts retain complete source
bytes, commands, diagnostics and object/archive hashes. GCC padding accounts
for 1,622 observations. Clang byte-buffer observations account for 8,017.

Both native compilers rebuilt the Make-selected Linux x86_64 multithreaded
runtime and startup objects before and after the header change. Each build used
the same exact 1,328-file runtime source packet previously captured for managed
field allocation checks. GCC used the aggressive library warning flags and
Clang used `-Weverything`. Both runs retained unsuppressed diagnostics.

| Native tool | Before | After | Removed |
| --- | ---: | ---: | ---: |
| GCC 14.2 | 5,059 | 1,963 | 3,096 |
| Clang 19.1.7 | 13,484 | 10,388 | 3,096 |

Every removed observation belongs to `-Wsign-conversion`. The six repeated
diagnostics came from 516 translation units. All 519 object hashes per native
tool match across the header change, giving 1,038 object comparisons. Runtime
archive and startup hashes also match. Both complete builds have zero errors.

This comparison qualifies the archived runtime source cut. Ten runtime or Make
inputs had changed concurrently by the time the qualification was verified.
Their recorded before/current hashes remain in the verification receipt. A
whole-current-runtime census retains its own source-coherence check.

## Native value and configuration controls

Sixty accepted native executions cover Linux x86_64 GCC/Clang, Windows x86_64
GCC/Clang and Windows x86 GCC. Each tool runs O0/O2, wide characters, signed
narrow characters and unsigned narrow characters against both header versions.
Every value result and all thirty corresponding probe-object hashes match.

The probe compares both classifiers and both mappings with direct native API
calls. It tests `FB_WEOF` separately, all BMP candidates representable in each
available locale, and a bounded sample of supplementary characters. `wcrtomb`
checks locale representation before wide calls. Narrow inputs stay in the
defined `ctype.h` domain, including the signed end marker and the unsigned
character representation. The locale list includes C and available native
UTF-8 or named locales.

Linux narrow controls compile the full runtime header. The first Windows
narrow experiment encountered conflicting declarations from the host wide
headers. Its failure receipt is retained. The accepted Windows retry compiles
the exact narrow type/macro branch and the four actual adapter bodies under
both character signednesses. Full DOS runtime qualification remains pending.

## Reproduction and retained evidence

All named artifacts are under `build/c-emitter-forward-views-20261105`.

| Evidence | Artifact |
| --- | --- |
| Exact complete runtime sources | `cleanup-linux-packet-v3-20261114.json.zlib` |
| Exact header delta and value probe | `unicode-adapters-packet-v1-20261114.json.zlib` |
| Linux complete builds and values | `unicode-adapters-linux-v1-20261114.json.zlib` |
| Windows rejected full-header probe | `unicode-adapters-windows-v1-20261114.json.zlib` |
| Windows accepted native controls | `unicode-adapters-windows-v2-20261114.json.zlib` |
| Combined qualification and product pins | `unicode-adapters-verified-v1-20261114.json.zlib` |
| Canonical header and current public-header checks | `unicode-adapters-final-v1-20261114.json.zlib` |

Place the two packets beside the corresponding worker and run Python with `-B`.
The Linux worker uses `/usr/bin/gcc`, `/usr/bin/clang`, Make and the system
development libraries. The Windows worker records its UCRT64 and MinGW32 tool
paths. Reproduction on another installation requires substituting those tool
paths while retaining the recorded flags and source bytes. Workers reconstruct
one temporary source tree, rebuild outputs in place and remove that tree on
completion. Linux scratch use peaks at 15,800,240 bytes, and Windows peaks at
4,204,596 bytes, within the 64 MiB per-worker bound.

The selected compiler remains `409dc1ef`. The protected installed-product
hashes are checked separately. Four additional native executions compile the
probe through the actual current canonical runtime header under GCC/Clang at
O0/O2. Current generated-C queue counts remain those
of the [managed-field qualification](c-emitter-cleanup-dependencies.md), including
Templari's 658 profiles and the 498 coreutils plus 555 SQLite profiles.

<!-- end of c-emitter-wide-adapters.md -->
