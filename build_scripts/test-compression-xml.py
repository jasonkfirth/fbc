#!/usr/bin/env python3
"""FreeBASIC native compression and XML example qualification
File: test-compression-xml.py
Purpose: Compare six real SDK ABIs and execute their original examples.
Responsibilities: Private archives/XML, independent decoding, failure statuses,
owned console input, SDK/source fingerprints and bounded allocations.
This file contains no substitute codecs, public network or user-file extraction.
"""
from pathlib import Path
import argparse
import bz2
import ctypes as c
from ctypes import wintypes as w
import gzip
import hashlib
import importlib.util
import json
import os
import re
import shlex
import shutil
import struct
import subprocess
import tempfile
import warnings
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc', type=Path, required=True)
    parser.add_argument('--bits', type=int, choices=(32, 64), default=64)
    parser.add_argument('--tool-dir', type=Path)
    parser.add_argument('--runtime-prefix', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    windows = os.name == 'nt'
    if windows and args.tool_dir is None:
        parser.error('--tool-dir is required for Windows SDK selection')
    if not windows and (not os.sys.platform.startswith('linux') or args.bits != 64):
        parser.error('The Unix lane currently requires native Linux x86-64')
    root = Path(__file__).resolve().parents[1]
    work = args.output.resolve()
    work.mkdir(parents=True, exist_ok=True)
    if not windows:
        probe = work/('case-probe-'+str(os.getpid()))
        probe.write_bytes(b'owned filesystem probe')
        try:
            assert not probe.with_name(probe.name.upper()).exists(), 'Linux cases require a case-sensitive filesystem'
        finally:
            probe.unlink()
    compiler = args.fbc.resolve()
    tools = args.tool_dir.resolve() if args.tool_dir else Path(shutil.which('gcc') or '/usr/bin/gcc').parent
    sdk = tools.parent
    suffix = '.exe' if windows else ''
    console_name = 'windows-test-console.py' if windows else 'linux-test-console.py'
    env = os.environ.copy()
    env['PATH'] = str(tools)+os.pathsep+env['PATH']
    for label in ('gcc', 'as', 'ld', 'ar'):
        env[label.upper()] = str(tools/(label+suffix))
    examples = {
        'zip': 'examples/compression/libzip.bas', 'zip-manual': 'examples/manual/libraries/zip.bas',
        'zip-pack': 'examples/manual/libraries/zip2.bas', 'expat': 'examples/xml/expat.bas',
        'expat-manual': 'examples/manual/libraries/expat.bas', 'libxml': 'examples/xml/libxml.bas',
        'libxml-manual': 'examples/manual/libraries/libxml.bas', 'bz2': 'examples/compression/bz2compress.bas',
        'zlib': 'examples/compression/zlib.bas', 'zlib-manual': 'examples/manual/libraries/zlib.bas',
        'lzo': 'examples/manual/libraries/lzo.bas',
    }
    names = list(examples.values())+['build_scripts/test-compression-xml.py', 'build_scripts/'+console_name,
        'tests/bindings/compression-xml-abi.c', 'tests/bindings/compression-xml-abi.bas',
        'inc/zip.bi', 'inc/expat.bi', 'inc/libxml/xmlreader.bi', 'inc/zlib.bi', 'inc/bzlib.bi',
        'inc/lzo/lzo1x.bi', 'inc/lzo/lzoconf.bi', 'inc/crt/win32/time.bi', 'inc/file.bi', 'inc/dir.bi']
    names += ['doc/manual/cache/ExtLib'+name+'.wakka' for name in ('Zip', 'expat', 'libxml', 'zlib', 'Lzo')]
    hashes = {name: hashlib.sha256((root/name).read_bytes()).hexdigest() for name in names}
    sdk_paths = [sdk/'include'/name for name in ('zip.h', 'zipconf.h', 'expat.h', 'expat_external.h',
                'libxml2/libxml/xmlreader.h', 'zlib.h', 'bzlib.h', 'lzo/lzo1x.h', 'lzo/lzoconf.h')]
    if windows:
        sdk_paths += [sdk/'bin'/name for name in ('libzip.dll', 'libexpat-1.dll', 'libxml2-16.dll',
                     'zlib1.dll', 'libbz2-1.dll', 'liblzo2-2.dll')]
        sdk_paths += [sdk/'lib'/('lib'+name+'.dll.a') for name in ('zip', 'expat', 'xml2', 'z', 'bz2', 'lzo2')]
        c_sdk_flags = ['-I'+str(sdk/'include'), '-I'+str(sdk/'include/libxml2'), '-L'+str(sdk/'lib'),
                       '-lzip', '-lexpat', '-lxml2', '-lz', '-lbz2', '-llzo2']
    else:
        c_sdk_flags = shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','libzip','expat','libxml-2.0','zlib'],text=True))+['-lbz2','-llzo2']
        for name in ('zip','expat','xml2','z','bz2','lzo2'):
            path = Path(subprocess.check_output([tools/'gcc','-print-file-name=lib'+name+'.so'],text=True).strip()).resolve()
            sdk_paths.append(path)
    sdk_hashes = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in sdk_paths}
    records = []
    receipt = {'status': 'pending', 'platform': 'windows' if windows else 'linux', 'bits': args.bits, 'compiler_sha256': hashlib.sha256(compiler.read_bytes()).hexdigest(),
               'source_sha256': hashes, 'sdk_sha256': sdk_hashes, 'records': records}
    target = work/'results.json'

    def save():
        target.write_text(json.dumps(receipt, indent=2)+'\n')

    save()
    spec = importlib.util.spec_from_file_location('private_console', root/'build_scripts'/console_name)
    console = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(console)

    def run(command, directory, label, expected=0):
        result = subprocess.run(list(map(str, command)), cwd=directory, env=env, stdin=subprocess.DEVNULL,
                                capture_output=True, timeout=30)
        (directory/(label+'.log')).write_bytes(repr(list(map(str, command))).encode()+b'\n'+result.stdout+result.stderr)
        assert result.returncode == expected, (label, result.returncode, result.stdout[:1000], result.stderr[:1000])
        return result.stdout

    def machine(binary):
        data = binary.read_bytes()
        if windows:
            offset = struct.unpack_from('<I', data, 0x3c)[0]
            assert struct.unpack_from('<H', data, offset+4)[0] == (0x14c if args.bits == 32 else 0x8664)
        else:
            assert data[:6] == b'\x7fELF\x02\x01' and struct.unpack_from('<H',data,18)[0] == 62

    def compile_fb(source, backend, label, directory):
        binary = directory/(label+suffix)
        flags = [compiler, '-i', root/'inc', '-p', sdk/'lib', '-gen', backend, '-w', 'all']
        flags += ['-prefix', args.runtime_prefix.resolve()]
        if windows and args.bits == 32:
            flags += ['-target', 'win32', '-prefix', args.runtime_prefix.resolve(), '-buildprefix', str(tools)+'/']
        run(flags+[root/source, '-x', binary], directory, label+'-compile')
        machine(binary)
        return binary

    def abi(data):
        return {line.split()[0]: list(map(int, line.split()[1:])) for line in data.decode().splitlines()
                if line.split() and line.split()[0] in ('ZIP', 'EXPAT', 'XML', 'ZLIB', 'BZ2', 'LZO')}

    def record(backend, name, case, **evidence):
        records.append({'backend': backend, 'example': examples.get(name, name), 'case': case, 'passed': True, **evidence})
        save()

    # -------------------------------------------------------------------------
    # Private archive and XML fixtures
    # -------------------------------------------------------------------------
    payload = bytes(range(256))*3073+b'finished'  # Larger than the extraction buffer.
    good = {'empty.bin': b'', 'folder/binary.bin': payload, 'folder/second.txt': b'alpha\nbeta\n'}

    def make_zip(path, entries, compression=zipfile.ZIP_DEFLATED):
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', UserWarning)
            with zipfile.ZipFile(path, 'w', compression=compression) as archive:
                for name, content in entries:
                    archive.writestr(name, content)

    fixtures = work/'fixtures'
    fixtures.mkdir(exist_ok=True)
    make_zip(fixtures/'good.zip', [('folder/', b'')]+list(good.items()))
    make_zip(fixtures/'duplicate.zip', [('same.txt', b'first'), ('same.txt', b'second')])
    make_zip(fixtures/'case-collision.zip', [('same.txt', b'first'), ('SAME.txt', b'second')])
    make_zip(fixtures/'directory-data.zip', [('folder/', b'not directory data')])
    make_zip(fixtures/'crc.zip', [('bad.bin', b'CRC payload')], zipfile.ZIP_STORED)
    bad = bytearray((fixtures/'crc.zip').read_bytes())
    header = struct.unpack_from('<IHHHHHIIIHH', bad, 0)
    bad[30+header[-2]+header[-1]] ^= 1
    (fixtures/'crc.zip').write_bytes(bad)
    make_zip(fixtures/'budget.zip', [('large.bin', b'')])
    bad = bytearray((fixtures/'budget.zip').read_bytes())
    central = bad.index(b'PK\x01\x02')
    struct.pack_into('<I', bad, 22, 1024**3+1)
    struct.pack_into('<I', bad, central+24, 1024**3+1)
    (fixtures/'budget.zip').write_bytes(bad)
    (fixtures/'not.zip').write_bytes(b'not an archive')
    bad_names = ('../escape.txt', '/rooted.txt', 'C:/drive.txt', 'folder/../escape.txt',
                 '..\\escape.txt', 'folder//empty.txt', './dot.txt', 'stream:payload',
                 'CON.txt', 'LPT1.txt', 'COM\u00b9.txt', 'CONIN$', 'trailing. ', 'wild*.txt')
    for index, name in enumerate(bad_names):
        make_zip(fixtures/('path-'+str(index)+'.zip'), [(name, b'owned fixture')])
    xml_cases = {
        'valid': b'<root k="value"><child>hello &amp; goodbye</child><empty/></root>',
        'long': b'<root>'+b'x'*8193+b'</root>',
        'mixed': b'<root>before<child>inside</child>after</root>',
        'unicode': '<root>caf\u00e9 \u03c0 \u4f60\u597d</root>'.encode(),
        'boundary': b'<root>'+b'b'*(1024*4-13)+b'</root>',
        'empty': b'', 'malformed': b'<root><child></root>', 'unfinished': b'<root>unfinished',
        'zero': b'<root>bad\x00text</root>',
    }
    for name, content in xml_cases.items():
        (fixtures/(name+'.xml')).write_bytes(content)

    # A sparse input verifies the pre-allocation 64-bit file-size check without
    # consuming several GiB. Refuse to proceed if the filesystem cannot keep it sparse.
    # Keep the large logical file outside a potentially synchronized checkout.
    task_temp = tempfile.TemporaryDirectory(prefix='fbc-codec-size-')
    task_temp_root = Path(task_temp.name).resolve()
    assert task_temp_root.parent == Path(tempfile.gettempdir()).resolve()
    assert task_temp_root.name.startswith('fbc-codec-size-')
    sparse = task_temp_root/'oversized.bin'
    if windows:
        import msvcrt
        kernel = c.WinDLL('kernel32', use_last_error=True)
        kernel.DeviceIoControl.argtypes = [w.HANDLE, w.DWORD, c.c_void_p, w.DWORD, c.c_void_p, w.DWORD,
                                          c.POINTER(w.DWORD), c.c_void_p]
        kernel.GetCompressedFileSizeW.argtypes = [w.LPCWSTR, c.POINTER(w.DWORD)]
        kernel.GetCompressedFileSizeW.restype = w.DWORD
        kernel.SetFilePointerEx.argtypes = [w.HANDLE, c.c_longlong, c.POINTER(c.c_longlong), w.DWORD]
        kernel.SetEndOfFile.argtypes = [w.HANDLE]
        with sparse.open('wb') as file:
            returned = w.DWORD()
            assert kernel.DeviceIoControl(msvcrt.get_osfhandle(file.fileno()), 0x000900c4,
                                          None, 0, None, 0, c.byref(returned), None)
            # CRT truncate may physically zero-fill even a sparse file on Windows.
            handle = msvcrt.get_osfhandle(file.fileno())
            assert kernel.SetFilePointerEx(handle, 2**32+1, None, 0)
            assert kernel.SetEndOfFile(handle)
        high = w.DWORD()
        allocated = kernel.GetCompressedFileSizeW(str(sparse), c.byref(high))+(high.value << 32)
    else:
        with sparse.open('wb') as file:
            os.ftruncate(file.fileno(),2**32+1)
        # Linux st_blocks counts allocated 512-byte units, including sparse files.
        allocated = sparse.stat().st_blocks*512
    assert allocated <= 65536, ('Sparse fixture consumed physical storage', allocated)
    receipt['sparse_fixture'] = {'logical_bytes': sparse.stat().st_size, 'allocated_bytes': allocated}

    try:
        native = work/('abi-c'+suffix)
        run([tools/('gcc'+suffix), '-Wall', '-Wextra', '-Werror', root/'tests/bindings/compression-xml-abi.c',
             *c_sdk_flags, '-o', native], work, 'c-compile')
        machine(native)
        reference = run([native], work, 'c-abi')
        receipt['native_reference'] = reference.decode().strip()
        for backend in ('gcc', 'gas' if args.bits == 32 else 'gas64'):
            directory = work/backend
            directory.mkdir(exist_ok=True)
            native_fb = compile_fb('tests/bindings/compression-xml-abi.bas', backend, 'abi', directory)
            actual = run([native_fb], directory, 'abi-run')
            assert abi(actual) == abi(reference), (actual, reference)
            record(backend, 'ABI', 'C layouts', actual=abi(actual))
            for name, source in examples.items():
                binary = compile_fb(source, backend, name, directory)
                if name in ('zip', 'zip-manual'):
                    destination = directory/(name+'-good')
                    # Use each run's own new root. Previously produced output is
                    # never silently deleted or reused by the qualification.
                    if destination.exists():
                        destination = directory/(name+'-good-'+str(os.getpid()))
                    run([binary, fixtures/'good.zip', destination], directory, name+'-good')
                    actual_files = {p.relative_to(destination).as_posix(): p.read_bytes()
                                    for p in destination.rglob('*') if p.is_file()}
                    assert actual_files == good
                    record(backend, name, 'exact extraction', files=len(good), bytes=sum(map(len, good.values())))
                    run([binary, fixtures/'good.zip', destination], directory, name+'-existing', expected=1)
                    assert actual_files == {p.relative_to(destination).as_posix(): p.read_bytes()
                                            for p in destination.rglob('*') if p.is_file()}
                    record(backend, name, 'existing root preserved', exit=1)
                    for case in ['not.zip', 'budget.zip', 'directory-data.zip', 'duplicate.zip', 'case-collision.zip', 'crc.zip']+[
                            'path-'+str(index)+'.zip' for index in range(len(bad_names))]:
                        output_dir = directory/(name+'-'+case+'-'+str(os.getpid()))
                        # Device names and trailing dots/spaces are ordinary
                        # Linux filenames. The source rejects their Windows
                        # interpretations only on Windows. Case variants stay
                        # distinct on the verified Linux filesystem as well.
                        allowed_native = not windows and (case == 'case-collision.zip' or
                            case in ['path-'+str(index)+'.zip' for index in range(8,13)])
                        if allowed_native:
                            run([binary,fixtures/case,output_dir],directory,name+'-'+case)
                            if case == 'case-collision.zip':
                                expected_files = {'same.txt':b'first','SAME.txt':b'second'}
                            else:
                                index = int(case.removeprefix('path-').removesuffix('.zip'))
                                expected_files = {bad_names[index]:b'owned fixture'}
                            actual_files = {p.relative_to(output_dir).as_posix():p.read_bytes()
                                            for p in output_dir.rglob('*') if p.is_file()}
                            assert actual_files == expected_files
                            record(backend,name,case,exit=0,files=len(actual_files),native_filename_semantics=True)
                            continue
                        output = run([binary, fixtures/case, output_dir], directory, name+'-'+case, expected=1)
                        if case.startswith('path-') or case in ('budget.zip', 'directory-data.zip'):
                            assert not output_dir.exists(), (case, output)
                        if case == 'budget.zip':
                            assert b'extraction size limit exceeded' in output
                        if case == 'crc.zip':
                            assert not (output_dir/'bad.bin').exists()
                        if case == 'duplicate.zip':
                            # ZIP_CHECKCONS can reject exact duplicates before
                            # extraction. If accepted, the first file must survive.
                            if output_dir.exists():
                                assert (output_dir/'same.txt').read_bytes() == b'first'
                        if case == 'case-collision.zip':
                            assert (output_dir/'same.txt').read_bytes() == b'first'
                        record(backend, name, case, exit=1)
                elif name == 'zip-pack':
                    pack_dir = directory/('pack-'+str(os.getpid()))
                    pack_dir.mkdir()
                    outcome = console.run_console([binary], pack_dir, env, timeout=10)
                    assert outcome['exit'] == 0 and outcome['normal_keypress'], outcome
                    with zipfile.ZipFile(pack_dir/'26.zip') as archive:
                        assert set(archive.namelist()) == {'file3.txt', 'BIMBO/file3.txt'}
                        for entry in archive.namelist():
                            assert archive.read(entry) == b'<My text           >......6...3..2'
                    record(backend, name, 'independent ZIP read and real wait', normal_keypress=True)
                    run([binary, directory/'absent-parent'/'failure.zip'], directory, name+'-open-error', expected=1)
                    record(backend, name, 'output open failure', exit=1)
                elif name.startswith(('expat', 'libxml')):
                    for case, content in xml_cases.items():
                        expected_exit = 1 if case in ('empty', 'malformed', 'unfinished', 'zero') else 0
                        output = run([binary, fixtures/(case+'.xml')], directory, name+'-'+case, expected=expected_exit)
                        if expected_exit == 0:
                            assert b'root' in output
                            if case == 'long':
                                assert output.count(b'x') == 8193+(1 if name.startswith('libxml') else 0)
                            if case == 'boundary':
                                assert output.count(b'b') == 1024*4-13
                            if case == 'unicode':
                                assert 'caf\u00e9 \u03c0 \u4f60\u597d'.encode() in output
                            if case == 'mixed':
                                assert all(word in output for word in (b'before', b'inside', b'after'))
                        record(backend, name, case, exit=expected_exit)
                    for case, argument in (('usage', None), ('missing file', fixtures/'missing.xml')):
                        run([binary]+([argument] if argument else []), directory, name+'-'+case.replace(' ', '-'), expected=1)
                        record(backend, name, case, exit=1)
                elif name == 'bz2':
                    for case, content in (('empty', b''), ('binary', payload), ('text', b'hello\n'*20000)):
                        input_file = directory/(case+'.dat')
                        input_file.write_bytes(content)
                        run([binary, input_file], directory, name+'-'+case)
                        assert bz2.decompress(Path(str(input_file)+'.bz2').read_bytes()) == content
                        record(backend, name, case, bytes=len(content))
                    for case, path in (('missing', directory/'missing.dat'), ('oversized', sparse)):
                        run([binary, path], directory, name+'-'+case, expected=1)
                        assert not Path(str(path)+'.bz2').exists()
                        record(backend, name, case, exit=1)
                elif name == 'zlib':
                    output_file = directory/'native-output.gz'
                    output = run([binary, output_file], directory, name+'-run')
                    assert b'inflate with dictionary: hello, hello!' in output and b'large_inflate(): OK' in output
                    assert gzip.decompress(output_file.read_bytes()) == b'hello, hello!\0'
                    record(backend, name, 'all native streams and independent gzip decode')
                    run([binary, directory/'absent-parent'/'failure.gz'], directory, name+'-open-error', expected=1)
                    record(backend, name, 'output open failure', exit=1)
                elif name == 'zlib-manual':
                    outcome = console.run_console([binary], directory, env, timeout=10)
                    assert outcome['exit'] == 0 and outcome['normal_keypress'], outcome
                    assert 'crc32 checksum      : PASSED' in outcome['output'] and '100000 bytes.' in outcome['output']
                    record(backend, name, 'checksum and real wait', normal_keypress=True)
                else:
                    output = run([binary], directory, name+'-run')
                    assert b'decompressing: ok:' in output and b"string to compress (or not, since it's so short)" in output
                    record(backend, name, 'checked LZO round trip')
            print('COMPRESSION_XML_BACKEND=PASS bits='+str(args.bits)+' backend='+backend, flush=True)
        assert hashes == {name: hashlib.sha256((root/name).read_bytes()).hexdigest() for name in names}
        assert sdk_hashes == {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in sdk_paths}
        assert receipt['compiler_sha256'] == hashlib.sha256(compiler.read_bytes()).hexdigest()
        receipt['status'] = 'passed'
        save()
        print('COMPRESSION_XML_EXAMPLES=PASS bits='+str(args.bits)+' records='+str(len(records)), flush=True)
    except Exception as error:
        receipt['status'] = 'failed'
        receipt['error'] = repr(error)
        save()
        raise
    finally:
        # This absolute, prefix-checked directory belongs only to this invocation.
        task_temp.cleanup()


if __name__ == '__main__':
    main()

# end of test-compression-xml.py
