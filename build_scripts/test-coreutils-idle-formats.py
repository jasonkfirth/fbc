"""Check complete emitted pinky/who modules against native idle-time output.

File: test-coreutils-idle-formats.py
Use the original helper bodies, controlled clocks and actual long-double runtime.
Application startup and terminal enumeration retain their own test workflows.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import runpy
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-c', type=Path, required=True)
    parser.add_argument('--program', choices=('pinky', 'who'), required=True)
    parser.add_argument('--rtlib-dir', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--objcopy', default='objcopy')
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    compiler, objcopy = shutil.which(args.cc), shutil.which(args.objcopy)
    assert compiler and objcopy
    raw = args.source_c.read_bytes()
    text = raw.decode()
    body = re.search(r'^static const char\* IDLE_STRING\([^;\n]*\)\r?\n\{(.*?)\n\}', text, re.M | re.S)
    assert body and 'c_audit_idle_call' not in text
    capacity = 22 if args.program == 'pinky' else 6
    assert re.search(r'static int8 \w+\[' + str(capacity) + r'\];', body[1])
    observer = ROOT / 'tests/compiler-support/coreutils-idle-formats.c'
    section_checker = Path(__file__).with_name('test-coreutils-stat-formats.py')
    executable_sections = runpy.run_path(str(section_checker))['executable_sections']
    support = args.rtlib_dir / 'math/longdouble_ops.c'
    inputs = [args.source_c, observer, section_checker, Path(__file__)]
    if args.program == 'who':
        inputs += [support, support.with_name('extended_math.h'), support.with_name('longdouble_parse.h')]
    pins = {str(path.resolve()): sha(path) for path in inputs}
    flags = json.loads(args.native_flags_json.read_text())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not flag.startswith('-O')]
    flags += ['-O' + args.optimization, '-fno-strict-aliasing', '-fwrapv',
        '-ffunction-sections', '-fdata-sections']
    state = dict(checks_complete=False, source_pins=pins, program=args.program,
        optimization=args.optimization, production_buffer_bytes=capacity,
        commands=[], executions=[], peak_owned_bytes=0)

    def run(command, work):
        command = list(map(str, command))
        result = subprocess.run(command, cwd=work, stdin=subprocess.DEVNULL,
            capture_output=True, text=True)
        output = result.stdout + result.stderr
        assert len(output.encode()) < 4 * 1024 * 1024
        row = dict(command=command, exit=result.returncode, output=output,
            warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        state['peak_owned_bytes'] = max(state['peak_owned_bytes'],
            sum(path.stat().st_size for path in work.iterdir() if path.is_file()))
        assert state['peak_owned_bytes'] < 8 * 1024 * 1024
        assert result.returncode == 0, output[-6000:]
        return row

    assert not args.receipt.exists()
    try:
        with tempfile.TemporaryDirectory(prefix='fbc-idle-formats-', dir=args.scratch_root) as directory:
            work = Path(directory)
            module, obj = work / 'module.c', work / 'module.o'
            invocation = 'IDLE_STRING(access)' if args.program == 'pinky' else 'IDLE_STRING(access, boot)'
            wrapper = '''
/* This entry observes the unchanged production helper. */
const char* c_audit_idle_call(long, long);
const char* c_audit_idle_call(long access, long boot)
{
    (void)boot;
    return INVOCATION;
}
'''.replace('INVOCATION', invocation)
            module.write_bytes(raw + wrapper.encode())
            compiled = run([compiler, *flags, '-c', module, '-o', obj], work)
            state['production_warnings'] = compiled['warnings']
            sections = executable_sections(obj)
            run([objcopy, '--redefine-sym', 'main=c_audit_unused_idle_main', obj], work)
            assert executable_sections(obj) == sections
            state['executable_sections_unchanged_by_entry_symbol_rename'] = sections
            objects = [obj]
            support_flags = [flag for flag in flags if flag != '-nostdinc']
            if args.program == 'who':
                helper = work / 'longdouble.o'
                run([compiler, *support_flags, '-c', support.resolve(), '-o', helper], work)
                objects.append(helper)
            observed = work / 'observer.o'
            mode = ['-DIDLE_PINKY'] if args.program == 'pinky' else []
            run([compiler, *support_flags, *mode, '-c', observer, '-o', observed], work)
            executable = work / 'values'
            run([compiler, '-Wl,--gc-sections', '-Wl,--wrap=rpl_time', '-Wl,--wrap=sprintf',
                '-Wl,--wrap=dcgettext', *objects, observed, '-lm', '-o', executable], work)
            for clock in range(7):
                executed = run([executable, str(clock)], work)
                match = re.fullmatch(r'IDLE_FORMAT_PASS cases=(\d+) checks=(\d+) max_length=(\d+)\s*', executed['output'])
                assert match, executed['output']
                state['executions'].append(dict(clock=clock, cases=int(match[1]),
                    checks=int(match[2]), maximum_formatted_length=int(match[3])))
        assert all(sha(path) == pin for path, pin in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists(),
            cases=sum(row['cases'] for row in state['executions']),
            checks=sum(row['checks'] for row in state['executions']))
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists())
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('IDLE_FORMAT_QUALIFIED', args.program, args.optimization, state['cases'], state['checks'])
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-coreutils-idle-formats.py
