<!--
Project FreeBASIC runtime qualification
File c-emitter-vt100-symbol.md
Purpose Record the VT100 C identifier repair and its ABI checks.
Responsibilities Explain default ownership, overrides and measured diagnostics.
Terminal behavior retains the existing platform execution checks.
-->

# VT100 flag C identifier

The Unix runtime declares `fb_enable_vt100_escapes` through
`FB_RTLIB_SYMBOL("__fb_enable_vt100_escapes")`. The existing ABI helper preserves
the target's C label prefix. The flag retains its C `int` type, initial value
of `-1` and mutable address. Unix, Solaris and RISC OS terminal-output helpers
use the ordinary C identifier. Their exact preimages are recovered by reversing
the identifier change and removing the declaration's assembler annotation.

The default definition remains in its own archive member. An application can
supply `__fb_enable_vt100_escapes` in an earlier object, then update it at runtime.
Zero disables hard-coded VT100 output and every nonzero value enables it.
The existing BASIC declaration and example keep their established linker name.

## Native checks

The public runner compiles separate current and legacy clients. Its 32 baseline
invocations cover GCC and Clang, ILP32 and LP64, C and C++, and O0 and O2. All
64 default/override executions pass. Another 32 executions link the actual
fresh MT and non-MT archives. They check address identity, the default value,
application override precedence and updates across signed integer limits.

Eleven Clang target configurations produce identical before/after objects in
both C and C++, including Linux, Win32, Win64, Windows ARM64, both Darwin
architectures, Haiku, FreeBSD, OpenBSD and Solaris. These probes qualify the
symbol binding. Native Solaris and RISC OS terminal execution retains its
platform scope.

Complete Linux builds compile 2,072 object profiles with aggressive unsuppressed
diagnostics. GCC remains at zero. Clang MT falls from 10,021 to 9,505 and non-MT
from 10,019 to 9,503, removing 1,032 reserved-identifier observations. Every
runtime object, archive and startup object matches its preceding cut. All 478
existing recorded execution outcomes match their expectations, including 56
original-source failures. The sixteen unchanged limits controls retain their
existing receipt through the exact object and source identities.

The current Haiku header closure compiles another 24 Clang profiles, removing
16 observations. Their objects match the exact objects previously executed on
Haiku. The installed Haiku Clang remains unavailable with exit code 3, so this
check uses the existing Windows Clang and captured native Haiku SDK.

## Reproduction

Run on Linux with the current source tree and installed native tools.

```sh
python3 -B build_scripts/test-vt100-symbol.py --cc gcc --bits 64 \
    --optimization 2 --language c --receipt /tmp/vt100-gcc.json
python3 -B build_scripts/test-vt100-symbol.py --cc clang --bits 64 \
    --optimization 2 --language c++ --receipt /tmp/vt100-clang.json
```

Use a fresh receipt path for each invocation. `--runtime` accepts an already
built archive and `--mt` selects the matching multithreaded header context.
The runner retires its temporary files. The qualification workers run serially,
at low priority, with Make `-j1`. The largest scratch tree is 23,637,276 bytes.

The established disjoint GCC census remains 184. Ten captured platform source
versions, two finite compiler format-provenance cases and application-level
qualification remain open. Receipts live under
`build/c-emitter-forward-views-20261105`, headed by
`vt100-symbol-qualification-v1-20261117.json.zlib`.

<!-- end of c-emitter-vt100-symbol.md -->
