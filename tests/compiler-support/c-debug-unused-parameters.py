"""Project: FreeBASIC C backend regression tests
File: c-debug-unused-parameters.py
Purpose: Qualify ignored parameters and executable private debug entries.
Responsibilities: Check source warnings, body references, native values and
the opaque-assembly boundary under both C dialects and producers.
This file does not install a compiler or claim debugger execution.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

from native_c_support import has_assembly_function, native_command, native_machine_flags


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--clang')
    parser.add_argument('--reference', help='also compare exact prior debug C')
    parser.add_argument('--gdb', help='also inspect formal values in a native debugger')
    parser.add_argument('--work-dir', type=Path, required=True)
    args = parser.parse_args()
    machine_flags = native_machine_flags(args.fbc)
    root = Path(__file__).resolve().parents[2]
    source = root / 'tests/compiler-support/c-debug-unused-parameters.bas'
    environment = os.environ.copy()
    environment['PATH'] = str(Path(args.gcc).resolve().parent) + os.pathsep + environment['PATH']
    tools = [('gcc', args.gcc)]
    if args.clang:
        tools.append(('clang', args.clang))
    marker = re.compile(r' enum \{ fb_c_abi_ignored_[0-9]+ = '
                        r'sizeof\(&([A-Za-z_][A-Za-z_0-9]*)\) \};')
    body_retention = re.compile(r'(^static [^\n;]*) __attribute__\(\(used\)\)'
                                r'(?= [A-Za-z_][A-Za-z_0-9]*\( )', re.MULTILINE)
    values = comparisons = controls = debugger_checks = native_entries = 0

    def run(command, success=True):
        result = subprocess.run(native_command(command), cwd=root, env=environment,
                                capture_output=True, text=True, timeout=180)
        if (result.returncode == 0) != success:
            raise RuntimeError(f'{command}\n{result.stdout}{result.stderr}')
        return result.stdout + result.stderr

    with tempfile.TemporaryDirectory(prefix='debug-parameters-', dir=args.work_dir) as directory:
        work = Path(directory)
        cpp, obj = work / 'parameters.c', work / 'parameters.o'
        assembly = work / 'parameters.s'
        exe = work / ('parameters.exe' if os.name == 'nt' else 'parameters')
        for producer, _ in tools:
            command = ['-prefix', args.prefix, '-gen', producer, '-g', '-exx',
                       '-w', 'all', '-m', source.stem, '-r', source, '-o', cpp]
            previous = None
            if args.reference:
                run([args.reference, *command])
                previous = cpp.read_text(encoding='utf-8')
            output = run([args.fbc, *command])
            if output.count('warning 54(') != 7:
                raise RuntimeError('BASIC unread-parameter diagnostics changed\n' + output)
            generated = cpp.read_text(encoding='utf-8')
            retained = re.findall(r'^static [^\n;]* __attribute__\(\(used\)\) '
                                  r'([A-Za-z_][A-Za-z_0-9]*)\(', generated, re.MULTILINE)
            if len(retained) != 1 or not retained[0].endswith('DORMANTDEBUGBODY'):
                raise RuntimeError('Native retention escaped the debugger-only source entry')
            names = marker.findall(generated)
            if len(names) != 7 or len(set(names)) != 7:
                raise RuntimeError('Expected seven unevaluated formal-storage references')
            for name in names:
                # Every acknowledgement must still name a declared formal,
                # rather than replace it with an unnamed C23 argument.
                if not re.search(r'^static [^\n;]*\([^\n;]*\b' + re.escape(name) +
                                 r'\b[^\n;]*\)\s*$', generated, re.MULTILINE):
                    raise RuntimeError('Named debug parameter missing: ' + name)
            if previous is not None:
                normalized = body_retention.sub(r'\1', marker.sub('', generated))
                previous = body_retention.sub(r'\1', marker.sub('', previous))
                if normalized != previous:
                    raise RuntimeError('Debug C changed beyond storage and body retention')
                comparisons += 1
            for standard in ('gnu11', 'gnu23'):
                for native, compiler in tools:
                    for optimization in ('0', '2'):
                        flags = [*machine_flags, '-std=' + standard, '-nostdinc', '-fno-strict-aliasing',
                                 '-fwrapv', '-g', '-O' + optimization, '-Wall', '-Wextra',
                                 '-Wpedantic', '-Wconversion', '-Werror=unused-parameter',
                                 '-Werror=unused-function']
                        if native == 'clang':
                            flags.append('-Weverything')
                        run([compiler, *flags, '-c', cpp, '-o', obj])
                        if optimization == '2':
                            run([compiler, *flags, '-S', cpp, '-o', assembly])
                            if not has_assembly_function(generated, assembly.read_text(), retained[0]):
                                raise RuntimeError('Native optimizer discarded the private debug entry')
                            native_entries += 1
                        run([args.fbc, '-prefix', args.prefix, '-g', obj, '-x', exe])
                        if run([exe]).strip() != 'C_DEBUG_UNUSED_PARAMETERS_PASS':
                            raise RuntimeError('Argument evaluation or caller storage changed')
                        if args.gdb and standard == 'gnu23' and optimization == '0':
                            inspected = run([
                                args.gdb, '--batch', '-nx',
                                '-ex', 'set pagination off', '-ex', 'set confirm off',
                                '-ex', 'break IGNOREDDEBUG', '-ex', 'run',
                                '-ex', 'info args', '-ex', 'continue', str(exe)])
                            for formal, value in (('VALUE', r'41\b'),
                                                  ('POINTERVALUE', '0x0'),
                                                  ('REFERENCEVALUE', r'\S'),
                                                  ('ARRAYVALUE', r'\S'),
                                                  ('CALLBACKVALUE', '0x0'),
                                                  ('RECORDVALUE', r'\S')):
                                pattern = (r'\bfb_c_sym_' + formal +
                                           r'_[A-Za-z_0-9]+ = ' + value)
                                if not re.search(pattern, inspected):
                                    raise RuntimeError('Debugger lost a formal value: ' +
                                                       formal + '\n' + inspected)
                            if 'C_DEBUG_UNUSED_PARAMETERS_PASS' not in inspected:
                                raise RuntimeError('Debugger run did not finish the fixture')
                            debugger_checks += 1
                        values += 1
            normal_command = [part for part in command if part != '-g']
            run([args.fbc, *normal_command])
            if 'DORMANTDEBUGBODY' in cpp.read_text(encoding='utf-8'):
                raise RuntimeError('Uncalled private body retained without debug information')
            run([args.fbc, *command, '-d', 'DEBUG_OPAQUE_CONTROL'])
            opaque = cpp.read_text(encoding='utf-8')
            if any('OPAQUEVALUE' in name for name in marker.findall(opaque)):
                raise RuntimeError('General assembly lost its conservative boundary')
            for _, compiler in tools:
                native_control = [compiler, *machine_flags, '-std=gnu23', '-nostdinc', '-g',
                                  '-Wall', '-Wextra', '-c', cpp, '-o', obj]
                # First prove that this is valid target assembly. A syntax
                # error must not satisfy an expected warning failure.
                run(native_control)
                diagnostic = run([*native_control, '-Werror=unused-parameter'], success=False)
                if 'OPAQUEVALUE' not in diagnostic or 'unused-parameter' not in diagnostic:
                    raise RuntimeError('Opaque unused-parameter control lost its diagnostic\n' + diagnostic)
                controls += 1
    print(f'C_DEBUG_UNUSED_PARAMETERS_PASS native_values={values} '
          f'exact_debug_C_pairs={comparisons} opaque_controls={controls} '
          f'debugger_checks={debugger_checks} optimized_private_entries={native_entries}')


if __name__ == '__main__':
    main()

# end of c-debug-unused-parameters.py
