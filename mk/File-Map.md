# FreeBASIC Build System File Map

Project: FreeBASIC Build System
--------------------------------

File: File-Map.md

Purpose:

Provide a file-by-file map of the `mk/` tree so a new developer can quickly
find the module that owns a specific build concern.

Responsibilities:

- list the purpose of each root `.mk` file
- describe the role of the `build/`, `bootstrap/`, and `tests/` subtrees
- help future maintainers avoid editing the wrong layer

This document intentionally does NOT contain:

- the full contents of each make fragment
- detailed target usage examples
- policy rationale that is already covered in `README.md`

## Root Modules

- `archives.mk`: defines the final runtime artifacts and linker-script outputs.
- `build-layout.mk`: defines the in-tree directory layout for objects,
  libraries, and compiler binaries.
- `compiler-config.mk`: chooses C/C++/binutils tools, resolves local or system
  `fbc`, and derives the compiler used during the build.
- `cpu.mk`: derives ARM sub-architecture and default ARM CPU policy.
- `dist.mk`: stages and archives a distributable build tree.
- `feature-policy.mk`: applies high-level feature toggles that do not belong in
  target detection.
- `host-tools.mk`: chooses host-side utility behavior such as install/copy
  commands.
- `inst_uninst.mk`: installs, uninstalls, and creates simple package payloads.
- `layout.mk`: defines install-prefix and install-runtime layout policy.
- `maketests.mk`: loads the build-system test harness and high-level test
  targets.
- `multilib.mk`: applies `MULTILIB` settings to FreeBASIC and C toolchains.
- `naming.mk`: defines installed namespace and linker-script names.
- `os-flags.mk`: carries narrowly-scoped OS quirks that do not belong in the
  broader flag policy.
- `platform-features.mk`: translates target identity into feature toggles such
  as PIC, threading, graphics, and hardening.
- `platform.mk`: canonicalizes target identity, runtime target naming, and
  packaging directory naming.
- `prereqs.mk`: checks host tools, libraries, compiler availability, and
  bootstrap recovery state.
- `source-graph.mk`: selects source files and derives canonical object lists.
- `supported_targets.mk`: defines the supported bootstrap distribution matrix.
- `tests.mk`: provides direct wrappers for the language test suites.
- `toolchain-flags.mk`: translates build policy into concrete compiler and
  linker flags.
- `version.mk`: exposes the project version used by packaging and diagnostics.

## Build Mechanics Subtree

The `mk/build/` directory owns the core build rules.

- `build/build-dirs.mk`: creates every object and output directory used by the
  graph.
- `build/compile-rules.mk`: contains the compile rules for compiler, runtime,
  graphics, and sound objects.
- `build/dependency-rules.mk`: includes generated `.d` dependency files for
  runtime objects.
- `build/archive-rules.mk`: archives object sets into the final static
  libraries.
- `build/build-targets.mk`: defines high-level build targets such as `compiler`
  and `libs`.
- `build/clean-rules.mk`: centralizes normal and deep clean behavior.

## Bootstrap Subtree

The `mk/bootstrap/` directory owns compiler recovery and bootstrap packaging.

- `bootstrap/bootstrap-core.mk`: builds a minimal compiler from emitted C/ASM
  sources and now also exposes the manual `bootstrap-seed-peer` recovery path.
- `bootstrap/bootstrap-emit.mk`: uses a runnable `fbc` to emit bootstrap C/ASM
  sources for the current target.
- `bootstrap/bootstrap-dist.mk`: stages and archives bootstrap source
  distributions, including matrix helpers.

## Test Subtree

The `mk/tests/` directory owns build-system verification rather than language
implementation logic.

- `tests/harness.mk`: shared helpers, temporary directories, and command
  wrappers.
- `tests/build/structure.mk`: checks that the `mk/` tree contains the expected
  modules and rejects stray backup files.
- `tests/build/graph.mk`: exercises the build graph, rebuild behavior, and
  selected matrix combinations.
- `tests/build/clean.mk`: verifies clean behavior and idempotence.
- `tests/build/dependency.mk`: checks generated dependency handling.
- `tests/bootstrap/bootstrap.mk`: verifies bootstrap build and bootstrap source
  emission.
- `tests/bootstrap/stage.mk`: checks staged bootstrap layouts.
- `tests/bootstrap/dist.mk`: checks bootstrap distribution output.
- `tests/bootstrap/matrix.mk`: checks bootstrap matrix generation.
- `tests/compiler/smoke.mk`: verifies that a resolved compiler can compile a
  trivial program.
- `tests/compiler/language.mk`: runs the language-suite wrappers and captures
  logs.
- `tests/packaging/packaging.mk`: checks packaging targets.
- `tests/packaging/install.mk`: checks install and uninstall behavior.

## Practical Lookup Table

If you are asking one of these questions, start here:

- "Why did the target resolve to this OS or architecture?" -> `platform.mk`
- "Why is PIC or threading enabled?" -> `platform-features.mk`
- "Why is this exact warning or hardening flag present?" -> `toolchain-flags.mk`
- "Why is the runtime landing in this directory?" -> `layout.mk` or
  `build-layout.mk`
- "Why did this source file get selected?" -> `source-graph.mk`
- "Why did a library or linker script not get created?" -> `archives.mk` and
  `build/archive-rules.mk`
- "Why did the compiler fail before it even started?" -> `compiler-config.mk`
  and `prereqs.mk`
- "Why can bootstrap work on one target but not another?" ->
  `bootstrap/bootstrap-core.mk` and `bootstrap/bootstrap-emit.mk`
- "Where should I add a regression test?" -> `mk/tests/`

<!-- end of File-Map.md -->
