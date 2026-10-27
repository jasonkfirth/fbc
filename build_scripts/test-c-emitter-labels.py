"""Project: FreeBASIC C backend regression tests
File: test-c-emitter-labels.py
Purpose: Verify retained ASM/GOSUB targets and removal of unused C labels.
Responsibilities: Check real Win32/Win64 objects, debug source tables and native
    control flow at O0/O2, retaining the full unsuppressed warning profiles.
This file intentionally does NOT install tools or execute platform I/O tests.
"""
import argparse
import gzip
import importlib.util
import json
from pathlib import Path
import re


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc','gcc','clang','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--baseline',type=Path)
    parser.add_argument('--runtime-dir',type=Path)
    parser.add_argument('--resume',action='store_true')
    options = parser.parse_args()
    output = options.output.resolve()
    if not output.is_relative_to(root/'build'):
        parser.error('output must be below build/')
    output.mkdir(parents=True,exist_ok=options.resume)
    spec = importlib.util.spec_from_file_location('audit_c',root/'build_scripts/audit-c-emitter.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    sources = [(root/'tests/quirk/c-label-entries.bas','c-label-entries-ok'),
               (root/'tests/quirk/c-branch-exits.bas','c-branch-exits-ok'),
               (root/'tests/semantic-sidecar/gosub-computed.bas','computed-gosub-ok')]
    names = ['fbc','gcc','clang']+(['baseline'] if options.baseline else [])
    tools = {name:dict(path=str(getattr(options,name).resolve()),
        sha256=audit.digest(getattr(options,name).resolve())) for name in names}
    source_pins = {str(path):audit.digest(path) for path,_ in sources}
    runtime = options.runtime_dir.resolve() if options.runtime_dir else None
    runtime_pins = ({str(runtime/name):audit.digest(runtime/name)
        for name in ('fbrt0.o','libfb.a')} if runtime else {})
    versions = [('candidate',options.fbc.resolve())]
    if options.baseline:
        versions.insert(0,('baseline',options.baseline.resolve()))
    records = []
    executions = 0
    existing = {}
    if options.resume:
        previous = json.loads((output/'results.json').read_text())
        assert previous['tools']==tools and previous['source_pins']==source_pins
        assert previous['runtime_pins']==runtime_pins
        records,executions = previous['records'],previous['executions']
        existing = {tuple(row[k] for k in ('version','source','debug','target','backend')):row
                    for row in records}
        assert len(existing)==len(records)

    def publish():
        audit.write_json(output/'results.json',dict(records=records,tools=tools,
            source_pins=source_pins,runtime_pins=runtime_pins,executions=executions))

    for version,compiler in versions:
        for source,expected_text in sources:
            for debug in (False,True):
                for target,width in [('win32','32'),('win64','64')]:
                    for backend in ('gcc','clang'):
                        folder = output/f'{version}-{source.stem}-{target}-{backend}-debug{int(debug)}'
                        key = (version,str(source),debug,target,backend)
                        folder.mkdir(exist_ok=key in existing)
                        row = existing.get(key)
                        if row is None:
                            row = dict(version=version,source=str(source),debug=debug,
                                target=target,backend=backend,cases=[])
                            records.append(row)
                        command = [str(compiler),'-prefix',str(root),'-gen',backend,
                            '-target',target,'-r','-m',source.stem,'-asm','intel']
                        if debug:
                            command += ['-g']
                        command += [str(source),'-o',str(folder/'unit.o')]
                        if 'emit' in row:
                            assert row['emit']['command']==command
                        else:
                            row['emit'] = audit.invoke(command,folder,folder/'emit.log',30)
                        publish()
                        assert row['emit']['exit']==0
                        cfile = folder/'unit.c'
                        if 'c_sha256' in row:
                            assert row['c_sha256']==audit.digest(cfile)
                        else:
                            row['c_sha256'] = audit.digest(cfile)
                        if source.name=='c-label-entries.bas':
                            assert re.search(r'\b(?:__asm__|asm)\s+(?:[A-Za-z_]+\s+)*goto\s*\(',
                                             cfile.read_text()),'ASM target profile did not emit asm goto'
                        for optimization in ('O0','O2'):
                            warnings = (audit.GCC_WARNINGS+['-fno-diagnostics-show-caret'] if backend=='gcc'
                                        else ['-Weverything','-fno-caret-diagnostics'])
                            if version=='candidate':
                                warnings += ['-Werror=unused-label']
                            obj = folder/(optimization+'.o')
                            found = [case for case in row['cases'] if case['optimization']==optimization]
                            assert len(found)<=1
                            case = found[0] if found else dict(optimization=optimization)
                            if not found:
                                row['cases'].append(case)
                            flags = [*audit.SEMANTICS,'-'+optimization,'-m'+width,'-masm=intel',
                                *warnings,'-fdiagnostics-color=never']+(['-g'] if debug else [])
                            command = [tools[backend]['path'],*flags,'-c','unit.c','-o',str(obj)]
                            if 'object' in case:
                                assert case['object']['command']==command
                            else:
                                case['object'] = audit.invoke(command,folder,folder/(optimization+'.log'),30)
                            publish()
                            assert case['object']['exit']==0
                            if obj.exists():
                                payload = obj.read_bytes()
                                if 'object_sha256' in case:
                                    assert case['object_sha256']==audit.digest(obj)
                                case['COFF_machine'] = int.from_bytes(payload[:2],'little')
                                case['object_sha256'] = audit.digest(obj)
                                if debug:
                                    assert b'.debug_line' in payload
                                    assert source.name.encode().lower() in payload.lower()
                                    case['debug_source_table_verified'] = True
                            assert case['COFF_machine']=={'32':0x14c,'64':0x8664}[width]
                            if debug:
                                assert case['debug_source_table_verified']
                            publish()
                            if runtime and target=='win64':
                                exe = folder/(optimization+'.exe')
                                command = [tools['gcc']['path'],str(obj),*runtime_pins,
                                    '-o',str(exe),'-lm','-mthreads']
                                if 'link' in case:
                                    assert case['link']['command']==command
                                else:
                                    assert obj.exists()
                                    case['link'] = audit.invoke(command,folder,folder/(optimization+'-link.log'),30)
                                publish()
                                assert case['link']['exit']==0
                                if 'native' not in case:
                                    case['native'] = audit.invoke([str(exe)],folder,
                                        folder/(optimization+'-run.log'),10)
                                    executions += 1
                                publish()
                                assert case['native']['exit']==0
                                assert gzip.decompress(Path(case['native']['log']).read_bytes()).decode().strip()==expected_text
                                if exe.exists():
                                    exe.unlink()
                            if obj.exists():
                                obj.unlink()
    assert all(audit.digest(Path(path))==pin for path,pin in source_pins.items())
    assert all(audit.digest(Path(tool['path']))==tool['sha256'] for tool in tools.values())
    assert all(audit.digest(Path(path))==pin for path,pin in runtime_pins.items())
    publish()
    print(f'C_EMITTER_LABELS_PASS emissions={len(records)} objects={2*len(records)} executions={executions}')
    return 0


if __name__=='__main__':
    raise SystemExit(main())

# end of test-c-emitter-labels.py
