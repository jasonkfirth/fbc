"""Project: FreeBASIC C backend regression tests
File: c-debug-local-storage.py
Purpose: Qualify unread debug values and unevaluated record-query storage.
Responsibilities: Check source diagnostics, effects, native code and debugger values.
This file does not install products or replace the corpus qualification.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

from native_c_support import native_command, native_machine_flags


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--clang')
    parser.add_argument('--reference', help='compare ordinary C with the preceding compiler')
    parser.add_argument('--gdb', help='inspect the four unread source values')
    parser.add_argument('--win32-objects', action='store_true',
                        help='also use an x86-capable native toolchain for Win32 objects')
    parser.add_argument('--work-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    source = root / 'tests/compiler-support/c-debug-local-storage.bas'
    tools = [('gcc', args.gcc)]
    if args.clang:
        tools.append(('clang', args.clang))
    machine = native_machine_flags(args.fbc)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(args.gcc).resolve().parent) + os.pathsep + environment['PATH']
    marker = re.compile(r' enum \{ fb_c_debug_local_[0-9]+ = sizeof\(&([A-Za-z_][A-Za-z_0-9]*)\) \};')
    values = controls = debugger = normal_pairs = foreign_objects = 0

    def run(command, success=True):
        result = subprocess.run(native_command(command), cwd=root, env=environment,
                                capture_output=True, text=True, timeout=180)
        if (result.returncode == 0) != success:
            raise RuntimeError(f'{command}\n{result.stdout}{result.stderr}')
        return result.stdout + result.stderr

    with tempfile.TemporaryDirectory(prefix='debug-locals-', dir=args.work_dir) as directory:
        work = Path(directory)
        cpp, obj = work / 'locals.c', work / 'locals.o'
        helper, helper_obj = work / 'checkpoint.c', work / 'checkpoint.o'
        exe = work / ('locals.exe' if os.name == 'nt' else 'locals')
        helper.write_text(
            '/* Project: FreeBASIC regression tests\n'
            '   File: checkpoint.c\n'
            '   Purpose: Supply a debugger entry outside the BASIC translation unit.\n'
            '   Responsibilities: Keep the native call boundary available.\n'
            '   This file does not read BASIC locals or own memory. */\n'
            'void debug_local_checkpoint(void);\n'
            'void debug_local_checkpoint(void) {}\n'
            '/* end of checkpoint.c */\n', encoding='utf-8')
        for producer, _ in tools:
            command = ['-prefix', args.prefix, '-gen', producer, '-g', '-exx',
                       '-w', 'all', '-m', source.stem, '-r', source, '-o', cpp]
            output = run([args.fbc, *command])
            if output.count('warning 55(') != 4:
                raise RuntimeError('The four source unread-variable diagnostics changed\n' + output)
            generated = cpp.read_text(encoding='utf-8')
            names = marker.findall(generated)
            if len(names) != 4 or len(set(names)) != 4:
                raise RuntimeError('Expected four named source-storage acknowledgements')
            selected = {}
            for name in names:
                for source_name in ('LOCALINTEGER', 'LOCALFLOAT', 'LOCALPOINTER', 'LOCALFLAG'):
                    if source_name in name:
                        selected[source_name] = name
            if len(selected) != 4:
                raise RuntimeError('An acknowledgement escaped the four source locals')
            for standard in ('gnu11', 'gnu23'):
                for native, compiler in tools:
                    for optimization in ('0', '2'):
                        flags = [*machine, '-std=' + standard, '-g', '-O' + optimization,
                                 '-Wall', '-Wextra', '-Wpedantic', '-Wconversion',
                                 '-Werror=unused-variable', '-Werror=unused-but-set-variable']
                        if native == 'clang':
                            flags.append('-Weverything')
                        cpp.write_text(generated, encoding='utf-8')
                        run([compiler, *flags, '-c', helper, '-o', helper_obj])
                        output = run([compiler, *flags, '-nostdinc', '-fno-strict-aliasing',
                                      '-fwrapv', '-c', cpp, '-o', obj])
                        if '-Wdeclaration-after-statement' in output:
                            raise RuntimeError('The acknowledgement introduced a statement boundary')
                        run([args.fbc, '-prefix', args.prefix, '-g', obj, helper_obj, '-x', exe])
                        if run([exe]).strip() != 'C_DEBUG_LOCAL_STORAGE_PASS':
                            raise RuntimeError('A call, TYPEOF query or record lifetime changed')
                        if args.gdb and standard == 'gnu23' and optimization == '0':
                            inspected = run([
                                args.gdb, '--batch', '-nx', '-ex', 'set pagination off',
                                '-ex', 'set confirm off', '-ex', 'break debug_local_checkpoint',
                                '-ex', 'run', '-ex', 'up', '-ex', 'info locals',
                                '-ex', 'continue', exe])
                            for source_name, value in (('LOCALINTEGER', r'13\b'),
                                                       ('LOCALFLOAT', r'12\.5\b'),
                                                       ('LOCALPOINTER', r'0x0\b'),
                                                       ('LOCALFLAG', r'1\b')):
                                if not re.search(r'\b' + re.escape(selected[source_name]) + ' = ' + value, inspected):
                                    raise RuntimeError('Debugger lost ' + source_name + '\n' + inspected)
                            if 'C_DEBUG_LOCAL_STORAGE_PASS' not in inspected:
                                raise RuntimeError('Debugger run did not complete the fixture')
                            debugger += 1
                        values += 1
            if args.reference:
                ordinary = [part for part in command if part != '-g']
                run([args.fbc, *ordinary])
                current = cpp.read_bytes()
                run([args.reference, *ordinary])
                if current != cpp.read_bytes():
                    raise RuntimeError('Ordinary C changed outside debug storage')
                normal_pairs += 1
            run([args.fbc, *command, '-d', 'DEBUG_OPAQUE_LOCAL_CONTROL'])
            if marker.search(cpp.read_text(encoding='utf-8')):
                raise RuntimeError('Opaque assembly lost its conservative local-storage boundary')
            for _, compiler in tools:
                native_control = [compiler, *machine, '-std=gnu23', '-nostdinc', '-g',
                                  '-Wall', '-Wextra', '-c', cpp, '-o', obj]
                # Keep syntax/assembler failures separate from the required
                # unused-local diagnostic, including on non-x86 targets.
                run(native_control)
                diagnostic = run([*native_control, '-Werror=unused-variable',
                                  '-Werror=unused-but-set-variable'], success=False)
                if 'LOCALINTEGER' not in diagnostic or 'unused-but-set-variable' not in diagnostic:
                    raise RuntimeError('Opaque local control lost its native diagnostic\n' + diagnostic)
                controls += 1
            # Cross-target object checks exercise the hidden aggregate result
            # slots without claiming that those objects were executed.
            if args.win32_objects:
                run([args.fbc, *command, '-target', 'win32'])
                for _, compiler in tools:
                    run([compiler, '-m32', '-std=gnu23', '-nostdinc', '-g', '-O2', '-Wall',
                         '-Wextra', '-Werror=unused-variable', '-Werror=unused-but-set-variable',
                         '-c', cpp, '-o', obj])
                    foreign_objects += 1
    print(f'C_DEBUG_LOCAL_STORAGE_PASS native_values={values} debugger_checks={debugger} '
          f'opaque_controls={controls} '
          f'ordinary_C_pairs={normal_pairs} Win32_objects={foreign_objects}')


if __name__ == '__main__':
    main()

# end of c-debug-local-storage.py
