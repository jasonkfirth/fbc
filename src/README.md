<!--
Project: FreeBASIC source documentation
File: src/README.md
Purpose: Provide an entry point for readers opening the implementation tree.
Responsibilities: Link source-tree guides and local subsystem ownership maps.
This file intentionally does NOT duplicate the language manual or build policy.
-->

# FreeBASIC implementation sources

Start with the [source code guide](../docs/source-code/README.md) for the
repository map and the path from a BASIC program through compilation to its
runtime libraries.

| Tree | Guide |
| --- | --- |
| `compiler` | [Compiler phases and source tree](../docs/source-code/compiler.md), [detailed ownership](compiler/README.md) |
| `rtlib` | [Core runtime and native selection](../docs/source-code/runtime.md), [detailed ownership](rtlib/README.md) |
| `gfxlib2`, `gfxlib3` | [Graphics source trees](../docs/source-code/graphics.md), [software map](gfxlib2/README.md), [queued map](gfxlib3/README.md) |
| `sfxlib` | [Sound source tree and audio flow](../docs/source-code/sound.md) |
| `tools` | [Target helpers and regression tests](../docs/source-code/tools-and-tests.md) |

The main regression suite is at [../tests](../tests/). `src/tests/tmp`
contains separate checked-in fixture data. Source selection is owned by
[../mk](../mk/README.md); generated `obj` directories are build output.

<!-- end of src/README.md -->
