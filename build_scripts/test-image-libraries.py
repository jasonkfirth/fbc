#!/usr/bin/env python3
"""FreeBASIC native image-library qualification
File: test-image-libraries.py
Purpose: Check genuine JPEG, FreeImage, SDL1 and GD SDKs and original examples.
Responsibilities: Native layouts, independent GDI+ decoding, pixels, malformed
inputs, owned window interaction and source/SDK fingerprints.
This file contains no substitute codec, SDK installation or desktop capture.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import os
import re
import struct
import subprocess
import zlib


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--bits', type=int, choices=(32, 64), default=64)
    parser.add_argument('--tool-dir', type=Path, required=True)
    parser.add_argument('--runtime-prefix', type=Path, required=True)
    parser.add_argument('--freeimage-header', type=Path, required=True)
    parser.add_argument('--freeimage-dll', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('This qualification requires real Windows SDKs and GDI+')
    root = Path(__file__).resolve().parents[1]
    work = args.output.resolve()
    work.mkdir(parents=True, exist_ok=True)
    compiler, tools = args.fbc.resolve(), args.tool_dir.resolve()
    sdk = tools.parent
    env = os.environ.copy()
    env['PATH'] = str(tools)+';'+str(args.freeimage_dll.resolve().parent)+';'+env['PATH']
    env['FB_GFX_DRIVER'] = 'GDI'
    # The original SDL1 program uses a software video surface. WINDIB supplies
    # that real native backend without legacy DirectDraw capture restrictions.
    env['SDL_VIDEODRIVER'] = 'windib'
    for label in ('gcc', 'as', 'ld', 'ar'):
        env[label.upper()] = str(tools/(label+'.exe'))
    gd = args.bits == 64
    examples = {
        'freeimage': 'examples/files/FreeImage/fb.bas',
        'manual': 'examples/manual/extras/freeimage.bas',
        'sdl': 'examples/files/FreeImage/sdl.bas',
        'jpeg': 'examples/files/jpeglib/example.bas',
    }
    if gd:
        examples['gd'] = 'examples/files/gd/gd.bas'
    names = list(examples.values())+['inc/jpeglib.bi', 'inc/FreeImage.bi', 'inc/gd.bi', 'inc/crt/setjmp.bi',
        'inc/SDL/SDL.bi', 'tests/bindings/image-abi.c', 'tests/bindings/image-abi.bas',
        'tests/bindings/image-codec-reference.c', 'tests/bindings/image-runtime.bas',
        'build_scripts/test-image-libraries.py', 'build_scripts/windows-test-images.py']
    names += ['doc/manual/cache/ExtLib'+name+'.wakka' for name in ('freeimage', 'jpeglib', 'gd')]
    fingerprint = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    sdk_files = [args.freeimage_header.resolve(), args.freeimage_dll.resolve()]
    sdk_files += [sdk/'include'/name for name in ('jpeglib.h', 'jconfig.h', 'jmorecfg.h', 'SDL/SDL_version.h', 'SDL/SDL_video.h')]
    sdk_files += [sdk/'bin'/name for name in ('libjpeg-8.dll', 'SDL.dll')]
    sdk_files += [sdk/'lib'/name for name in ('libjpeg.dll.a', 'libSDL.dll.a')]
    if gd:
        sdk_files += [sdk/'include/gd.h', sdk/'bin/libgd.dll', sdk/'lib/libgd.dll.a', sdk/'lib/libfreeimage.dll.a']
    else:
        sdk_files += [tools/'gendef.exe', tools/'dlltool.exe']
    records = []
    receipt = {'status': 'pending', 'bits': args.bits, 'compiler_sha256': fingerprint(compiler),
               'source_sha256': {name: fingerprint(root/name) for name in names},
               'sdk_sha256': {str(path): fingerprint(path) for path in sdk_files},
               'records': records}
    target = work/'results.json'

    def save():
        target.write_text(json.dumps(receipt, indent=2)+'\n')

    def run(command, directory, label, expected=0):
        result = subprocess.run(list(map(str, command)), cwd=directory, env=env,
                                stdin=subprocess.DEVNULL, capture_output=True, timeout=30)
        (directory/(label+'.log')).write_bytes(repr(list(map(str, command))).encode()+b'\n'+result.stdout+result.stderr)
        assert result.returncode == expected, (label, result.returncode, result.stdout[:1000], result.stderr[:1000])
        return result.stdout

    def record(backend, example, case, **evidence):
        records.append({'backend': backend, 'example': examples.get(example, example), 'case': case,
                        'passed': True, **evidence})
        save()

    def machine(binary):
        data = binary.read_bytes()
        offset = struct.unpack_from('<I', data, 0x3c)[0]
        assert struct.unpack_from('<H', data, offset+4)[0] == (0x14c if args.bits == 32 else 0x8664)

    def fb_flags(backend):
        flags = [compiler, '-i', root/'inc', '-p', native_lib, '-p', sdk/'lib',
                 '-d', '__JPEGLIB_VER__=8', '-gen', backend, '-w', 'all']
        if gd:
            flags += ['-d', 'IMAGE_CHECK_GD']
        if args.bits == 32:
            flags += ['-target', 'win32', '-prefix', args.runtime_prefix.resolve(), '-buildprefix', str(tools)+'/']
        return flags

    def compile_fb(source, backend, label, directory, extra=()):
        binary = directory/(label+'.exe')
        run(fb_flags(backend)+list(extra)+[root/source, '-x', binary], directory, label+'-compile')
        machine(binary)
        return binary

    def abi(data):
        return {line.split()[0]: list(map(int, line.split()[1:])) for line in data.decode().splitlines()
                if line.split() and line.split()[0] in ('JPEG', 'FREEIMAGE', 'GD', 'SDL')}

    def raw_image(path):
        data = path.read_bytes()
        width, height = struct.unpack_from('<II', data)
        assert len(data) == 8+width*height*4 and 0 < width <= 4096 and 0 < height <= 4096
        return width, height, data[8:]

    save()
    header_dir = work/'native-headers'
    header_dir.mkdir(exist_ok=True)
    (header_dir/'FreeImage.h').write_bytes(args.freeimage_header.read_bytes())
    native_lib = sdk/'lib'
    if args.bits == 32:
        # Keep gendef's actual export aliases, including the MSVC leading
        # underscore. The previous installed archive discarded those aliases.
        native_lib = work/'native-lib'
        native_lib.mkdir(exist_ok=True)
        run([tools/'gendef.exe', args.freeimage_dll.resolve()], native_lib, 'freeimage-exports')
        definition = native_lib/(args.freeimage_dll.stem+'.def')
        assert 'FreeImage_GetVersion@0 == _FreeImage_GetVersion@0' in definition.read_text()
        run([tools/'dlltool.exe', '-d', definition, '-l', native_lib/'libFreeImage.dll.a'],
            native_lib, 'freeimage-import')
        receipt['derived_import'] = {'def_sha256': fingerprint(definition),
            'archive_sha256': fingerprint(native_lib/'libFreeImage.dll.a'),
            'DLL_unchanged': fingerprint(args.freeimage_dll.resolve()),
            'scope': 'Import names derived from the genuine DLL; no function implementation or calling-convention shim'}
    spec = importlib.util.spec_from_file_location('private_images', root/'build_scripts/windows-test-images.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    images = module.Images()
    try:
        native = work/'abi-c.exe'
        flags = [tools/'gcc.exe', '-Wall', '-Wextra', '-Werror', '-I'+str(header_dir),
                 root/'tests/bindings/image-abi.c', '-L'+str(native_lib), '-L'+str(sdk/'lib'),
                 '-lFreeImage', '-ljpeg', '-lSDL', '-o', native]
        if gd:
            flags += ['-DIMAGE_CHECK_GD', '-lgd']
        run(flags, work, 'abi-c-compile')
        machine(native)
        reference = run([native], work, 'abi-c-run')
        assert b'JPEG 80 ' in reference and b'FREEIMAGE_VERSION 3.18.0' in reference
        receipt['native_reference'] = reference.decode().strip()
        native_codec = work/'codec-c.exe'
        run([tools/'gcc.exe', '-Wall', '-Wextra', '-Werror', root/'tests/bindings/image-codec-reference.c',
             '-L'+str(sdk/'lib'), '-ljpeg', '-o', native_codec], work, 'codec-c-compile')
        machine(native_codec)
        fixtures = work/'fixtures'
        fixtures.mkdir(exist_ok=True)
        references = {}
        for mode in ('gray', 'rgb', 'cmyk'):
            path = fixtures/(mode+'.jpg')
            run([native_codec, path, mode], work, 'fixture-'+mode)
            if mode != 'cmyk':
                raw = fixtures/(mode+'.bgra')
                run([native_codec, path, 'decode', raw], work, 'reference-'+mode)
                references[mode+'.jpg'] = raw_image(raw)
                decoded = images.decode(path)
                assert decoded[:2] == references[mode+'.jpg'][:2]
                delta = max(abs(a-b) for a,b in zip(decoded[2], references[mode+'.jpg'][2]))
                assert delta <= 2, ('Independent JPEG decoder', mode, delta)
                record('C', 'jpeg', 'independent GDI+ decoder '+mode, maximum_byte_difference=delta)
        # Odd widths exercise source padding and FB's independently padded rows.
        width, height = 13, 7
        rgb = bytes(value for y in range(height) for x in range(width)
                    for value in ((x*29+y*3) % 256, (x*7+y*37) % 256, (x*11+y*19) % 256, 255))

        def bitmap(name, bits, palette=None):
            pitch = ((width*bits+31)//32)*4
            colors = palette or []
            raw = bytearray()
            expected = bytearray()
            for y in range(height):
                for x in range(width):
                    expected.extend(colors[(x+y) % len(colors)] if colors else rgb[(y*width+x)*4:(y*width+x+1)*4])
            for y in reversed(range(height)):
                row = bytearray(pitch)
                for x in range(width):
                    if bits == 24:
                        row[x*3:x*3+3] = rgb[(y*width+x)*4:(y*width+x)*4+3]
                    else:
                        index = (x+y) % len(colors)
                        shift = 8-bits-(x*bits % 8)
                        row[x*bits//8] |= index << shift
                raw.extend(row)
            table = b''.join(bytes(color[:3])+b'\0' for color in colors)
            offset = 54+len(table)
            data = struct.pack('<2sIHHI', b'BM', offset+len(raw), 0, 0, offset)
            data += struct.pack('<IiiHHIIiiII', 40, width, height, 1, bits, 0, len(raw), 0, 0, len(colors), 0)
            (fixtures/name).write_bytes(data+table+raw)
            references[name] = width, height, bytes(expected)

        bitmap('rgb.bmp', 24)
        bitmap('palette.bmp', 8, [(30,20,10,255),(50,30,220,255),(60,200,20,255),(220,80,20,255)])
        bitmap('one-bit.bmp', 1, [(0,0,0,255),(255,255,255,255)])
        bitmap('four-bit.bmp', 4, [(i*11,i*7,i*13,255) for i in range(16)])

        def chunk(label, data):
            return struct.pack('>I', len(data))+label+data+struct.pack('>I', zlib.crc32(label+data))

        rgba = bytearray(rgb)
        rgba[3::4] = bytes(128+index % 128 for index in range(width*height))
        rows = []
        for y in range(height):
            row = rgba[y*width*4:(y+1)*width*4]
            row[0::4], row[2::4] = row[2::4], row[0::4]
            rows.append(b'\0'+row)
        (fixtures/'alpha.png').write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
            +chunk(b'IDAT', zlib.compress(b''.join(rows)))+chunk(b'IEND', b''))
        references['alpha.png'] = width, height, bytes(rgba)
        for name in ('rgb.bmp', 'palette.bmp', 'one-bit.bmp', 'four-bit.bmp', 'alpha.png'):
            decoded = images.decode(fixtures/name)
            assert decoded[:2] == references[name][:2]
            delta = max(abs(a-b) for a,b in zip(decoded[2], references[name][2]))
            assert delta <= 2, (name, delta)
            record('GDI+', 'fixtures', 'independent format decode '+name, maximum_byte_difference=delta)
        for name, data in (('empty.jpg',b''), ('garbage.jpg',b'not a JPEG'), ('unfinished.jpg',b'\xff\xd8\xff\xd9'),
                           ('garbage.png',b'not a PNG')):
            (fixtures/name).write_bytes(data)
        oversized = bytearray((fixtures/'rgb.jpg').read_bytes())
        frame = oversized.index(b'\xff\xc0')
        struct.pack_into('>HH', oversized, frame+5, 4097, 4096)
        (fixtures/'oversized.jpg').write_bytes(oversized)
        oversized = bytearray((fixtures/'rgb.bmp').read_bytes())
        struct.pack_into('<ii', oversized, 18, 4096, 4097)
        (fixtures/'oversized.bmp').write_bytes(oversized)
        # These are tiny metadata fixtures, not large pixel files or sparse files.
        receipt['oversized_metadata_fixture_bytes'] = {
            name: (fixtures/name).stat().st_size for name in ('oversized.jpg','oversized.bmp')}
        for backend in ('gcc', 'gas' if args.bits == 32 else 'gas64'):
            directory = work/backend
            directory.mkdir(exist_ok=True)
            binary = compile_fb('tests/bindings/image-abi.bas', backend, 'abi', directory)
            actual = run([binary], directory, 'abi-run')
            assert abi(actual) == abi(reference), (actual, reference)
            record(backend, 'ABI', 'native C layouts', scalar_comparisons=sum(map(len, abi(actual).values())))
            for dialect in ('fb', 'fblite'):
                binary = compile_fb('tests/bindings/image-runtime.bas', backend, 'runtime-'+dialect, directory,
                                    ['-lang', dialect])
                families = ('jpeg', 'freeimage', 'manual') if dialect == 'fb' else ('jpeg',)
                for family in families:
                    cases = ('gray.jpg', 'rgb.jpg') if family == 'jpeg' else tuple(references)
                    for name in cases:
                        raw = directory/(family+'-'+dialect+'-'+name+'.bgra')
                        run([binary, family, fixtures/name, raw], directory, family+'-'+dialect+'-'+name)
                        actual = raw_image(raw)
                        assert actual == references[name], (family, dialect, name, actual[:2])
                        record(backend, family, dialect+' pixels '+name, pixels=actual[0]*actual[1])
                    negatives = ('empty.jpg', 'garbage.jpg', 'unfinished.jpg', 'cmyk.jpg', 'oversized.jpg') if family == 'jpeg' else (
                        'missing.bmp', 'empty.jpg', 'garbage.png', 'oversized.jpg', 'oversized.bmp')
                    for name in negatives:
                        raw = directory/(family+'-'+dialect+'-'+name+'.bgra')
                        run([binary, family, fixtures/name, raw], directory, family+'-'+dialect+'-'+name, 1)
                        assert not raw.exists()
                        record(backend, family, dialect+' failure '+name, exit=1)
                if dialect == 'fb' and gd:
                    destination = directory/'deterministic'
                    # A larger preexisting output exposes accidental nontruncating writes.
                    (directory/'deterministic.png').write_bytes(b'old bytes'*16384)
                    run([binary, 'gd', 'unused', destination], directory, 'gd-save')
                    for extension in ('png','gif','jpg'):
                        decoded = images.decode(directory/('deterministic.'+extension))
                        assert decoded[:2] == (32,16)
                        for x, expected in ((4,(40,30,240)), (24,(70,190,20))):
                            pixel = decoded[2][(8*32+x)*4:(8*32+x)*4+3]
                            tolerance = 12 if extension == 'jpg' else 0
                            assert all(abs(a-b) <= tolerance for a,b in zip(pixel, expected)), (extension,pixel)
                        record(backend, 'gd', 'independent '+extension+' pixels and write failures', size=decoded[:2])
                    assert (directory/'deterministic.png').stat().st_size < 16384*8
                # Both unoptimized and optimized code must recover from native longjmp.
                optimized = compile_fb('tests/bindings/image-runtime.bas', backend, 'runtime-'+dialect+'-O2', directory,
                                       ['-lang', dialect, '-O', '2'])
                for label, executable in (('normal', binary), ('O2', optimized)):
                    report_file = directory/('failure-loop-'+dialect+'-'+label+'.txt')
                    run([executable, 'failure-loop', fixtures/'garbage.jpg', report_file],
                        directory, 'failure-loop-'+dialect+'-'+label)
                    outcome = report_file.read_text()
                    assert 'JPEG_FAILURE_LOOP=PASS cycles=500' in outcome
                    record(backend, 'jpeg', dialect+' '+label+' native error cleanup', actual=outcome.strip())
            for name, source in examples.items():
                binary = compile_fb(source, backend, 'original-'+name, directory)
                if name == 'gd':
                    run([binary], directory, 'original-gd')
                    for extension in ('gif','png','jpg'):
                        decoded = images.decode(directory/('test.'+extension))
                        assert decoded[:2] == (320,200) and len(set(decoded[2][0::4])) > 16
                        record(backend, name, 'original '+extension+' independent decode', size=decoded[:2])
                    continue
                if name == 'jpeg':
                    refs = [(0,0,*references['gray.jpg'],0), (320,0,*references['rgb.jpg'],0)]
                    outcome = images.run_graphics([binary, fixtures/'gray.jpg', fixtures/'rgb.jpg'], directory, env,
                        refs, directory/'original-jpeg-window.log')
                    record(backend, name, 'original grayscale/RGB window and final SLEEP', **outcome)
                else:
                    for fixture in ('rgb.bmp', 'palette.bmp', 'one-bit.bmp', 'four-bit.bmp', 'alpha.png'):
                        rw,rh,pixels = references[fixture]
                        x,y = (800//2-rw//2,600//2-rh//2) if name == 'sdl' else (0,0)
                        outcome = images.run_graphics([binary, fixtures/fixture], directory, env,
                            [(x,y,rw,rh,pixels,8 if name == 'sdl' else 0)],
                            directory/(name+'-'+fixture+'-window.log'), sdl=name=='sdl')
                        record(backend, name, 'original client '+fixture, **outcome)
                command = [binary, fixtures/'missing.bmp']
                if name == 'jpeg':
                    command += [fixtures/'rgb.jpg']
                run(command, directory, name+'-missing-file', 1)
                record(backend, name, 'original missing input', exit=1)
        assert all(fingerprint(root/name) == value for name,value in receipt['source_sha256'].items())
        assert all(fingerprint(Path(name)) == value for name,value in receipt['sdk_sha256'].items())
        receipt['status'] = 'passed'
        save()
        print('NATIVE_IMAGES=PASS bits='+str(args.bits)+' records='+str(len(records)))
    finally:
        images.close()


if __name__ == '__main__':
    main()
# end of test-image-libraries.py
