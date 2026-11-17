<!--
Project: FreeBASIC C-backend verification
File: c-emitter-templari-callback.md
Purpose: Record Templari's callback correction and current GCC qualification.
Responsibilities: Preserve source, diagnostic, execution and build identities.
Application deployment and other platform graphs have separate qualification.
-->

# Templari callback and production GCC checks

The subsequent [managed-field qualification](c-emitter-cleanup-dependencies.md)
selects compiler `409dc1ef` and completes 658 paired Win64 profiles. GCC records
two qualified API conversions. Clang records 52,408 observations, comprising
51,731 padding, 675 startup registrations and those two strict API conversions.
Seven source profiles extend the earlier cut. An added module exposed a discarded
child destructor in compiler-generated ERROR cleanup. The compiler repair passes
native lifetime, allocation, cross-target and normal backend-suite checks.
The portable source snapshot records all 1,167 opened source/header inputs.

The earlier November 14 production replay covers 651 Win64 object profiles with two
GCC warnings. The client contributes 370 profiles and the server contributes
281. All profiles emit C through the selected development compiler
`cd32db5bb8e210a84c10c62ba38bb3bf61e5bf023cbbcda4663e1bef71f02b3f`.
The preceding 641-profile capture had three GCC warnings. Ten production
sources were added by concurrent project work before this replay.

## Worker signature correction

`E:\Templari\Templari-Server\src\registration_mail.bas` declared its worker as
a pointer-returning function. FBC's `ThreadCreate` contract takes
`Sub(ByVal userdata As Any Ptr)`. The worker now uses that signature and its
unused `Return 0` is removed. The queue operations and worker body retain their
previous text. A source comparison verifies the exact functional change.
The file header records mutex ownership and join-before-release behavior.

FBC already diagnoses the original mismatch with warnings 3 and 41. Both
diagnostics remain present in the original-source controls. The repaired
source removes the mismatch and its native function-pointer cast warning.
The language reference already documents the required callback signature in
`doc/manual/cache/KeyPgThreadCreate.wakka`.

Both C producers emit the actual production module with its real include
context at each pointer width. GCC 16.2 and Clang 22.1.7 compile every capture
with aggressive unsuppressed diagnostics. Counts below apply to each producer.

| Module target | GCC before | GCC after | Clang before | Clang after |
| --- | ---: | ---: | ---: | ---: |
| Win64 | 1 | 0 | 12 | 11 |
| Win32 | 3 | 2 | 10 | 9 |

The two remaining Win32 GCC observations concern GNU label addresses. Padding,
startup registration and GNU label observations remain counted by Clang.
The complete Win64 production graph has a GCC check. Its complete Clang graph
remains a pending check.

## Worker execution regression

`E:\Templari\tests\test_registration_mail_worker.py` compiles the production
worker with controlled local providers. It starts and joins an idle worker,
starts another worker, queues one job, checks its counters and joins before
reading the provider's final count. The actual queue and cancellation callback
run in the test.

Eight executions pass across original/repaired source, both C producers and
O0/O2. Original-source builds must reproduce FBC warnings 3 and 41 and the
native cast diagnostic. Repaired-source builds must clear those diagnostics.
Every native warning flag remains enabled. Strict source-only lint retains
two pre-existing advisories and reports zero new findings.

The permanent regression accepts explicit compiler and prefix paths. Its
qualified invocation also supplies the saved native flags and original source.

```powershell
$audit = 'C:\Nextcloud\games\FreeBASIC_packages\fbc\build\c-emitter-forward-views-20261105'
$env:PATH = 'C:\freebasic-js\toolchain\ucrt64\bin;' + $env:PATH
$env:GCC = 'C:\freebasic-js\toolchain\ucrt64\bin\gcc.exe'
$env:CLANG = 'C:\freebasic-js\toolchain\ucrt64\bin\clang.exe'
& C:\freebasic-js\toolchain\ucrt64\bin\python.exe -B `
  E:\Templari\tests\test_registration_mail_worker.py `
  --fbc C:\Nextcloud\games\FreeBASIC_packages\fbc\build\main-gcc\bin\fbc.exe `
  --prefix E:\fbc-c-audit-20261105-forward-views-r2-qualification\crt-candidate\canonical\gcc-parameters `
  --before-source "$audit\templari-thread-before-20261114.bas" `
  --native-flags-json "$audit\templari-thread-native-flags-v1-20261114.json" `
  --scratch-root $audit
```

## Remaining dynamic API conversions

The two production GCC observations are in
`Templari-Client/src/message_local_platform.bas`. They convert `GetProcAddress`
results to the exact typed signatures for `SystemTimeToTzSpecificLocalTimeEx`
and `GetDynamicTimeZoneInformation`. The existing null checks precede calls.
Microsoft documents typed function-pointer conversion for dynamic procedure
lookup. The SDK signatures agree with Templari's declarations. These two
conversions retain their diagnostics and recorded disposition.
[GetProcAddress](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress),
[time conversion](https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-systemtimetotzspecificlocaltimeex),
[zone information](https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-getdynamictimezoneinformation).

Eight C-startup executions cover Win32/Win64, both C producers and both native
tools. They use the actual typedefs and conversion statements, compare the
dynamic call with the native SDK call, and verify two record sizes and seventeen
named field offsets. Native-header assertions check both function signatures.
Win32 objects link with GCC 16.1 and its matching toolchain environment. The
fixture uses the SDK declaration gate needed by its local headers.

## Production replay and reproducibility

The production list comes from `E:\Templari\scripts\build_native.py`.
All 651 profiles were re-emitted. The initial replay reused native results for
578 exact parent C identities and compiled 73 changed/new C files with GCC.
Five later concurrent source changes reached nine profiles. All nine were
re-emitted and freshly compiled, preserving the two-warning total. The final
capture pins 1,158 opened inputs. The source snapshot also includes the build
recipe and permanent regression, totaling 1,160 files and 12,391,743 raw bytes
in one 2,892,948-byte compressed file.

Receipts live under `build/c-emitter-forward-views-20261105`.

| Evidence | File |
| --- | --- |
| Complete production graph | `current-templari-gcc-v2-20261114.sqlite3` |
| Late source refresh | `templari-current-refresh-v3-20261114.json.zlib` |
| Exact original source | `templari-thread-source-before-v1-20261114.json.zlib` |
| Functional change and lint comparison | `templari-thread-source-check-v1-20261114.json.zlib` |
| Before/after native module diagnostics | `templari-thread-capture-{before,after}-v1-20261114.json.zlib` |
| Eight worker executions | `templari-thread-runtime-v2-20261114.json` |
| Eight SDK API executions and layout checks | `templari-windows-api-values-v2-20261114.json.zlib` |
| Portable opened-input snapshot | `templari-current-source-cut-v1-20261114.json.zlib` |
| Source correction and permanent regression | `templari-worker-fix-v1-20261114.patch` |
| Delivery identities | `templari-worker-delivery-v1-20261114.json.zlib` |
| Final verification | `templari-worker-final-v1-20261114.json.zlib` |

The production receipt retains commands, native diagnostics, changed C,
exact-parent C identities and input hashes. The delivery receipt binds the patch and source snapshot to the
compiler and graph database. A replay elsewhere needs the recorded compiler
and native tools, the matching FB runtime for worker executions, the snapshot's
`Templari` and `SDK` include namespaces, and corresponding path substitutions.
The snapshot contains the exact build inputs from this qualification.

The wider cleanup ledger remains at 1,466 owning-source proofs and 1,471 pending
contexts on its recorded compiler cuts. Templari belongs to the additional
project scope. The broader cleanup remains active.

<!-- end of c-emitter-templari-callback.md -->
