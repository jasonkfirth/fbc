"""Run the existing direct parser vectors or check the actual runtime entry.

File test-longdouble-parser.py
Keep parser selection, storage ABI, diagnostics and exact input versions visible.
Native operating-system integration retains its target-specific build context.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def entry_source():
    source = r'''
/* FreeBASIC runtime parser entry qualification, generated entry.c. */
#include <stdio.h>
#include OPS_SOURCE
#include PARSER_HEADER
static unsigned long parser_entry_calls;
static long double checked_parser_entry(const char* text, size_t* consumed)
{
    long double parsed = fb_hParseLongDouble(text, consumed);
    if (*consumed != 0 && text[*consumed] == 0) {
        long double stored;
        parser_entry_calls++;
        fb_LongDoubleFromString(&stored, text, (unsigned int)sizeof(stored));
        long double value = fb_LongDoubleToNative(&stored, (unsigned int)sizeof(stored));
        if (!(isnan(value) && isnan(parsed)) &&
            (value != parsed || (value == 0 && !!signbit(value) != !!signbit(parsed)))) {
            fprintf(stderr, "PARSER_ENTRY_FAIL input=%.96s actual=%La expected=%La\n", text, value, parsed);
            exit(1);
        }
    }
    return parsed;
}
int parser_reference_main(void);
#define fb_hParseLongDouble checked_parser_entry
#define main parser_reference_main
#include FIXTURE_SOURCE
#undef main
#undef fb_hParseLongDouble
int main(void)
{
    int result = parser_reference_main();
    if (result != 0) return result;
#ifdef FB_LONGDOUBLE_LOCAL_PARSER
    int selected = 1;
#else
    int selected = 0;
#endif
    printf("LONGDOUBLE_ENTRY_PASS calls=%lu local=%d bytes=%zu mantissa=%d\n",
        parser_entry_calls, selected, sizeof(long double), LDBL_MANT_DIG);
    return parser_entry_calls == 0;
}
/* end of generated entry.c */
'''
    for marker, name in (('OPS_SOURCE', 'src/rtlib/math/longdouble_ops.c'),
        ('PARSER_HEADER', 'src/rtlib/math/longdouble_parse.h'),
        ('FIXTURE_SOURCE', 'tests/rtlib/longdouble-parse.c')):
        source = source.replace(marker, json.dumps((ROOT / name).as_posix()))
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--mode', choices=('direct', 'runtime-entry'), required=True)
    parser.add_argument('--bits', choices=('32', '64'), required=True)
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    compiler = shutil.which(args.cc)
    assert compiler
    fixture = ROOT / 'tests/rtlib/longdouble-parse.c'
    inputs = [Path(__file__), fixture, ROOT / 'src/rtlib/math/longdouble_parse.h',
        ROOT / 'src/rtlib/math/longdouble_ops.c', ROOT / 'src/rtlib/math/extended_math.h']
    pins = {str(path): sha(path) for path in inputs}
    flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    # The runtime uses the platform C assembler dialect. Emitted BASIC inline
    # assembly selects Intel syntax separately from these native C headers.
    flags = [flag for flag in flags if flag != '-nostdinc' and not flag.startswith(('-std=', '-O', '-masm='))]
    flags += ['-std=gnu11', '-m' + args.bits, '-O' + args.optimization]
    state = dict(checks_complete=False, mode=args.mode, bits=args.bits, optimization=args.optimization,
        source_pins=pins, compiler_sha256=sha(compiler), commands=[], peak_owned_bytes=0,
        native_library_assembly_mode='platform-default')

    def run(command, work):
        options = {}
        if os.name == 'nt':
            options['creationflags'] = subprocess.CREATE_NO_WINDOW | subprocess.BELOW_NORMAL_PRIORITY_CLASS
        result = subprocess.run(list(map(str, command)), cwd=work, stdin=subprocess.DEVNULL,
            capture_output=True, text=True, **options)
        output = result.stdout + result.stderr
        assert len(output.encode()) < 4 * 1024 * 1024
        row = dict(command=list(map(str, command)), exit=result.returncode, output=output,
            warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        state['peak_owned_bytes'] = max(state['peak_owned_bytes'],
            sum(path.stat().st_size for path in work.iterdir() if path.is_file()))
        assert state['peak_owned_bytes'] < 8 * 1024 * 1024
        assert result.returncode == 0, output[-4000:]
        return row

    assert not args.receipt.exists()
    try:
        with tempfile.TemporaryDirectory(prefix='longdouble-parser-', dir=args.scratch_root) as directory:
            work = Path(directory)
            source = fixture
            if args.mode == 'runtime-entry':
                source = work / 'entry.c'
                source.write_text(entry_source())
            executable = work / ('values.exe' if os.name == 'nt' else 'values')
            run([compiler, *flags, source, '-lm', '-o', executable], work)
            result = run([executable], work)
            match = re.search(r'^LONGDOUBLE_PARSE_PASS checks=(\d+)$', result['output'], re.M)
            assert match
            state['checks'] = int(match[1])
            if args.mode == 'runtime-entry':
                match = re.search(r'^LONGDOUBLE_ENTRY_PASS calls=(\d+) local=([01]) bytes=(\d+) mantissa=(\d+)$',
                    result['output'], re.M)
                assert match and int(match[1]) > 0
                state.update(entry_calls=int(match[1]), local_parser_selected=bool(int(match[2])),
                    storage_bytes=int(match[3]), mantissa_bits=int(match[4]))
        assert all(sha(path) == pin for path, pin in pins.items())
        assert sha(compiler) == state['compiler_sha256']
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists())
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('LONGDOUBLE_PARSER_QUALIFIED', args.mode, args.bits, args.optimization, state['checks'])
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-longdouble-parser.py
