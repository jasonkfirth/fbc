#!/usr/bin/env python3
"""FreeBASIC PDFlib binding qualification
File: test-pdflib.py
Purpose: Compare real PDFlib C and FreeBASIC document generation.
Responsibilities: ABI facts, callback ownership and independently parsed PDFs.
This file contains no compiler build, SDK substitute or installed-tool mutation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--sdk-prefix', type=Path, required=True)
    parser.add_argument('--tool-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--pypdf-dir', type=Path)
    parser.add_argument('--target', choices=('native', 'win32'), default='native')
    parser.add_argument('--runtime-prefix', type=Path)
    parser.add_argument('--renderer-tool-dir', type=Path,
                        help='Dependency bin directory matching the Poppler executable architecture')
    parser.add_argument('--poppler-bin', type=Path, required=True,
                        help='Native Poppler bin directory for independent PDF rendering')
    args = parser.parse_args()
    if args.target == 'win32' and (os.name != 'nt' or args.runtime_prefix is None):
        parser.error('win32 qualification requires Windows and --runtime-prefix')
    root = Path(__file__).resolve().parents[1]
    sdk, tools, output = args.sdk_prefix.resolve(), args.tool_dir.resolve(), args.output.resolve()
    compiler = args.fbc.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if args.pypdf_dir:
        sys.path.insert(0, str(args.pypdf_dir.resolve()))
    from pypdf import PdfReader
    import pypdf
    environment = os.environ.copy()
    environment['PATH'] = str(tools) + os.pathsep + environment['PATH']
    environment['PATH'] = str(args.poppler_bin.resolve()) + os.pathsep + environment['PATH']
    renderer_environment = environment.copy()
    renderer_tools = args.renderer_tool_dir.resolve() if args.renderer_tool_dir else tools
    renderer_environment['PATH'] = str(args.poppler_bin.resolve()) + os.pathsep + str(renderer_tools) + os.pathsep + os.environ['PATH']
    suffix = '.exe' if os.name == 'nt' else ''
    for name in ('gcc', 'as', 'ld', 'ar'):
        environment[name.upper()] = str(tools / (name + suffix))

    def run(arguments, tag, selected_environment=None):
        result = subprocess.run([str(value) for value in arguments], cwd=output,
                                env=selected_environment or environment,
                                capture_output=True, text=True, timeout=60)
        (output / (tag + '.log')).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(tag + ' exit ' + str(result.returncode) + '\n' + result.stdout + result.stderr)
        return result.stdout

    c_binary = output / ('pdflib-c' + suffix)
    link = ['-L', sdk / 'lib', '-lpdf', '-ltiff', '-lpng', '-lz']
    if os.name == 'nt':
        link += ['-lgdi32']
    else:
        link += ['-lm']
    run([tools / ('gcc' + suffix), '-Wall', '-Wextra', '-Werror', '-I', sdk / 'include',
         root / 'tests/bindings/pdflib-native.c', *link, '-o', c_binary], 'compile-c')
    native_abi = run([c_binary], 'abi-c').strip()
    logo = root / 'examples/fblogo.gif'
    dimensions = [int.from_bytes(logo.read_bytes()[index:index + 2], 'little') for index in (6, 8)]

    def inspect_pdf(path):
        reader = PdfReader(path, strict=True)
        assert len(reader.pages) == 1, path
        page = reader.pages[0]
        assert list(page.mediabox) == [0, 0, 320, 240], path
        text = page.extract_text().strip()
        assert text == 'PDFlib native ABI', (path, text)
        images = [value.get_object() for value in page['/Resources']['/XObject'].values()]
        assert len(images) == 1, path
        image = images[0]
        assert image['/Subtype'] == '/Image', path
        assert [image['/Width'], image['/Height']] == dimensions, path
        # PDFlib's GIF passthrough uses LZW EarlyChange=0. pypdf 6.20 ignores
        # that decode parameter, so compare encoded images here and let the
        # independent Poppler renderer check their actual decoded pixels.
        image_bytes = image._data
        assert image_bytes, path
        operations = page.get_contents().operations
        assert any(operator == b're' for _, operator in operations), path
        raster_stem = path.with_suffix('')
        renderer = args.poppler_bin.resolve() / ('pdftoppm' + suffix)
        run([renderer, '-singlefile', '-r', '72', path, raster_stem], path.stem + '-render', renderer_environment)
        raster = raster_stem.with_suffix('.ppm').read_bytes()
        # Poppler's native RGB output has one pixel per PDF point at 72 DPI.
        header, pixels = raster.split(b'\n255\n', 1)
        assert b'320 240' in header and len(pixels) == 320 * 240 * 3, path
        assert min(pixels) < 128 and max(pixels) == 255, path
        return {'text': text, 'image_sha256': hashlib.sha256(image_bytes).hexdigest(),
                'operations': repr(operations), 'image_dimensions': dimensions,
                'render_sha256': hashlib.sha256(pixels).hexdigest()}

    references = {}
    records = []
    for mode in (0, 1, 2):
        path = output / ('c-' + str(mode) + '.pdf')
        observed = run([c_binary, path, logo, mode], 'c-document-' + str(mode))
        assert 'PDFLIB_DOCUMENT=PASS' in observed, observed
        references[mode] = inspect_pdf(path)
        records.append({'language': 'C', 'mode': mode, 'document': path.name, 'status': 'pass'})
    print('PDFLIB_NATIVE_C=PASS documents=3', flush=True)
    backends = ('gcc', 'gas' if args.target == 'win32' else 'gas64')
    for backend in backends:
        binary = output / ('pdflib-' + backend + suffix)
        flags = [compiler, '-prefix', root, '-i', root / 'inc', '-p', sdk / 'lib',
                 '-gen', backend, '-w', 'all']
        if os.name == 'nt':
            flags += ['-p', tools.parent / 'lib']
        if args.target == 'win32':
            flags += ['-target', 'win32', '-prefix', args.runtime_prefix.resolve(),
                      '-buildprefix', str(tools) + '/']
        run([*flags, root / 'tests/bindings/pdflib-native.bas', '-x', binary], 'compile-' + backend)
        assert run([binary], 'abi-' + backend).strip() == native_abi, backend
        for mode in (0, 1, 2):
            path = output / (backend + '-' + str(mode) + '.pdf')
            observed = run([binary, path, logo, mode], backend + '-document-' + str(mode))
            assert 'PDFLIB_DOCUMENT=PASS' in observed, observed
            assert inspect_pdf(path) == references[mode], (backend, mode)
            records.append({'language': 'FreeBASIC', 'backend': backend, 'mode': mode,
                            'document': path.name, 'status': 'pass'})
        print('PDFLIB_BINDING=PASS ' + backend + ' documents=3 callbacks_owned_blocks=0', flush=True)
        example = output / ('example-' + backend + suffix)
        run([*flags, root / 'examples/files/pdflib/test.bas', '-x', example], 'compile-example-' + backend)
        environment['FB_EXAMPLE_AUTOPLAY'] = '1'
        for has_logo in (True, False):
            path = output / ('example-' + backend + ('-logo' if has_logo else '-missing-logo') + '.pdf')
            selected_logo = logo if has_logo else output / 'absent-logo.gif'
            observed = run([example, path, selected_logo], path.stem)
            assert 'PDF document completed:' in observed, observed
            reader = PdfReader(path, strict=True)
            assert len(reader.pages) == 1, path
            page = reader.pages[0]
            assert list(page.mediabox) == [0, 0, 595, 842], path
            text = page.extract_text()
            for expected in ('horizontal text in Helvetica-Bold', 'vertical text', 'Courier-BoldOblique'):
                assert expected in text, (path, expected)
            images = page['/Resources'].get('/XObject', {})
            assert len(images) == (1 if has_logo else 0), path
            renderer = args.poppler_bin.resolve() / ('pdftoppm' + suffix)
            run([renderer, '-singlefile', '-r', '72', '-png', path, path.with_suffix('')], path.stem + '-render', renderer_environment)
            records.append({'test': 'original-example', 'backend': backend,
                            'logo': has_logo, 'document': path.name, 'status': 'pass'})
        for filename, tag in ((output / 'absent-parent/denied.pdf', 'bad-output'), ('x' * 1024, 'long-output')):
            result = subprocess.run([str(example), str(filename), str(logo)], cwd=output,
                                    env=environment, capture_output=True, text=True, timeout=15)
            (output / (backend + '-' + tag + '.log')).write_text(result.stdout + result.stderr)
            assert result.returncode == 1, (backend, tag, result.returncode)
            records.append({'test': tag, 'backend': backend, 'status': 'pass'})
        print('PDFLIB_EXAMPLE=PASS ' + backend + ' logo/missing_logo/bad_output/long_output', flush=True)
    receipt = {'status': 'pass', 'target': args.target, 'native_C_ABI': native_abi, 'records': records,
               'independent_pdf_reader': 'pypdf ' + pypdf.__version__,
               'renderer': str(args.poppler_bin.resolve() / ('pdftoppm' + suffix)),
               'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
               'library_sha256': hashlib.sha256((sdk / 'lib/libpdf.a').read_bytes()).hexdigest()}
    (output / 'results.json').write_text(json.dumps(receipt, indent=2) + '\n')


if __name__ == '__main__':
    main()
# end of test-pdflib.py
