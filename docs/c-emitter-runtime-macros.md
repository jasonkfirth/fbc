<!--
Project: FreeBASIC runtime C namespace qualification
File: c-emitter-runtime-macros.md
Purpose: Record owned macro repairs and the remaining SDK namespace boundary.
Responsibilities: Bind full native diagnostics, object identities and source delivery.
This file intentionally does NOT claim another-host or full-campaign completion.
-->

# Owned runtime macro names

Fifteen project-owned macro names now use the ordinary FB_ namespace. This covers
eleven header guards, the wide/TCHAR literal helpers and the two console handle
accessors. Seventy-five runtime C/header files and one coupled C fixture change.
A C++ fixture and a generated NuttX smoke header also follow the literal helper.
The token update preserves unrelated bytes, comments, strings and line counts.
No BASIC source, application example or PNG loader changes.

| Native runtime profile | GCC before / after | Clang before | Clang after |
| --- | ---: | ---: | ---: |
| ST, 522 members | 1,067 / 1,067 | 22,546 | 19,889 |
| MT, 522 members | 1,067 / 1,067 | 22,553 | 19,896 |
| Startup | 2 / 2 | 39 | 34 |

All 2,090 direct checks pass with unchanged aggressive, unsuppressed diagnostics.
Only reserved-macro observations decrease: 2,657 per runtime variant and five
at startup, totaling 5,319. All 1,045 GCC objects are byte-identical to the
preceding C/ABI symbol repair. Its actual normal archives and compiler image
therefore remain valid without another archive or executable copy.

Twenty C/C++ Win32/Win64 consumer object pairs are identical, including real
wide-literal use and repeated inclusion of the umbrella header. Twelve Win64
executions use the existing actual ST/MT archives. The string helper matrix
passes 24 executions and six Win32 objects. Sixteen current canonical provider
checks retain eight exact object pairs; optional native-handler execution is
not reopened. Foreign SDK runtime execution is not claimed.

The structural check passes 126 graph cases across 1,252 sources. Eight controls
verify a new common-header macro guard. Four SDK spellings remain allowed only
in their owning headers: _NO_OLDNAMES/_FILE_OFFSET_BITS in fb_config.h and the
existing __USE_ISOC95/__USE_ISOC99 selectors in fb_unicode.h. They control SDK
declarations and cannot be renamed like owned helpers. The initial guard lacked
the latter two exceptions; its rejected version and failure are retained.
The 518 remaining Win64 reserved-macro observations per variant belong to
_NO_OLDNAMES. They stay visible, with their external-contract reason recorded.

Per-unit native receipts retain exact commands, every warning-class count and
full-output hashes. The unchanged baseline retains complete diagnostic text;
another full copy of those logs is avoided. Any new failing log is retained in
full. Exact source and tool pins bind the compact receipt.

The cumulative runtime-macro-source-update.zip updates 192 runtime
files and the manifest in 289,059 bytes. Extract it over the original
portable-source.zip before the first build; it replaces earlier runtime updates.
All 609 composed inputs match the qualified provider, with no native binaries,
object caches or thin-archive references. The source verifier and build commands
are unchanged. The compiler/bootstrap cut remains frozen before unrelated
concurrent frontend work. See [portable source build](c-emitter-portable-build.md).

Fresh local source compilation is demonstrated. Another-host rebuild, complete
external corpus replay and broader experimental promotion remain unverified.
The emitted-C corpus tables retain their measured cuts; native runtime gains
are not subtracted from them. No installed binary is replaced. The complete
retained component remains below 160 MiB, with reused scratch and at most two
qualification workers. Historical 90-second failures are old receipts, not an
active acceptance deadline; current native commands have a 600-second hang bound.

The next GCC review is signed/unsigned conversion, 473 observations per runtime
variant, followed by 160 other conversions. The full diagnostic census and the
site-ranked receipt remain authoritative; no class is suppressed or silently
declared fixed. The overall warning-cleanup campaign remains active.

Evidence: runtime-macro-native.json and the losslessly archived runtime-macro-native.sqlite3.zlib,
runtime-macro-consumers.json,
runtime-macro-canonical.json, runtime-macro-guard.json, header-matrix-macros,
runtime-macro-source-update.json and runtime-warning-priorities.json in the
existing E:/fbc-c-audit-20261105-forward-views-r2-qualification/crt-candidate/canonical area.

<!-- end of c-emitter-runtime-macros.md -->
