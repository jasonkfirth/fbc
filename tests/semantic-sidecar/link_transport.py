"""Project: FreeBASIC native link observations
File: link_transport.py
Purpose: Exercise the production link diagnostic transport independently of AST.
Responsibilities: Native resolution, source closure and unchanged compiler output.
This file intentionally does NOT run any linked fixture program.
"""
import hashlib
import codecs
import os
from pathlib import Path
import subprocess

from link_diagnostics import LinkDiagnostics
from sidecar import Model

# These fixtures inspect x86-64 assembly without assembling or linking it.
# Select its target explicitly so ARM hosts exercise the same observer paths.
GAS64_EMISSION_TARGET = ('-target', 'linux-x86_64')


def source_text(filename, body, language='fb'):
    return ("' Project: FreeBASIC native link observations\n"
            "' File: " + filename + "\n"
            "' Purpose: Exercise compiler-owned linkage decisions.\n"
            "' Responsibilities: A bounded source, binding or link control.\n"
            "' This file intentionally does NOT run its linked program.\n\n"
            '#lang "' + language + '"\n\n' + body +
            "\n\n' end of " + filename + '\n')


def invoke(test, sources, backend, *, link=False, extra=(), observed=True,
           model=False, compiler=None):
    test.sequence += 1
    artifact = test.working / f'link-{test.sequence}.lnk'
    ast = test.working / f'link-{test.sequence}.sem'
    command = [str(compiler or test.compiler), '-prefix', str(test.toolchain_prefix),
               '-i', str(test.root / 'inc'), '-gen', backend]
    if backend == 'gas':
        command += ['-target', 'win32' if test.native_windows else 'linux-x86']
    if not link:
        command += ['-r']
    if observed:
        command += ['-semantic-link-diagnostics', str(artifact)]
    if model:
        command += ['-semantic-model', str(ast)]
    command += [*map(str, extra), *map(str, sources)]
    result = subprocess.run(command, cwd=test.working, capture_output=True, timeout=60)
    return result, artifact, ast


def check_sources(test, observation):
    for row in observation.sources:
        raw = Path(row[3]).read_bytes()
        test.assertEqual(int(row[5]), len(raw))
        test.assertEqual(row[4], hashlib.sha256(raw).hexdigest())


def check_artifact(test, artifact, result, native_reader=None):
    observation = LinkDiagnostics.read(artifact, result.returncode)
    check_sources(test, observation)
    if native_reader:
        root = observation.modules[0][2] if observation.modules else ''
        accepted = subprocess.run([str(native_reader), str(artifact), root, 'valid', str(result.returncode)],
                                  capture_output=True, timeout=30)
        test.assertEqual(accepted.returncode, 0, accepted.stdout + accepted.stderr)
        counts = [int(value) for value in accepted.stdout.decode('ascii').split()[1:]]
        test.assertEqual(counts, [len(observation.modules), len(observation.sources),
                                  len(observation.procedures), len(observation.callbacks)])
    return observation


def check_frontend(test, native_reader=None):
    body = ('Declare Function LinkAbsent() As Long\n'
            '#If 0\nDeclare Function InactiveAbsent() As Long\n#EndIf\n'
            'Type Handler As Function() As Long\n')
    source = test.source(source_text('frontend.bas', body), 'frontend.bas')
    for backend in test.backends:
        for with_ast in (False, True):
            with test.subTest(backend=backend, ast=with_ast):
                before, _, _ = invoke(test, [source], backend, observed=False)
                emitted = test.emission_path(source.stem, backend)
                baseline = emitted.read_bytes()
                result, artifact, ast = invoke(test, [source], backend, model=with_ast)
                test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                test.assertEqual((result.stdout, result.stderr), (before.stdout, before.stderr))
                test.assertEqual(emitted.read_bytes(), baseline)
                observation = check_artifact(test, artifact, result, native_reader)
                test.assertEqual(observation.link[1:], ['0', '0', '0', 'unavailable', ''])
                test.assertEqual([row[5] for row in observation.procedures.values()], ['LINKABSENT'])
                test.assertEqual([row[6] for row in observation.procedures.values()],
                                 ['unobserved'])
                test.assertEqual([row[12] for row in observation.procedures.values()], ['0'])
                if with_ast:
                    Model.read(ast)
    invalid = test.source(source_text('invalid.bas',
        'Declare Function Rejected() As Long Extern'), 'invalid.bas')
    for backend in test.backends:
        result, artifact, ast = invoke(test, [invalid], backend, model=True)
        test.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        observation = check_artifact(test, artifact, result, native_reader)
        test.assertFalse(observation.succeeded)
        if ast.exists():
            with test.assertRaises(ValueError):
                Model.read(ast)


def check_native(test, baseline_compiler=None, native_reader=None):
    if not test.native_windows:
        test.skipTest('Native Windows linker matrix requires the Windows toolchain')
    cases = {
        'unused': ('Declare Function LinkAbsent() As Long', 'fb', 0, 0),
        'unbound': ('Declare Function LinkAbsent() As Long\nPrint LinkAbsent()', 'fb', 1, 1),
        'present': ('Declare Function LinkPresent() As Long\nFunction LinkPresent() As Long\n'
                    'Return 23\nEnd Function\nPrint LinkPresent()', 'fb', 0, 0),
        'extern-c': ('Extern "c"\nDeclare Function LinkAbsent() As Long\nEnd Extern\n'
                     'Print LinkAbsent()', 'fb', 1, 1),
        'alias': ('Declare Function LinkAbsent Cdecl Alias "actual_missing_link_name"() As Long\n'
                  'Print LinkAbsent()', 'fb', 1, 1),
        'namespace': ('Namespace LinkSpace\nDeclare Function LinkAbsent() As Long\n'
                      'End Namespace\nPrint LinkSpace.LinkAbsent()', 'fb', 1, 1),
        'stdcall': ('Declare Function LinkAbsent Stdcall() As Long\nPrint LinkAbsent()', 'fb', 1, 1),
        'keyword-parameter': ('Option NoKeyword Extern\nDeclare Function LinkPresent(ByVal Extern As Long) As Long\n'
                              'Function LinkPresent(ByVal Value As Long) As Long\nReturn Value\n'
                              'End Function\nPrint LinkPresent(23)', 'deprecated', 0, 0),
        'keyword-procedure': ('Option NoKeyword Extern\nDeclare Function Extern() As Long\n'
                              'Function Extern() As Long\nReturn 23\nEnd Function\nPrint Extern()', 'deprecated', 0, 0),
        'inactive': ('#If 0\nDeclare Function LinkAbsent() As Long\nPrint LinkAbsent()\n#EndIf\nPrint 23', 'fb', 0, 0),
    }
    for backend in ('gas64', 'gas'):
        executable = test.working / (backend + '.exe')
        object_path = test.working / (backend + '.o')
        extra = ('-C', '-x', executable, '-o', object_path)
        for case, (body, language, expected_exit, expected_callbacks) in cases.items():
            source = test.source(source_text(case + '.bas', body, language), case + '.bas')
            for with_ast in (False, True):
                with test.subTest(backend=backend, case=case, ast=with_ast):
                    previous, _, _ = invoke(test, [source], backend, link=True, extra=extra,
                                            observed=False, compiler=baseline_compiler)
                    previous_object = object_path.read_bytes()
                    result, artifact, ast = invoke(test, [source], backend, link=True,
                                                    extra=extra, model=with_ast)
                    test.assertEqual(result.returncode, expected_exit, result.stdout + result.stderr)
                    test.assertEqual((result.returncode, result.stdout, result.stderr),
                                     (previous.returncode, previous.stdout, previous.stderr))
                    test.assertEqual(object_path.read_bytes(), previous_object)
                    observation = check_artifact(test, artifact, result, native_reader)
                    test.assertEqual(observation.link[1:3], ['1', '1'])
                    test.assertEqual(observation.link[4], 'available')
                    test.assertEqual(len(observation.callbacks), expected_callbacks)
                    for callback in observation.callbacks:
                        matches = [row for row in observation.procedures.values()
                                   if row[6] == 'observed' and row[7] == callback[3]]
                        test.assertEqual(len(matches), 1, (callback, observation.procedures))
                        test.assertEqual(matches[0][9], '1')
                    if with_ast:
                        if expected_exit:
                            if ast.exists():
                                with test.assertRaises(ValueError):
                                    Model.read(ast)
                        else:
                            Model.read(ast)
        callee = test.source(source_text('callee.bas',
            'Function LinkSeparate() As Long\nReturn 23\nEnd Function'), 'callee.bas')
        caller = test.source(source_text('caller.bas',
            'Declare Function LinkSeparate() As Long\nPrint LinkSeparate()'), 'caller.bas')
        compiled, _, _ = invoke(test, [callee], backend, link=True, observed=False,
                               extra=('-c', '-o', object_path))
        test.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
        for source_list in ([caller, object_path], [caller, callee]):
            result, artifact, _ = invoke(test, source_list, backend, link=True,
                                         extra=('-x', executable))
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            observation = check_artifact(test, artifact, result, native_reader)
            test.assertFalse(observation.callbacks)
            test.assertEqual(len(observation.modules), 1 if object_path in source_list else 2)
        # Missing libraries are another typed callback kind, independent of
        # procedure bindings. Force native ld to own this failure with -Wl.
        result, artifact, _ = invoke(test, [caller, object_path], backend, link=True,
            extra=('-x', executable, '-Wl', '-lfbc_semantic_missing_library_9287'))
        test.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        observation = check_artifact(test, artifact, result, native_reader)
        test.assertEqual([row[2:] for row in observation.callbacks],
                         [['missing-lib', '-lfbc_semantic_missing_library_9287']])
        missing = test.source(source_text('response.bas',
            'Declare Function ResponseAbsent() As Long\nPrint ResponseAbsent()'), 'response.bas')
        paths = tuple(value for index in range(40) for value in
                      ('-p', str(test.working / ('long-link-search-directory-' + str(index)))))
        result, artifact, _ = invoke(test, [missing], backend, link=True,
            extra=('-x', executable, *paths))
        test.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        observation = check_artifact(test, artifact, result, native_reader)
        test.assertGreater(len(observation.arguments), 2047)
        test.assertEqual(len(observation.callbacks), 1)
        test.assertEqual(observation.link[4], 'available')

    # A configured user callback belongs to the native link closure. Keep it
    # intact and report unavailable coverage instead of silently replacing it.
    handler = test.source(source_text('handler.bas', 'End 0'), 'handler.bas')
    helper = test.working / 'handler.exe'
    result, _, _ = invoke(test, [handler], 'gas64', link=True, observed=False, extra=('-x', helper))
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result, artifact, _ = invoke(test, [missing], 'gas64', link=True,
        extra=('-x', executable, '-Wl', '--error-handling-script=' + str(helper)))
    test.assertEqual(result.returncode, 1, result.stdout + result.stderr)
    observation = check_artifact(test, artifact, result, native_reader)
    test.assertFalse(observation.callbacks)
    test.assertEqual(observation.link[4], 'unavailable')


def check_source_ownership(test, native_reader=None):
    child = test.source(source_text('declarations.bi',
        'Declare Function IncludedAbsent() As Long'), 'declarations.bi')
    first = test.source(source_text('first.bas', '#Include "declarations.bi"\n'
        '#Macro Header()\nDeclare Function GeneratedAbsent() As Long\n#EndMacro\n'
        'Header()\n#Line 700 "logical.bas"\nDeclare Function RemappedAbsent() As Long'), 'first.bas')
    second = test.source(source_text('second.bas', '#Include "declarations.bi"\n'
        'Declare Function SecondAbsent() As Long\n#Line 900\n'
        'Declare Function LineOnlyAbsent() As Long'), 'second.bas')
    for backend in test.backends:
        result, artifact, _ = invoke(test, [first, second], backend)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        observation = check_artifact(test, artifact, result, native_reader)
        test.assertEqual(len(observation.modules), 2)
        test.assertEqual(len(observation.sources), 4)
        included = [row for row in observation.procedures.values() if row[5] == 'INCLUDEDABSENT']
        test.assertEqual({int(row[2]) for row in included}, {1, 2})
        test.assertTrue(all(row[8] == test.compiler_path(child) for row in included))
        generated = next(row for row in observation.procedures.values() if row[5] == 'GENERATEDABSENT')
        remapped = next(row for row in observation.procedures.values() if row[5] == 'REMAPPEDABSENT')
        line_only = next(row for row in observation.procedures.values() if row[5] == 'LINEONLYABSENT')
        test.assertEqual(generated[9:12], ['0', '0', '0'])
        test.assertEqual(remapped[9:12], ['0', '0', '0'])
        test.assertEqual(line_only[9:12], ['0', '0', '0'])
    text = source_text('encoded.bas', 'Declare Function EncodedAbsent() As Long')
    encodings = [('utf8', b'', 'utf-8'), ('utf8bom', codecs.BOM_UTF8, 'utf-8'),
                 ('utf16le', codecs.BOM_UTF16_LE, 'utf-16-le'),
                 ('utf16be', codecs.BOM_UTF16_BE, 'utf-16-be'),
                 ('utf32le', codecs.BOM_UTF32_LE, 'utf-32-le'),
                 ('utf32be', codecs.BOM_UTF32_BE, 'utf-32-be')]
    for label, bom, encoding in encodings:
        source = test.working / (label + '.bas')
        source.write_bytes(bom + text.encode(encoding))
        result, artifact, _ = invoke(test, [source], 'gas64', extra=GAS64_EMISSION_TARGET)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        observation = check_artifact(test, artifact, result, native_reader)
        procedure = next(iter(observation.procedures.values()))
        test.assertEqual(procedure[9:12], ['1', '9', '17'])


def check_publication(test):
    source = test.source(source_text('protected.bas', 'Print 23'), 'protected.bas')
    child = test.source(source_text('protected.bi', 'Declare Sub Included()'), 'protected.bi')
    root = test.source(source_text('root.bas', '#Include "protected.bi"\nPrint 23'), 'root.bas')
    sentinel = test.working / 'sentinel.lnk'
    sentinel.write_bytes(b'original destination\n')
    hardlink = test.working / 'source-link.lnk'
    os.link(source, hardlink)
    for destination, input_source in ((source, source), (child, root), (hardlink, source)):
        original = destination.read_bytes()
        result, _, _ = invoke(test, [input_source], 'gas64', observed=False,
            extra=GAS64_EMISSION_TARGET + ('-semantic-link-diagnostics', destination))
        test.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        test.assertIn(b'could not write semantic link diagnostics', result.stdout)
        test.assertEqual(destination.read_bytes(), original)
    for suffix in ('.asm', '.o'):
        output = test.working / ('owned' + suffix)
        result, _, _ = invoke(test, [source], 'gas64', observed=False,
            extra=GAS64_EMISSION_TARGET + ('-semantic-link-diagnostics', output, '-o', output))
        test.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        test.assertFalse(output.exists() and output.read_bytes().startswith(b'FBCLNK'))
    # The shared checked writer must also refuse collisions between distinct
    # semantic artifacts, including a destination that already has contents.
    for other_option in ('-semantic-model', '-semantic-diagnostics'):
        result, _, _ = invoke(test, [source], 'gas64', observed=False,
            extra=GAS64_EMISSION_TARGET + ('-semantic-link-diagnostics', sentinel, other_option, sentinel))
        test.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        test.assertEqual(sentinel.read_bytes(), b'original destination\n')
    result, _, _ = invoke(test, [source], 'gas64', observed=False,
        extra=GAS64_EMISSION_TARGET + ('-semantic-link-diagnostics', test.working / 'missing' / 'diagnostic.lnk'))
    test.assertEqual(result.returncode, 1, result.stdout + result.stderr)
    result, _, _ = invoke(test, [source], 'gas64', observed=False,
        extra=GAS64_EMISSION_TARGET + ('-semantic-link-diagnostics', sentinel))
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    LinkDiagnostics.read(sentinel, 0)
    test.assertFalse(list(test.working.glob('.fb-semantic-*')))


def check_lifetimes(test, native_reader=None):
    # Procedure bodies release local variables and labels before later native
    # headers reuse the symbol pool. Real procedure registrations must remain
    # distinct from those released non-procedure identities.
    body = '\n'.join(f'Sub Owner{index}()\nDim ReleasedLocal As Long\n'
        'GoTo Finished\nFinished:\nPrint ReleasedLocal\nEnd Sub\n'
        f'Declare Function Later{index}() As Long' for index in range(1000))
    source = test.source(source_text('released.bas', body), 'released.bas')
    result, artifact, _ = invoke(test, [source], 'gas64', extra=GAS64_EMISSION_TARGET)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    observation = check_artifact(test, artifact, result, native_reader)
    test.assertEqual(len(observation.procedures), 2000)
    test.assertEqual(set(observation.procedures), set(range(1, 2001)))
    test.assertEqual(len({row[5] for row in observation.procedures.values()}), 2000)
    test.assertEqual(sum(row[6] == 'observed' for row in observation.procedures.values()), 1000)
    test.assertEqual(sum(row[6] == 'unobserved' for row in observation.procedures.values()), 1000)
    # Callback dispatch must not create a public artifact when its invocation
    # capsule is malformed. The existing pathname is never truncated.
    journal = test.working / 'invalid-journal.lnk'
    journal.write_bytes(b'original journal\n')
    result = subprocess.run([str(test.compiler), 'undefined-symbol', 'absent'],
        capture_output=True, timeout=30, env={**os.environ,
        'FBC_SEMANTIC_LINK_CALLBACK_V1': str(journal) + '\t' + '0' * 48})
    test.assertEqual(result.returncode, 2)
    test.assertEqual(journal.read_bytes(), b'original journal\n')

# end of link_transport.py
