<!--
Project FreeBASIC C-emitter qualification
File c-emitter-volatile-read-addresses.md
Purpose Record read-only addresses for C2FB volatile load adapters.
Responsibilities Bind generator changes, owning modules and native observations.
Complete applications and other converter defects have separate qualification.
-->

# Volatile read addresses, November 17

C2FB's scalar and pointer load adapters now accept `Const Any Ptr`, and their
callers preserve that address qualification. Loads borrow their storage
address. The corresponding store adapters require writable addresses.
The refresh covers 52 load helpers and 88 calls in 21 Coreutils modules.
Their other source bytes are verified against exact preimages.

The complete 498-profile Coreutils census reaches GCC 155 / Clang 1,778.
The qualifier class reaches nine observations in each tool, removing one
observation per tool from `sort`'s cleanup traversal. The disjoint GCC total
reaches 171. Its other owning groups record retained corpus 14, Templari 2,
SQLite zero and the four additional E-drive project queues zero. All 129
reviewed nonliteral observations retain their counted diagnostic dispositions.

The final input check observes a concurrent edit to Templari Server's
`external_matrix_user_rooms.bas`. Its owning context receives fresh paired
emission and complete native GCC/Clang compilation. Templari's 677-profile
census records GCC 2 / Clang 54,317, and source-only strict lint of that file
reports zero errors and zero warnings. The edit is preserved.

## Generator and complete owning modules

The updated converter is
`build/c-emitter-forward-views-20261105/c2fb-volatile-read-v1-20261117.exe`,
SHA256 `98a949bfa61d7839f6baabb1c9f185e896dca00eed879aecf8c3d22f76a4d9ae`.
Both FreeBASIC C producers emit identical complete converter C. Its GCC build
has zero warnings. Its Clang build records the existing 156 padding and two
startup-registration observations. Both native converter images generate the
same regression BASIC. The selected FreeBASIC compiler has SHA256 prefix
`4a477330`.

All 21 complete owners receive before/after emission through both C producers,
giving 84 C captures. GCC and Clang compile the complete modules at O0 and O2,
giving 168 measured native profiles. Warning-map differences consist of the
single qualifier observation in `sort`. The source recipe supplies its original
per-module native flags, including `-mno-red-zone` for inline assembly.

Eighty of 84 native object pairs match byte-for-byte. All 42 O2 pairs match.
Four O0 pairs, covering `chmod` and `savewd` under both native tools, change
when redundant pointer-representation copies disappear. The complete `chmod`
reporter passes the original-C comparison again under both tools and
optimizations. Both complete `savewd_errno` revisions also pass native signed
state/error and guarded-byte checks. The observer constructs the compiler's
canonical record inside the emitted unit and crosses a scalar C ABI.

The first comparison exposed embedded filenames from the temporary preimage
directory. The accepted baseline uses `#line 0` with the production filename,
preserving original source line one and `__FILE__`. This removes diagnostic-path
differences from the object comparison. Original captures remain available.

## Native values and source diagnostics

Eight emitted fixtures cover both converter revisions, both C producers and
Windows/Linux x86-64. GCC and Clang execute each at O0/O2. The resulting 32
native runs pass 21,632 cases and 3,158,272 checks. They exercise 8/16/32/64-bit
and pointer loads, fresh values after writes, signed-bit boundaries, pointer
identity, local padding and surrounding guards. A public CLI replay passes a
further 676 cases and 98,696 checks.

The complete owning-function replays add four `chmod` comparisons and eight
before/after `savewd` executions. Across the value matrix, public CLI and these
owning functions, 45 executable runs pass 71,876 cases and 6,998,328 checks.
The reporter replays also pass all 64 native format-type controls.

Two FreeBASIC source controls accept read-only load addresses. Two controls
passing those addresses to stores produce error 181. Eight 32-bit controls
retain the adapters' explicit x86-64 requirement. These controls account for
the existing target restriction.

The registered eight-function volatile input and new interface assertions pass
through the direct generic translation entry. The preprocessing entry's
Windows quoting failure is recorded below. Complete C2FB integration retains
its own scope.

Strict source-only lint checks 22 BASIC/include files with zero errors. The
preimage has 227 readability advisories and the repaired cut has 228. One
generated line crosses the existing length threshold after qualification is
added. Native diagnostics are enabled throughout. Each Linux Clang value
observer build records four pre-C11 compatibility, one padding and four bounded
buffer observations. Full native diagnostic maps are retained in the receipts.

## Structural follow-up and reproduction

The fixture exposes four scalar volatile assignments through local pointer
aliases that currently become ordinary stores. The packet preserves those
8/16/32/64-bit reproducers in both translations. Value comparisons verify their
results, while their volatile-store barrier lowering requires repair.

The registered Windows `translate-simple-generic-c` entry fails C preprocessing
when its POSIX shell quoting reaches the native Windows command interpreter.
Its actual command and failure output are retained. The direct generic entry
accepts the same eight-function input and passes the read/store interface
assertions. The broader integration follow-up includes this preprocessing
failure and the scalar-assignment gap. Generic translation also retains its
existing restriction on explicitly volatile-qualified formal parameters, so
the value fixture introduces volatile qualification through local aliases.

The source patch is `volatile-read-port-v1-20261117.patch`. It covers 23 files
against the exact preceding private source cut and passes forward, reverse
and resulting-byte checks. Port publication retains its existing follow-up.

The public runners are `build_scripts/test-c2fb-volatile-loads.py` and
`build_scripts/test-coreutils-volatile-state.py`. Supply a matching complete
`--module-c`, native `--cc`, `--native-flags-json`, `--optimization` and
`--receipt`. Both accept `--scratch-root` and remove their temporary directories.
The value runner exposes its C input as `SOURCE` for conversion through
`translate-simple-generic`; its `translate` and `check_shape` functions are
also used by the converter-build controller.

Receipts under `build/c-emitter-forward-views-20261105` include
`volatile-read-build-v1-20261117.json.zlib`,
`volatile-read-packet-v2-20261117.json.zlib`,
`volatile-read-native-v2-20261117.json.zlib`,
`volatile-read-fixtures-v1-20261117.json.zlib`,
`volatile-read-windows-v1-20261117.json.zlib`,
`volatile-read-linux-v1-20261117.json.zlib`,
`volatile-read-owners-v1-20261117.json.zlib`,
`volatile-read-public-v1-20261117.json.zlib`,
`volatile-read-registered-v1-20261117.json.zlib`,
`volatile-read-lint-v1-20261117.json.zlib`,
`volatile-read-delivery-v1-20261117.json.zlib`,
`volatile-read-census-v1-20261117.json.zlib` and
`volatile-read-qualification-v1-20261117.json.zlib`.
Templari's refreshed source cut is bound by
`volatile-read-templari-refresh-v1-20261117.json.zlib`.

The complete converter build peaks at 33,967,929 owned bytes. The owning-function
worker peaks at 699,079 bytes, and patch verification uses 4,015,268 bytes.
All scratch directories are removed. Protected product hashes bind the ten
installed or working product inputs to their preceding qualification.

<!-- end of c-emitter-volatile-read-addresses.md -->
