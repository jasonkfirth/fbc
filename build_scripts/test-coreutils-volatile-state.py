"""Exercise the complete emitted savewd error-state accessor.

File test-coreutils-volatile-state.py
Own native compilation, signed boundary values and bounded scratch receipts.
Directory and process operations retain application qualification.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import runpy
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module-c', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    observer = ROOT / 'tests/compiler-support/coreutils-volatile-state.c'
    support = ROOT / 'tests/compiler-support/native_c_support.py'
    digest = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
    pins = {str(path.resolve()): digest(path) for path in (args.module_c, observer, support, Path(__file__))}
    native_command = runpy.run_path(str(support))['native_command']
    flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not re.fullmatch(r'-O\w+', flag)]
    flags += ['-O' + args.optimization, '-ffunction-sections', '-fdata-sections']
    header_flags = [flag for flag in flags if flag != '-nostdinc' and not flag.startswith('-masm=')]
    state = dict(checks_complete=False, source_pins=pins, commands=[], peak_scratch_bytes=0)

    def run(command, work):
        command = native_command(command)
        result = subprocess.run(command, cwd=work, stdin=subprocess.DEVNULL, capture_output=True, text=True)
        output = result.stdout + result.stderr
        row = dict(command=command, exit=result.returncode, output=output,
                   warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        assert result.returncode == 0, output[-4000:]
        return row

    assert not args.receipt.exists()
    try:
        with tempfile.TemporaryDirectory(prefix='coreutils-volatile-state-', dir=args.scratch_root) as directory:
            work = Path(directory)
            text = '\n'.join(args.module_c.read_text().splitlines()) + '\n'
            record = re.search(r'^struct (\w*SAVEWD) \{\s*int32 STATE;', text, re.M)
            assert record
            # Construct the compiler's canonical record inside its own unit.
            # The observer crosses a scalar ABI and checks the full guarded image.
            wrapper = '\nint32 audit_savewd_state(int32, int32, uint32*);\n'
            wrapper += 'int32 audit_savewd_state(int32 state, int32 payload, uint32 *unchanged)\n{\n'
            wrapper += '    struct { uint64 before; struct ' + record[1] + ' value; uint64 after; } record;\n'
            wrapper += '''    uint8 snapshot[sizeof(record)];
    int32 result;
    __builtin_memset(&record, 0, sizeof(record));
    record.before = 0x123456789abcdef0ULL;
    record.after = 0xfedcba9876543210ULL;
    record.value.STATE = state;
    record.value.VAL.ERRNUM = payload;
    __builtin_memcpy(snapshot, &record, sizeof(record));
    result = savewd_errno(&record.value);
    *unchanged = (uint32)(__builtin_memcmp(snapshot, &record, sizeof(record)) == 0);
    return result;
}
'''
            source = work / 'module.c'
            source.write_text(text + wrapper)
            module, observed, binary = work / 'module.o', work / 'observer.o', work / 'state'
            run([args.cc, *flags, '-c', source, '-o', module], work)
            run([args.cc, *header_flags, '-c', observer, '-o', observed], work)
            run([args.cc, '-Wl,--gc-sections', module, observed, '-o', binary], work)
            executed = run([binary], work)
            match = re.fullmatch(r'VOLATILE_STATE_PASS cases=(\d+) checks=(\d+)\s*', executed['output'])
            assert match, executed['output']
            state.update(cases=int(match[1]), checks=int(match[2]),
                         peak_scratch_bytes=sum(path.stat().st_size for path in work.iterdir()))
            assert state['peak_scratch_bytes'] < 2 * 1024 * 1024
        assert all(digest(path) == expected for path, expected in pins.items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists() if 'work' in locals() else True)
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('VOLATILE_STATE_QUALIFIED', state['cases'], state['checks'])


if __name__ == '__main__':
    main()

# end of test-coreutils-volatile-state.py
