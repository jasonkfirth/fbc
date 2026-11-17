<!--
Project FreeBASIC native platform qualification
File c-emitter-native-platform-owners.md
Purpose Record current Haiku owners and the Windows UTF-16 header context.
Responsibilities Describe source selection, native evidence and remaining views.
Full applications and physical input retain their project qualification.
-->

# Native platform source owners

Seven pending Haiku source versions and the Windows UTF-16 header context now
have their owning native checks. Ten captured FreeBSD, OpenBSD and RISC OS
source versions remain pending. The established compiler/corpus GCC census
retains its 184 observations.

## Haiku source selection and execution

The Make graph selected both `fb_hGetExeName.c` and `sys_getexename.c`, which
export the same function. `sys_getexename.c` now owns the archive symbol.
The older source remains in the tree and has separate controls. Eight native
relocatable-link controls reproduce the duplicate definition. Sixteen corrected
whole-archive links and executions pass, covering GCC/Clang objects, MT/non-MT,
O0/O2 and both source implementations individually.

The public observer `tests/rtlib/haiku-source-owners.c` checks bounded name/path
buffers through the real image API, invalid arguments, memory queries, the
documented focus fallback and repeatable scancode-table initialization.
The runs pass 28,816 checks on native Haiku x86-64. Physical keyboard events and
graphical focus retain application testing.

GCC 13.3 compiles all six C owners in 24 unsuppressed profiles without warnings.
The seventh source is their shared header. The host's installed Clang exits 3
before reporting its version. Its executable remains untouched. The existing
Windows Clang produces another 24 Haiku objects against 58 captured native
SDK headers. Those exact objects link and execute on Haiku. Clang uses its own
builtin headers with the native OS header closure.

The Clang source profiles retain reserved-name, bounded-buffer, GNU variadic
macro and C90 declaration-order observations. These remain in their full
diagnostic receipts. The existing broad Clang review remains open.

The complete runtime structure check passes 1,275 files, 21 source policies,
126 graph cases and three collision controls. Its header check accepts the
existing natural-language scope notes and references to owning modules. All
required header fields and scope boundaries remain checked.

## Native Windows UTF-16

The public observer `tests/gfx/wide-glyphs.c` includes the actual
`gfx_unicode.h` helper with canonical runtime records and controlled allocation,
error and lock providers. Sixteen native executions cover GCC/Clang, Win32/Win64,
MT/non-MT and O0/O2. They pass 17,907,728 cases and 98,492,680 checks.
Each execution covers every Unicode scalar encoding, every lone UTF-16 unit,
malformed surrogate followers, embedded zero, allocation failure and length
boundaries. The helper preserves temporary ownership and balances its locks.

Both complete production includers, `gfx_print_wstr.c` and `gfx3_console.c`,
compile in 32 native profiles. Win64 GCC is clean. Win32 retains two padding
observations in the first owner and six in the second owner per configuration.
They describe existing runtime and gfxlib3 record tail padding. Their original
layouts and diagnostics are preserved. Clang observations remain recorded in
the supplemental native-source receipt. Rendering and the runtime allocator
retain their integration tests.

## Reproduction and resources

The audit directory retains the current source packet, native SDK closure,
Clang object receipt and native execution receipt. Their controllers record
exact flags, source hashes and commands. The main files are
`haiku-current-compile-v1-20261117.json.zlib`,
`haiku-clang-cross-v2-20261117.json.zlib`,
`haiku-native-owners-v1-20261117.json.zlib` and
`windows-wide-glyphs-v1-20261117.json.zlib`.

Workers run serially at reduced priority and remove their scratch directories.
The largest scratch set was 4,094,039 bytes. The owned Haiku workspace is
removed after preserving its receipts, releasing 1,158,682 bytes. Local raw
transport logs are removed separately. Installed compilers, runtimes and
application binaries retain their recorded hashes.

<!-- end of c-emitter-native-platform-owners.md -->
