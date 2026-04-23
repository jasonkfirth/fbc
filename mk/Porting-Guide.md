# FreeBASIC Build System Porting Guide

Project: FreeBASIC Build System
--------------------------------

File: Porting-Guide.md

Purpose:

Describe the practical workflow for bringing up a new operating system or
target tuple in the current build system, especially when compiler bootstrap
state is incomplete.

Responsibilities:

- explain the preferred target-identification workflow
- document the bootstrap recovery ladder
- describe the intended use of `bootstrap-seed-peer`

This document intentionally does NOT contain:

- every low-level flag used by each target
- language runtime porting details outside the build system
- a promise that peer-seeded bootstraps are ABI-correct without review

## Start With Target Identity

The first question in any bring-up is not "what flags do I need?"

It is "what does the build think this target is?"

The preferred diagnostic command is:

```sh
make print-config TARGET_TRIPLET=<your-triplet>
```

`TARGET_TRIPLET` is the canonical variable inside the current build layers.

`TARGET` still exists as a legacy input, but it is consumed early and then
cleared. For new work, `TARGET_TRIPLET` is the most direct way to inspect the
resolved target state.

One naming detail matters for Apple Silicon.

The build currently uses the canonical architecture key `aarch64`, so the
Darwin bootstrap matrix entry is `darwin-aarch64` even though that corresponds
to macOS arm64 hardware.

Things to verify first:

- `TARGET_OS`
- `TARGET_ARCH`
- `ISA_FAMILY`
- `FBC_TARGET`
- `FBTARGET`
- `FBPACK_DIR`
- install and build runtime directories

If those values are wrong, fix `platform.mk`, `cpu.mk`, or `layout.mk` first.
Do not start changing compile rules until identity is correct.

## Normal Bring-Up Ladder

When the build needs a runnable FreeBASIC compiler, the intended recovery ladder
is now explicit.

### Case 1: local compiler exists

If `bin/fbc` or `bin/fbc.exe` already exists, the build uses the in-tree
compiler.

This is the normal self-hosted path.

### Case 2: no local compiler, but system `fbc` exists

If the tree has no local compiler but the host system has an `fbc` in `PATH`,
the build can still emit bootstrap sources or build the compiler.

This is the normal "bootstrap from installed compiler" path.

### Case 3: no compiler, but current-target bootstrap sources exist

If there is no usable compiler but `bootstrap/<current-target>/` already
contains emitted `.c` or `.asm` files, the correct next step is:

```sh
make bootstrap-minimal
```

This builds a minimal compiler from the emitted bootstrap sources and installs
it into the tree.

### Case 4: no compiler and no current-target bootstrap sources

In this state the build now fails early through `prereqs-fbc`.

That is intentional.

The old failure mode was to continue into compiler recipes and let shell
execution degrade into nonsense once the compiler command was missing.

The new behavior tells you that there is no compiler and no bootstrap available
for the current target.

## Last-Resort Recovery: `bootstrap-seed-peer`

`bootstrap-seed-peer` exists for one narrow purpose:

seed the current target's bootstrap directory from a peer target of the same
architecture when no compiler is available and no current-target bootstrap
sources exist.

The intended workflow is:

1. Copy emitted `.c` and `.asm` sources from a peer target with the same
   architecture.
2. Build `bootstrap-minimal`.
3. Use that compiler to run `bootstrap-emit` for the current target.
4. Rebuild `bootstrap-minimal` from the newly emitted current-target sources.

That target is deliberately manual.

It is not part of the default graph because peer bootstrap sources are a
recovery tool, not a correctness guarantee. They are useful when the compiler
source is close enough across sibling targets to get a minimal compiler over the
line, but they still deserve review.

## Donor Selection Policy

The peer-seed workflow prefers same-architecture donors.

For BSD-family targets it prefers other BSD-family bootstrap directories first:

- OpenBSD
- NetBSD
- FreeBSD
- DragonFly

After that it falls back to other same-architecture bootstrap directories.

This bias exists because sibling BSD targets are usually closer to each other
than a BSD target is to Haiku, Solaris, or Windows, but the build still keeps
the broader same-architecture fallback available for manual recovery work.

## Recommended Bring-Up Sequence For A New Target

1. Run `make print-config TARGET_TRIPLET=<triplet>`.
2. Confirm that target identity, runtime naming, and install layout are right.
3. Run `make prereqs TARGET_TRIPLET=<triplet>` to validate host tools.
4. Run `make prereqs-fbc TARGET_TRIPLET=<triplet>` to see which bootstrap state
   you are in.
5. If bootstrap sources exist, run `make bootstrap-minimal TARGET_TRIPLET=<triplet>`.
6. If they do not exist but a peer bootstrap is acceptable as a last resort,
   run `make bootstrap-seed-peer TARGET_TRIPLET=<triplet>`.
7. Once a compiler exists, run `make bootstrap-emit TARGET_TRIPLET=<triplet>`.
8. Rebuild `make bootstrap-minimal TARGET_TRIPLET=<triplet>` so the current
   target uses its own emitted bootstrap sources.
9. Run `make sanity TARGET_TRIPLET=<triplet>` and then the relevant smoke or
   matrix tests.

## What To Change For Common Porting Problems

- wrong OS or architecture name: `platform.mk`
- wrong ARM subtype or ABI: `cpu.mk`
- wrong hardening or threading policy: `platform-features.mk`
- wrong toolchain flags for the target: `toolchain-flags.mk` or `os-flags.mk`
- wrong install or runtime directory layout: `layout.mk`
- wrong build-tree directory layout: `build-layout.mk`
- missing or wrong source override precedence: `source-graph.mk`
- unexpected C++ runtime compilation on a target: `source-graph.mk`
- bootstrap compiler recovery problems: `mk/bootstrap/*.mk`

## Important Constraint

Do not treat a successful peer-seeded bootstrap as final proof that the target
port is complete.

The goal of that recovery path is to get a runnable compiler so the tree can
emit native bootstrap sources for the target and then rebuild from those native
sources.

That is why the recovery target performs:

1. `bootstrap-minimal`
2. `bootstrap-emit`
3. `bootstrap-minimal`

The second minimal bootstrap is the real stabilization step.

<!-- end of Porting-Guide.md -->
