<!--
Project FreeBASIC C backend verification
File c-emitter-forwarded-formats.md
Purpose Record the two private forwarded-format contracts in Coreutils.
Responsibilities Describe callers, native checks, reproduction and test scope.
Filesystem operations retain the Coreutils project's qualification.
-->

# Coreutils forwarded formats

The current review accounts for every use of the private `emit_verbose` and
`make_dir_parents_private` procedures in the captured Coreutils port. Their
complete emitted modules compile with unsuppressed GCC and Clang diagnostics at
O0 and O2. Both original `-Wformat-nonliteral` observations remain counted.

`emit_verbose` has four direct callers. Its formats select copy, rename or
exchange descriptions with two string arguments. The complete original helper
executes with independent numbered quoting slots, optional backup names and
controlled translations. Tests include empty names, spaces, quotes, percent
characters, backslashes, tabs, newlines, UTF-8 and positional translations.
Real libc formatting produces the output checked by the observer.

Four native executions pass 18,432 cases and 425,988 checks. They observe
34,816 formatting calls, 16,384 backup suffixes and 18,432 final newlines.
Renaming the unused application entry symbol preserves executable sections.

`make_dir_parents_private` has one direct caller. Its format is either
`%s -> %s\n` or null. The source review accounts for its declaration, private
definition and sole call, including pointer-preserving casts. The emitted null
guard branches past its sole `printf` call. This resolves the format-warning
contract through caller and guard proof. The helper's filesystem operations
were not executed. Those operations retain their application qualification.

Sixteen diagnostic compiles check the original native call sites. All eight
correct string-format controls have the expected type-diagnostic outcome.
All eight controls replacing a string conversion with `%d` report a native
format-type warning. Original padding, identifier, reachability and dynamic
format diagnostics remain visible.

The public runner is `build_scripts/test-coreutils-forwarded-formats.py`, with
observer `tests/compiler-support/coreutils-forwarded-formats.c`. Run it with
`--copy-c`, `--cp-c`, `--copy-bas`, `--cp-bas`, `--cc`, `--optimization`,
`--native-flags-json` and `--receipt`. The C inputs must be complete emissions
from the matching BASIC sources. Repeat for GCC and Clang at O0 and O2 using
the captured native flags. An optional `--scratch-root` selects temporary
storage. Each invocation removes its temporary directory.

The final packet and native receipt are
`forwarded-format-packet-v3-20261116.json.zlib` and
`forwarded-format-native-v3-20261116.json.zlib` in the existing audit directory.
The largest inner scratch set was 1,367,454 bytes. The outer worker peaked at
982,511 bytes. Source-only lint of the two unchanged BASIC files reports zero
errors and 75 existing long-line warnings.

This closes the two remaining dynamic-format reviews. The ledger records
127 reviewed observations out of 129, with two finite compiler branch-join
cases pending. The disjoint GCC total remains 193. Development main remains
`4a477330`. Nine protected products retain their prior hashes. The final check
observed an intervening `bin/fbc.exe` change from `7082d411` to `d85cd126`.
The receipt records both hashes and preserves the current executable.
The final source check also observed an intervening `mk/tests.mk` launcher
edit. The earlier complete backend-suite result retains its recorded source
cut. This launcher edit requires its owning validation. The forwarded-format
runner invokes native tools directly and its inputs retain their tested hashes.

<!-- end of c-emitter-forwarded-formats.md -->
