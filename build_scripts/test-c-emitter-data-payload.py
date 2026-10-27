"""Project: FreeBASIC C backend regression tests
File: test-c-emitter-data-payload.py
Purpose: Preserve DATA payload qualifiers without changing READ/RESTORE behavior.
Responsibilities: Check both C backends and widths, retain optional old-compiler
    rejection controls, and run Win64 checks when its runtime is supplied.
This file intentionally does NOT qualify other platform layouts or run games.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import time


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc',type=Path,required=True)
    parser.add_argument('--baseline',type=Path)
    parser.add_argument('--gcc',type=Path,required=True)
    parser.add_argument('--clang',type=Path,required=True)
    parser.add_argument('--runtime-dir',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(root/'build') or output == root/'build':
        parser.error('Use a new output directory below this checkout build directory')
    output.mkdir(exist_ok=False)
    spec = importlib.util.spec_from_file_location('audit_c',root/'build_scripts/audit-c-emitter.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    source = root/'tests/quirk/c-data-payload-readonly.bas'
    compilers = [('candidate',args.fbc.resolve())]
    if args.baseline:
        compilers.insert(0,('baseline',args.baseline.resolve()))
    c_tools = {'gcc':args.gcc.resolve(),'clang':args.clang.resolve()}
    runtime = []
    if args.runtime_dir:
        runtime = [(args.runtime_dir/name).resolve() for name in ('fbrt0.o','libfb.a')]
        if not all(path.is_file() for path in runtime):
            parser.error('Native checks require the Win64 runtime startup and archive')
    pins = {str(path):audit.digest(path) for path in
            [source,*c_tools.values(),*(path for _,path in compilers),*runtime]}
    rows = []
    result = dict(source=str(source),source_sha256=pins[str(source)],pins=pins,records=rows,
        runtime_scope='Win64 only; Win32 has real target-width object checks')

    def save():
        # A Windows indexer may briefly open a new receipt without delete sharing.
        # Retry only the metadata replace, never an already completed compiler.
        for attempt in range(8):
            try:
                audit.write_json(output/'results.json',result)
                return
            except PermissionError:
                if attempt == 7:
                    raise
                time.sleep(min(0.05*(2**attempt),0.4))

    for version,fbc in compilers:
        for target in ('win32','win64'):
            for backend in ('gcc','clang'):
                folder = output/(version+'-'+target+'-'+backend)
                folder.mkdir()
                cfile = folder/'unit.c'
                row = dict(version=version,target=target,backend=backend,cases=[])
                rows.append(row)
                row['emit'] = audit.invoke([str(fbc),'-prefix',str(root),'-gen',backend,
                    '-target',target,'-r','-w','all','-m',source.stem,
                    str(source),'-o',str(folder/'unit.o')],root,folder/'emit.log',30)
                save()
                assert row['emit']['exit'] == 0
                assert row['emit']['log_bytes'] == 0,'Unexpected BASIC diagnostic in the DATA reduction'
                row['c_sha256'] = audit.digest(cfile)
                for tool,compiler in c_tools.items():
                    for optimization in (('O2',) if version=='baseline' else ('O0','O2')):
                        stem = tool+'-'+optimization
                        obj = folder/(stem+'.o')
                        warnings = audit.GCC_WARNINGS if tool=='gcc' else ['-Weverything','-Wwrite-strings']
                        case = dict(tool=tool,optimization=optimization)
                        row['cases'].append(case)
                        case['object'] = audit.invoke([str(compiler),*audit.SEMANTICS,
                            '-m32' if target=='win32' else '-m64','-'+optimization,*warnings,
                            '-Werror=cast-qual','-fdiagnostics-color=never','-c',str(cfile),
                            '-o',str(obj)],folder,folder/(stem+'.log'),60)
                        save()
                        stage = case['object']
                        if version=='baseline':
                            assert stage['exit'] != 0
                            assert any('cast-qual' in item['flag'] and item['severity']=='error'
                                       for item in stage['counts'])
                            continue
                        assert stage['exit'] == 0
                        assert not any('cast-qual' in item['flag'] for item in stage['counts'])
                        with obj.open('rb') as stream:
                            assert stream.read(2) == (b'\x4c\x01' if target=='win32' else b'\x64\x86')
                        case['object_sha256'] = audit.digest(obj)
                        if runtime and target=='win64':
                            exe = folder/(stem+'.exe')
                            case['link'] = audit.invoke([str(c_tools['gcc']),str(obj),*map(str,runtime),
                                '-o',str(exe),'-lm','-mthreads'],folder,folder/(stem+'-link.log'),30)
                            assert case['link']['exit'] == 0
                            case['executable_sha256'] = audit.digest(exe)
                            case['run'] = audit.invoke([str(exe)],folder,folder/(stem+'-run.log'),10)
                            assert case['run']['exit'] == 0
                        save()
                assert audit.digest(cfile) == row['c_sha256']
                print('DATA_PAYLOAD_CHECK',version,target,backend,flush=True)
    assert all(audit.digest(Path(path))==pin for path,pin in pins.items())
    result['complete'] = True
    save()
    print('C_EMITTER_DATA_PAYLOAD_PASS',
        sum(len(row['cases']) for row in rows if row['version']=='candidate'),'objects',
        sum('run' in case for row in rows for case in row['cases']),'native runs',flush=True)
    return 0


if __name__=='__main__':
    raise SystemExit(main())

# end of test-c-emitter-data-payload.py
