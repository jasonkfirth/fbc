<!--
Project: FreeBASIC C-backend qualification
File: c-emitter-obstack-callbacks.md
Purpose: Record the translated obstack allocation-failure contract repair.
Responsibilities: Bind regeneration, diagnostics, allocation and callback checks.
Other GNU interfaces and application execution retain their own qualification.
-->

# Obstack allocation-failure callbacks

The Coreutils port's translated obstack calls its allocation-failure handler
when an initial or replacement chunk cannot be allocated. The GNU interface
requires that handler to terminate or transfer control with a non-local jump.
The translated callback type carries an ordinary returning signature. Its
callers therefore expose four possible null dereferences to GCC. The GNU C
implementation declares the callback with a non-returning attribute.
[GNU interface contract](https://gcc.gnu.org/onlinedocs/libiberty/Preparing-for-Obstacks.html),
[GNU implementation](https://github.com/coreutils/gnulib/blob/master/lib/obstack.c)

Both failure branches now call `abort` if a replacement callback returns.
Successful allocations and callbacks that exit or jump away keep their
original behavior. The fallback gives a defined termination path to an invalid
returning callback before either branch uses its missing chunk. This is a
library-source resolution. No compiler exception, unreachable assumption or
warning suppression is added.

The port's normal source-profile generation applies the same change to
`lib/obstack.c` candidates. It requires exactly two matching callback sites and
accepts repeated application. Six permanent regression tests cover profile
selection, stale-shape rejection, exact source regeneration, and the normal
profile entry point in both generation modes. The port's existing C2FB test
target includes these checks.

## Native qualification

Development compiler `626a5305` emits the complete changed library module
through both C producers with identical C. GCC 14.2 and Clang 19.1.7 compile
the unchanged complete before/after modules at O0 and O2 under the original
aggressive diagnostic options. GCC O2 falls from four warnings to zero. Clang
retains its one unreachable-code and one missing-noreturn observation.

The permanent BASIC driver includes the entire production module. The C
observer calls the real public obstack exports. It independently declares the
selected GNU records with size_t chunk and alignment fields. System headers
can select an older interface with narrower fields. The fixture uses the
actual source profile's layout. It checks native record
offsets, callback context, successful initial allocation, aligned growth,
preserved object bytes, chunk membership, logical memory accounting and full
cleanup. The matrix uses plain and context-bearing allocators, five initial
chunk sizes, eight requested alignments and six growth sizes. Allocation
requests are bounded at 1 MiB and tracked across at most 128 chunks.

Failure cases cover initial allocation, replacement allocation and oversized
requests. Child processes exercise callbacks that exit, longjmp or return,
plus the library's default error handler. Static record storage and a retained
snapshot keep the longjmp observer state defined. Core dumps are disabled for
the expected abort controls.

Twenty-four passing executions cover 4,480 cases and 583,864 checks. Sixteen
before/after runs exercise valid callbacks. Eight additional current-source
runs require the returning callback to end with SIGABRT. Eight original-source
negative controls fail that requirement. Both C producers, both native tools
and both optimizations are exercised. The observer itself is GCC-clean under
the same options. Its bounded C array operations retain Clang observations in
the regression receipt.

The first allocation fixture incorrectly expected an oversized initial request
to bypass the allocator. The corrected fixture receives that request and
returns failure without allocating it. Early observer conversion and longjmp
diagnostics are corrected in the final source. The final native receipt is v5,
bound to source packet v7. Its scratch peak remains below 160 KiB.

Strict source-only lint reports zero errors, with three existing generated
line-length advisories in the library and no findings in the BASIC driver.
The production source warning lines are preserved.

## Reproduction and queue identity

The local audited port is under
`build/c-emitter-forward-views-20261105/github-projects/fbxl_gnu_coreutils`.
The four-file `obstack-failure-port-v1-20261116.patch` preserves the source
repair, regeneration profile and permanent profile tests. Independent patch
application reproduces all four files byte-for-byte. Publication to the port's
GitHub repository remains pending.

Run the generation checks with the port's existing test target or directly
with `python3 -B fb_port/tools/c2fb/tests/test_obstack_failure_profile.py`.
For the native regression, emit
`tests/compiler-support/coreutils-obstack.bas` with the selected generated
module directory on the include path and the normal Linux x86-64 production
defines. Compile the resulting C and `coreutils-obstack.c` with the recorded
native flags, retaining normal system includes for the C observer, then link
their objects with the native C compiler. The default observer runs all cases.
`--valid-only` exercises the supported callback contract on either revision.
The native worker records every emission, build, link and execution command.

The complete 1,053-profile port census retains the current SQLite module
override and replaces this Coreutils module. The other 1,051 C modules reuse
their exact preceding native contexts. Coreutils reaches GCC 161 / Clang
1,784, SQLite remains GCC 18 / Clang 7,696 and the disjoint GCC total reaches
195. The sixteen remaining port null observations and the other classes
retain their separate source review. Later concurrent source versions keep
their own pending replay inventory.

Receipts under `build/c-emitter-forward-views-20261105` are
`obstack-packet-v7-20261116.json.zlib`,
`obstack-native-v5-20261116.json.zlib`,
`obstack-delivery-v1-20261116.json.zlib` and
`obstack-qualification-v3-20261116.json.zlib`.

<!-- end of c-emitter-obstack-callbacks.md -->
