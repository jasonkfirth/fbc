#!/usr/bin/env python3
"""FreeBASIC translated Gnulib allocator regression.
File test-careadlinkat-allocator.py
Compare a complete emitted module with its original C and allocator header.
Source conversion and the aggregate warning census have separate owners.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module-c', type=Path, required=True)
    parser.add_argument('--reference-c', type=Path, required=True)
    parser.add_argument('--allocator-header', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    driver = root / 'tests/compiler-support/coreutils-careadlinkat.c'
    flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not flag.startswith('-O')] + ['-O' + args.optimization]
    header_flags = [flag for flag in flags if flag != '-nostdinc']
    inputs = [args.module_c, args.reference_c, args.allocator_header, driver]
    state = dict(checks_complete=False, commands=[], optimization=args.optimization,
                 source_pins={str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs})

    def run(command, work):
        command = list(map(str, command))
        options = dict(cwd=work, stdin=subprocess.DEVNULL, capture_output=True, text=True)
        if os.name == 'nt':
            options['creationflags'] = subprocess.CREATE_NO_WINDOW | subprocess.BELOW_NORMAL_PRIORITY_CLASS
        result = subprocess.run(command, **options)
        output = result.stdout + result.stderr
        row = dict(command=command, exit=result.returncode,
                   warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        assert result.returncode == 0, (command, result.returncode, output[-4000:])
        if len(output) < 32768:
            row['output'] = output
        else:
            row['output_sha256'] = hashlib.sha256(output.encode()).hexdigest()
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix='careadlinkat-allocator-', dir=args.scratch_root) as directory:
            work = Path(directory)
            (work / 'allocator.h').write_bytes(args.allocator_header.read_bytes())
            suffix = '.exe' if os.name == 'nt' else ''
            for name, source, options in [('driver', driver, header_flags),
                                          ('reference', args.reference_c, header_flags),
                                          ('module', args.module_c, flags)]:
                run([args.cc, *options, '-I', work, '-c', source, '-o', work / (name + '.o')], work)
                if name == 'module':
                    state['module_warnings'] = state['commands'][-1]['warnings']
            outputs = []
            for name in ('reference', 'module'):
                binary = work / (name + suffix)
                run([args.cc, work / (name + '.o'), work / 'driver.o', '-o', binary], work)
                output = run([binary], work)
                assert len(output.splitlines()) == 4801, (name, len(output.splitlines()))
                outputs.append(output)
            differences = [(index + 1, left, right) for index, (left, right) in enumerate(
                zip(outputs[0].splitlines(), outputs[1].splitlines())) if left != right]
            assert not differences, (len(differences), differences[:3])
            state.update(comparison_pairs=4801, conforming_cases=3600,
                         returning_die_controls=1200, synthetic_size_limit_controls=1,
                         output_sha256=hashlib.sha256(outputs[0].encode()).hexdigest(),
                         peak_scratch_bytes=sum(path.stat().st_size for path in work.iterdir() if path.is_file()))
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state['failure'] = repr(error)
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('CAREADLINKAT_ALLOCATOR_PASS 3600 cases, 1200 returning-provider controls, 1 size-limit control')


if __name__ == '__main__':
    main()

# end of test-careadlinkat-allocator.py
