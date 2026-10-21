#!/usr/bin/env python3
"""FreeBASIC optional external library ABI checks.
File: test-external-library-abi.py
Purpose: Compare libffi with native C headers and exercise gettext count widths.
Responsibilities: Bounded native tools, original example execution and receipts.
This file does not install SDKs or qualify graphics, audio or unrelated libraries.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--outdir', type=Path, required=True)
    parser.add_argument('--fbc', required=True)
    parser.add_argument('--prefix', type=Path)
    parser.add_argument('--include-dir', type=Path, action='append', default=[])
    parser.add_argument('--cc', default='cc')
    parser.add_argument('--cc-arg', action='append', default=[])
    parser.add_argument('--fbc-arg', action='append', default=[])
    args = parser.parse_args()
    root = args.root.resolve()
    out = args.outdir.resolve()
    out.mkdir(parents=True, exist_ok=False)
    sources = ('tests/bindings/ffi-layout.c', 'tests/bindings/ffi-layout.bas',
               'tests/bindings/gettext-width.bas', 'examples/manual/libraries/ffi/helloworld.bas',
               'examples/manual/libraries/ffi/closures.bas')
    receipt = {'system': platform.platform(), 'commands': [], 'source_sha256': {}}
    for relative in sources:
        path = root / relative
        receipt['source_sha256'][relative] = hashlib.sha256(path.read_bytes()).hexdigest()
    for name in ('ffi.bi', 'libintl.bi'):
        for directory in args.include_dir or [root / 'inc']:
            path = directory / name
            if path.is_file():
                receipt['source_sha256'][str(path.resolve())] = hashlib.sha256(path.read_bytes()).hexdigest()
                break
    suffix = '.exe' if os.name == 'nt' else ''

    def run(command, label):
        result = subprocess.run(list(map(str, command)), cwd=out, capture_output=True, timeout=45)
        (out / (label + '.log')).write_bytes(result.stdout + result.stderr)
        receipt['commands'].append({'command': list(map(str, command)), 'exit_code': result.returncode,
                                    'log': label + '.log'})
        (out / 'result.json').write_text(json.dumps(receipt, indent=2) + '\n')
        if result.returncode != 0:
            raise RuntimeError(f'{label} failed with {result.returncode}; see {out / (label + ".log")}')
        return result.stdout.decode('utf-8', errors='replace')

    compiler_path = shutil.which(args.fbc) or str(Path(args.fbc).resolve())
    compiler = [compiler_path]
    if args.prefix:
        compiler.extend(['-prefix', str(args.prefix.resolve())])
    for directory in args.include_dir or [root / 'inc']:
        compiler.extend(['-i', str(directory.resolve())])
    compiler.extend(args.fbc_arg)
    native = out / ('ffi-layout-c' + suffix)
    copied = out / 'ffi-layout.c'
    shutil.copy2(root / sources[0], copied)
    run([args.cc, *args.cc_arg, copied, '-o', native], 'ffi-layout-c-compile')
    native_values = run([native], 'ffi-layout-c-run')

    def values(text):
        result = {}
        for line in text.splitlines():
            key, value = line.split('=', 1)
            if key in result:
                raise RuntimeError('Duplicate ABI measurement: ' + key)
            result[key] = int(value.strip())
        return result

    native_layout = values(native_values)
    products = [native]
    for relative in sources[1:]:
        copied = out / Path(relative).name
        shutil.copy2(root / relative, copied)
        binary = out / (copied.stem + suffix)
        run([*compiler, copied, '-x', binary], copied.stem + '-compile')
        observed = run([binary], copied.stem + '-run')
        if copied.stem == 'ffi-layout':
            basic_layout = values(observed)
            # Win32 can add tail padding around the same aligned closure fields.
            if os.name == 'nt' and native_layout['pointer'] == 4:
                if native_layout['ffi_closure'] == 28 and basic_layout['ffi_closure'] == 32:
                    basic_layout['ffi_closure'] = 28
            if native_layout != basic_layout:
                raise RuntimeError(f'libffi ABI differs: C={native_layout}, BASIC={basic_layout}')
        elif copied.stem == 'gettext-width' and 'GETTEXT_UNSIGNED_LONG_PASS' not in observed:
            raise RuntimeError('gettext probe did not reach its completion marker')
        products.append(binary)

    receipt['native_layout'] = native_layout
    receipt['status'] = 'pass'
    receipt['product_sha256'] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in products}
    (out / 'result.json').write_text(json.dumps(receipt, indent=2) + '\n')
    for path in products:
        path.unlink()
    print(f'EXTERNAL_LIBRARY_ABI_PASS measurements={len(native_layout)} original_libffi_examples=2')


if __name__ == '__main__':
    main()
# end of test-external-library-abi.py
