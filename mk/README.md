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
- `compiler-sources.mk`, included by `source-graph.mk`, owns compiler subsystem
  discovery and host policy replacement.
- `gfx-sources.mk` lists shared graphics subsystem owners and the native
  `drivers` roots. Graphics target
  replacements remain in `source-graph.mk`, which rejects colliding archive
  member names across C, C++ and assembly sources. See the
  [graphics maintenance guide](../docs/gfx-maintenance.md).
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
5. `compiler-config.mk` chooses the concrete C/C++/binutils tools.
6. `toolchain-flags.mk`, `host-tools.mk`, and `os-flags.mk` turn those tools
   and policy toggles into concrete flags and command behavior.
7. `naming.mk`, `layout.mk`, `build-layout.mk`, and `multilib.mk` derive the
   names and paths used by the rest of the tree.
8. `prereqs.mk` exposes environment checks but does not own the build graph.
9. `source-graph.mk` and `archives.mk` define the artifact graph.
10. `mk/build/*.mk` attach the concrete rules.
11. `mk/bootstrap/*.mk`, `dist.mk`, `tests.mk`, `maketests.mk`, and
    `inst_uninst.mk` add specialized workflows on top.
12. `mk/build/config-rules.mk` compares the resolved commands and attaches
    rebuild dependencies after those workflows have defined their variables.

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
- `gfxlib3`
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

`MULTILIB=32` and `MULTILIB=64` select an x86 ABI before source and directory
selection. BASIC, C, C++, and final C-driver links use the same width. The
selected SDK must provide that width. Other processor families use an explicit
`TARGET_TRIPLET`; a conflicting explicit `FBC_TARGET` is rejected.

`CC` selects the target compiler. `HOST_CC`, when supplied, identifies the
machine running Make. Host detection otherwise prefers an independent native
driver, so a cross compiler cannot make a target-only `fbc` look runnable.
Missing inferred cross tools remain inspectable with `print-config`, but
concrete builds fail prerequisite checks unless an equivalent SDK compiler
was explicitly supplied.

Source-only `bootstrap-emit*` and `bootstrap-dist*` workflows use a runnable
host compiler to emit or archive sources without a target C compiler. SDK
checks still apply when the same invocation also requests a concrete build.

Source and header dependencies, embedded runtime changes, archive-member
selection, and effective build commands are tracked incrementally. Changing
compile flags rebuilds the affected normal compiler/runtime variants. Changing
only archive or link flags republishes the affected outputs without recompiling
their objects. The first build after adopting configuration tracking rebuilds
each requested variant once.

Each compiler variant owns a `.build-config` record in its object directory.
Runtime archives and startup objects own records under
`$(libdir)/.build-config`. These hidden files stay out of installed packages.
A record describes the last successful output and is replaced only after the
archive, startup object or compiler link succeeds. Failed builds retry the
pending configuration; `make -n` and unchanged builds write no records.
An artifact's `.pending` marker is created before recompilation and removed
after publication. It also forces a rebuild when flags are reverted after a
failed attempt, preventing reuse of partially reconfigured objects.
Compiler links use a temporary executable, preserving the previous working
compiler if linking fails. One build process must own a given output tree;
configuration records do not provide interprocess locking.

The fingerprints use POSIX `cksum` with effective commands, flags, target
identity, included product Make rules and selected tool contents. Replacing a
compiler, assembler, archive tool or visible command wrapper at the same path
invalidates its affected outputs. These checks detect changes; they are not
security identities. Private wrapper dependencies and changing SDK
headers/libraries still need component cleanup when absent from the graph:

```sh
make clean-compiler clean-libs
make all
```

The compiler built from bootstrap C/assembly has its own successful-output
record and tracks compile, assembler and link options. Fresh bootstrap emission
uses an owned temporary source tree, preserving intermediates beside maintained
BASIC files. Its record identifies the producing configuration; consumers may
use different host tools and paths. Unfinished attempts are rejected before
bootstrap compilation. Run `make bootstrap-emit` to regenerate sources after
changing emission options. Pre-generated source bundles without an emission
record remain supported; their target comes from the bootstrap directory.

Independent compiler/library builds can choose distinct output roots:

```sh
make -j2 compiler rtlib gfxlib2 BUILD_ROOT=build/debug CFLAGS='-O0 -g'
make -j2 compiler rtlib gfxlib2 BUILD_ROOT=build/release CFLAGS=-O2
```

Each root must be below the checkout's `build/` directory and contain no spaces.
Objects, libraries, binaries, bootstrap output and configuration records stay
under that root. `make clean BUILD_ROOT=build/debug` affects those products.
The built compiler uses that root's runtime libraries; pass `-i /source/inc`
when a test needs maintained include files. Keep one Make owner per root.
Language tests, full/quick workflows, installation and packaging use shared
source paths and require separate worktrees.

`make warning-tests` compares fresh reports without modifying references;
`make warning-tests-update` explicitly accepts an intentional diagnostic change.
`make source-docs-test` checks source-guide links and native navigation, and
`make source-docs-render-test FBDOC=/path/to/fbdoc` checks fresh HTML, fbhelp
and text. These small checks leave no retained output trees.

Keep the same target configuration on both commands. These component clean
targets preserve compiler executables and emitted bootstrap sources. Compiler
BI dependencies are deliberately conservative, so an interface change can
rebuild more modules than a minimal include graph would require.

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

Distribution cleanup precedes every staging branch, and packaging waits for
the complete stage. Runtime staging reuses installation's platform payload
selection. Install manifests record copied source-tree files, retaining
unrelated destination headers. A failed uninstall keeps its manifest for a
retry.

Archives are indexed before replacing their previous image. Their selected
members are recorded under `$(libdir)/.archive-members`; this private metadata
detects source removals and is excluded from installed runtime payloads.

## Test Entrypoints

There are two broad test entry styles:

- direct test wrappers in `tests.mk` for unit/log/warning suites
- build graph and packaging tests in `maketests.mk` and `mk/tests/**/*.mk`

Useful high-level targets:

- `make sanity`
- `make make-contracts-test` (isolated graph, packaging, and ownership regressions)
- `make source-artifacts-test` (reject generated analyzer data under `src/`)
- `make quick-test`
- `make full-test`
- `make compiler-smoke`
- `make compiler-semantic-model-smoke`
- `make compiler-semantic-model-test`
- `make compiler-semantic-corpus-test` (the maintained OMA programs)
- `make compiler-structure-test`
- `make compiler-host-policy-test`
- `make compiler-storage-test`
- `make compiler-test-harness-test`
- `make compiler-backends-test`
- `make bootstrap-emit-test`

The test harness is designed to preserve enough state to avoid rebuilding the
entire world unnecessarily while still checking that the graph behaves the way
the build expects.

`make-contracts-test` uses small disposable fixtures and the production Make
rules. Quick/full workflows also perform shared clean, bootstrap, and packaging
operations; run one such workflow per owned worktree. Their cleanup is broader
than the isolated contract test.

Native smoke commands select the compiler after their prerequisites finish.
This allows an invocation that starts without `bin/fbc` to test the compiler
it just built. Cross builds continue to use the preserved host compiler.
The semantic smoke explicitly depends on the compiler build.

## Where To Edit Common Behavior

Use this rule of thumb when changing the system:

- change `platform.mk` when you are adding or fixing target detection
- change `platform-features.mk` when the target identity is correct but the
  enabled feature set is wrong
- change `toolchain-flags.mk` or `os-flags.mk` when the feature policy is
  correct but the emitted compiler/linker flags are wrong
- change `layout.mk` for install-tree paths
- change `build-layout.mk` for in-tree artifact paths
- change `compiler-sources.mk` for compiler subsystem directories and host
  policy selection; change `source-graph.mk` for runtime source discovery
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
