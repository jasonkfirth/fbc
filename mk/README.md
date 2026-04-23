# FreeBASIC Build System

Project: FreeBASIC Build System
--------------------------------

File: README.md

Purpose:

Explain how the `mk/` build system is organized, how the major layers fit
together, and how a normal build flows from target identity to final
artifacts.

Responsibilities:

- describe the architectural layers loaded by `GNUmakefile`
- explain the main build flow for compiler, runtime, bootstrap, packaging,
  and tests
- document the intended boundaries between policy, layout, graph, and rule
  modules

This document intentionally does NOT contain:

- a line-by-line explanation of every `.mk` file
- a complete target reference for every test helper
- detailed porting instructions for bootstrap recovery workflows

## Section Map

1. Build system mental model
2. Include order and why it matters
3. Normal build flow
4. Bootstrap and compiler availability
5. Packaging and installation
6. Test entrypoints
7. Where to edit common behavior

## Build System Mental Model

The current build is intentionally layered.

The important design rule is that each layer answers one class of question:

- `platform.mk` answers "what are we building for?"
- `platform-features.mk` and `feature-policy.mk` answer "what should be enabled?"
- `toolchain-flags.mk`, `os-flags.mk`, and `compiler-config.mk` answer
  "how do we express that policy to the compiler tools?"
- `naming.mk`, `layout.mk`, and `build-layout.mk` answer
  "what are the artifacts called and where do they live?"
- `source-graph.mk` and `archives.mk` answer
  "what gets built?"
- `mk/build/*.mk` answer "how do we build and clean it?"
- `mk/bootstrap/*.mk` answer "how do we recover or regenerate the compiler?"
- `mk/tests*.mk` and `mk/tests/*.mk` answer "how do we verify the build graph?"

That separation matters for long-term maintenance.

If target detection leaks into packaging rules, or install layout leaks into
compile rules, small target additions turn into wide refactors. The current
structure is meant to keep those changes local.

## Include Order And Why It Matters

`GNUmakefile` is the entrypoint and loads `mk/` in a specific order.

The order is not arbitrary:

1. `version.mk` exposes the release identity.
2. `platform.mk` normalizes the host/target triplet, OS, architecture, and
   runtime target naming.
3. `cpu.mk` fills in ARM sub-architecture details that depend on the platform
   layer.
4. `platform-features.mk` and `feature-policy.mk` translate identity into
   feature toggles.
5. `toolchain-flags.mk`, `host-tools.mk`, `os-flags.mk`, and
   `compiler-config.mk` turn those toggles into concrete tools and flags.
6. `naming.mk`, `layout.mk`, `build-layout.mk`, and `multilib.mk` derive the
   names and paths used by the rest of the tree.
7. `prereqs.mk` exposes environment checks but does not own the build graph.
8. `source-graph.mk` and `archives.mk` define the artifact graph.
9. `mk/build/*.mk` attach the concrete rules.
10. `mk/bootstrap/*.mk`, `dist.mk`, `tests.mk`, `maketests.mk`, and
    `inst_uninst.mk` add specialized workflows on top.

Because the later layers depend on the earlier ones, debugging almost always
starts by checking `make print-config` and confirming that target identity and
layout resolved the way you expected.

## Normal Build Flow

The default target is `all`.

`all` expands to:

- `libs`
- `compiler-stage`

`libs` builds:

- `rtlib`
- `fbrt`
- `gfxlib2`
- `sfxlib`

`compiler-stage` then builds the compiler after the runtime pieces it needs are
available.

In practical terms, the build flow looks like this:

1. Resolve the target and toolchain variables.
2. Decide which runtime variants exist for the target:
   PIC, non-PIC, multithreaded, or combinations of those.
3. Discover source files for compiler, runtime, graphics, and sound layers.
4. Compile objects into per-target object directories under `src/*/obj`.
5. Archive runtime libraries into the canonical build runtime directory.
6. Link the compiler against the runtime selected for the current policy.

Runtime C++ compilation is currently opt-in by target.

At the moment only Haiku enables runtime/backend `.cpp` sources in `gfxlib2`
and `sfxlib`. Other targets stay on the pure-C path unless a target is
explicitly promoted into that policy later.

The runtime directory key is `FBTARGET`.

That value is intentionally separate from packaging aliases such as
`linux-amd64` or `mingw-x86_64`. The build tree and install tree use the
canonical runtime key so that different packaging names do not silently create
different runtime layouts.

## Bootstrap And Compiler Availability

The build now distinguishes between three states:

1. A local compiler already exists at `$(FBC_EXE)`.
2. No local compiler exists, but a system `fbc` is available.
3. No compiler exists, so the build must rely on bootstrap sources.

Targets that actually need a runnable FreeBASIC compiler now flow through
`prereqs-fbc`.

That check deliberately fails early instead of allowing a recipe to degrade into
a malformed command line where shell execution starts at the first compiler
flag.

The intended recovery path is:

- If current-target bootstrap sources exist: run `make bootstrap-minimal`
- If they do not exist: stop with an error
- If a peer same-architecture bootstrap exists and you explicitly want a
  last-resort recovery path: run `make bootstrap-seed-peer`

`bootstrap-seed-peer` is intentionally not part of the default graph.

It is a manual recovery tool for bring-up work, not a normal build step.

## Packaging And Installation

There are two packaging layers:

- `dist.mk` stages a distributable tree rooted under `dist/`
- `inst_uninst.mk` installs into the configured prefix and records an install
  manifest for later uninstall

Installation layout policy lives in `layout.mk`.

Build-tree layout lives in `build-layout.mk`.

That separation is important because a target may need one runtime layout for
the in-tree compiler and a different destination prefix for final installation.

## Test Entrypoints

There are two broad test entry styles:

- direct test wrappers in `tests.mk` for unit/log/warning suites
- build graph and packaging tests in `maketests.mk` and `mk/tests/**/*.mk`

Useful high-level targets:

- `make sanity`
- `make quick-test`
- `make full-test`
- `make compiler-smoke`
- `make bootstrap-emit-test`

The test harness is designed to preserve enough state to avoid rebuilding the
entire world unnecessarily while still checking that the graph behaves the way
the build expects.

## Where To Edit Common Behavior

Use this rule of thumb when changing the system:

- change `platform.mk` when you are adding or fixing target detection
- change `platform-features.mk` when the target identity is correct but the
  enabled feature set is wrong
- change `toolchain-flags.mk` or `os-flags.mk` when the feature policy is
  correct but the emitted compiler/linker flags are wrong
- change `layout.mk` for install-tree paths
- change `build-layout.mk` for in-tree artifact paths
- change `source-graph.mk` when the wrong source files are being selected
- change `mk/build/*.mk` when the graph is correct but the rules are wrong
- change `mk/bootstrap/*.mk` for compiler recovery and source emission flows
- change `mk/tests/**/*.mk` when adding new build-system regression coverage

## Recommended First Commands

For someone opening this tree for the first time, the most useful commands are:

```sh
make print-config
make sanity
make mk-structure-test
make prereqs-fbc
```

Those commands give a quick picture of target resolution, environment health,
and whether the current tree already has enough compiler/bootstrap state to
proceed.

<!-- end of README.md -->
