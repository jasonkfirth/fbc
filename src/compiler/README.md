<!--
Project: FreeBASIC compiler
File: README.md
Purpose: Explain subsystem ownership and the compilation flow.
Responsibilities: Describe directory boundaries, shared contracts, and builds.
This file intentionally does NOT contain the language specification or test results.
-->

# Compiler structure

## Compilation flow

`driver` owns one invocation, its files, external tools, and final link.
`core` owns the compiler environment and the lifetime of each source module.
The lexer and preprocessor supply tokens to the recursive descent parser.
The parser resolves declarations through `symbols` and builds typed `ast`
nodes. AST nodes validate and lower operations through `backend/ir.bi` and
the runtime call builders in `runtime`.

The selected backend writes assembly, C, or LLVM IR. The driver then runs
the required external compiler, assembler, archiver, or linker. `tooling`
observes compiler-selected facts; it does not perform independent parsing
or decide language semantics.

`tooling/semantic-hooks.bi` exposes observations needed by parser, symbol, and
AST code before their storage is released. Serialization and exporter helpers
remain behind `tooling/semantic-private.bi`, which only exporter modules include.

## Directory ownership

| Directory | Responsibility |
| --- | --- |
| `core` | Compiler environment, options, targets, module lifecycle, implicit main |
| `driver` | Invocation state, files, metadata, external tools, linking |
| `driver/platforms` | Toolchain and library policy for compilation targets |
| `diagnostics` | Error and warning identities, source context, reporting |
| `lexer` | Source decoding, bounded tokens, character lookahead, source locations |
| `preprocessor` | Definitions, macros, conditions, pragmas, source directives |
| `parser` | Parser coordination, identifiers, shared error recovery |
| `parser/declarations` | Types, variables, constants, labels, initializers |
| `parser/expressions` | Expression precedence, calls, member and array access |
| `parser/procedures` | Procedure signatures, bodies, overload and argument syntax |
| `parser/statements` | Assignments, compound control flow, inline assembly |
| `parser/intrinsics` | BASIC statements and functions with special syntax |
| `symbols` | Identity, lookup, visibility, type layout, mangling, ABI metadata |
| `ast` | Typed trees, shared builders, effects, temporary lifetimes |
| `ast/nodes` | Construction, checking, and lowering for individual node families |
| `ast/optimize` | Tree simplification and vectorization |
| `runtime` | Runtime intrinsic declarations and typed AST call construction |
| `backend` | Shared IR contracts and dispatch |
| `backend/c` | C emission and C compiler interoperability |
| `backend/llvm` | LLVM IR, SSA values, layouts, intrinsic emission |
| `backend/gas64` | x86-64 assembly and register allocation |
| `backend/x86` | Three-address IR, x86/x87/SSE emission, register classes |
| `backend/debug` | Assembly debug information |
| `support/containers` | Pooled nodes, arenas, stacks, size classes, hash lookup |
| `support/strings` | Compiler-owned character buffers and literal encoding |
| `support/numeric` | Host constant arithmetic and numeric serialization contracts |
| `platform` | Small replacements for compiler host constraints |
| `tooling` | Semantic fact export with compiler-owned identities and source ranges |

## Shared contracts and ownership

The established entry points and node layouts remain in their subsystem
headers. Includes name paths relative to `src/compiler`, making dependencies
visible without a global search path for every subsystem. `core/fbint.bi`,
`symbols/symb.bi`, `ast/ast.bi`, and `backend/ir.bi` are still central contracts.
Their current type dependencies are part of the compiler architecture and must
be accounted for when splitting those interfaces further.

`mk/compiler-sources.mk` also lists the public runtime headers included by the
compiler and their transitive includes. Changing those declarations rebuilds
compiler objects, including callers whose external ABI aliases have changed.
The structure check verifies this closure alongside the internal headers.

Compiler state is owned by the corresponding subsystem between its `Init`
and `End` operations. One compiler process parses and emits modules serially.
The shared contexts, scratch buffers, and static helpers are not thread safe.
Parallel builds use independent compiler processes and invocation-private
temporary files.

### Driver phases

`driver/fbc.bas` owns invocation initialization, shutdown, and the phase
sequence. Its implementation modules have separate translation units:

| Module | Responsibility |
| --- | --- |
| `fbc-options.bas` | Option spellings, target aliases, argument parsing, validation |
| `fbc-files.bas` | Compiler paths, output names, temporary files, response files |
| `fbc-tools.bas` | External tool catalog, discovery, execution, toolchain queries |
| `fbc-compile.bas` | Source compilation, parser restarts, assembly, resources, archives |
| `fbc-link.bas` | Object metadata, default libraries, startup objects, linking |
| `fbc-help.bas` | Usage and version presentation |
| `fbc-platform.bas` | Target hook implementations and dispatch |

`fbc-private.bi` is a driver implementation contract. The invocation context
and tool catalog each have one owner; other driver modules refer to their
extern declarations. Helpers used by only one phase remain private. Exported
implementation helpers use driver-specific names. Parser and backend code
must not include this header; their existing public observations remain in
the compiler interfaces.

Target hook bodies are included only by `fbc-platform.bas`. They can inspect
the selected target, but they do not own option parsing or host arithmetic.
DOS short filenames and response file rules live in the shared file/tool
pipeline, so DOS builds receive the same semantic model and driver fixes as
other hosts.

### Containers

| Container | Lifetime and allocation policy |
| --- | --- |
| `TLIST` | Reusable fixed-size pooled nodes; optional used and free links |
| `TFLIST` | Append-only items layered on `TLIST`; reset invalidates all items together |
| `TPOOL` | Variable-size payloads selected from `TLIST` size classes |
| `TSTACK` | Block storage reused in LIFO order for nested operations |
| `THASH` | String-key buckets with a shared item pool and explicit key ownership |
| `TSTRSET` | Ordered strings combining `TLIST` traversal and `THASH` membership |

These containers have different invalidation rules. They must not be exchanged
solely because each stores linked nodes. A container owns its storage; callers
remain responsible for pointers and string descriptors in payloads unless the
API explicitly transfers ownership.

Allocation belongs to `support/allocate.bas`, rather than to a list
implementation. It checks host-sized additions, products, and alignment before
allocating storage. Lists, stacks, arenas, size classes, and hash buckets use
that contract. Small initial pools always grow by at least one node; list and
stack strides preserve pointer alignment for hosts that reject unaligned loads.

### Strings

Compiler `DZSTRING` and `DWSTRING` buffers are implementation storage. The
source language's dynamic `STRING` and `USTRING`, fixed `STRING * N`, `ZSTRING`, and `WSTRING`
representations have distinct lengths, termination rules, and runtime calling
conventions. Parsing describes those types; symbols record their storage;
AST checking chooses valid conversions; `runtime` builds the appropriate calls.
Literal byte and wide encoding belongs to `support/strings`.

Native UTF-8 `USTRING` uses the same architectural boundaries. Its scalar
indexing nodes and runtime call builders own the distinction between byte
storage and Unicode scalar operations. Existing libraries that declare their
own `UString` type can define `FB_NO_USTRING` before those declarations, or pass
`-d FB_NO_USTRING`. This disables the native type keyword for that module;
ordinary macros named `ustring` also retain their established behavior.

Byte and wide compiler buffers share checked capacity arithmetic. They retain
their own element widths and terminators. This sharing does not change the
source language's string storage or conversion rules.

### Host and target policies

A compiler host is the machine on which the compiler binary runs. A compilation
target is the machine for the user's program. `platform/dos/fp-policy.bas`
preserves DJGPP arithmetic constraints without duplicating AST or parser
modules. `platform/riscos/fp-bits.bas` normalizes APCS DOUBLE word order before
serialization. Driver target hooks remain under `driver/platforms`; selecting
a user's target must not select a compiler host arithmetic implementation.

## Build integration

`mk/compiler-sources.mk` lists subsystem directories explicitly and provides
the canonical source list for normal, variant, and bootstrap builds. A host
policy replaces the generic file with the same basename. Duplicate selected
basenames are rejected because objects and bootstrap C files retain stable
flat names. Objects remain under `src/compiler/obj/<target>`.

When adding a source file, put it in its owning subsystem. When adding a
subsystem, update `FBC_SOURCE_GROUPS` and this map. Every source needs a header
describing its responsibilities and exclusions, and a footer identifying its
path. Run `make compiler-structure-test` to check coverage and include paths.
Use `make compiler-host-policy-test` to exercise the numeric policy interface.
`make compiler-storage-test` checks container growth, payload alignment,
buffer reuse, and rejected size arithmetic with GCC, LLVM, and Clang when
available. `make compiler-test-harness-test` verifies that a failed, incomplete,
or timed out language-test log causes make to fail. GNU timeout bounds individual
log tests when available; `LOG_TEST_TIMEOUT=0` disables that limit.
`make compiler-backends-test` checks typed C aliases, procedure addresses,
inline assembly register/stack preservation, large System V aggregate and
`va_list` C calls, and builtin signatures across
available native backends. Clang builtin prototypes also receive C syntax
checks for Linux, Darwin, FreeBSD, OpenBSD, and NetBSD without requiring SDKs.

`make compiler-semantic-model-test` checks the versioned semantic sidecar with
an independent reader and compiler fixtures. It covers resolved identities,
signatures, layouts, target types, constants, operators, cleanup, source ranges,
dependencies, transactions, and deliberate omissions across C and LLVM
emission. The record contract is in `doc/compiler-semantic-model.md`.

`make compiler-semantic-self-test` extends those checks to the compiler's own
native source graph on GCC, LLVM, and Clang. It freezes sources and Make policy,
validates full and compact exports, and requires the emitted code to match
export-disabled compilation. `make compiler-semantic-corpus-test` runs the
same export checks against the maintained OMA programs.

`tooling/semantic-output.bas` owns checked sidecar staging and publication.
`tooling/semantic-source-file.bas` hashes and verifies the actual opened source
stream. Both use the runtime's fixed-width metadata query; native filesystem
structures stay in the runtime. Compiler sources and bootstrap inputs remain
BASIC.
The exporter serializes records through the writer's opaque interface. Make
builds both modules through the ordinary BASIC rules in each compiler variant,
and bootstrap emission translates them along with the rest of the compiler.

### LLVM interoperability

`backend/llvm/ir-llvm-abi.bi` contains parameter policies shared by declarations,
calls, and local parameter storage. Large trivial System V x86-64 aggregates
need LLVM's `byval` attribute to match C's stack copy. C's `va_list` array
typedef instead decays to a pointer; treating its backing struct as a value
would pass the wrong address to functions such as `vsprintf`. The
[LLVM parameter contract](https://llvm.org/docs/LangRef.html#parameter-attributes)
also applies `byval` alignment to the source pointer. `ir-llvm-memory.bi`
therefore copies argument values into aligned storage, including values read
from packed fields.

`backend/llvm/ir-llvm-asm.bi` owns assembly constraints and string escaping.
Consecutive BASIC assembly instructions are emitted together so LLVM cannot
overwrite a physical register between instructions. Variable operands use
memory constraints, and procedures containing assembly reserve a frame and
disable the red zone because user instructions may push onto the stack.
Assembly that jumps to BASIC labels still needs LLVM control-flow lowering;
it is not covered by the straight-line assembly contract.

<!-- end of README.md -->
