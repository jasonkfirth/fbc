<!--
Project: FreeBASIC source documentation
File: source-code/compiler.md
Purpose: Explain the compiler source tree and how to trace a language operation.
Responsibilities: Map phases, shared contracts, host policy, and change locations.
This file intentionally does NOT specify BASIC grammar or semantic wire records.
-->

# Compiler source tree

The compiler lives in [src/compiler](../../src/compiler/). Its `.bas` files
implement the compiler itself; its `.bi` files describe internal interfaces.
The [subsystem README](../../src/compiler/README.md) has the detailed ownership
and lifetime contracts. This page gives a reading route through those modules.

```text
compiler/
    driver/                 Invocation, options, paths, tools and final linking
        platforms/          Policy for the user's compilation target
    core/                   Compiler environment and source-module lifecycle
    diagnostics/            Errors, warnings and source context
    lexer/                  Source characters, tokens and locations
    preprocessor/           Macros, conditional compilation and directives
    parser/
        declarations/       Variables, types, constants and initializers
        expressions/        Precedence, values, calls and member access
        procedures/         Signatures, arguments and procedure bodies
        statements/         Assignment and compound control flow
        intrinsics/         Statements/functions with special BASIC syntax
    symbols/                Names, visibility, types, layouts and ABI metadata
    ast/
        nodes/              Typed operation builders and lowering
        optimize/           Tree simplification and vectorization
    runtime/                Runtime declarations and typed call builders
    backend/                IR interface and backend dispatch
        c/ llvm/ gas64/ x86/ debug/
    support/                Allocation, containers, strings and numeric helpers
    platform/               Compiler-host implementation policies
    tooling/                Compiler-derived semantic observations and export
```

## Start at the phase that owns the behavior

| Question | Starting point |
| --- | --- |
| Which option, input file or external tool was used? | [driver/fbc.bas](../../src/compiler/driver/fbc.bas) and the driver's phase modules |
| How is a source module initialized and compiled? | [core/fb.bas](../../src/compiler/core/fb.bas) |
| How does source text become a token? | [lexer/lex.bas](../../src/compiler/lexer/lex.bas), then [preprocessor/pp.bas](../../src/compiler/preprocessor/pp.bas) |
| Which grammar branch accepts or rejects a form? | The corresponding `parser` family, with its positive and negative tests |
| Why does a name resolve or a type have this layout? | `symbols`, with [symbols/symb.bi](../../src/compiler/symbols/symb.bi) as the shared contract |
| Why is a conversion, cleanup or runtime call inserted? | `ast/nodes` and the relevant `runtime/rtl-*.bas` module |
| Why does emitted code use this instruction or ABI form? | [backend/ir.bi](../../src/compiler/backend/ir.bi), then the selected backend |
| Where does a semantic sidecar fact originate? | `tooling` and its observation hooks in the owning compiler phase |

The parser is recursive descent. A parser module may describe the common
grammar in comments, but its branches, dialect checks and recovery behavior
determine what is accepted. An accepted parse is only part of compilation:
symbol and AST checks still enforce types, argument modes and lifetime rules.

## Trace PRINT from syntax to runtime

For a string-valued `PRINT` argument, the useful path is:

1. [parser/intrinsics/parser-quirk-file.bas](../../src/compiler/parser/intrinsics/parser-quirk-file.bas)
   implements `cPrintStmt()`. It parses the destination, expressions and
   formatting separators and asks the runtime lowering layer to build calls.
2. [runtime/rtl-print.bas](../../src/compiler/runtime/rtl-print.bas) describes
   printing entry points and chooses a typed operation for the expression.
   The string family includes the runtime symbol `fb_PrintString`.
3. The resulting AST call passes through the selected backend. That backend
   emits the target's call sequence rather than printing anything itself.
4. At program execution, [rtlib/console/io_print.c](../../src/rtlib/console/io_print.c)
   implements `fb_PrintString()`. It handles the string descriptor, formatting
   and temporary-string cleanup and dispatches through the runtime's file
   output machinery.

This separation matters when fixing a bug. Incorrect parsing of a separator
belongs in the parser; a wrong parameter declaration belongs in the call
builder; an output or temporary-lifetime bug belongs in the runtime. A
graphics-mode output path may also involve the runtime hooks described in the
[runtime guide](runtime.md).

## Keep host policy separate from target code generation

`driver/platforms` describes how to build programs for supported targets.
`platform` supplies implementations needed by the machine running the
compiler. For example, compiler-host numeric representation is a different
question from the target program's calling convention.

The C, LLVM and assembly backends implement a common IR interface, but have
different emission and ABI details. Check the affected backend's implementation
and interoperability tests before changing parameter layout, aggregate
passing, inline assembly or debug emission. A fix in one emitter does not
automatically cover the others.

`tooling` records facts selected by the compiler. Its observers must obtain
identities, types and source ranges before the corresponding compiler storage
is released. It must not reparse text to invent a competing interpretation.
The [semantic model contract](../../doc/compiler-semantic-model.md) describes
the exported format separately from these implementation boundaries.

## Adding or moving a compiler module

[mk/compiler-sources.mk](../../mk/compiler-sources.mk) lists source groups
explicitly. Compiler objects and bootstrap emission retain flat module names,
so selected modules must have unique basenames. The host policy can replace
a generic module by its basename; an unrelated duplicate is an error.

Put a new module with the subsystem that owns its state. Name includes from
the compiler source root, preserve the shared interfaces, and update the source
group list when adding a directory. Keep driver-private state out of parser
and backend modules. Shared compiler contexts are used serially within a
process; parallel Make jobs run separate compiler processes.

`make compiler-structure-test` checks the maintained source map and include
contracts. Behavior changes also need a regression in the relevant language,
runtime or backend suite. See [tools and tests](tools-and-tests.md).

<!-- end of source-code/compiler.md -->
