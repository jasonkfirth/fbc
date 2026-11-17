"""Qualify generated volatile loads and stores at their native boundaries.

File test-c2fb-volatile-loads.py
Own a five-width converter fixture, helper contracts and native value checks.
Complete converter builds and production module census have separate receipts.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = '''
unsigned char audit_read8(unsigned char const *value) {
    unsigned char const volatile *slot = value; return *slot;
}
unsigned short audit_read16(unsigned short const *value) {
    unsigned short const volatile *slot = value; return *slot;
}
unsigned int audit_read32(unsigned int const *value) {
    unsigned int const volatile *slot = value; return *slot;
}
unsigned long long audit_read64(unsigned long long const *value) {
    unsigned long long const volatile *slot = value; return *slot;
}
void *audit_read_pointer(void * const *value) {
    void * const volatile *slot = value; return *slot;
}
void audit_write8(unsigned char *slot, unsigned char value) {
    unsigned char volatile *target = slot; *target = value;
}
void audit_write16(unsigned short *slot, unsigned short value) {
    unsigned short volatile *target = slot; *target = value;
}
void audit_write32(unsigned int *slot, unsigned int value) {
    unsigned int volatile *target = slot; *target = value;
}
void audit_write64(unsigned long long *slot, unsigned long long value) {
    unsigned long long volatile *target = slot; *target = value;
}
void audit_write_pointer(void **slot, void *value) {
    void * volatile *target = slot; *target = value;
}
'''


def run(command, work):
    creationflags = 0
    if os.name == 'nt':
        creationflags = subprocess.CREATE_NO_WINDOW | subprocess.BELOW_NORMAL_PRIORITY_CLASS
    command = list(map(str, command))
    result = subprocess.run(command, cwd=work, stdin=subprocess.DEVNULL,
                            capture_output=True, text=True, creationflags=creationflags)
    output = result.stdout + result.stderr
    row = dict(command=command, exit=result.returncode, output=output,
               warnings=dict(Counter(re.findall(r'warning:.*?\[(-W[^\]]+)\]', output))))
    assert result.returncode == 0, output[-6000:]
    return row


def translate(converter, work):
    source, candidate = work / 'volatile.c', work / 'volatile.bas'
    source.write_text(SOURCE, encoding='utf-8', newline='\n')
    receipt = run([converter, 'translate-simple-generic', source, candidate], work)
    assert 'translated 10 simple functions; skipped 0' in receipt['output'], receipt['output']
    return candidate, receipt


def check_shape(text):
    loads = re.findall(r'^Private Function c2fb_volatile_(?:pointer|scalar)_load_\w+ '
                       r'\(ByVal address As (.*?)\)', text, re.M)
    stores = re.findall(r'^Private Sub c2fb_volatile_(?:pointer|scalar)_store_\w+ '
                        r'\(ByVal address As (.*?),', text, re.M)
    assert len(loads) == len(stores) == 6, (loads, stores)
    assert all(value == 'Const Any Ptr' for value in loads), loads
    assert all(value == 'Any Ptr' for value in stores), stores
    assert len(re.findall(r'\bc2fb_volatile_(?:pointer|scalar)_load_\w+\(CPtr\(Const Any Ptr,', text)) == 5
    assert len(re.findall(r'\bc2fb_volatile_(?:pointer|scalar)_store_\w+\(CPtr\(Any Ptr,', text)) == 1


def check_native(module, compiler, flags, optimization, scratch):
    support = ROOT / 'tests/compiler-support/native_c_support.py'
    native_command = runpy.run_path(str(support))['native_command']
    observer = ROOT / 'tests/compiler-support/c2fb-volatile-loads.c'
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    flags = [flag for flag in flags if not re.fullmatch(r'-O\w+', flag)]
    flags += ['-O' + optimization]
    header_flags = [flag for flag in flags if flag != '-nostdinc' and not flag.startswith('-masm=')]
    result = dict(commands=[], peak_scratch_bytes=0)
    with tempfile.TemporaryDirectory(prefix='c2fb-volatile-loads-', dir=scratch) as directory:
        work = Path(directory)
        module_object, observer_object, binary = work / 'module.o', work / 'observer.o', work / 'values'
        for command in ([compiler, *flags, '-c', module, '-o', module_object],
                        [compiler, *header_flags, '-c', observer, '-o', observer_object],
                        [compiler, module_object, observer_object, '-o', binary]):
            result['commands'].append(run(native_command(command), work))
        executed = run([binary], work)
        result['execution'] = executed
        match = re.fullmatch(r'VOLATILE_LOAD_PASS cases=(\d+) checks=(\d+)\s*', executed['output'])
        assert match, executed['output']
        result.update(cases=int(match[1]), checks=int(match[2]))
        result['peak_scratch_bytes'] = sum(path.stat().st_size for path in work.iterdir())
        assert result['peak_scratch_bytes'] < 4 * 1024 * 1024
    result['scratch_directory_removed'] = not work.exists()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--module-c', type=Path, required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--native-flags-json', type=Path, required=True)
    parser.add_argument('--optimization', choices=('0', '2'), required=True)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    digest = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
    paths = (args.module_c, Path(__file__), ROOT / 'tests/compiler-support/c2fb-volatile-loads.c',
             ROOT / 'tests/compiler-support/native_c_support.py')
    pins = {str(path.resolve()): digest(path) for path in paths}
    result = check_native(args.module_c, args.cc, json.loads(args.native_flags_json.read_bytes()),
                          args.optimization, args.scratch_root)
    assert all(digest(path) == expected for path, expected in pins.items())
    args.receipt.write_text(json.dumps(dict(checks_complete=True, source_pins=pins, native=result),
                                     separators=(',', ':')))
    print('VOLATILE_LOAD_QUALIFIED', result['cases'], result['checks'])


if __name__ == '__main__':
    main()

# end of test-c2fb-volatile-loads.py
