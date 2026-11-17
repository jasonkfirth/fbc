"""Exercise complete emitted SQLite functions with controlled dependencies.

File test-sqlite-insertion-phases.py
Retain canonical emitted types, layout assertions and function bodies. The C
observer supplies deterministic dependencies and checks phase transitions.
Whole-module diagnostics and whole-engine SQL execution have separate scopes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile


def declarations(prefix):
    """Split top-level declarations while preserving nested records and strings."""
    start = 0
    depth = 0
    quote = None
    escaped = False
    for position, char in enumerate(prefix):
        if quote:
            if escaped:
                escaped = False
            elif char == '\\':
                escaped = True
            elif char == quote:
                quote = None
            continue
        if char in ('"', "'"):
            quote = char
        elif char == '{':
            depth += 1
        elif char == '}':
            depth -= 1
        elif char == ';' and depth == 0:
            yield prefix[start:position + 1].strip()
            start = position + 1
    assert not prefix[start:].strip()


def extract(text, function):
    definitions = list(re.finditer(r'(?m)^[^\n;{}]+\([^\n]*\)\n\{', text))
    selected = [match for match in definitions
                if re.search(r'\b' + re.escape(function) + r'\(', match[0])]
    assert len(selected) == 1
    match = selected[0]
    end = text.index('\n}\n', match.end()) + 3
    body = text[match.start():end]
    calls = set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', body))
    units = []
    for unit in declarations(text[:definitions[0].start()]):
        if unit.startswith(('typedef ', 'struct ', 'union ', 'enum ')):
            units.append(unit)
        elif any(re.search(r'\b' + re.escape(name) + r'\s*\(', unit) for name in calls):
            units.append(unit)
        elif unit.startswith('static const char '):
            name = re.search(r'static const char (\w+)', unit)[1]
            if re.search(r'\b' + re.escape(name) + r'\b', body):
                units.append(unit)
    return '\n'.join(units) + '\n', body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module', type=Path, required=True)
    parser.add_argument('--observer', type=Path, required=True)
    parser.add_argument('--function', choices=('sqlite3BtreeInsert', 'generateWithRecursiveQuery'), required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--scratch-root', type=Path, required=True)
    parser.add_argument('--receipt', type=Path, required=True)
    parser.add_argument('--missing-anchor', action='store_true')
    args = parser.parse_args()
    sha = lambda raw: hashlib.sha256(raw).hexdigest()
    raw = args.module.read_bytes()
    prefix, body = extract(raw.decode().replace('\r\n', '\n').replace('\r', ''), args.function)
    observer = args.observer.read_bytes()
    flags = json.loads(args.native_flags_json.read_text())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    # The observer uses the host C library. Emitted modules are freestanding
    # with respect to C headers, so their recipe otherwise uses -nostdinc.
    flags = [flag for flag in flags if flag != '-nostdinc' and not re.fullmatch(r'-O\w+', flag)]
    state = dict(checks_complete=False, module_sha256=sha(raw), observer_sha256=sha(observer),
                 prefix_sha256=sha(prefix.encode()), body_sha256=sha(body.encode()),
                 peak_scratch_bytes=0)
    try:
        with tempfile.TemporaryDirectory(prefix='sqlite-insertion-phases-', dir=args.scratch_root) as directory:
            work = Path(directory)
            source, binary = work / 'phases.c', work / 'phases'
            # The observer sees the exact production body and canonical types
            # in one translation unit, including private helper signatures.
            source.write_bytes((prefix + '\n' + body + '\n').encode() + observer)
            command = [args.cc, *flags, '-O' + args.optimization, str(source), '-o', str(binary)]
            if args.missing_anchor:
                command.insert(1, '-DTEST_MISSING_ANCHOR=1')
            result = subprocess.run(command, stdin=subprocess.DEVNULL, capture_output=True, text=True)
            state['compilation'] = dict(command=command, exit=result.returncode, output=result.stdout + result.stderr)
            assert result.returncode == 0, state['compilation']['output'][-6000:]
            result = subprocess.run([str(binary)], stdin=subprocess.DEVNULL, capture_output=True, text=True)
            state['execution'] = dict(exit=result.returncode, output=result.stdout + result.stderr)
            assert result.returncode == 0, state['execution']['output'][-6000:]
            state['cases'] = len(re.findall(r'(?m)^CASE ', result.stdout))
            assert state['cases'] > 0
            state['peak_scratch_bytes'] = sum(path.stat().st_size for path in work.iterdir())
            assert state['peak_scratch_bytes'] < 4 * 1024 * 1024
        state.update(checks_complete=True, scratch_directory_removed=not work.exists())
    except BaseException as error:
        state.update(failure=repr(error), scratch_directory_removed=not work.exists() if 'work' in locals() else True)
        raise
    finally:
        args.receipt.write_text(json.dumps(state, indent=2), encoding='utf-8', newline='\n')
    print('SQLITE_INSERTION_PHASES', args.function, state['cases'])


if __name__ == '__main__':
    main()

# end of test-sqlite-insertion-phases.py
