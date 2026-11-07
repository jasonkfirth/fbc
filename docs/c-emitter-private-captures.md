<!--
Project: FreeBASIC C backend private capture layout
File: c-emitter-private-captures.md
Purpose: Record declaration-only ERROR capture grouping and its qualification.
Responsibilities: Separate completed compiler gates from pending full censuses.
This file intentionally does NOT claim every remaining warning is repaired.
-->

# Private ERROR capture layout

The qualified compiler is selected at `build/main-gcc/bin/fbc.exe`, SHA256
`d7676a4406975e29b62f998b9711647a81164469adbdbfd3bcdefce4d202e45e`. Canonical compiler sources contain the repair.
The immediate rollback is `gcc-opaque-main.exe` (D44B6DDA). Installed binaries
and matching normal ST/MT archives are unchanged.

The C backend groups private heap-capture declarations by alignment while keeping
their logical IDs. The runtime prefix and resource flags stay fixed. Initializers,
value accesses and cleanup keep their source order. Wider fields remain boundaries
between runs so an unaligned fixed prefix cannot cause record growth. Native
`va_list` substitutions keep their original declaration order. Public BASIC and
foreign record declarations are unchanged.

## Completed checks

- All 224 compiler units reproduce their emitted C at the self-host fixed point.
  GCC reports zero warnings; only the C backend object needed recompiling.
- All 703 fbctests C outputs match the predecessor exactly and retain zero GCC
  warnings. The complete GCC backend suite passes in 187.211 seconds with no
  whole-suite timeout.
- All 95 affected corpus profiles pass with both C generators and both native
  compilers. C outside private record field declarations is byte-identical after
  canonicalizing only those declaration orders.
- Complete Clang record dumps include unused private definitions. GCC static
  guards verify every size, alignment and member offset. No capture grows; fixed
  prefix and borrowed-reference offsets stay unchanged.
- The mixed capture fixture passes 32 native value runs across both C generators,
  GCC/Clang, debug modes and ST/MT links. Sixteen Win32/Win64 field maps cover every
  declaration. Win32 qualification is definitions only; its predecessor's computed
  GOTO observations stay counted and do not increase.
- Three strict lints have zero errors/warnings. Structural checks pass 405 compiler
  files / 21 host graphs and 1,260 runtime files / 126 graphs / three guards.
- The selected public view passes emission, unsuppressed GCC compilation, automatic
  SDK lookup, linking and the actual regression executable.

The cohort's private padding observations change from 205 internal plus five tail
observations to 128 tail observations. All 205 measured internal gaps are removed;
the total diagnostic count drops by 82 because natural end padding is still
reported. There are no increases in other warning classes. Capture sizes save
280 bytes when summed across the cohort's profile-specific records, per generator;
this is a static layout sum, not a measured per-process memory saving.

The large Scorched Earth graphics profile passes: GCC 75.812 seconds before and
67.888 after, padding 74 to 70 and 16 bytes saved across its capture records.
These are individual observations, not a general performance guarantee.

Original native warning counts are kept separate from forced layout-dump
diagnostics. Forcing layouts also diagnoses otherwise unused public types, so its
larger count is never substituted for the original unsuppressed census.

## Full queue refresh

Both full original diagnostic queues and the separate normal-mode GCC replay
are complete for the selected compiler. The totals are 231 GCC / 10,491 Clang;
normal-mode production/additional has three GCC observations. The whole Clang
reduction is exactly 82 padding observations; every other warning class is unchanged.
The eight deliberate/closed Exampleageddon GCC observations remain explicit.
PNG, moving Optical and closed optional/native/assembler boundaries remain closed.

There are 10,920 verified source pins. The same bounded work directories are used,
with 160 MiB retained / 192 MiB transient limits. The immediate predecessor is
retained; the older 64566329 compiler image is retired after selection. Historical
C and object snapshots are preserved in a shared compressed journal that exactly
reconstructs all four original JSON and compressed-receipt hashes, saving 1,681,097
bytes. No corpus application is executed. The overall campaign remains active.

Evidence: `generated-c-capture-layout-{build-v4,fixture-v4,field-maps,suite,fbctests,
compiler-clang,structure,cohort-v5,padding-disposition,publish,inputs}.json.zlib`
in the existing qualification directory. Controllers are in
`build/c-emitter-forward-views-20261105`. Failed fixture/harness attempts remain
archived; completed source/native proofs are reused only with explicit identities.

<!-- end of c-emitter-private-captures.md -->
