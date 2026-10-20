<!--
Project: FreeBASIC source documentation
File: source-code/README.md
Purpose: Introduce the repository and its maintained source trees.
Responsibilities: Explain subsystem boundaries, build inputs, and reading routes.
This file intentionally does NOT replace the language manual or porting guides.
-->

# FreeBASIC source code guide

FreeBASIC has two sides: the compiler that translates BASIC programs, and the
runtime libraries those programs call. The compiler is mostly FreeBASIC. The
runtime libraries are mostly C, with assembly and native platform adapters
where needed. A language change can involve both sides, but they have separate
owners, build inputs, and lifetimes.

Start with the tree below, then follow the guide for the subsystem involved.
Paths in the tree are relative to the repository root. Generated object
directories are omitted.

```text
src/
    compiler/       Compiler, from command-line options through code generation
    rtlib/          Core language runtime, startup, console, files and OS services
    gfxlib2/        Software graphics and native display/input adapters
    gfxlib3/        Queued graphics with renderer backends and native adapters
    sfxlib/         Sound commands, synthesis, mixing, capture and audio drivers
    tools/          Target packaging, launch wrappers and application bridges
    tests/tmp/      Checked-in .ss fixture data, separate from the main test suite
inc/                BASIC include files and external library declarations
tests/              Compiler and runtime regression suites
examples/           Programs demonstrating language and library behavior
mk/                 Source selection, build rules, installation and build tests
build_scripts/      Build automation, validators, packaging and target checks
doc/                Language manual sources, documentation tools and API notes
docs/               Maintainer guides, platform guides and verification records
```

## Read by subsystem

| Guide | What it explains |
| --- | --- |
| [Compiler](compiler.md) | Front end, symbols, typed trees, backends, driver and semantic tooling |
| [Core runtime](runtime.md) | Runtime calls, descriptors, startup, platform replacements and library variants |
| [Graphics](graphics.md) | Software drawing, queued rendering, images, input and native drivers |
| [Sound](sound.md) | Command handlers, voices, mixing, buffers, audio devices and timing |
| [Tools and tests](tools-and-tests.md) | Target wrappers, declarations, build ownership and regression placement |

The detailed ownership maps live beside the implementation:
[compiler](../../src/compiler/README.md),
[runtime](../../src/rtlib/README.md),
[gfxlib2](../../src/gfxlib2/README.md), and
[gfxlib3](../../src/gfxlib3/README.md).
Use these maps when choosing a file for a change. The guides here connect
those local maps and explain how an operation crosses subsystem boundaries.

## From a BASIC program to a running process

1. The compiler driver reads options and selects the language dialect, target,
   backend, inputs and output paths.
2. The lexer and preprocessor supply tokens. The parser uses the symbol table
   to resolve names and types and constructs typed abstract syntax trees.
3. AST processing checks and lowers operations. Some operations become emitted
   instructions; others become calls into a runtime library.
4. A backend emits assembly, C or LLVM IR. The driver coordinates the external
   tools needed to produce objects and the requested final output.
5. When the program runs, its startup objects initialize the runtime. Calls
   from generated code reach portable runtime implementations or the native
   implementations selected when that runtime was built.

`src/compiler/runtime` belongs to step 3: it describes and builds calls. It
does not implement the library functions executed in step 5. Those functions
belong to `src/rtlib`, `src/gfxlib2`, `src/gfxlib3` or `src/sfxlib`.

The compiler host is the machine running the compiler. The compilation target
is the machine that will run the resulting program. A cross compiler must
keep host file handling and arithmetic separate from the target's ABI,
startup objects and system libraries.

## Which files actually enter a build

The Make source graph is authoritative. A recursive list of every C or BASIC
file includes other platforms, excluded implementations and generated output.

| Build input | Owner |
| --- | --- |
| Compiler subsystem list and host replacements | [mk/compiler-sources.mk](../../mk/compiler-sources.mk) |
| Portable runtime groups and native replacement rules | [mk/rtlib-sources.mk](../../mk/rtlib-sources.mk) |
| Shared graphics subsystem lists | [mk/gfx-sources.mk](../../mk/gfx-sources.mk) |
| Library source selection and derived object lists | [mk/source-graph.mk](../../mk/source-graph.mk) |
| Object paths, compilation and archive publication | [mk/build-layout.mk](../../mk/build-layout.mk) and [mk/build](../../mk/build/) |

The Make system retains optional `src/fbrt` rules for a BASIC runtime layer.
This checkout has no such source directory. The `fbrt0.c`, `fbrt1.c` and
`fbrt2.c` startup sources are under `src/rtlib/static`; their names do not
imply a separate populated `src/fbrt` tree.

`src/*/obj`, `bin`, runtime archives under `lib`, emitted bootstrap sources,
and build/test logs are build products. Their exact paths depend on the
selected configuration. Keep source ownership in the explicit source lists
and keep generated output out of those lists. See the
[Make system guide](../../mk/README.md) for the build layers and configuration.

## Follow a change across interfaces

For a language feature, follow syntax to symbols and AST checking, then to
backend emission or a runtime call builder. For a runtime defect, start at
the exported function and inspect its shared header and selected platform
replacement. For a target failure, establish which source and toolchain were
selected before changing the portable implementation.

When an interface crosses these boundaries, check argument widths, calling
convention, descriptor layout, allocation ownership and error behavior on
both sides. A declaration that compiles can still describe the wrong ABI.
The [tools and tests guide](tools-and-tests.md) shows where to put the
corresponding regression.

<!-- end of source-code/README.md -->
