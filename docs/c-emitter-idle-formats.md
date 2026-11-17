<!--
Project: FreeBASIC compiler verification
File: c-emitter-idle-formats.md
Purpose: Resolve the two additional O0 idle-time format range observations.
Responsibilities: Bind helper ranges, actual native output and clock boundaries.
Terminal enumeration and full application execution retain their project scope.
-->

# Idle-time format ranges

The expanded time-format matrix found one GCC O0 `-Wformat-overflow` observation
in each of Coreutils' `pinky` and `who` modules. Both calls format hours and
minutes after the source helper has established a positive sub-day interval.
The original O2 queues remain at the previously qualified warning totals.

`pinky` returns blanks for intervals below sixty seconds. Its hour/minute branch
therefore receives intervals from 60 through 86,399 seconds, yielding hours
0 through 23 and minutes 0 through 59. The five output bytes and terminator fit
its 22-byte buffer. Its day branch has a positive signed 64-bit interval divided
by 86,400, whose longest result occupies sixteen bytes including the `d` suffix.

`who` first establishes that access follows boot, access precedes or equals the
cached clock and the difference fits a signed 32-bit integer. Its sub-day branch
has the same hour/minute ranges. Its six-byte buffer holds the five formatted
bytes and terminator. Other paths return the dot marker or the translation
provider's returned pointer.

The public regression compiles each complete emitted module with its original
unsuppressed diagnostic flags, then appends an entry that calls the existing
private helper. The `who` execution links the actual long-double runtime used by
the source range check. Native wrappers observe clock reads, format bytes,
variadic argument types and values, and translation pointer identity before
calling the real libc formatter.

The independent reference uses unsigned subtraction and representation copies
for the signed wrapping arithmetic selected by the production profile. It
exhausts every second from zero through the day boundary, varies the boot
ordering and includes signed integer limits. Seven separate clock settings
exercise both helpers' cache sentinels and the extreme timestamps.

Eight GCC/Clang O0/O2 contexts pass 56 executable invocations, 19,358,872 cases
and 183,842,904 checks. The largest observed formatted result is sixteen bytes
for `pinky` and five for `who`. The two O0 diagnostics remain recorded as native
range-analysis limitations. The source helpers and original diagnostics remain
preserved.

The runner accepts a complete Linux LP64 emitted module, its original native
flags JSON file and the runtime sources being qualified.

```text
python -B build_scripts/test-coreutils-idle-formats.py --source-c /path/to/pinky.c --program pinky --rtlib-dir src/rtlib --cc gcc --optimization 0 --native-flags-json /path/to/flags.json --receipt /path/to/result.json
```

The immutable packet and native results reside under
`build/c-emitter-forward-views-20261105` as
`idle-format-packet-v1-20261116.json.zlib` and
`idle-format-native-v1-20261116.json.zlib`. Each worker removes its own scratch
directory and records its peak disk use.

<!-- end of c-emitter-idle-formats.md -->
