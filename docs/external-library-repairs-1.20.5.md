<!--
Project: FreeBASIC external library maintenance
File: external-library-repairs-1.20.5.md
Purpose: Record native repairs and complete Debian optional SDK coverage.
Responsibilities: Fix contracts, direct execution evidence and remaining limits.
This file does not claim a full package rebuild or universal external API coverage.
-->

# External library repairs after the 1.20.5 audit

The [original five-target sweeps](external-library-qualification-1.20.5.md)
remain unchanged. This follow-up repairs the reproducible source defects and
qualifies missing QuickLZ SDKs. No library was removed: removal requires failure
on every applicable target, and Linux already links all 313 applicable external
examples. Machine-readable evidence is in
[the repair results](external-library-repair-results.json).

## Binding and example repairs

- `inc/ffi.bi` now selects the System V x86 ABI on Haiku, including the native
  ABI enumeration, argument width and closure layout. ARM selections and other
  operating systems retain their previous branches. The Haiku x86-64 layout
  matches its native libffi 3.4.6 headers.
- `inc/libintl.bi` links the separate gettext library on FreeBSD and Haiku.
  Plural counts now use C unsigned long, and category arguments use C int.
  The old Unix x86-64 declaration truncated 4,294,967,297 to one and selected
  the singular form. The regression exits with status 2 against the old binding
  and succeeds against the repaired binding. Windows retains its 32-bit C long.
- Exampleageddon builds its C++ helper libraries with the native `c++` driver
  and libc++ on FreeBSD and Darwin. Explicit GCC drivers retain libstdc++.
  All three affected C++ examples compile and execute on FreeBSD. Darwin driver
  selection has a contract regression; no native Darwin run was performed.
- QuickLZ needed a native SDK on FreeBSD and Haiku. A private static build of the
  preserved 1.5.0 C source, with `QLZ_MEMORY_SAFE`, compression level 1 and no
  streaming, supplies that SDK. No upstream codec code was changed. The matching
  archives remain in each repair workspace under `quicklz-result/libquicklz.a`.

## Native checks

Windows, Linux, FreeBSD and Haiku passed 17 libffi layout/constant comparisons
against native C headers, the original call and closure examples, and the
gettext plural-width check. The additional closure example also executes on
Linux, FreeBSD and Haiku. The GTK calculator now links on FreeBSD and Haiku;
its GUI interaction was not tested in this follow-up.

The existing QuickLZ regression suite passed on FreeBSD and Haiku with both
GCC and GAS64 backends. Each backend passed 12 ABI measurements, four binary
round trips and 24 file-example checks, including malformed packets, protected
output files and the allocation-size limit. The initial direct native round
trip on each host also reproduced all 7,040 input bytes.

These focused results bring external compile/link coverage to 255/313 on
FreeBSD and 163/313 on Haiku when overlaid on the original sweeps. They are
focused overlays, not new full sweeps. Other missing or incompatible SDKs and
the previously recorded runtime-package gaps remain explicit in the original
report.

Strict source lint on the two changed bindings and two new BASIC probes found
zero errors and warnings for Windows, Linux, BSD and Haiku. Native compilation
and C ABI comparisons provide the semantic/runtime evidence. All seven
Exampleageddon contract tests passed, and the three changed Wakka pages were
rendered successfully with the existing manual converter.

## Debian optional development packages

`debian/optional-dev-packages.json` records 126 current optional SDK/tool
packages and three newer suggestions. The inventory covers the shipped library
families and native Debian providers found for their library names. Extra SDKs
remain suggestions for `freebasic-bindings` and `freebasic-examples`; the full
metapackage recommends the newly added current SDKs. SDL3 mixer/net and raylib
remain suggestions because the checked trixie repositories do not provide them.

The curl development relationship accepts GnuTLS or OpenSSL, preferring GnuTLS.
Debian's MediaInfo SDK depends on GnuTLS curl; requiring OpenSSL curl at the same
time made the complete SDK set impossible to install. APT simulation now resolves
all 126 current packages together. On the test guest it planned replacement of
the existing OpenSSL development package; no transaction was executed.

The actual Debian metadata hook runs
`build_scripts/check-debian-optional-packages.py` before `dh_gencontrol`.
It rejects missing SDK coverage, mandatory SDK dependencies and separated curl
alternatives. The hook passed, and a deliberately omitted FreeImage SDK was
rejected. `dpkg-gencontrol` and metadata-only `.deb` archive generation passed
for all three consumer packages. This does not constitute a full compiler
package build or hosted CI qualification.

Legacy SDKs without matching repository packages remain documented rather than
being replaced by incompatible APIs. For example, PCRE2 does not supply PCRE1,
and GLFW3 does not supply GLFW2. QuickLZ, older SQLite, wx-c, GDSL and several
other historical SDKs still require a separate matching build. Optional package
availability is a dated repository snapshot, not proof that every API in every
installed SDK version works with its binding.

Receipts, source hashes, compiler commands and compact native archives are kept
under `out/ea1205-fixes/`. Existing dirty source changes, installed SDKs and the
original qualification workspaces were preserved.

<!-- end of external-library-repairs-1.20.5.md -->
