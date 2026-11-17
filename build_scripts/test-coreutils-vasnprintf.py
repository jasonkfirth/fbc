"""Project: FreeBASIC Coreutils formatter regression
File: test-coreutils-vasnprintf.py
Purpose: Qualify all typed formatter calls and real provider integration.
Responsibilities: Run bounded value/cleanup tests and optional GCC line coverage.
The caller supplies emitted production C and its native warning options.
"""
import argparse
from collections import Counter
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def split_arguments(text):
    result, begin, depth, quoted, escaped = [], 0, 0, None, False
    for position, char in enumerate(text):
        if quoted:
            if escaped:
                escaped = False
            elif char == '\\':
                escaped = True
            elif char == quoted:
                quoted = None
        elif char in ('"', "'"):
            quoted = char
        elif char in '([{':
            depth += 1
        elif char in ')]}':
            depth -= 1
        elif char == ',' and depth == 0:
            result.append(text[begin:position].strip())
            begin = position + 1
    result.append(text[begin:].strip())
    assert depth == 0 and quoted is None
    return result


def contracts(source):
    # The source's 33 argument categories each dispatch zero, one or two
    # leading integer star operands before the value and final count pointer.
    # gnulib deliberately holds its uchar/ushort branches in unsigned int
    # locals. Check those declared types with %u. The execution matrix checks
    # their narrowed %hhu/%hu formatting and representable argument values.
    conversions = ['hhd', 'u', 'hd', 'u', 'd', 'u', 'ld', 'lu', 'lld', 'llu',
        'hhd', 'hhu', 'hd', 'hu', 'd', 'u', 'ld', 'lu', 'hhd', 'hhu',
        'ld', 'lu', 'ld', 'lu', 'ld', 'lu', 'g', 'Lg', 'c', 'lc', 's', 'ls', 'p']
    rewritten, sites = [], []
    for line_number, line in enumerate(source.splitlines(), 1):
        match = re.search(r'\bsnprintf\(\s*(.*)\s*\);\s*$', line)
        if match and '=' in line[:match.start()]:
            index = len(sites)
            assert index < 99
            arguments = split_arguments(match[1])
            stars = index % 3
            assert len(arguments) == 5 + stars, (index, len(arguments))
            literal = '%d' * stars + '%' + conversions[index // 3] + '%n'
            arguments[2] = json.dumps(literal)
            rewritten.append('if (0) (void)snprintf(' + ', '.join(arguments) + ');')
            sites.append(dict(line=line_number, category=index // 3 + 1, stars=stars,
                literal=literal, probe_line=len(rewritten)))
        rewritten.append(line)
    assert len(sites) == 99, len(sites)
    return '\n'.join(rewritten) + '\n', sites


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module-dir', type=Path, required=True)
    parser.add_argument('--rtlib-dir', type=Path, required=True)
    parser.add_argument('--observer', type=Path,
        default=Path(__file__).resolve().parents[1] / 'tests/compiler-support/coreutils-vasnprintf.c')
    parser.add_argument('--cc', default=os.environ.get('GCC') or 'gcc')
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--optimization', choices=('0', '2'), default='2')
    parser.add_argument('--coverage', action='store_true')
    parser.add_argument('--gcov', default='gcov')
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    names = ('vasnprintf', 'printf-args', 'printf-parse', 'realloc', 'xsize', 'mbszero')
    sources = {name: args.module_dir / (name + '.c') for name in names}
    support = args.rtlib_dir / 'math/longdouble_ops.c'
    paths = [*sources.values(), support, support.with_name('extended_math.h'),
        support.with_name('longdouble_parse.h'), args.observer, Path(__file__)]
    pins = {str(path.resolve()): sha(path) for path in paths}
    flags = [flag for flag in json.loads(args.native_flags_json.read_text()) if not flag.startswith('-O')]
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    state = dict(checks_complete=False, source_pins=pins, commands=[],
        optimization=args.optimization, coverage=args.coverage, peak_scratch_bytes=0)

    def run(command, work, accepted=True):
        result = subprocess.run(list(map(str, command)), cwd=work, stdin=subprocess.DEVNULL,
            capture_output=True, text=True)
        output = result.stdout + result.stderr
        row = dict(command=list(map(str, command)), exit=result.returncode, output=output,
            warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
        state['commands'].append(row)
        state['peak_scratch_bytes'] = max(state['peak_scratch_bytes'],
            sum(path.stat().st_size for path in work.rglob('*') if path.is_file()))
        assert state['peak_scratch_bytes'] < 32 * 1024 * 1024
        if accepted:
            assert result.returncode == 0, output[-8000:]
        return row

    try:
        with tempfile.TemporaryDirectory(prefix='fbc-vasnprintf-', dir=args.scratch_root) as directory:
            work = Path(directory)
            objects, builds = [], []
            coverage = ['--coverage'] if args.coverage else []
            for name, source in sources.items():
                local = work / (name + '.c')
                local.write_bytes(source.read_bytes())
                obj = work / (name + '.o')
                result = run([args.cc, *flags, '-O' + args.optimization,
                    '-ffunction-sections', '-fdata-sections', *coverage, '-c', local, '-o', obj], work)
                objects.append(obj)
                builds.append(dict(module=name, result=result))
            observer_flags = [flag for flag in flags if flag != '-nostdinc']
            for name, source in (('longdouble', support), ('observer', args.observer)):
                obj = work / (name + '.o')
                run([args.cc, *observer_flags, '-O' + args.optimization,
                    '-ffunction-sections', '-fdata-sections', '-c', source, '-o', obj], work)
                objects.append(obj)
            executable = work / 'values'
            linked = run([args.cc, *objects, *coverage, '-Wl,--gc-sections',
                '-Wl,--wrap=malloc', '-Wl,--wrap=realloc', '-Wl,--wrap=free', '-lm', '-o', executable], work)
            executed = run([executable], work)
            match = re.search(r'VASNPRINTF_MATRIX_PASS cases=(\d+) checks=(\d+) allocation_failures=(\d+) recoveries=(\d+) fatal_allocations=(\d+)', executed['output'])
            assert match, executed
            cases, checks, failures, recoveries, fatal_allocations = map(int, match.groups())
            rewritten, sites = contracts(sources['vasnprintf'].read_text())
            checked_source = work / 'type-contracts.c'
            checked_source.write_text(rewritten)
            checked = run([args.cc, *flags, '-O2', '-c', checked_source, '-o', work / 'type-contracts.o'], work)
            assert not any(checked['warnings'].get(key) for key in ('-Wformat=', '-Wformat', '-Wformat-signedness')), checked
            # Both controls keep the actual production calls intact. Only the
            # newly inserted, unevaluated canonical type contract is changed.
            controls = []
            for old, replacement in (('%hhd%n', '%s%n'), ('%lu%n', '%llu%n')):
                wrong = rewritten.replace(json.dumps(old), json.dumps(replacement), 1)
                assert wrong != rewritten
                checked_source.write_text(wrong)
                result = run([args.cc, *flags, '-O2', '-c', checked_source, '-o', work / 'type-contracts.o'], work, False)
                assert re.search(r'\[-W(?:error=|error,-W)?format(?:=)?\]', result['output']), result
                controls.append(result)
            covered, bypassed = [], []
            if args.coverage:
                result = run([args.gcov, '--json-format', work / 'vasnprintf.gcda'], work)
                reports = list(work.glob('*.gcov.json.gz'))
                assert reports, result
                line_counts = {}
                for report in reports:
                    data = json.loads(gzip.decompress(report.read_bytes()))
                    for file in data['files']:
                        if Path(file['file']).name == 'vasnprintf.c':
                            line_counts.update({row['line_number']: row['count'] for row in file['lines']})
                covered = [site['line'] for site in sites if line_counts.get(site['line'], 0) > 0]
                # Narrow and wide strings take the formatter's earlier string
                # handlers. The generic switch retains six fallback calls.
                # Keep their type contracts, and report their absent execution.
                bypassed = [site for site in sites if site['line'] not in covered]
                assert len(covered) == 93 and len(bypassed) == 6, (len(covered), bypassed)
                assert {(site['category'], site['stars']) for site in bypassed} == {
                    (category, stars) for category in (31, 32) for stars in range(3)}
            state.update(checks_complete=True, builds=builds, link=linked, execution=executed,
                cases=cases, checks=checks, allocation_failure_controls=failures,
                allocation_recoveries=recoveries, static_type_contracts=len(sites),
                fatal_automatic_storage_allocations=fatal_allocations,
                static_type_check=checked, retained_wrong_type_controls=controls,
                covered_native_format_calls=len(covered), format_sites=sites)
            state['bypassed_string_fallback_calls'] = bypassed
        assert all(sha(path) == pin for path, pin in pins.items())
        state['scratch_directory_removed'] = not work.exists()
    except BaseException as error:
        state['checks_complete'] = False
        state['failure'] = repr(error)
        raise
    finally:
        args.receipt.write_text(json.dumps(state, separators=(',', ':')))
    print('VASNPRINTF_CONTRACTS_PASS', cases, checks, len(covered))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-coreutils-vasnprintf.py
