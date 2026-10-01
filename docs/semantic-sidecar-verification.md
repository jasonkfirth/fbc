<!--
Project: FreeBASIC compiler verification
File: semantic-sidecar-verification.md
Purpose: Record semantic sidecar fixes and the OMA completeness audit.
Responsibilities: Identify verified compilers, checks, evidence, and remaining limits.
This file intentionally does NOT contain compiler implementation or gameplay results.
-->

# Semantic sidecar verification

The later [compiler source audit](compiler-source-semantic-audit.md) adds two
corrections and expands the suite to 47 semantic contract test methods.

Verified on Linux x86-64 on October 1, 2026. The three failures recorded in
[the restructuring report](compiler-restructure-verification.md) are resolved:
STRING descriptor storage is 12 bytes for x86 and 24 bytes for x86-64, and
virtual calls retain their statically selected method. Existing feature work
provided these corrections; this audit verifies them and adds coverage.

## Additional fixes found by the audit

- Full OMA exports initially crashed because address-of nodes were treated as
  branch nodes. Their unused extra-operand field can hold recycled pool data.
  The exporter now reads that field only for node classes that initialize it.
- Preliminary procedure headers bypassed the ordinary symbol allocation path.
  Reused headers could retain a local variable's or label's export identity.
  Both allocation paths now reset identity and cleanup provenance, while
  completing an existing header preserves its fresh identity.
- The primitive table called the scalar alignment helper for XMMWORD, an
  internal register datatype. An assertions build exposed the invalid query.
  XMMWORD retains its 16-byte size and advertises no source scalar alignment.
- Procedure-pointer signatures copied from methods retain THIS's raw compiler
  attributes. Their explicit THIS argument now has ordinal zero and a clear
  implicit-instance flag; the method itself retains ordinal minus one.
- An assertions build exposed an LLVM procedure-address error, also with export
  disabled. Procedure and label symbols have no variable-storage layout.
  Address preparation and pointer argument loading now check the symbol class
  before calling the variable layout helper.
- New public header overloads needed their runtime ABI aliases preserved.
  The runtime convention is also retained for Windows declarations. Regression
  coverage checks the exported conventions for Linux and Windows, on both x86
  and x86-64, and links/calls the existing byte interfaces on Linux.

The independent reader checks metadata closure in addition to exporter totals:
symbol classes, canonical replacements and cycles, signatures and parameters,
type layouts, named constant values, module contexts, every primitive datatype
slot, and AST parent/child integrity. Corruption tests adjust footer totals
after removing metadata and still require rejection. A source vocabulary test
checks coverage of the current compiler symbol, AST, and datatype enums.

## Results

| Check | Normal compiler | Compiler with assertions |
| --- | ---: | ---: |
| Semantic contract test methods | 45 passed | 45 passed |
| OMA program/backend cases | 36/36 | 36/36 |
| OMA compiler invocations, across three export modes | 108 | 108 |
| Runtime alias and procedure-pointer regression tests, per backend | 2 passed | 2 passed |
| Assertions in those runtime regression tests, per backend | 12 passed | 12 passed |

Both compiler builds cover GCC, LLVM, and Clang. The OMA audit includes Behold,
Duel999, four kinematics programs, Nietzsche, Quest for a King, Rambo vs Kitty
Cat, StarPhalanx, OpenMarket, and Slicks. Slicks contributes nine translation
units, for 20 translation units per backend and export mode.

Every OMA case:

- exports full and compact models with valid completion markers;
- validates every identity reference and the required metadata closure;
- checks physical ranges against actual dependency files and UTF-16 columns;
- retains every expected translation unit and an identical dependency closure;
- produces matching full/compact expression facts;
- produces byte-identical backend emission with export disabled, full, and compact.

Full-model totals below apply to either compiler build. Lowered AST counts can
differ between backends; source expression and binding totals agree.

| Full-model fact | GCC | LLVM | Clang |
| --- | ---: | ---: | ---: |
| Modules | 20 | 20 | 20 |
| Procedures | 1,898 | 1,898 | 1,898 |
| Symbols | 120,857 | 120,857 | 120,857 |
| Bindings | 95,555 | 95,555 | 95,555 |
| Source expression facts | 138,099 | 138,099 | 138,099 |
| Typed AST nodes | 515,043 | 515,352 | 515,043 |
| Procedure signatures | 17,687 | 17,687 | 17,687 |
| Parameter facts | 39,969 | 39,969 | 39,969 |
| Primitive datatype facts | 540 | 540 | 540 |

The Make integration includes the dedicated contract suite, established
semantic smoke, structure check, and test-harness regressions. The structure
check covers 226 documented compiler files and 21 host source graphs. It also
checks the public runtime include closure used by compiler objects.

## Reproduce

```sh
make -j6 compiler-semantic-model-test compiler-semantic-model-smoke \
    compiler-structure-test compiler-test-harness-test
make compiler-semantic-corpus-test
```

For retained corpus evidence:

```sh
python3 build_scripts/test-compiler-semantic-corpus.py --fbc bin/fbc \
    --output out/semantic-audit-new
```

The runner freezes the compiler, include files, OMA BASIC inputs, and validator.
It saves input SHA-256 hashes, logs, sidecars, generated code, and JSON results.
The output directory must be new. The corpus target remains an explicit
extended check rather than increasing the default quick-test workload.

## Frozen evidence

The checked compiler binaries are retained under
`out/semantic-sidecar-completeness-20261001/verified/`:

- Normal `fbc` SHA-256:
  `64ae840d5ec033ecc8ea8ded8ebdef3a1a6ac35c7a8e73ce3d11c2c08659b679`.
- `fbc-assertions` SHA-256:
  `ed8a93962ed0d03a352454cc63d7a466f5a03e27704bc66efed3951f58678b62`.

The corpus matrices are `oma-verified-matrix/` and
`oma-verified-assertions-matrix/` under the same artifact root. Contract logs
are `verified/contracts-passed.log` and
`verified/assertions-contracts-passed.log`; native ABI results are in
`verified/abi-results.json`. Initial crash logs and the LLVM assertion trace
are retained alongside the passing runs.

Other feature work continued in the shared workspace. Frozen binary and input
hashes identify these results; they do not assign the results to subsequent
edits or silently replace the older restructuring matrix.

## Coverage limits

This checks the compiler-selected model for the maintained OMA programs and
the dedicated fixtures. It is not a proof for every possible BASIC program.
The sidecar contains typed compiler facts and lowered procedure ASTs, not a
complete concrete source tree, macro expansion graph, incremental workspace
index, or the dynamically selected override of a virtual call. Informational
locations may be unavailable or nonphysical, as specified in the schema.

The corpus audit emits code with `-r`; it does not link or run the games.
Previous OMA build and startup checks remain in the restructuring report.
Windows checks here verify declarations and emitted metadata without executing
Windows binaries. The earlier LLVM limitation for assembly jumps to BASIC
labels remains outside this sidecar audit.

<!-- end of semantic-sidecar-verification.md -->
