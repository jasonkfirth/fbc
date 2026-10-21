<!--
Project: FreeBASIC source documentation
File: source-code/tools-and-tests.md
Purpose: Explain support trees and how implementation changes reach tests.
Responsibilities: Map wrappers, declarations, Make ownership and regressions.
This file intentionally does NOT replace SDK setup or deployment instructions.
-->

# Supporting tools and tests

The implementation trees rely on declarations, target wrappers, Make policy
and regression harnesses maintained elsewhere in the repository. These trees
answer different questions: how a program calls an API, how it is packaged,
which sources enter its libraries, and how its behavior is checked.

## Target wrappers and application bridges

[src/tools](../../src/tools/) contains tools shipped or used with particular
targets. These tools coordinate SDKs and application lifecycle details outside
the compiler's language implementation.

| Directory | Responsibility and entry points |
| --- | --- |
| [android](../../src/tools/android/) | `fbc-android` coordinates native compilation and APK packaging; Java input/activity classes and `fb_android_app.c` bridge the Android application environment |
| [js](../../src/tools/js/) | `fbc-js-app` chooses browser output and stages assets through the Emscripten wrapper |
| [wii](../../src/tools/wii/) | `fbc-wii` supplies devkitPro/libogc tools and ELF/DOL packaging |
| [wince](../../src/tools/wince/) | Windows CE compiler wrappers and architecture-specific invocation defaults |
| [win32](../../src/tools/win32/) | The `fbc-win32` wrapper |
| [xbox](../../src/tools/xbox/) | `fbc-xbox-xiso` packages an Xbox application image |
| [riscos](../../src/tools/riscos/) | Native application `!Run`, `!Boot` and `!Help` integration |

A wrapper can select a compiler and SDK or construct an application package.
The compiler still owns BASIC parsing and code generation. Native services
remain in the runtime libraries. When tracing Android input, for example,
follow the Java/native bridge and the selected runtime driver as well as the
packaging script.

[build_scripts](../../build_scripts/) contains developer automation, source
validators, packaging and target qualification runners. Start at the script
called by the relevant Make target instead of running every similarly named
script. The root `tools` directory also contains local probes and automation;
it is separate from the target helper tree under `src/tools`.

## BASIC declarations and examples

[inc](../../inc/) supplies BASIC-facing declarations for runtime interfaces,
native APIs and external libraries. A `.bi` file describes how compiled code
calls an implementation; it does not necessarily supply that implementation.
Read its calling convention, type widths and structure layout alongside the
runtime or external API it describes.

[examples](../../examples/) demonstrates language and library use. The
`examples/manual` tree supplies compact language examples, while graphics,
sound and platform directories demonstrate larger APIs. Some examples need
optional libraries or SDKs. An example is useful as a reproduction, but a
permanent regression should assert the specific behavior in the test suite.

## Choose the test that owns the failure

The main compiler/runtime suite is [tests](../../tests/), with the harness
described in [tests/readme.txt](../../tests/readme.txt). The `.ss` fixtures
under `src/tests/tmp` are a separate data tree and are not the main language
test entry point.

| Change | Regression starting point |
| --- | --- |
| Accepted/rejected language form or emitted diagnostic | Topic directory under `tests`, using the appropriate `TEST_MODE` |
| Runtime result, lifetime or descriptor behavior | Topic tests built through [unit-tests.mk](../../tests/unit-tests.mk), or a focused native/runtime runner |
| Multi-module or C interoperability behavior | A `.bmk` fixture consumed by [bmk-make.mk](../../tests/bmk-make.mk) |
| Software graphics or queued-renderer behavior | [tests/gfx](../../tests/gfx/) and [tests/gfx3](../../tests/gfx3/) |
| Sound state, timing or captured samples | [tests/sfx](../../tests/sfx/) |
| Source ownership, includes or selected archive members | `check-compiler-structure.py`, `check-rtlib-structure.py` or `check-gfx-source-graph.py` under `build_scripts` |
| Build ordering, tool failures, packaging or cleanup | [mk/tests](../../mk/tests/) and [test-make-contracts.py](../../build_scripts/test-make-contracts.py) |

[tests/log-tests.mk](../../tests/log-tests.mk) handles compile-only,
compile-and-run and multi-module expectations. Its saved records distinguish
expected compiler rejection from abnormal failure or timeout. The
[log-test guide](../compiler-test-harness.md) explains retry selections and
artifact ownership.

`make warning-tests` generates diagnostics in disposable storage and compares
all five target profiles against `tests/warnings/r`. Differences fail the
target without changing the references. Use `make warning-tests-update` only
for an intentional diagnostic change, then review the reference diff.

`make gfx-locking-test` checks the executable
[runtime/graphics lock contract](../gfx-locking.md), including input callbacks,
allocation failure and mode shutdown with native MT libraries.

## Build ownership and generated files

The root [GNUmakefile](../../GNUmakefile) composes the layers under
[mk](../../mk/). Platform identity and feature policy precede source selection;
compile and archive rules consume the resulting lists. Installation and
distribution consume completed artifacts. The
[Make file map](../../mk/File-Map.md) identifies the owner of each decision.

Use `make print-config` to inspect the effective configuration. Structural
entry points include `make compiler-structure-test`, `make gfx-structure-test`,
and `make make-contracts-test`. The runtime structure checker is
[build_scripts/check-rtlib-structure.py](../../build_scripts/check-rtlib-structure.py).
These checks validate ownership and build contracts; behavioral changes still
need the corresponding executable regression.

Object trees, emitted bootstrap sources, linked libraries and test logs are
generated inputs or outputs. Do not edit them to repair the maintained source.
Quick/full build workflows include cleanup and bootstrap/package operations,
so use one workflow per owned worktree. Follow the
[Make guide](../../mk/README.md) for automatic command/flag tracking, record
ownership and selected-tool content fingerprints. Independent compiler/library
builds can select different `BUILD_ROOT=build/name` output directories. Test
and packaging workflows still need their own source worktrees.
`make source-artifacts-test` rejects generated `.dump` and `.ctu-info` files
under maintained source directories; analysis output belongs in build storage.
An incomplete scan fails rather than accepting an unreadable directory.

[doc](../../doc/) contains the generated manual's source cache and tools as
well as API notes. [docs](../README.md) contains maintainer and platform guides.
Keep language syntax documentation, implementation explanations and validation
records linked to each other, with each retaining its own purpose.
`make source-docs-test` checks source-guide links and native manual navigation.
`make source-docs-render-test FBDOC=/path/to/fbdoc` also generates fresh HTML,
fbhelp and text pages in disposable storage and checks their output.

<!-- end of source-code/tools-and-tests.md -->
