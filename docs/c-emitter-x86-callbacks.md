<!--
Project: FreeBASIC compiler verification
File: c-emitter-x86-callbacks.md
Purpose: Record the typed x86 callback repair and its qualification boundary.
Responsibilities: Explain dispatch ownership, warning changes and saved checks.
This file intentionally does NOT qualify concurrent semantic-tooling edits.
-->

# Typed x86 callbacks, November 2, 2026

The x86 opcode table now stores the concrete callback signatures used by its
node classes. Previously every handler was cast to `SUB()` on assignment and
cast back during dispatch. The new union keeps one pointer per slot and names
the eleven callback families. The node class selects the corresponding member.

The base initializer explicitly assigns all 145 opcode slots, including seven
disabled slots, before the 46 SSE overrides are installed. This also resets the
base table on subsequent initialization. Two fixed x87 conversion delegates
replace mutable cached fallback pointers. The handlers and opcode ordering are
unchanged.

`build_scripts/check-x86-emitter-callbacks.py` checks each binding against its
handler signature, the complete opcode set, disabled slots, dispatch members,
SSE overrides, fallback delegates and initialization order. The ordinary compiler
structure checker invokes it. Its six in-memory negative controls reject a wrong
member, missing slot, wrong same-signature handler, wrong SSE member, missing
base reset and erased callback storage.

## Pinned compiler result

Evidence lives in `build/c-emitter-x86-callbacks-20261102`. The noninstalled
candidate is `candidate/fbc.exe`, SHA-256
`777d9c34f9cda3d4264476fcf10693a117bb1b3954b60bc66ae37f5e12e59454`.
Its receipt records the complete build-time source snapshot.

All 224 compiler C units were freshly emitted. Five changed C units were
compiled with expanded, unsuppressed GCC diagnostics and Clang
`-Weverything -Wwrite-strings`; unchanged C retained its exact prior object
checks. Reproducing the old link first verified the inherited object graph.

| Compiler diagnostic | Before | After |
| --- | ---: | ---: |
| GCC warnings | 0 | 0 |
| Clang strict function-pointer casts | 183 | 0 |
| Clang unsafe buffer usage | 3,104 | 3,102 |
| All Clang warnings | 4,975 | 4,790 |

These are warning instances in the selected compilation profiles. Other warning
groups remain counted. No warning flag or diagnostic suppression was added.

## Verification

- All 703 selected unit profiles produced byte-identical old/new Win32 assembly
  with both x87 and SSE: 1,406 successful comparisons, with no frontend failures.
  This checks emitted assembly; it does not claim Win32 execution.
- The native GCC/Clang backend regression suite passed.
- Strict source-only lint passed for the five edited BASIC files: zero errors
  and zero warnings. This is not compiler-backed semantic lint acceptance.
- The callback guard and all six negative controls passed.
- The full live-tree structure check currently fails on missing header
  declarations in 13 concurrently added semantic-tooling include files. Those
  files were preserved. `structure.log` retains every failure.

`c-identity/receipt.json` records fresh non-PNG C comparisons for the selected
unit, production and additional-project cohorts against their latest accepted
objects, including the later JSON source repair. Each emission removes its
scratch output first. Unchanged C does not trigger another object compilation.

## Current source boundary and resources

Parser and semantic-tooling changes arrived after this candidate was built.
The candidate represents its recorded source snapshot. Its binary comparisons
do not qualify those later compiler edits, and the full live source tree has
not passed integration. Callback and SDK source pins are checked independently.

Installed compiler and runtime binaries remain unchanged. Work uses one scratch
C/assembly output, compressed receipts and five incremental compiler objects.
PNG work remains excluded. The broader warning campaign is incomplete.

<!-- end of c-emitter-x86-callbacks.md -->
