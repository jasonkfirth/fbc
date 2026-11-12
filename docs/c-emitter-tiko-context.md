<!--
Project: FreeBASIC C backend warning qualification
File: c-emitter-tiko-context.md
Purpose: Record Tiko's maintained entry context and demonstrated source faults.
Responsibilities: Bind warning counts, source repairs, value checks and limits.
This file does not qualify editor rendering or optional feature initialization.
-->

# Tiko maintained entry, November 12

`C:/fb_corpus/tiko-editor/_compile.bat` and `_compile.sh` build `src/tiko.bas`
with `-O 2` and `-i src/omaGUI-main`. The audit retains that native Win64,
gfxlib2 context, adds `-w all`, and emits C with the immutable qualified
conversion compiler `56cc73187b5208e476725afd1e7df9cd5f90944c7ae86c02aefb8747dbc2179a`.
Both native compilers use the existing aggressive, unsuppressed diagnostic
profiles. The packaging commands and editor are not executed.

The initial entry passes C compilation but produces ten GCC null-dereference
warnings in completion and call-tip popup paths. In this source graph the two
shared popup pointers have no assignments. Geometry routines dereference those
owners, and update routines can continue without a popup or its data object.
These are application source faults, not a reason to weaken generated-C
diagnostics or change valid examples.

Four checks now return before those accesses: each update requires its popup
data, and each geometry routine requires its editor and popup owner. Call-tip
geometry also requires its data object. The change does not create or enable
the missing optional popups. When they are unavailable the routines return;
when valid objects exist the placement calculations remain unchanged.

The repairs touch only `src/tiko_native_completion.bi` and
`src/tiko_native_calltips.bi`. Both files already contain uncommitted project
work, so their full contents are not staged or committed as this audit's work.
Exact preimages and postimages remain in the compressed repair receipt. The
[four source hunks](c-emitter-tiko-popup-guards.patch) can be reviewed or applied
to a matching Tiko source tree without copying the surrounding dirty files.

| Maintained entry | GCC | Clang |
| --- | ---: | ---: |
| Before popup checks | 10 | 42 |
| After popup checks | 0 | 42 |

The forty padding and two required startup-registration Clang observations
remain visible. The full entry is freshly emitted and compiled by both native
tools after the source repair. All 168 opened input paths are pinned; the only
changed inputs are the two repaired headers. Compiler, native tools, diagnostic
flags and installed artifacts remain unchanged.

Eight focused native value runs use the exact production geometry bodies and
actual public Widget, TextBoxData and popup data types. Two baseline runs retain
the valid placement result. Six repaired runs cover GCC and Clang at O0/O2,
GAS64 and LLVM, checking absent owners, empty call-tip data, unchanged placement
and bottom/right clamping. Text measurement and screen size use a fixed adapter;
these checks do not establish whole-editor or graphics behavior. Both edited
headers are strict-lint clean. The fixture retains one advisory about its test
success print; its line endings were normalized and checked again.

The entry supplies owning-profile evidence for fifty previously pending,
non-image BASIC paths. Its included raster-image implementation remains outside
the requested image-work scope and is not claimed as a reviewed repair.
The other Tiko tests, archived UI baselines and standalone units remain pending.
This single completed profile remains separate from the unfinished 241-program
JRPG sweep and the queued 194-profile fb-ext-lib sweep.

The previous four completed queues remain 6,418 profiles / 6,312 accepted C
units, GCC 198 / Clang 15,969. Adding this current Tiko entry gives 6,419 profiles /
6,313 accepted C units, GCC 198 / Clang 16,011. The forty-two added Clang
observations come from expanded coverage, not a compiler regression. Partial
JRPG warnings are not included in either completed total.

The entry's maximum owned disk footprint is 18,617,340 bytes under a 64 MiB
limit. C, object and log files are removed after each completed run; existing
audit storage is reused. No installed binary or library is replaced. The selected
development executable stays pinned for the live JRPG worker.

Evidence in `build/c-emitter-forward-views-20261105/`:

- `current-tiko-main-context-20261112.json.zlib`: original entry and ten warnings.
- `current-tiko-popup-repair-20261112.json.zlib`: exact source changes, production
  fragments, eight value runs, normalized fixture and final lint diagnostics.
- `current-tiko-main-fixed-context-20261112.json.zlib`: repaired full entry.
- `current-tiko-context-reconciliation-20261112.json.zlib`: source/tool/flag
  reconciliation and the separate partial project coverage snapshot.

The broader compiler/corpus warning goal remains active.

<!-- end of c-emitter-tiko-context.md -->
