<!--
Project: FreeBASIC corpus warning qualification
File: c-emitter-tiko-widget-guards.md
Purpose: Record Tiko widget lifecycle guards and their native qualification.
Responsibilities: Describe source ownership, warning counts and runtime limits.
This file does not qualify editor rendering or install compiler/runtime binaries.
-->

# Tiko widget lifecycle guards, November 13

The completed 125-context Tiko/toolkit cut on compiler
`fc092d79d744a7491bf980fade21705900fbd39dd430dcde0898327f089f85f9`
has 29 GCC observations. Twenty-eight identify nullable accesses in three
application helpers. They recur in five debug contexts that retain the source
bodies. The compiler and native diagnostic options keep their existing policies.

Three small source checks address those paths:

- `tiko_PrepareTextRenderState` rejects an absent widget before comparing it
  with editor handles. An absent handle could otherwise match an absent widget.
- `tiko_ShowOutputPage` returns when its output widget is absent. A nonnull
  widget without data retains the previous page-selection behavior.
- `tiko_MenuBarSwitch` checks the popup after `tiko_ShowMenu`. That routine can
  return without a popup; its caller must check before reading coordinates.

The changes touch `src/tiko.bas` and `src/tiko_native_ui.bi` in
`C:/fb_corpus/tiko-editor`. These files contain unrelated project edits, which
remain intact. The [reviewable patch](c-emitter-tiko-widget-guards.patch)
contains only the guards and the added `tiko_widget_lifecycle_smoke.bas` test.
It applies to a matching source snapshot; it does not publish the surrounding
dirty tree. Patch application was checked against the recorded preimages, and
the resulting logical source matches the repaired files.

| Tiko/toolkit coverage | GCC | Clang |
| --- | ---: | ---: |
| Previous 125 contexts | 29 | 4,582 |
| Same 125 after the guards | 1 | 4,582 |
| Added lifecycle fixture | 0 | 46 |
| Complete expanded 126 contexts | 1 | 4,628 |

All 126 contexts emit C and pass GCC and Clang under the original aggressive,
unsuppressed flags. Every non-null native warning line in the original 125 is
preserved after mapping the inserted source lines. Its one remaining GCC bounds
observation remains open. Clang retains natural padding and startup-registration
diagnostics. The extra 46 observations belong to the newly added fixture.

All original source diagnostics are preserved: 2,078 parameter warnings,
801 variable warnings and 122 fixed-range warnings. The new fixture adds
39 parameter, 22 variable and one fixed-range observation from its full source
context. No source diagnostic is removed to obtain the GCC reduction.

The headless fixture passes twelve assertions with each native backend. It
covers absent editor handles, missing widget data, a normal text owner, output
page state and missing/inactive menus. It does not initialize the GUI or test
rendering. A compatible complete installation can run it from the matching
Tiko checkout:

```sh
fbc -gen gcc -g -Wc -g0 -exx -i src/omaGUI-main tests/tiko_widget_lifecycle_smoke.bas -x lifecycle-smoke.exe
./lifecycle-smoke.exe
```

Repeat with `-gen clang`. BASIC debug retention and asserts remain enabled;
native `-g0` avoids unnecessary DWARF in this value test. The local development
prefix contains only the core runtime archives. Qualification supplied temporary
copies of the installed graphics/sound archives and pinned their hashes, keeping
the selected runtime and installed archives unchanged. The value-test scratch
peak is 78,837,415 bytes under a 96 MiB cap. Strict lint has no errors and one
pre-existing large-file banner advisory in `tiko_native_ui.bi`.

The complete retained census was also freshly emitted: 6,419 profiles,
6,313 accepted C units, GCC 14 / Clang 15,823. Only the maintained Tiko entry's
C changes; every native warning class and source warning line remains unchanged.
The other 6,418 checkpoints were preserved when correcting the audit's source
warning location parser. This parser correction changes no profile or diagnostic
policy. Recovered fbrtLib and fb-ext-lib inputs and compiler hashes are unchanged.

The later CI commit `feb2c2921` changes two recorded domain files: the separate
aggregate-varargs test and the semantic-test Make recipe. Neither is an input
to any of these retained emissions. Their previous hashes match `00df987a2`;
the current compiler, observed profile inputs and other domain hashes still
match. A separate publication seal records that reconciliation without
rewriting the completed databases or repeating identical native work.

Receipts reuse `build/c-emitter-forward-views-20261105/`:
`current-tiko-widget-guards-20261113.sqlite3`,
`tiko-widget-retained-refresh-20261113.sqlite3`,
`tiko-widget-guards-close-20261113.json.zlib`,
`tiko-widget-values-20261113.json.zlib`, and
`tiko-widget-patch-check-20261113.json.zlib`, and
`tiko-widget-current-seal-20261113.json.zlib`.
Their 64/96 MiB census caps reuse fixed C/object/log slots. Full application
execution, rendering, foreign providers and unrelated native-library edits
remain outside this qualification.

<!-- end of c-emitter-tiko-widget-guards.md -->
