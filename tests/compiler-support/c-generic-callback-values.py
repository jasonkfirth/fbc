"""Project: FreeBASIC C backend regression tests
File: c-generic-callback-values.py
Purpose: Exercise generic callback storage through a foreign dispatcher.
Responsibilities: Check exact calls, identity, nulls and evaluation across C dialects.
This file intentionally does NOT invoke an erased or mismatched call signature.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
from native_c_support import native_command


def run(command, execute=False):
    command = native_command(command)
    result = subprocess.run(command, capture_output=True, text=True, timeout=15 if execute else 90,
        creationflags=subprocess.BELOW_NORMAL_PRIORITY_CLASS if os.name == 'nt' else 0)
    output = result.stdout + result.stderr
    if result.returncode or 'warning:' in output:
        raise RuntimeError('generic callback check failed: ' + repr(command) + '\n' + output[-5000:])
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--gcc', required=True)
    parser.add_argument('--clang')
    parser.add_argument('--work-dir', type=Path, required=True)
    parser.add_argument('--standard', choices=('gnu11', 'gnu23'), action='append')
    parser.add_argument('--optimization', choices=('0', '2'), action='append')
    parser.add_argument('--mode', choices=('e', 'exx'), action='append')
    args = parser.parse_args()
    sources = Path(__file__).resolve().parent
    parent = args.work_dir.resolve(strict=True)
    tools = [('gcc', args.gcc)] + ([('clang', args.clang)] if args.clang else [])
    profiles = [(std, opt) for std in (args.standard or ('gnu11', 'gnu23'))
        for opt in (args.optimization or ('0', '2'))]
    programs = 0
    with tempfile.TemporaryDirectory(prefix='generic-callback-', dir=parent) as directory:
        work = Path(directory).resolve(strict=True)
        if work == parent or not work.is_relative_to(parent):
            raise RuntimeError('generic callback scratch escaped the selected directory')
        cf, obj, consumer = [work / name for name in ('unit.c', 'unit.o', 'consumer.o')]
        exe = work / ('unit.exe' if os.name == 'nt' else 'unit')
        for producer in ('gcc', 'clang'):
            for mode in (args.mode or ('e', 'exx')):
                run([args.fbc, '-prefix', args.prefix, '-gen', producer, '-' + mode,
                    '-m', 'c-generic-callback-values', '-r', sources / 'c-generic-callback-values.bas', '-o', cf])
                for name, compiler in tools:
                    diagnostics = ['-Weverything', '-Wwrite-strings', '-ferror-limit=0'] if name == 'clang' else [
                        '-Wall', '-Wextra', '-Wpedantic', '-Wcast-function-type', '-Wconversion',
                        '-Wsign-conversion', '-Wcast-qual', '-Wwrite-strings', '-Wstrict-prototypes',
                        '-Wmissing-prototypes', '-Wformat=2', '-Wdeclaration-after-statement']
                    for standard, optimization in profiles:
                        flags = ['-std=' + standard, '-O' + optimization, '-fno-strict-aliasing', '-fwrapv', *diagnostics]
                        if sum(p.stat().st_size for p in work.iterdir() if p.is_file()) > 8 * 1024 * 1024:
                            raise RuntimeError('generic callback scratch exceeded its byte cap')
                        run([compiler, *flags, '-nostdinc', '-c', cf, '-o', obj])
                        run([compiler, *flags, '-c', sources / 'c-generic-callback-values.c', '-o', consumer])
                        run([args.fbc, '-prefix', args.prefix, obj, consumer, '-x', exe])
                        output = run([exe], execute=True)
                        if output.strip() != 'CALLBACK_ERASURE_VALUES_PASS':
                            raise RuntimeError('generic callback observers did not finish: ' + output)
                        programs += 1
        print('C_GENERIC_CALLBACK_VALUES_PASS runs=' + str(programs))


if __name__ == '__main__':
    main()

# end of c-generic-callback-values.py
