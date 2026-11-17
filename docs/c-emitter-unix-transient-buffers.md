<!--
Project: FreeBASIC runtime verification
File: c-emitter-unix-transient-buffers.md
Purpose: Record process/printer storage repair and native warning qualification.
Responsibilities: Bind ownership, byte values, native diagnostics and open owners.
Project application execution keeps its separate qualification.
-->

# Unix process and printer buffers, November 15

The complete current Linux MT runtime and startup build has GCC 89 /
Clang 10,095 across 1,036 native object profiles. This removes nineteen GCC
and thirteen Clang observations from the preceding console cut. All twelve GCC
alloca observations are closed. The remaining GCC classes are twenty padding,
nineteen sign conversions, fifteen other conversions, fourteen unused parameters,
six cast-qual, six discarded qualifiers, five switch-default, three missing
prototypes and one redundant declaration.

Unix EXEC owns its parsed argument bytes and argv vector on the heap until the
launch returns. Temporary input strings are released on success and allocation
failure. The count and pointer-array product are checked before allocation.
Failed launches restore console state and release both allocations. The shared
argument parser checks its int result limit before incrementing. Its character
path conversion uses size_t indexes, and the child-status write uses a checked
positive signed result before the size_t subtraction.

The printer driver uses checked heap storage for spooler commands and one common
cleanup path for command/protocol ownership. Direct device names use fixed local
storage, with negative ports rejected before subtraction. Wide output uses fixed
short-call scratch and one checked heap allocation for longer calls. Conversion
and writes use actual returned byte counts, preserving embedded zero bytes and
reporting short writes. Empty byte and wide buffers complete successfully.

Eight native component executions cover GCC/Clang, 32/64-bit and O0/O2. They pass
377,240 checks, including real forked and replacement process launches, quoted
and empty arguments, allocation refusal/cleanup, printer command values, output
bytes and file-I/O failure. All four GCC contexts have zero warnings under the
full unsuppressed options. Long argument and wide-output calls run on a 64 KiB
worker stack with every test allocation capped at 1 MiB.

Original-body controls retain two passing ordinary execution groups. Four
small-stack controls fail across the two tools, and two printer groups reproduce
the old negative-port behavior. Two additional controls exercise the newly
introduced heap-allocation failure contract. The original routines use stack
storage and proceed to launch under those heap-refusal controls.

Eight ordinary BASIC EXEC runs pass through both C producers, both native tools
and O0/O2 against the rebuilt MT archives. The complete rebuild also reruns eight
console component executions, eight counted UTF executions, eight BASIC PTY
executions and twelve date/serial API executions. The shared parser has sixteen
original/current Windows object compiles covering both tools, widths and
optimization levels, retaining identical exported symbols. Those Windows checks
qualify compilation. Strict source-only BASIC lint has zero errors and warnings.

Run the normal component entry point with
`python3 -B build_scripts/test-unix-transient-warning-fixes.py --cc gcc`.
It also accepts `--bits 32`, `--optimization 0` and a JSON diagnostic-options
array through `--native-flags-json`. Compile
`tests/rtlib/unix-exec-runtime.bas` as a Linux MT executable with an explicit main
module and run the resulting program for the ordinary BASIC control.

Evidence is under `build/c-emitter-forward-views-20261105` in
`runtime-current-packet-v25-20261115.json.zlib`,
`runtime-current-linux-v22-20261115.json.zlib` and
`transient-buffer-parser-windows-v1-20261115.json.zlib`.
The complete guest cut peaks at 16,356,545 owned bytes. The Windows parser matrix
peaks at 3,399 bytes. Both temporary trees are removed, and all ten protected
installed/development products keep their recorded hashes.

Other compiler and project queues retain their exact input/compiler/context
receipts. The disjoint GCC total is 292. Remaining owners include the conversion
classes, four finite-format compiler sites, twenty-four dynamic-format runtime
contracts and the measured Clang source/ABI classes. The overall goal is active.

<!-- end of c-emitter-unix-transient-buffers.md -->
