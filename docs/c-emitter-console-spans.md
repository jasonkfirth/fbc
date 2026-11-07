<!--
Project: FreeBASIC runtime qualification
File: c-emitter-console-spans.md
Purpose: Record console transfer spans and representable coordinate advancement.
Responsibilities: Bind before/after behavior, native diagnostics and source delivery.
This file intentionally does NOT claim device execution or whole-campaign completion.
-->

# Console writer spans

The shared raw writer could repeatedly call its hook with zero characters when
the viewport had no columns. A hook returning TRUE left the input length
unchanged, so the writer could spin forever. The regression hook rejects that
zero-progress call in the old implementation; both repaired narrow/wide writers
return without calling it. A cursor just past the last column wraps without a
zero-length transfer.

Both console templates form coordinate differences in int64_t and admit only
positive widths representable by the existing int coordinate contract. Raw
output wraps without forming Right + 1 in int. TTY tab counts remain one to
eight cells; coordinate advancement checks its result before narrowing. Pending
output flushes before a virtual row overflows, then follows the real scroll-hook
position. A bounded starting row at INT_MAX can scroll before advancing. The
Bottom == -1 sentinel still disables automatic scrolling. Hook structures,
public signatures, ordinary output and character counts remain unchanged.

| Native profile | GCC before / after | Clang before / after |
| --- | ---: | ---: |
| ST, 522 members | 901 / 889 | 14,541 / 14,535 |
| MT, 522 members | 901 / 889 | 14,548 / 14,542 |
| Startup | 2 / 2 | 24 / 24 |

This removes 24 GCC and 12 Clang observations under unchanged unsuppressed
flags, with no warning-class increase. Only four C units include these private
templates. Sixteen fresh isolated compiles cover GCC/Clang ST/MT; 2,074 native
checks inherit exact source/tool/flag identities. Eight GCC objects change and
the other 1,037 retain their qualified identities. Sixteen additional compiles
of the actual current-checkout writers produce the same eight GCC objects and
the same warning classes as the isolated provider.

Sixteen before/after executions cover GCC/Clang, GNU11/GNU23 and O0/O2. Each run
checks 96 ordinary cases; their output, hook events and final-coordinate hash
match exactly. These include raw/TTY, narrow/wide BMP units, shifted origins,
bounded scrolling, the unbounded sentinel, control characters, a 1,200-character
buffer-flush case, empty input and immediate raw-hook failure. Each repaired run
also passes 48 endpoint/invalid-view controls. TTY's existing void API does not
propagate partial raw-hook failure; that pre-existing behavior is not redesigned.

Eight actual Win32 definition compiles and four C/C++ client objects agree on
all four C symbol decorations. Win32 execution and foreign SDK execution are
not claimed. All 126 runtime graph cases and three collision guards pass across
1,254 files and 21 groups. No console, printer or graphics device is opened by
the new in-memory writer tests.

Both matched normal libraries preserve 522-member order and the export multiset.
The exact portable checker passes eight more C/C++ executions through these
archives. The relinked frozen compiler changes to 468f19e499d7, so all 224
self-host C identities and both existing complete GCC/Clang backend regression
suites run afresh. The suites use the matched candidate runtime prefix.

The 123,998-byte console-span-source-update.zip combines the previous
word, RND and C declaration updates. Extract portable-source.zip, then
string-length-source-update.zip, then console-span-source-update.zip before the
first build; separate word/RND/prototype archives are not required. All 611
runtime source identities pass the unchanged delivered verifier. Follow
[the portable build](c-emitter-portable-build.md), then run the shipped
verify-runtime-console.py with --tool-dir. Its delivered bytes match the checker
actually executed against the qualified archives. No full source/object tree
copy, compiler installation or additional dated test ladder is created.

The first regression run exposed the bounded starting-row recovery gap and was
retained before correction. Archive preflights also caught a cross-volume
hardlink attempt and disproved the assumption that this compiler image would
stay identical; the controller copies only the two small cross-volume inputs
and performs fresh compiler qualification. Installed files and unrelated dirty
work remain unchanged. PNG and user-closed optional/native investigations stay
outside this repair.

Another-host rebuilding, full corpus replay from this source package and
experimental compiler promotion remain unverified. Main emitted-C census totals
remain broad 2,139 GCC / 6,966 Clang and Exampleageddon 3,495 / 8,916 at their
measured cuts. Retained allocation stays below 160 MiB and integration below
192 MiB, with at most two owned heavy workers. Temporary namespaces, programs,
archives and object slots are retired. The overall campaign remains active.

Evidence: console-span native/current/values/Win32/archive/structure/source-update
receipts in the existing qualification directory; the component seal and verifier
are under build/c-emitter-forward-views-20261105.

<!-- end of c-emitter-console-spans.md -->
