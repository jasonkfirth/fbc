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

## Remaining entry contexts

The retained `src/tiko_windows_legacy.bas` reaches an external SDK input defect
before emitting C. The bundled AfxNova `AfxGdiplus.inc` and `AfxCom.inc` files
are case-only compatibility stubs that resolve back to themselves on this Windows
volume. Their real providers are not loaded. The installed compiler and the
qualified candidate produce the same eleven frontend diagnostics with the same
176 opened source inputs. The legacy entry and its included `logging.bas`
remain open; neither receives C-census credit. No provider implementation is
reviewed or changed by this check.

A further 125 active entry profiles are prepared from the maintained README
recipes: twelve editor tests with `-g -exx`, 110 toolkit tests, and three toolkit
demos/tools using their ordinary context. The image-specific raster smoke entry
is excluded. Five real navigation entries provide the optional define needed
for the four navigation/layout implementation fragments. These are planned
contexts; the completed baseline below does not qualify foreign platforms.

The supervisor waited on the preceding project's actual process handles, then
refreshed the unstarted plan before launching one checkpointed worker. The
worker independently rejected competing project workers and pinned the fully
recounted immutable `9d60dd50` compiler at startup. It reused C/object/log slots.
The first attempt stopped before saving a profile: the checked editor produced
63,192,259 bytes of C, and its combined C/object/log set reached 67,396,330 bytes
against the original 64 MiB cap. That empty checkpoint is preserved. A serial
replacement used a finite 96 MiB cap and recorded component sizes. All 125
profiles and input pins now reconcile: 124 accepted C units, one stale
localization test, GCC 99 / Clang 4,614, and a 68,087,210-byte peak on
`9d60dd50`. The original raw refused context remains preserved.
It emitted C and compiled objects without running these applications. Normal GUI
includes remain pinned, including incidental image dependencies; image-specific
entries and implementation review stay outside this queue. The twelve archived
UI-baseline source paths remain pending provenance/context review.

At that original compiler cut, the four completed queues had 6,418 profiles /
6,312 accepted C units, GCC 198 / Clang 15,969. Adding the Tiko entry gave 6,419 profiles /
6,313 accepted C units, GCC 198 / Clang 16,011. The forty-two added Clang
observations came from expanded coverage, not a compiler regression. JRPG had
not finished and was excluded from those dated totals.

The entry's maximum owned disk footprint is 18,617,340 bytes under a 64 MiB
limit. C, object and log files are removed after each completed run; existing
audit storage is reused. No installed release binary or library is replaced.
JRPG has now finished and released the development executable; the selected
development compiler is now `3da82d36`. A subsequent private-debug-body recount uses immutable `3da82d36`,
independently of development-prefix promotion.

Evidence in `build/c-emitter-forward-views-20261105/`:

- `current-tiko-main-context-20261112.json.zlib`: original entry and ten warnings.
- `current-tiko-popup-repair-20261112.json.zlib`: exact source changes, production
  fragments, eight value runs, normalized fixture and final lint diagnostics.
- `current-tiko-main-fixed-context-20261112.json.zlib`: repaired full entry.
- `current-tiko-context-reconciliation-20261112.json.zlib`: source/tool/flag
  reconciliation and the separate partial project coverage snapshot.
- `current-tiko-legacy-context-20261112.json.zlib` and
  `current-tiko-legacy-disposition-20261112.json.zlib`: original SDK rejection,
  installed-compiler comparison and self-resolving alias evidence.
- `current-tiko-extra-plan-20261112.json.zlib`: 125 planned active entry contexts;
  the original completed sweep is retained in the v2 context database.
- `current-tiko-extra-queue-20261112.ps1` and
  `current-tiko-extra-context-20261112.py`: sequential admission and checkpointed
  object-only qualification.

The broader compiler/corpus warning goal remains active.

## Localization test recovery and current-compiler recount

The one refusal used the removed private `ConfirmDialogData.confirmButton` and
`cancelButton` fields. The existing public two-choice factory exposes Confirm
at index zero and Cancel at index one. Replace those two private-field calls
with `confirmdialog_GetActionButton(d->deleteDialog, 0)` and
`confirmdialog_GetActionButton(d->deleteDialog, 1)` respectively, retaining the
existing `localeSmoke_Click` wrapper and all assertions. The repaired test is
strict-lint clean and emits 63,281,386 bytes of C with `3da82d36`. Both native
tools accept its object: GCC 1 / Clang 47, peak 67,529,786 bytes. The GCC warning
is an unread `fontPointSize` source variable. No GUI execution is claimed.
The source was already untracked; only the small accessor repair is owned here.
The [accessor patch](c-emitter-tiko-localization-accessor.patch) contains only
these hunks, preserving the surrounding project work.

The completed 125-profile current-compiler recount preserves the old raw database and pins
the repaired source explicitly. It reuses native diagnostics only for exact C,
tools and flags; changed debug bodies receive fresh checks. Added diagnostics
remain counted, and losing a pre-existing non-function warning still stops the
controller. In the first checked editor, all four existing null warnings remain
and two more appear in `tiko_PrepareTextRenderState`: a null widget can match a
null editor global before `w->data` is accessed. That source guard remains open.
All 125 contexts now accept C, with GCC 40 / Clang 4,593. The 68 unused-function
observations disappear from each tool. Every pre-existing non-function
diagnostic line is preserved, and eight additional GCC null observations arise
from the same source issue across four debug contexts. The remaining GCC
classes are 28 null-dereference, eleven unread-variable and one array-bounds
observation. All 2,078 source parameter warnings and 801 variable warnings
remain. Exact native-result reuses cover 120 profiles per tool; five profiles
receive fresh checks. The owned peak is 67,508,187 bytes under a 96 MiB cap.
This completed cut stays separate from overlapping retained/project contexts.

Evidence uses `current-tiko-extra-terminal-reconciliation-20261112.json.zlib`,
`current-tiko-localization-accessor-preimage-20261112.json.zlib`,
`current-tiko-localization-accessor-20261112.json.zlib`,
`current-tiko-extra-debug-body-v5-20261112.sqlite3`,
`current-tiko-debug-body-cohort-close-v5-20261112.json.zlib`, and
`current-tiko-debug-body-new-null-review-20261112.json.zlib` in the existing audit
directory. The source preimage and public accessor calls make this test repair
reviewable without publishing unrelated untracked project work.

<!-- end of c-emitter-tiko-context.md -->
