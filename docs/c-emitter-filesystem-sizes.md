<!--
Project: FreeBASIC runtime verification
File: c-emitter-filesystem-sizes.md
Purpose: Record filesystem and OS-query count conversion qualification.
Responsibilities: Bind native contracts, executed boundaries and current totals.
Application and foreign-platform execution retain separate evidence.
-->

# Filesystem and OS-query sizes, November 15

The complete current Linux MT runtime and startup build has GCC 77 /
Clang 10,083 across 1,036 native object profiles. This closes eleven GCC sign
conversions and one unused-parameter observation, removing twelve observations
from each native tool. Remaining GCC conversions contribute eight sign
conversions and fifteen other conversions.

Linux executable-name/path helpers validate the destination and signed capacity
before clearing it. readlink receives the full positive capacity. A returned
count below capacity retains a terminator from the initial zeroing, while a full
buffer is rejected as truncated and cleared at its first byte. Exact fits with
terminator space, root directories and ordinary names preserve their values.
Current-directory queries convert the already positive capacity to size_t. A
successful getcwd includes its terminator within that signed bound, allowing its
result length to return through ssize_t.

Directory permission masks use mode_t. Positive clipped path-prefix counts use
size_t for copies, and returned names are checked against FB_STRSIZEMSK before
temporary-string allocation. Both Unix localized-name providers perform the same
count check before allocation, preserving their existing fallback contract.
File-copy writes convert the remaining positive count after checking read results
and loop offsets. The existing alias, overwrite and non-regular-file regression
retains its complete checks.

Linux FRE rejects nonpositive page-count/page-size query results with zero. Its
positive multiplication uses size_t, capped at SIZE_MAX when physical bytes exceed
the native unsigned return range. The compatibility mode argument keeps its
existing ignored behavior. The cap preserves the closest representable byte
count on narrower targets.

Eight native component executions cover GCC/Clang, 32/64-bit and O0/O2 with
3,072 checks. They exercise every small executable-path capacity, NULL paths,
getcwd success/failure, invalid OS counts, unsigned multiplication limits,
localized-name selection and refusal, directory enumeration and read-only flags.
Synthetic oversized lengths are reported through callbacks, keeping actual
buffers below 8 KiB. Eight executions of the unchanged file-copy alias regression
pass alongside them. All GCC component and copy compiles are warning-free under
the full unsuppressed diagnostic options.

Original-body controls reproduce ten failures across the two native tools in
truncation, NULL paths, memory-query errors and oversized locale/directory name
counts. Two original getcwd groups and two original file-copy executions pass.
Eight ordinary BASIC programs pass through both C producers, native tools and
O0/O2 against rebuilt MT archives. They exercise CURDIR, EXEPATH, locale names,
FRE, exact copied bytes and directory enumeration. Strict source-only BASIC lint
reports zero errors and warnings.

The complete rebuild also reruns the console, UTF, process/printer and date/serial
controls, totaling 76 passing native executions in the current worker. GCC changes
three object identities and Clang changes six, while all other runtime objects
retain exact preceding native identities. Two redundant terminator writes from
the first complete cut are removed by using the existing zero initialization.
The final cut also removes their two Clang index observations.

Run the normal entry point with
`python3 -B build_scripts/test-linux-filesystem-warning-fixes.py --cc gcc`.
It accepts `--bits 32`, `--optimization 0` and a JSON diagnostic-options array
through `--native-flags-json`. Compile
`tests/rtlib/linux-filesystem-runtime.bas` as a Linux MT executable with its
explicit main module and run it for the ordinary BASIC control.

Evidence is under `build/c-emitter-forward-views-20261105` in
`runtime-current-packet-v27-20261115.json.zlib`,
`runtime-current-linux-v24-20261115.json.zlib` and
`filesystem-size-qualification-v1-20261115.json.zlib`.
Installed/development products retain all ten protected hashes. The temporary
native tree is removed. Templari
receives a current source and build-graph refresh. Its 674 paired contexts have
GCC 2 / Clang 53,469. The other compiler/project
input and native-context identities remain exact. Other platform runtime cuts and project application
execution retain their separate evidence.

The disjoint GCC total is 280. Remaining owners include the 23 native conversion
observations, layout/parameter/qualifier contracts, four finite-format compiler
sites, twenty-four dynamic-format runtime contracts and the Clang source/ABI
classes. The complete warning-cleanup goal remains active.

The latest Templari source replay qualifies 674 paired Win64 profiles at
GCC 2 / Clang 53,469. It reuses 670 exact native contexts from the intermediate
source replay and qualifies four additional contexts through both C producers.
The intermediate 670-profile run observed a build-recipe update before its final
graph check. The new 674-profile graph and current recipe hashes close that
check. All other project and compiler inputs retain their preceding identities.

<!-- end of c-emitter-filesystem-sizes.md -->
