<!--
Project FreeBASIC C-emitter qualification
File c-emitter-volatile-updates.md
Purpose Record C2FB volatile assignment and increment lowering.
Responsibilities Bind access widths, expression results and native regressions.
Complete applications and converter preprocessing have separate qualification.
-->

# Volatile updates, November 17

C2FB now emits its explicit volatile store adapters for integer assignments,
compound assignments and prefix/postfix updates through local pointer aliases.
Compound assignments and increments load the old value once. Assignment and
prefix results use the converted value computed before the store. Postfix
results use the saved old value. Pointer updates follow the same sequencing.
Pointer store adapters accept values pointing to read-only data while requiring
writable storage addresses.

Mapped record fields select their physical FreeBASIC storage type before
choosing an adapter width. A logical C `int` mapped to a FreeBASIC `Byte` uses
an eight-bit access and sign extension. The unsigned mapping uses zero
extension. Volatile bitfields receive an explicit unsupported-storage
diagnostic because their storage-unit access needs separate lowering. Ordinary
bitfield translations retain positive controls. The existing floating-point
and x86-64 adapter boundaries have explicit controls.

GCC documents that volatile scalar assignment expressions use the assigned
value without rereading the object. Its bitfield discussion also explains the
shared-storage concern. [GCC volatile semantics](https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html)

## Converter and native evidence

The complete updated converter is
`build/c-emitter-forward-views-20261105/c2fb-volatile-update-v1-20261117.exe`,
SHA256 `5e37b04ffb90565f504bcac2825408d59dc5c6ef01d2404c5865cf77fc57b78d`.
Both FreeBASIC C producers emit the same complete converter C. Its GCC build
has zero warnings. Its Clang build retains 156 padding and two startup
observations. Both native converter builds produce the same regression BASIC.

The primary matrix covers signed and unsigned 8/16/32/64-bit storage,
assignments, addition, XOR, prefix/postfix increment and decrement, sequential
stores and pointer updates. Address and RHS evaluation counts are checked.
The complete emitted modules compile under GCC and Clang at O0/O2 on Windows
and Linux, through both FreeBASIC C producers. All 64 runs have their expected
outcomes, including 16 original-converter access-count failures. The successful
runs pass 584,928 cases and 3,291,808 checks.

Tracked runs insert observation calls around the real helper assembly. The
after-write callback checks the stored bytes and changes the slot, proving
that expression results use their saved values. Hook insertion reverses to the
normalized original emitted text. Moving the read hook immediately before the
physical assembly access removed declaration-placement observations introduced
by the first harness version. Plain and corrected tracked primary emitted
modules have zero native warnings under both tools.

Mapped-field checks add 64 Windows/Linux executions, with 32 expected original
failures and 7,776 successful cases containing 77,760 checks. The bridge shares
the actual emitted canonical record and surrounds its one-byte field with
initialized guards. This reproduces the original four-byte overread within the
test object. The repaired signed and unsigned translations use one-byte
accesses, preserve guards and return the expected converted values. Combined
accepted matrices contain 128 execution outcomes, 592,704 successful cases and
3,369,568 checks.

Native diagnostics remain enabled. The independent original-C primary reference
records 36 sign-conversion observations under GCC and Clang, with three
additional bounded-buffer observations under Clang. Its observer has its own
padding, pre-C11, declaration-placement and bounded-buffer observations. The
mapped-field bridge records two redundant declarations in tracked GCC builds
and four pre-C11 plus eight bounded-buffer observations in Clang builds.
Full maps and command lines are retained. The uniform primary FreeBASIC
signatures produce 32 existing unused-parameter observations per emission.

Strict source-only lint of both changed converter include files reports zero
errors and zero warnings for the original and repaired cuts. The normal port
test entry checks 75 public update functions and 19 storage controls. The
mapped-field extension preserves the original primary fixture's source and
shape-check ASTs. Twelve additional FreeBASIC controls check read-only data
values, writable store addresses and 32-bit target rejection.

## Census and reproduction

This converter repair retains the production GCC census of 171. Coreutils
records 155, retained corpus 14 and Templari 2. SQLite, Midisoft, Rotator3d,
FS4 and HART record zero in their last qualified GCC queues. Existing
production modules retain their preceding source cuts and measured diagnostic
receipts. Future regeneration can use the repaired converter.

The port patch is `volatile-update-port-v1-20261117.patch`. It contains the two
converter includes, normal test registration and two Python regression files,
against the exact preceding private source cut. Forward application, reverse
checking and resulting bytes are verified. Port publication remains pending.

The public native runner is `build_scripts/test-c2fb-volatile-updates.py`.
Supply `--module-c`, `--reference-c`, `--cc`, `--native-flags-json`,
`--optimization`, `--scratch-root` and `--receipt`. The tracked mode adds
`--tracked`; original-converter access controls also use
`--expect-access-failure`. The port's
`fb_port/tools/c2fb/tests/test_volatile_updates.py` supplies the C source and
normal converter-entry assertions.

Receipts under `build/c-emitter-forward-views-20261105` include
`volatile-update-build-v1-20261117.json.zlib`,
`volatile-update-fixtures-v2-20261117.json.zlib`,
`volatile-update-windows-v2-20261117.json.zlib`,
`volatile-update-linux-v2-20261117.json.zlib`,
`volatile-narrow-fixtures-v1-20261117.json.zlib`,
`volatile-narrow-windows-v1-20261117.json.zlib`,
`volatile-narrow-linux-v1-20261117.json.zlib`,
`volatile-update-lint-v1-20261117.json.zlib`,
`volatile-update-delivery-v1-20261117.json.zlib` and
`volatile-update-qualification-v1-20261117.json.zlib`.

The complete converter build peaks at 34,175,323 owned bytes. Each corrected
primary native worker uses less than one MiB and each mapped-field worker
uses less than 100 KiB. Temporary directories are removed after each run.
The ten protected installed or working products retain their recorded hashes.

Windows generic-C preprocessing command quoting, complete C2FB integration,
GUI helper reachability, native platform replay and whole-application execution
retain their recorded follow-up. The scalar volatile update reproducer is
closed by this batch.

<!-- end of c-emitter-volatile-updates.md -->
