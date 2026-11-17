<!--
Project FreeBASIC C-emitter qualification
File c-emitter-shared-default-switches.md
Purpose Record direct lowering of grouped C case/default labels.
Responsibilities Bind converter source, native values and remaining format work.
Production regeneration and complete C2FB integration retain their own scope.
-->

# Shared case/default lowering, November 17

C2FB now emits a grouped case/default body once under `Case Else`. Values
explicitly grouped with default enter that body through the ordinary unmatched
path. The converter retains its state-machine form for genuine fallthrough.
Named entry labels stay attached to the shared body, and the selector is
evaluated once.

The previous lowering introduced a dispatch loop solely to avoid emitting the
same body for both explicit case values and default. The direct form removes
that loop and its artificial zero-state entry. The two Coreutils finite-format
sites in `chmod` and `chown-core` use this grouped-label shape.

The production warning census remains GCC 172. Those owning application
sources still use their preceding generated bodies. The subsequent
[change-reporting review](c-emitter-change-formats.md) qualifies their native
format contracts. Their generated-source refresh, the complete C2FB
integration run and port-patch publication remain open.

## Converter and source qualification

The qualified converter is
`build/c-emitter-forward-views-20261105/c2fb-shared-default-v1-20261117.exe`,
SHA256 `e4dd32e4f9d745df9152f31fb9a565f08cfae30d86d400ca04bc68f6923116a1`.
The main FreeBASIC compiler remains `4a477330` on its preceding source cut.

Both FreeBASIC C producers emit identical complete converter C. GCC and Clang
build the converter and produce identical translated BASIC for the regression.
The complete native converter is GCC-clean. Its Clang build retains the
existing 156 padding and two startup-registration observations.

The registered regression has eight switch functions and one selector helper.
It covers shared default groups at the start, middle and end, multiple labels,
a default-only group, returning branches, named entries, and fallthrough into
and out of default. The original converter fails the new direct-dispatch shape
check. The repaired converter reduces eight generated state declarations to
two, preserving the two genuine fallthrough cases.

Strict source-only fblint reports zero errors and the existing single
large-file documentation advisory. The three-file incremental patch applies
to the exact recorded converter/Makefile preimages, produces identical repaired
bytes and passes reverse checking.

## Native execution and public entry

Sixteen actual emitted fixtures cover both converter revisions, both C
producers and Windows/Linux at 32 and 64 bits. GCC and Clang execute each at
`-O0` and `-O2`, giving 64 native runs and 215,040 cases. Every case compares
the returned result and selector/body side effects with an independently
compiled C reference. Inputs include both signed integer limits and every
nearby case/default selector.

Five further normal CLI runs pass 16,800 cases. They cover both Windows widths,
both C producers, both native tools and executable paths containing spaces.
The spaced-path check uses hard links and removes them afterward. The combined
69 executions pass 231,840 cases.

The emitted fixtures are GCC- and Clang-clean. The independent C reference
retains two deliberate fallthrough observations per build. The native observer
retains five Clang buffer-use observations on its bounded arrays. All native
diagnostic flags remain enabled.

The first CLI check exposed POSIX splitting of a literal Windows compiler
path. The runner now recognizes existing executable paths before parsing a
compiler command. A subsequent link check caught DLL-entry emission in the
function-only fixture. The accepted runner emits an object module, whose
functions link directly into the native reference checker. The final matrix
and public CLI runs use this corrected runner.

The converter build peaks at 33,969,883 owned bytes. Native Windows and Linux
workers peak at 314,541 and 69,571 bytes. All worker scratch directories are
automatically removed. Installed compiler/runtime products and the preceding
converter remain protected inputs.

## Reproduction and receipts

The port's normal C2FB `test` target registers
`tools/c2fb/tests/test_shared_default.py`. The standalone CLI accepts a
converter, FreeBASIC compiler, native C compiler, target/architecture, supplied
native flag list and receipt path. Its comparison checks remain active when
native assertions are disabled.

Receipts under `build/c-emitter-forward-views-20261105` retain exact source,
tool, command and diagnostic identities.

- `shared-default-build-v1-20261117.json.zlib`
- `shared-default-packet-v1-20261117.json.zlib`
- `shared-default-windows-v1-20261117.json.zlib`
- `shared-default-linux-v1-20261117.json.zlib`
- `shared-default-public-v1-20261117.json.zlib`
- `shared-default-lint-v1-20261117.json.zlib`
- `shared-default-delivery-v1-20261117.json.zlib`
- `shared-default-port-v1-20261117.patch`
- `shared-default-qualification-v1-20261117.json.zlib`

<!-- end of c-emitter-shared-default-switches.md -->
