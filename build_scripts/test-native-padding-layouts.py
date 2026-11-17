"""FreeBASIC native layout and key-node regression.
File: test-native-padding-layouts.py
Compile verbatim production type definitions with the actual native headers,
compare semantic field layouts and exercise the production Unix key tree.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
TYPE_FIELDS = {
    'DEV_TCP_INFO': ['hSocket', 'pszDevice', 'timeout', 'is_server', 'is_closed'],
    'FB_PROFILER_GLOBAL': ['profiler_ctx', 'strings', 'strings_hash', 'ignores_hash', 'filename', 'launch_time', 'options'],
    'SER_NODE': ['kind', 'size', 'offset', 'count', 'dims', 'storage', 'child', 'next', 'minimum', 'lower', 'upper'],
    'FB_SIGHANDLER': ['errnum', 'oldhnd'],
    'FormatMaskInfo': ['mask_type', 'has_decimal_point', 'has_thousand_sep', 'has_percent',
        'has_exponent', 'exponent_add_plus', 'has_sign', 'sign_add_plus', 'num_digits_fix',
        'num_digits_frac', 'num_digits_omit', 'exp_digits', 'has_ampm', 'length_min', 'length_opt'],
    'USTR_READER': ['file', 'input', 'pending', 'index_before', 'raw', 'raw_size', 'status'],
    'NODE': ['key', 'code', 'next', 'child'],
    'KEY_DATA': ['cap', 'code'],
    'KeysymToScancode': ['keysym', 'scancode']}
DEFINITION_FILES = {
    'SER_NODE': 'objects/serialization.c', 'FB_SIGHANDLER': 'errors/signals.c',
    'FormatMaskInfo': 'strings/str_format.c', 'USTR_READER': 'strings/unicode/ustr_io.c',
    'NODE': 'drivers/unix/io_inkey.c', 'KEY_DATA': 'drivers/unix/io_inkey.c',
    'KeysymToScancode': 'drivers/unix/scancodes_x11.c'}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def production_definition(native, name):
    text = (native / DEFINITION_FILES[name]).read_text()
    definitions = list(re.finditer(r'typedef\s+struct(?:\s+\w+)?\s*\{[^{}]*\}\s*(\w+)\s*;', text, re.S))
    return next(match[0] for match in definitions if match[1] == name)


def probe_source(native, platform):
    names = list(TYPE_FIELDS)
    if platform == 'windows':
        names = names[:6]
    text = '#include "fb.h"\n#include "fb_profile.h"\n#include "devices/dev_tcp_private.h"\n#include <stddef.h>\n'
    required = 'HOST_WIN32' if platform == 'windows' else 'HOST_UNIX'
    text += '#ifndef ' + required + '\n#error Native platform headers are required\n#endif\n'
    if platform == 'unix':
        text += '#include <X11/Xlib.h>\n'
    enum_text = (native / 'strings/str_format.c').read_text()
    text += re.search(r'typedef\s+enum\s+FB_MASK_TYPE\s*\{[^{}]*\}\s*eMaskType\s*;', enum_text, re.S)[0] + '\n'
    for name in names:
        if name in DEFINITION_FILES:
            text += production_definition(native, name) + '\n'
    text += 'int main(void) {\n'
    for name in names:
        text += 'printf("' + name + ' %zu %zu\\n", sizeof(' + name + '), _Alignof(' + name + '));\n'
        for field in TYPE_FIELDS[name]:
            text += 'printf("' + name + '.' + field + ' %zu %zu\\n", offsetof(' + name + ', ' + field + '), sizeof(((' + name + ' *)0)->' + field + '));\n'
    text += 'return 0;\n}\n'
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--link-cc')
    parser.add_argument('--bits', choices=('32', '64'), default='64')
    parser.add_argument('--optimization', choices=('0', '2'), default='2')
    parser.add_argument('--platform', choices=('unix', 'windows'), default='unix')
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--before-root', type=Path, required=True)
    parser.add_argument('--native-flags-json', type=Path)
    parser.add_argument('--scratch-root', type=Path)
    parser.add_argument('--receipt', type=Path)
    args = parser.parse_args()
    args.source_root = args.source_root.resolve()
    args.before_root = args.before_root.resolve()
    compiler = shutil.which(args.cc)
    linker = shutil.which(args.link_cc or args.cc)
    assert compiler and linker, 'Native compiler and linker drivers must be available'
    args.cc, args.link_cc = compiler, linker
    flags = ['-Wall', '-Wextra', '-Wpedantic', '-Wconversion', '-Wsign-conversion', '-Wpadded']
    if args.native_flags_json:
        flags = json.loads(args.native_flags_json.read_bytes())
    assert not any(flag == '-w' or flag.startswith('-Wno-') for flag in flags)
    files = ['fb.h', 'fb_profile.h', 'devices/dev_tcp_private.h', *sorted(set(DEFINITION_FILES.values()))]
    pins = {str(root / 'src/rtlib' / name): sha(root / 'src/rtlib' / name)
        for root in (args.before_root, args.source_root) for name in files}
    pins[str(Path(__file__).resolve())] = sha(__file__)
    node_source = ROOT / 'tests/rtlib/termcap-key-nodes.c'
    pins[str(node_source)] = sha(node_source)
    rows = []

    def run(command, work):
        environment = dict(os.environ)
        environment['PATH'] = str(Path(command[0]).resolve().parent) + os.pathsep + environment.get('PATH', '')
        process = subprocess.run(list(map(str, command)), cwd=work, env=environment,
            stdin=subprocess.DEVNULL, capture_output=True, text=True)
        result = dict(command=list(map(str, command)), exit=process.returncode,
            output=process.stdout + process.stderr)
        if process.returncode:
            raise RuntimeError(json.dumps(result))
        return result

    with tempfile.TemporaryDirectory(prefix='fbc-native-layouts-', dir=args.scratch_root) as directory:
        work = Path(directory).resolve()
        for cut, root in [('before', args.before_root), ('current', args.source_root)]:
            native = root / 'src/rtlib'
            source, obj, executable = work / 'layout.c', work / 'layout.o', work / 'layout.exe'
            source.write_text(probe_source(native, args.platform))
            common = [args.cc, '-std=gnu11', '-m' + args.bits, '-O' + args.optimization,
                '-DENABLE_MT', '-fno-strict-aliasing', '-fwrapv', *flags, '-I' + str(native)]
            compiled = run([*common, '-c', source, '-o', obj], ROOT)
            linked = run([args.link_cc or args.cc, '-m' + args.bits, obj, '-o', executable], ROOT)
            executed = run([executable], work)
            layout = {}
            for line in executed['output'].splitlines():
                parts = line.split()
                layout[parts[0]] = list(map(int, parts[1:]))
            row = dict(cut=cut, compiled=compiled, linked=linked, executed=executed, layout=layout)
            if args.platform == 'unix':
                node = work / 'nodes.exe'
                node_flags = ['-DFB_TEST_ORIGINAL'] if cut == 'before' else []
                built = run([*common, *node_flags, '-ffunction-sections', '-fdata-sections', node_source,
                    '-Wl,--wrap=malloc', '-Wl,--wrap=calloc', '-Wl,--wrap=free', '-Wl,--wrap=tgetstr',
                    '-Wl,--gc-sections', '-o', node], ROOT)
                observed = run([node], work)
                assert 'TERMCAP_KEY_NODES_PASS' in observed['output']
                row['nodes'] = dict(compiled=built, executed=observed)
            rows.append(row)
        assert rows[0]['layout'] == rows[1]['layout']
        peak = sum(path.stat().st_size for path in work.iterdir() if path.is_file())
        assert peak < 8 * 1024 * 1024
    assert all(sha(path) == digest for path, digest in pins.items())
    result = dict(checks_complete=True, source_pins=pins, rows=rows, layouts_identical=True,
        compared_layout_properties=len(rows[0]['layout']), peak_owned_bytes=peak,
        scratch_directory_removed=not work.exists())
    if args.receipt:
        assert not args.receipt.exists()
        args.receipt.write_text(json.dumps(result, indent=2) + '\n')
    print('NATIVE_PADDING_LAYOUTS_PASS', len(rows[0]['layout']), flush=True)


if __name__ == '__main__':
    main()

# end of test-native-padding-layouts.py
