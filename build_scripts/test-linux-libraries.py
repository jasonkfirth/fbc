#!/usr/bin/env python3
"""FreeBASIC Linux native SDK qualification
File: test-linux-libraries.py
Purpose: Compare real Linux C headers with bindings on both native backends.
Responsibilities: SDK calls, exact guarded glyph pixels and source provenance.
This file contains no substitute libraries, package installation or desktop input.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess


def fingerprint(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def numeric_rows(output, labels):
    rows = {}
    for line in output.decode().splitlines():
        match = re.match(r'^([A-Z0-9_]+)\s+(.*)$', line)
        if match and match[1] in labels:
            rows[match[1]] = [int(value) for value in match[2].split()]
    if set(rows) != set(labels):
        raise RuntimeError('Missing ABI rows: '+repr(output))
    return rows


def check_bounds(path):
    data = path.read_bytes()
    assert len(data) == 216*60
    coverage = [[0,64,128,192,255], [255,192,128,64,0], [17,33,99,177,233]]
    position = 0
    for flow in range(2):
        rows = coverage if flow == 0 else coverage[::-1]
        for x in range(-6,6):
            for y in range(-4,5):
                expected = bytearray()
                for dy in range(3):
                    for dx in range(4):
                        alpha = rows[dy-y][dx-x] if 0 <= dy-y < 3 and 0 <= dx-x < 5 else 0
                        colors = [(src*alpha+dst*(255-alpha)+127)//255
                                  for src,dst in zip((10,130,220),(60,40,20))]
                        expected += bytes(colors)+b'\xff'
                    expected += b'\xa5'*4
                assert data[position:position+60] == expected, (flow,x,y)
                position += 60


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--sdk-prefix', type=Path)
    parser.add_argument('--font', type=Path, default=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'))
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--select', action='append')
    args = parser.parse_args()
    if not os.sys.platform.startswith('linux'):
        parser.error('This runner requires native Linux')
    root = Path(__file__).resolve().parents[1]
    work = args.output.resolve()
    work.mkdir(parents=True, exist_ok=True)
    compiler = args.fbc.resolve()
    environment = os.environ.copy()
    sdk_flags = []
    c_prefix = []
    if args.sdk_prefix:
        sdk = args.sdk_prefix.resolve()
        sdk_flags = ['-p', str(sdk/'lib')]
        c_prefix = ['-I'+str(sdk/'include'), '-L'+str(sdk/'lib')]
        environment['LD_LIBRARY_PATH'] = str(sdk/'lib')+os.pathsep+environment.get('LD_LIBRARY_PATH','')
        environment['PKG_CONFIG_PATH'] = str(sdk/'lib/pkgconfig')+os.pathsep+environment.get('PKG_CONFIG_PATH','')
    families = {
        'compression-xml': ('compression-xml-abi', ['libzip','expat','libxml-2.0','zlib'], ['-lbz2','-llzo2'],
                            ['ZIP','EXPAT','XML','ZLIB','BZ2','LZO'], [], []),
        'gmp': ('gmp-native', ['gmp'], [], ['ABI'], [], []),
        'ffi': ('ffi-native', ['libffi'], [], ['ABI'], ['abi'], []),
        'gsl': ('gsl-native', ['gsl'], [], ['ABI'], [], []),
        'lua': ('lua-abi', ['lua5.3'], [], ['ABI'], [], []),
        'image': ('image-abi', ['sdl','libjpeg','gdlib'], ['-lfreeimage'],
                  ['JPEG','FREEIMAGE','SDL','GD'], [], ['-d','__JPEGLIB_VER__=6','-d','IMAGE_CHECK_GD']),
        'freetype-cairo': ('freetype-cairo', ['freetype2','cairo'], [], ['FT','CAIRO'], [], []),
        'regex': ('regex-abi', ['libpcre','libpcre16','libpcre32','tre'], [], ['PCRE','TRE'], [], []),
    }
    records = []
    receipt = {'status':'pending', 'compiler_sha256':fingerprint(compiler), 'records':records,
               'source_sha256':{'build_scripts/test-linux-libraries.py':fingerprint(Path(__file__).resolve())},
               'linked_library_sha256':{}, 'header_sha256':{}, 'font_sha256':fingerprint(args.font)}
    target = work/'results.json'

    def save():
        target.write_text(json.dumps(receipt, indent=2)+'\n')

    def run(command, directory, label):
        result = subprocess.run(list(map(str, command)), cwd=directory, env=environment,
                                stdin=subprocess.DEVNULL, capture_output=True, timeout=60)
        (directory/(label+'.log')).write_bytes(repr(list(map(str, command))).encode()+b'\n'+result.stdout+result.stderr)
        if result.returncode:
            raise RuntimeError(label+' exit '+str(result.returncode)+': '+(result.stdout+result.stderr)[-2000:].decode(errors='replace'))
        return result.stdout

    def libraries(binary):
        output = run(['ldd', binary], binary.parent, binary.name+'-ldd')
        for name in re.findall(rb'=>\s+(/[^\s]+)', output):
            path = Path(os.fsdecode(name)).resolve()
            receipt['linked_library_sha256'][str(path)] = fingerprint(path)

    for family, (stem, packages, libraries_flags, labels, runtime_args, flags) in families.items():
        if args.select and family not in args.select:
            continue
        directory = work/family
        directory.mkdir(exist_ok=True)
        record = {'family':family, 'passed':False, 'backends':[]}
        records.append(record)
        save()
        try:
            c_source = root/'tests/bindings'/(stem+'.c')
            fb_source = c_source.with_suffix('.bas')
            for path in (c_source, fb_source):
                receipt['source_sha256'][path.relative_to(root).as_posix()] = fingerprint(path)
            pkg_flags = shlex.split(run(['pkg-config','--cflags','--libs',*packages], directory, 'pkg-config').decode())
            native = directory/'reference'
            native_flags = ['-DIMAGE_CHECK_GD'] if family == 'image' else []
            dependency_file = directory/'native.d'
            run(['gcc','-M','-MF',dependency_file,*native_flags,*c_prefix,c_source,*pkg_flags], directory, 'c-headers')
            dependencies = dependency_file.read_text().replace('\\\n',' ').split(':',1)[1]
            for name in shlex.split(dependencies):
                path = Path(name).resolve()
                receipt['header_sha256'][str(path)] = fingerprint(path)
            run(['gcc','-Wall','-Wextra','-Werror',*native_flags,*c_prefix,c_source,*pkg_flags,*libraries_flags,'-o',native], directory, 'c-compile')
            libraries(native)
            c_args = [args.font.resolve(), directory] if family == 'freetype-cairo' else runtime_args
            reference = run([native,*c_args], directory, 'c-run')
            expected = numeric_rows(reference, labels)
            record['reference'] = reference.decode().strip()
            for backend in ('gcc','gas64'):
                binary = directory/backend
                run([compiler,'-prefix',args.prefix.resolve(),'-i',root/'inc','-gen',backend,'-w','all',
                     *sdk_flags,*flags,fb_source,'-x',binary], directory, backend+'-compile')
                libraries(binary)
                output_file = directory/(backend+'-bounds.bin')
                fb_args = [output_file] if family == 'freetype-cairo' else runtime_args
                actual = run([binary,*fb_args], directory, backend+'-run')
                observed = numeric_rows(actual, labels)
                assert observed == expected, (family,backend,expected,observed)
                check = {'backend':backend, 'scalars':sum(map(len,expected.values())), 'abi':observed}
                if family == 'freetype-cairo':
                    assert b'GLYPH_BOUNDS=PASS' in actual
                    check_bounds(output_file)
                    check.update(guarded_cases=216, pixels=2592)
                if family == 'ffi':
                    output = run([binary], directory, backend+'-closure')
                    assert b'LIBFFI_NATIVE=PASS' in output
                    check['closure'] = output.decode().strip()
                if family == 'regex':
                    wrapper = root/'examples/regex/PCRE/CRegex'
                    runtime_source = root/'tests/bindings/regex-runtime.bas'
                    sources = [runtime_source, *wrapper.glob('*.bas'), *wrapper.glob('*.bi'),
                               root/'examples/regex/PCRE/preg_class.bas', root/'examples/regex/PCRE/match-progress.bi',
                               root/'examples/regex/TRE/replace.bas', root/'examples/regex/TRE/subexpr.bas']
                    for path in sources:
                        receipt['source_sha256'][path.relative_to(root).as_posix()] = fingerprint(path)
                    archive_directory = directory/(backend+'-library')
                    archive_directory.mkdir(exist_ok=True)
                    archive = archive_directory/'libCRegex.a'
                    run([compiler,'-prefix',args.prefix.resolve(),'-i',root/'inc','-gen',backend,*sdk_flags,
                         '-lib',wrapper/'CRegex.bas','-x',archive], directory, backend+'-wrapper')
                    runtime = directory/(backend+'-runtime')
                    run([compiler,'-prefix',args.prefix.resolve(),'-i',root/'inc','-i',wrapper,'-gen',backend,
                         *sdk_flags,'-p',archive_directory,runtime_source,'-x',runtime], directory, backend+'-runtime-compile')
                    output = run([runtime], directory, backend+'-runtime-run')
                    assert b'REGEX_RUNTIME=PASS' in output
                    check['runtime'] = output.decode().strip()
                record['backends'].append(check)
            record['passed'] = True
        except Exception as error:
            record['error'] = repr(error)
        save()
        print('LINUX_SDK',family,'PASS' if record['passed'] else record['error'],flush=True)
    for name,digest in receipt['source_sha256'].items():
        assert fingerprint(root/name) == digest,name
    for name,digest in receipt['linked_library_sha256'].items():
        assert fingerprint(Path(name)) == digest,name
    for name,digest in receipt['header_sha256'].items():
        assert fingerprint(Path(name)) == digest,name
    assert fingerprint(compiler) == receipt['compiler_sha256']
    receipt['status'] = 'passed' if records and all(record['passed'] for record in records) else 'failed'
    save()
    return 0 if receipt['status'] == 'passed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
# end of test-linux-libraries.py
