<!--
Project: FreeBASIC compiler tests
File: README.md
Purpose: Describe the semantic sidecar's contract and regression tests.
Responsibilities: Explain execution, fixtures, and positive/negative coverage.
This file intentionally does NOT contain the sidecar implementation.
-->

# Semantic sidecar tests

Run `make compiler-semantic-model-test` from the repository root. To use an
already built compiler:

```sh
python3 build_scripts/test-compiler-semantic-model.py --fbc ./bin/fbc
```

The default emission checks use GCC, Clang, and LLVM. These checks stop at
compiler emission and do not require the corresponding external compiler.
The behavior comparison builds and runs a small native program with the
normal C toolchain. `--backend gas64` selects native assembly emission on a
supported host. Repeat `--backend` to select several backends. `--test` selects
an individual `test_...` method.

`sidecar.py` independently validates schema 20. `test_sidecar.py` checks the
compiler's records against source locations and known types, layouts, values,
and target relationships. The BASIC fixtures own separate responsibilities:

| Fixture | Coverage |
| --- | --- |
| `bindings.bas` | declarations, qualified lookup, USING, scopes, WITH, labels, assembly |
| `types.bas` | packed/union/bitfield layouts, inheritance, visibility, arrays, qualifiers, literals |
| `procedures.bas` | overloads, canonical formals, defaults, variadics, by-reference results, pointer calls |
| `lifetimes.bas` | each implicit construction/destruction relationship and nonphysical omissions |
| `control-flow.bas` | conversions, calls, branches, normalized jump tables, and memory access |

Generated cases cover the operator vocabulary, constant folding, repeated
includes, all five BOM encodings, long physical lines, language restarts,
multi-module transactions, error recovery, interruption, dependency limits,
and recycled symbol pools. Negative assertions check that unrelated identifiers,
inactive code, unresolved assembler names, unknown values, runtime array bounds,
and generated callee spellings never become fabricated facts. Mutated sidecars
exercise rejection of bad shapes, references, flags, ranges, and totals.

The callback regression checks canonical replacements for reused anonymous
procedure headers and their formal parameters. It distinguishes different
default arguments and preserves BYVAL, BYREF, array, and variadic signatures.
The LLVM literal regression checks decoded wide characters and zero padding
in all three export modes. On glibc it poisons fresh allocations to expose
reads beyond initialized text. It also checks a static wide-string initializer.
`make compiler-semantic-self-test` applies the export checks to the native
compiler modules selected by Make; `make compiler-semantic-corpus-test` applies
them to the maintained OMA programs.

All compiler outputs and dependency-limit fixtures use private temporary
directories. Failed tests return a failing process status and report the
fixture, backend, or invalid contract.

<!-- end of README.md -->
