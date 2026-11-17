<!--
Project FreeBASIC C-emitter qualification
File c-emitter-process-adapters.md
Purpose Record C2FB native Windows preprocessing and portable process checks.
Responsibilities Bind process arguments, directory handling and native evidence.
Body lowering and complete application qualification have their own receipts.
-->

# Process adapters, November 17

C2FB's Windows preprocessing entry now launches native tools through
`CreateProcessA`. Compiler arguments use the child's C-runtime quoting rules,
including backslash runs before quotes and trailing backslashes. Standard
input, output and error handles are inherited explicitly. The adapter waits
for the child, reads its exit status and closes both returned handles.

The native directory adapter finds an existing ancestor and creates missing
components through Windows filesystem calls. Existing directories succeed,
and an ordinary file in the requested path produces the existing adapter
error. The POSIX execution path retains its shell argument contract.

The implementation follows Microsoft's process and command-line interfaces.
[CreateProcessA](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa),
[C argument parsing](https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments),
[CreateDirectoryA](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createdirectorya)

## Complete converter builds and normal entries

The updated converter is
`build/c-emitter-forward-views-20261105/c2fb-process-platform-v1-20261117.exe`,
SHA256 `1e31ae64d57713162be182f056489a70ef8f2f719a845df89ec640ee7a7cbde9`.
The selected FreeBASIC compiler has SHA256 prefix `4a477330`.
Both FreeBASIC C producers emit identical complete converter C on Windows
and on Linux. GCC builds the complete converter with zero warnings on both
hosts. Native Clang observations remain enabled and recorded. Windows records
158 padding and two startup observations. Linux records 156 padding and four
array-element observations for the broader Clang review.

The native Windows records are checked against the SDK at both pointer widths.
Both FreeBASIC C producers participate, with native GCC on Win32 and native
GCC/Clang on Win64. Six contexts pass 156 size, alignment and field-offset
assertions. GCC reports zero warnings. Win64 Clang records two required record
padding observations and 30 pre-C11 compatibility observations per context.
The augmented SDK fixture gives its `boolean` typedef a local test spelling
to coexist with the emitted BASIC boolean representation.

Four Windows public runs cover both native converter builds and both real
preprocessors. They pass 52 normal-entry checks. Four Linux public runs pass
48 checks with the same native-tool combinations. The 25 forwarded argument
values include empty strings, spaces, apostrophes, quotes, backslashes, tabs,
newlines and ordinary macro punctuation. The child records exact argument
bytes. Other checks cover PATH lookup, actual GCC/Clang preprocessing,
inherited streams, exit statuses 7/37/255, missing executables, successful
children producing no output, partial-output cleanup, nested paths, repeated
creation, relative paths and ordinary-file blockers. Windows adds a mixed
separator/trailing separator case. All native diagnostics are unsuppressed.

The Linux worker reconstructs the qualified non-MT runtime serially from its
recorded source packet. GCC reports zero warnings, and the archive and startup
object match the preceding qualified hashes. The initial Linux run completed
its GCC converter and both public replays before a harness assertion expected
the Windows Clang classes. The accepted continuation reuses those exact GCC
results, reconstructs the same runtime and records the Linux Clang classes
and public replays. The complete runtime and converter temporary directories
are removed.

The Windows native converter replay sets the original `SOURCE_DATE_EPOCH` so
its rebuilt Clang executable matches the recorded image. The process test's
diagnostic assertions were corrected to use C2FB's stdout diagnostic stream.
The child process's separate stdout and stderr remain checked directly.

## Registered runtime regression and retained census

The complete registered eight-function volatile DLL test now passes its
native and GCC FreeBASIC backends through normal preprocessing. Its Windows
test library adds the eight explicit exports required for ctypes symbol lookup
after the generated candidate passes the original shape checks. This export
policy belongs to the test DLL.

The first registered replay exposed `dlltool`'s temporary-basename limit in
the long audit directory. The accepted replay uses a short system temporary
directory and the original registered compile calls. A child Python process
owns loaded DLLs, then exits before the parent removes their files. The failed
long-path command remains recorded for driver/toolchain follow-up.

The 75-function update fixture produces exactly the preceding BASIC after
normalizing its source filename. Its normal test entry also passes all 19
storage controls. Those identities carry the preceding native value evidence.
Strict source-only lint checks the two existing process/argument includes and
the new Windows include on both platform targets, with zero errors and
warnings in each repaired cut.

The last qualified production GCC census remains 171, comprising Coreutils
155, retained compiler/corpus 14 and Templari 2. SQLite, Midisoft, Rotator3d,
FS4 and HART retain GCC zero. The source repair closes Windows preprocessing
quoting. The broader C2FB integration suite, the recorded long-path import
library case and other existing compiler/platform checks retain follow-up.

## Reproduction and ownership

The normal port test entry is `fb_port/tools/c2fb/tests/test_process.py`.
Pass `--converter`, `--cc`, `--scratch-root` and `--receipt`.
`--native-flags-json` accepts the recorded unsuppressed diagnostic list.
The test compiles its native child probe and removes its temporary directory.
The Make test target registers this entry.

The source patch is `process-platform-port-v1-20261117.patch`. Its seven
files include the platform process implementation, quoting, test registration,
child probe, public process checks and Windows DLL export correction.
Forward application, reverse checking and resulting bytes are verified against
the preceding private port source cut. Port publication remains pending.

Receipts under `build/c-emitter-forward-views-20261105` include
`process-platform-build-v1-20261117.json.zlib`,
`process-platform-packet-v1-20261117.json.zlib`,
`process-platform-windows-v2-20261117.json.zlib`,
`process-platform-linux-v2-20261117.json.zlib`,
`process-platform-registered-v3-20261117.json.zlib`,
`process-platform-regression-v1-20261117.json.zlib`,
`process-platform-lint-v1-20261117.json.zlib`,
`process-platform-delivery-v1-20261117.json.zlib` and
`process-platform-qualification-v1-20261117.json.zlib`.

The complete Windows converter build peaks at 34,204,504 owned bytes. The
Linux worker records 41,262,906 bytes. One local native worker and one serial,
low-priority guest worker perform the compilation. Protected installed and
working product hashes are checked against the preceding qualification.

<!-- end of c-emitter-process-adapters.md -->
