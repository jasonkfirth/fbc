"""Exercise the current VT100 C name with legacy declarations and overrides.

File test-vt100-symbol.py
Compile separate current and legacy clients against a real or minimal archive.
Terminal output and device initialization retain their existing runtime tests.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--cc', required=True)
    parser.add_argument('--ar', default='ar')
    parser.add_argument('--bits', choices=('32', '64'), required=True)
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--language', choices=('c', 'c++'), default='c')
    parser.add_argument('--before', action='store_true')
    parser.add_argument('--mt', action='store_true')
    parser.add_argument('--runtime', type=Path)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    root = args.root.resolve()
    cc, ar = shutil.which(args.cc), shutil.which(args.ar)
    assert cc and ar and not args.receipt.exists()
    sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
    current = root / 'tests/rtlib/vt100-symbol-current.c'
    legacy = root / 'tests/rtlib/vt100-symbol-legacy.c'
    default = root / 'src/rtlib/drivers/unix/hinit_enable_vt100_escapes.c'
    inputs = [current, legacy, default, root / 'src/rtlib/drivers/unix/fb_unix.h', root / 'src/rtlib/fb_abi.h']
    if args.runtime: inputs.append(args.runtime)
    state = dict(checks_complete=False, input_pins={str(path): sha(path) for path in inputs},
        commands=[], executions=[], peak_owned_bytes=0, before=args.before, language=args.language)
    tool = subprocess.run([cc, '--version'], capture_output=True, text=True, check=True).stdout.lower()
    common = ['-m' + args.bits, '-O' + args.optimization, '-fno-strict-aliasing', '-fwrapv',
        '-I', str(root / 'src/rtlib')]
    if args.mt: common += ['-DENABLE_MT']
    warnings = ['-Weverything'] if 'clang' in tool else ['-Wall', '-Wextra', '-Wpedantic',
        '-Wconversion', '-Wsign-conversion', '-Wcast-qual', '-Wmissing-declarations']

    def run(command):
        result = subprocess.run(list(map(str, command)), cwd=work,
            stdin=subprocess.DEVNULL, capture_output=True, text=True)
        output = result.stdout + result.stderr
        row = dict(command=list(map(str, command)), exit=result.returncode, output=output,
            warnings=re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))
        state['commands'].append(row)
        assert result.returncode == 0, output[-5000:]
        state['peak_owned_bytes'] = max(state['peak_owned_bytes'],
            sum(path.stat().st_size for path in work.iterdir() if path.is_file()))
        assert state['peak_owned_bytes'] < 4 * 1024 * 1024
        return row

    try:
        with tempfile.TemporaryDirectory(prefix='vt100-symbol-', dir=args.scratch_root) as directory:
            work = Path(directory)
            archive = args.runtime
            if archive is None:
                archive = work / 'runtime.a'
                run([cc, *common, *warnings, '-std=gnu11', '-c', default, '-o', work / 'default.o'])
                run([ar, 'rcs', archive, work / 'default.o'])
            for override in (False, True):
                defines = ['-DVT100_TEST_OVERRIDE'] if override else []
                run([cc, *common, *warnings, '-std=gnu11', *defines, '-x', 'c',
                    '-c', legacy, '-o', work / 'legacy.o'])
                current_flags = common + warnings + defines
                if args.before: current_flags += ['-Dfb_enable_vt100_escapes=__fb_enable_vt100_escapes']
                current_flags += ['-std=gnu++17' if args.language == 'c++' else '-std=gnu11', '-x', args.language]
                run([cc, *current_flags, '-c', current, '-o', work / 'current.o'])
                run([cc, '-m' + args.bits, work / 'current.o', work / 'legacy.o', archive, '-o', work / 'probe'])
                executed = run([work / 'probe'])
                assert executed['output'].strip() == 'VT100_SYMBOL_PASS'
                state['executions'].append(dict(override=override, result=executed))
        assert all(sha(path) == pin for path, pin in state['input_pins'].items())
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists() if 'work' in locals() else True)
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('VT100_SYMBOL_QUALIFIED', args.bits, args.language, 'executions', len(state['executions']))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-vt100-symbol.py
