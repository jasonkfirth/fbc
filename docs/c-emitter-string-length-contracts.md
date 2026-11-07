<!--
Project: FreeBASIC runtime string qualification
File: c-emitter-string-length-contracts.md
Purpose: Record signed string lengths, const views and normal archive qualification.
Responsibilities: Bind source changes, unsuppressed diagnostics and rebuild evidence.
This file intentionally does NOT claim another-host or whole-campaign completion.
-->

# String length and descriptor contracts

The shared string macros now read descriptor metadata through const views and
make the existing ssize_t length contract explicit when using libc strlen().
Caller-owned ZSTRING content must fit the runtime's signed length type. The
change preserves scanning, allocation, layouts and public signatures.

File GET previously used one size_t variable for string setup and actual bytes
read. It now obtains a signed capacity, then converts it once at the file I/O
boundary. All setup branches produce nonnegative lengths for valid inputs.
Fixed STRING storage still receives no terminator; ZSTRING and descriptor-backed
buffers retain their existing terminator and short-read behavior.

| Native profile | GCC before / after | Clang before / after |
| --- | ---: | ---: |
| ST, 522 members | 1,067 / 1,009 | 19,889 / 19,830 |
| MT, 522 members | 1,067 / 1,009 | 19,896 / 19,837 |
| Startup | 2 / 2 | 34 / 34 |

All 2,090 direct checks pass with unchanged aggressive, unsuppressed diagnostics.
This removes 116 GCC and 118 Clang observations; no warning class increases.
There are 1,043 byte-identical GCC objects. The two file-GET objects change
register copies and function alignment after introducing the capacity local.
They are retained exactly in the compressed native checkpoint. The initial
exact-byte stop and the reviewed, source-specific change remain documented.

Thirty C/C++ executions compare binary STRING, ZSTRING content/capacity and
const descriptors across both header cuts. Four old C const-cast controls fail
as expected. Sixteen further executions use the actual old/new file-GET bodies
with a controlled driver; eight Win32 driver-fixture objects and eight actual
Win32 file-GET objects compile. The independent C++ wide-search cast remains
visible; it is not treated as a descriptor-query failure or suppressed.

Both normal archives contain their actual updated member, retain 522-member
order and preserve every exported name. Eight C/C++ archive-linked executions
perform real file open/read/close calls. The relinked frozen experimental
compiler passes both complete backend suites and regenerates all 224 bootstrap
C files byte for byte. Its image is 3dd76a4d5fc0; it is not installed.
Sixteen current canonical provider compiles retain eight exact object pairs.
Eleven existing graphics/sound consumers retain 44 Win32/Win64 object pairs.
The runtime structural check passes all 126 graph cases across 1,252 sources.

One backend runner also needed a real repair: the parent passes a PATH name
such as gcc, while the child tried to hash that name as a relative file.
The child now resolves fbc/GCC/Clang before pinning and running them. The complete
backend suites pass through that corrected default path. A new C++ fixture's
missing extern-C wrapper was corrected without changing production declarations.

The cumulative 291,012-byte string-length-source-update.zip replaces the
earlier runtime updates. Extract it over the original portable-source.zip before
the first build, then run verify-runtime-warning-update.py, build.py and
verify-smoke.py as described in [the portable build](c-emitter-portable-build.md).
All 609 composed inputs match the qualified provider; the delivered verifier
also passes unmodified through a temporary directory junction, without another
source-tree copy. The update contains no executable, object, runtime archive or
cache dependency. The compiler/bootstrap cut remains frozen before unrelated
concurrent parser/string and semantic-model work in the main checkout.

The new archive/compiler/C-output slots are reused and retired after checking.
Retained data stays below 160 MiB. Integration checks use a separate bounded
192 MiB transient allowance; their observed allocation peak is 169,889,158 bytes.
Raw diagnostic text from an older rejected force-include wrapper was retired;
its commands, counts, object hashes and rejection disposition remain in a
compact summary. That rejected raw-log database is no longer fully restorable.
All accepted receipts remain intact. Previously rejected scratch deletions were
not retried. Installed binaries and unrelated dirty sources remain preserved.

Another-host rebuilding, wider SDK execution, complete external corpus replay
and experimental promotion remain unverified. The emitted-C queue totals keep
their measured source/compiler cuts and are unchanged. No BASIC source,
application example or PNG loader changes. The overall campaign remains active.

Evidence: string-length-native.json/json.zlib, values/get/archive/selfhost and
consumer receipts, string-length-extra.json and string-length-source-update.json
in the existing E:/fbc-c-audit-20261105-forward-views-r2-qualification/crt-candidate/canonical area.

<!-- end of c-emitter-string-length-contracts.md -->
