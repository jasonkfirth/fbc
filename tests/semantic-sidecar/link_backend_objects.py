"""Project: FreeBASIC native link observations
File: link_backend_objects.py
Purpose: Verify C/LLVM emission-selected names against native object semantics.
Responsibilities: ABI variants, compiler output preservation and exact symbols.
This file intentionally does NOT execute any linked fixture program.
"""
from pathlib import Path
import subprocess

from link_diagnostics import LinkDiagnostics
from link_transport import (source_text, check_artifact, windows_targets,
                            windows_tool, windows_environment, callback_coverage)


CASES = {
    'ordinary': 'Declare Function Missing(ByVal Value As Long) As Long\nPrint Missing(1)',
    'cdecl': 'Declare Function Missing Cdecl(ByVal Value As Long) As Long\nPrint Missing(1)',
    'alias': 'Declare Function Missing Cdecl Alias "native_missing_alias"(ByVal Value As Long) As Long\nPrint Missing(1)',
    'alias-unused': 'Declare Function Missing Cdecl Alias "native_missing_alias"(ByVal Value As Long) As Long\n'
        'Declare Function UnusedSibling Cdecl Alias "native_missing_alias"(ByVal Value As Long) As Long\nPrint Missing(1)',
    'address': 'Declare Function Missing() As Long\nDim As Function() As Long Handler = ProcPtr(Missing)\nPrint CUInt(Handler)\nPrint Handler()',
    'extern-c': 'Extern "c"\nDeclare Function Missing(ByVal Value As Long) As Long\nEnd Extern\nPrint Missing(1)',
    'namespace': 'Namespace NativeSpace\nDeclare Function Missing(ByVal Value As Long) As Long\nEnd Namespace\nPrint NativeSpace.Missing(1)',
    'stdcall': 'Declare Function Missing Stdcall(ByVal A As Byte, ByVal B As Double) As Long\nPrint Missing(1, 2)',
    'pascal': 'Declare Function Missing Pascal(ByVal Value As Long) As Long\nPrint Missing(1)',
    'fastcall': 'Declare Function Missing __fastcall(ByVal A As Byte, ByVal B As Double) As Long\nPrint Missing(1, 2)',
    'thiscall': 'Declare Function Missing __thiscall(ByVal Context As Any Ptr, ByVal Value As Long) As Long\nPrint Missing(0, 2)',
    'fastcall-byref': 'Declare Function Missing __fastcall(ByRef A As Long, ByVal B As LongInt) As Long\nDim As Long Value\nPrint Missing(Value, 2)',
    'fastcall-result': 'Type ResultRecord\nA As Long\nB As Long\nC As Long\nEnd Type\n'
        'Declare Function Missing __fastcall(ByVal Value As Long) As ResultRecord\nDim As ResultRecord Value = Missing(1)\nPrint Value.A',
    'fastcall-record': 'Type ArgumentRecord\nA As Long\nB As Long\nC As Long\nEnd Type\n'
        'Declare Function Missing __fastcall(ByVal Value As ArgumentRecord) As Long\nDim As ArgumentRecord Value\nPrint Missing(Value)',
    'fastcall-nontrivial': 'Type ArgumentRecord\nA As Long\nB As Long\nC As Long\nDeclare Constructor()\nEnd Type\n'
        'Constructor ArgumentRecord()\nEnd Constructor\n'
        'Declare Function Missing __fastcall(ByVal Value As ArgumentRecord) As Long\nDim As ArgumentRecord Value\nPrint Missing(Value)',
    'defined': 'Declare Function Present(ByVal Value As Long) As Long\nFunction Present(ByVal Value As Long) As Long\nReturn Value\nEnd Function\nPrint Present(1)',
    'unused': 'Declare Function Unused() As Long',
}


def invoke(test, source, backend, target, *, observe=True, link=False, compiler=None):
    test.sequence += 1
    artifact = test.working / ('backend-' + str(test.sequence) + '.lnk')
    command = [str(compiler or test.compiler), '-prefix', str(test.toolchain_prefix),
               '-i', str(test.root / 'inc'), '-target', target, '-gen', backend]
    if observe:
        command += ['-semantic-link-diagnostics', str(artifact)]
    if link:
        command += ['-C', '-x', str(test.working / 'backend.exe'), '-o', str(test.working / 'backend.o')]
    else:
        command += ['-r', '-o', str(test.working / ('backend.ll' if backend == 'llvm' else 'backend.c'))]
    command.append(str(source))
    environment = windows_environment(test, target, backend) if test.native_windows else None
    result = subprocess.run(command, capture_output=True, cwd=test.working,
                            env=environment, timeout=60)
    return result, artifact


def object_symbols(test, path):
    # This is the native object's structured symbol table, not program text or
    # human linker errors. Only exact names are needed to check compiler facts.
    tool = windows_tool(test, 'llvm-nm')
    result = subprocess.run([str(tool), '--format=posix', str(path)], capture_output=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    return {row.split()[0].decode('ascii') for row in result.stdout.splitlines() if row.split()}


def check_c_backends(test, baseline=None, native_reader=None):
    if not test.native_windows:
        test.skipTest('Native C/Clang object matrix requires the Windows toolchain')
    for backend in ('gcc', 'clang'):
        for target in windows_targets(test):
            for case, body in CASES.items():
                with test.subTest(backend=backend, target=target, case=case):
                    source = test.source(source_text(case + '.bas', body), case + '.bas')
                    before, _ = invoke(test, source, backend, target, observe=False, compiler=baseline)
                    emitted = (test.working / 'backend.c').read_bytes()
                    result, artifact = invoke(test, source, backend, target)
                    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    test.assertEqual((result.returncode, result.stdout, result.stderr),
                                     (before.returncode, before.stdout, before.stderr))
                    test.assertEqual((test.working / 'backend.c').read_bytes(), emitted)
                    check_artifact(test, artifact, result, native_reader)
                    result, artifact = invoke(test, source, backend, target, link=True)
                    expected_exit = 0 if case in ('defined', 'unused') else 1
                    test.assertEqual(result.returncode, expected_exit, result.stdout + result.stderr)
                    observation = check_artifact(test, artifact, result, native_reader)
                    test.assertEqual(observation.version, '2')
                    symbols = object_symbols(test, test.working / 'backend.o')
                    available = callback_coverage(test, observation)
                    test.assertEqual(len(observation.callbacks), expected_exit if available else 0)
                    for callback in observation.callbacks:
                        test.assertEqual(callback[2], 'undefined-symbol')
                        matches = [row for row in observation.procedures.values()
                                   if row[6] == 'observed' and row[7] == callback[3] and row[12] == '1']
                        test.assertEqual(len(matches), 1, (callback, observation.procedures))
                        test.assertIn(callback[3], symbols)
                    if case == 'defined':
                        selected = next(row for row in observation.procedures.values() if row[5] == 'PRESENT')
                        test.assertEqual(selected[6], 'observed')
                        test.assertIn(selected[7], symbols)
                    if case in ('unused', 'alias-unused'):
                        spelling = 'UNUSED' if case == 'unused' else 'UNUSEDSIBLING'
                        selected = next(row for row in observation.procedures.values() if row[5] == spelling)
                        test.assertEqual(selected[12], '0')


def check_llvm_objects(test, baseline=None, native_reader=None):
    if not test.native_windows:
        test.skipTest('LLVM object oracle uses the installed Windows Clang toolchain')
    compiler = windows_tool(test, 'clang')
    targets = {'win64': 'x86_64-w64-windows-gnu', 'win32': 'i686-w64-windows-gnu',
               'linux-x86_64': 'x86_64-unknown-linux-gnu', 'darwin-x86_64': 'x86_64-apple-darwin'}
    for target, triple in targets.items():
        for case, body in CASES.items():
            with test.subTest(target=target, case=case):
                source = test.source(source_text(case + '.bas', body), case + '.bas')
                before, _ = invoke(test, source, 'llvm', target, observe=False, compiler=baseline)
                emitted = (test.working / 'backend.ll').read_bytes()
                corrected = emitted.replace(b'x86_fastcall ', b'x86_fastcallcc ').replace(
                    b'x86_thiscall ', b'x86_thiscallcc ')
                if target == 'win32' and case == 'address':
                    corrected = corrected.replace(b'x86_stdcallcc i32 (  )*', b'i32 (  )*')
                if corrected != emitted:
                    # Keep evidence of the preceding emitter failure. Only the
                    # convention keywords change; observation itself must not
                    # change the fixed compiler's output.
                    rejected = subprocess.run([str(compiler), '--target=' + triple,
                        '-Wno-override-module', '-c', '-x', 'ir',
                        str(test.working / 'backend.ll'), '-o', str(test.working / 'old-llvm.o')],
                        capture_output=True, timeout=60)
                    test.assertNotEqual(rejected.returncode, 0)
                    test.assertIn(b'expected type', rejected.stderr)
                    unobserved, _ = invoke(test, source, 'llvm', target, observe=False)
                    test.assertEqual(unobserved.returncode, 0, unobserved.stdout + unobserved.stderr)
                    test.assertEqual((test.working / 'backend.ll').read_bytes(), corrected)
                result, artifact = invoke(test, source, 'llvm', target)
                test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                test.assertEqual((result.returncode, result.stdout, result.stderr),
                                 (before.returncode, before.stdout, before.stderr))
                test.assertEqual((test.working / 'backend.ll').read_bytes(), corrected)
                if target == 'win32' and case == 'address':
                    test.assertIn(b'call x86_stdcallcc i32 %', corrected)
                observation = check_artifact(test, artifact, result, native_reader)
                test.assertEqual(observation.version, '2')
                object_path = test.working / 'llvm.o'
                compiled = subprocess.run([str(compiler), '--target=' + triple, '-Wno-override-module',
                    '-c', '-x', 'ir', str(test.working / 'backend.ll'), '-o', str(object_path)],
                    capture_output=True, timeout=60)
                test.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
                symbols = object_symbols(test, object_path)
                if case != 'unused':
                    spelling = 'PRESENT' if case == 'defined' else 'MISSING'
                    selected = [row for row in observation.procedures.values() if row[5] == spelling]
                    test.assertEqual(len(selected), 1)
                    test.assertEqual(selected[0][6], 'observed')
                    test.assertEqual(selected[0][12], '1')
                    test.assertIn(selected[0][7], symbols)

# end of link_backend_objects.py
