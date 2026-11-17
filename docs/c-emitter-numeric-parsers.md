<!--
Project: FreeBASIC runtime qualification
File: c-emitter-numeric-parsers.md
Purpose: Record checked COM options and bounded numeric date fields.
Responsibilities: Bind original failures, canonical source, native diagnostics and public APIs.
The existing compiler and expanded-project cuts retain their own qualification.
-->

# Checked numeric COM and date fields

OPEN COM checks all nine unsigned numeric option conversions before narrowing.
The shared private helper checks the full `strtoul` result, its range status and
negative nonzero input. Empty extended values, leading whitespace, positive
signs, leading zeros and signed zero retain their existing values. Failed
conversion returns error 5 before the platform serial opener is called.

Date parsing accumulates each numeric field inside the supplied byte span and
checks each decimal digit against the signed C-int range before multiplication.
It preserves both signed endpoints, numeric prefixes, regional field order and
short-year handling. Whitespace, separators and optional month-name dots are
read only while their positions remain inside that span. Failed parsing leaves
all caller output fields unchanged.

The complete current Linux MT runtime has GCC 147 / Clang 10,148, with all 1,328
canonical input hashes verified after qualification. GCC loses fourteen
conversion observations across the two repaired units. Clang loses fourteen
narrowing observations and reports eight additional generic buffer-usage
observations on the new guarded cursor operations. Its net reduction is six.
Those observations remain present in the unsuppressed diagnostic receipts.

The two repaired units change under both native tools. The other 1,032 native
object comparisons and both startup objects match the preceding qualification.
The runtime archives contain the newly compiled parser implementations.

## Native values and public entry points

Sixty-four original/repaired parser executions cover Windows/Linux, 32/64 bits,
GCC/Clang and O0/O2. All 32 original controls reproduce a parser failure, and all
32 repaired executions pass. The COM fixture checks every numeric destination,
its full unsigned limit, defaults, whitespace, signs, suffix errors and overflow.
The date fixture checks all six field orders, every day/month combination in
five calendar years, signed limits, oversized values and truncated byte spans.

The public Windows DateParse, IsDate and DateValue matrix adds 32 executions
across both widths, both native tools, O0/O2 and base/MT archives. Its sixteen
original-source overlays reproduce the fault and its sixteen repaired-source
overlays pass. The objects precede read-only runtime archives in the test link.
Every archive and startup product retains its original hash.

The fresh Linux runtime archives pass four public date executions and eight
existing OPEN COM pseudo-terminal executions. The serial test checks requested
framing, binary bytes in both directions and restoration of the previous state.
Signed-endpoint controls exercise the internal field parser. Public DateValue
checks cover ordinary dates and the newly rejected overflowing fields.

## Registered regressions and retained evidence

`build_scripts/test-library-warning-fixes.py` registers
`tests/rtlib/com-option-ranges.c`, `date-parse-bounds.c` and
`date-parse-runtime.c` through its existing source/archive test paths. The
reviewed BASIC serial fixture has clean source-only strict lint. The unchanged
BASIC date fixture retains one existing trailing-whitespace advisory.

The existing `build/c-emitter-forward-views-20261105` work area retains
`runtime-number-qualification-v1-20261115.json.zlib`, the v1 numeric preimages,
the v15 source packet, both v2 native parser receipts, the v1 public Windows
receipt and the v14 full Linux runtime receipt. The first Linux original-source
fixture compile exposed a missing direct `limits.h` include in the test. The
corrected fixture and complete native matrices pass.

GCC keeps the aggressive runtime diagnostic list and Clang keeps `-Weverything`.
The generic cursor observations have their explicit byte-span and conversion
controls while remaining visible in the totals. Native jobs run serially at
reduced priority. Parser scratch peaks remain below 5 MiB within 16 MiB, the
Windows public overlays below 1 MiB within 8 MiB, and the full Linux rebuild
below 15 MiB within 64 MiB. All temporary trees are removed on exit.

<!-- end of c-emitter-numeric-parsers.md -->
