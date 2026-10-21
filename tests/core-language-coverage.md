# Core Language Coverage Audit

This audit tracks language features against the normal `fbctests` inputs. The
feature inventory comes from the core language pages under
`doc/manual/cache/CatPg*.wakka`, the compiler keyword table in
`src/compiler/symbols/symb-keyword.bas`, and parser families under
`src/compiler/parser`.

The generated files `unit-tests.inc` and `log-tests-*.inc` determine what the
standard `make unit-tests` and `make log-tests` commands actually run. A
keyword occurrence is only a discovery check; focused runtime or compile-fail
tests are still needed for behavior that can be asserted.

The root `make tests-test` target now runs the strict coverage discovery check
after unit, log, and warning tests so a new keyword or preprocessor form with
no fbctests source witness fails the language-test gate.

## Feature families

| Feature family | fbctests evidence |
| --- | --- |
| Primitive numeric and Boolean types | `boolean/`, `numbers/`, including `numbers/casting.bas` and `numbers/integer128.bas` |
| Bit/byte manipulation and numeric conversions | `numbers/hiword.bas`, `numbers/casting.bas`, `boolean/boolean_bitfield.bas`, `pp/bit___.bas` |
| Mathematical intrinsics | `numbers/`, `optimizations/consteval.bas`, `command-sweep/rtlib-command-sweep.bas` |
| Strings and literals | `string/`, `wstring/`, `udt-wstring/`, `udt-zstring/`, `comments/`, `pp/` |
| Date and time intrinsics | `datetime/` |
| Miscellaneous statements and runtime forms | `quirk/runtime-assertions.bas`, `quirk/legacy-system-commands.bas`, `fblite/legacy-printer.bas`, `optimizations/vector.bas` |
| Variables, constants and storage | `dim/`, `const/`, `scopes/`, `typedef/`, `fblite/options-smoke.bas` |
| Fixed and dynamic arrays | `dim/`, `quirk/lubound.bas`, `fbc-int/array-size.bas`, `fblite/options-smoke.bas` |
| Expressions and operators | `boolean/`, `expressions/`, `numbers/casting.bas`, `pointers/`, `structs/ops.bas` |
| Control flow | `compound/`, `quirk/on-goto.bas`, `quirk/gosub.bas`, `fblite/resume.bas` |
| Procedures, arguments and calling conventions | `functions/`, `overload/`, `virtual/`, `functions/any-byref.bas`, `functions/va_cva_api.bas`, `functions/callconv_mode.bas` |
| User-defined types and object behavior | `structs/`, `virtual/`, `visibility/`, `typedef/` |
| Namespaces and linkage | `namespace/`, `visibility/`, FBLite `option-private.bmk`, `functions/lib-declaration.bas`, `pp/libpath-link.bmk` |
| Preprocessor and compiler options | `pp/`, `warnings/`, `pp/line-directive.bas`, `pp/libpath-directive.bas`, `pp/libpath-link.bmk`, `quirk/hash-lang.bas`, `quirk/meta-lang.bas`, `fblite/meta-include.bas`, `fblite/options-smoke.bas`, `fblite/def-default-types.bas` |
| File, console, input and error statements | `file/`, including `file/lock-range.bas`; `console/`, `command-sweep/`, `fblite/resume.bas` |
| Memory and pointer operations | `pointers/`, `fbc-int/memory.bas`, `structs/`, `overload/op_new_delete.bas` |
| Thread runtime and thread-local storage | `threads/`, including `threads/thread-local.bas` |
| Compiler-assisted serialization | `serialization/roundtrip.bas` is part of the standard FreeBASIC log-test profile; `build_scripts/test-serialization.py` has deeper backend and wire-format checks |

## Dialect and rejection coverage

The FBLite profile now tests `OPTION BASE`, `BYVAL`, `DYNAMIC`, `STATIC`,
`ESCAPE`, `EXPLICIT`, `GOSUB`, `NOGOSUB`, `NOKEYWORD`, and `PRIVATE`. It also
checks all `DEF*` default types, `ON LOCAL ERROR`, `RESUME`, and `RESUME NEXT`.
Compile-fail fixtures check the disabled implicit variable, fixed-array REDIM,
and disabled GOSUB cases.
The preprocessor tests also cover `#LIBPATH`, `#LINE`, and the `$INCLUDE`,
`$LANG`, `$DYNAMIC`, and `$STATIC` metacommands.

`CLASS` and `IMPLEMENTS` remain reserved but unimplemented according to the
manual. Compile-fail fixtures cover both keywords so they cannot silently
become accepted syntax.

The thread-local feature has a runtime FBCUnit test that checks separate
storage across worker threads and preserves the main thread's initial value.
The 128-bit integer suite exercises arithmetic, comparisons, bit operations,
conversions, calls, and boundary shifts.

## Verification

The root `make tests-test` gate passes on the Linux host. It runs the FBCUnit
suite, all four dialect log-test profiles, warning tests, and the strict
coverage check. The FBCUnit run reports 2,451 test cases and 1,622,146
assertions; each log-test profile finishes with `None Found`.

## Current audit boundary

Run `python3 build_scripts/check-core-language-test-coverage.py --strict
--output tests/core-language-feature-coverage.tsv` after the normal unit/log
inventories have been generated. It currently resolves 2,553 BASIC sources,
including BASIC files referenced by multi-module tests. All 258 keyword-table
rows, all 26 preprocessor directive forms, and all 11 selected miscellaneous
forms have a test-source occurrence.
The TSV records the witness file and test mode for each entry. It maps the
compiler-internal `__FB_SERIALIZE`/`__FB_DESERIALIZE` tokens to tests of the
public `Serialize`/`Deserialize` API. This remains a discovery check, not a
semantic proof.

The Windows `IMPORT` data test is now a multi-module fbctests case. It builds
the real C DLL on Windows; non-Windows profiles use a no-op entry, while
`functions/import-attribute.bas` covers conditional syntax compilation. The
separate `build_scripts/test-import-data.py` still checks both Windows pointer
widths and both native backends. `LOCK` and `UNLOCK` have runtime coverage for
single positions, inclusive ranges, and random-file records. On Unix, child
processes verify that overlapping byte and record ranges remain unavailable
until the parent unlocks each exact range.

The source-feature inventory is complete for the documented core language
surface covered here. Hardware and legacy forms with no safe, portable runtime
contract have compile-only or expected-rejection tests, as noted above. The
Windows `IMPORT` branch is included in the fbctests multi-module profile, but
the Linux host run uses its no-op branch; Windows DLL behavior is separately
qualified by `build_scripts/test-import-data.py`.

For a quick repeat of the discovery scan:

```sh
python3 build_scripts/check-core-language-test-coverage.py --strict \
    --output tests/core-language-feature-coverage.tsv
```
