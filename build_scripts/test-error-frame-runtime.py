"""Project: FreeBASIC runtime qualification
File: test-error-frame-runtime.py
Purpose: Check the C error-frame runtime bridge on Windows.
Responsibilities: Build real Win32/Win64 objects, exercise native legacy/thread
    and allocation-failure paths, and retain all unsuppressed diagnostics.
This file intentionally does NOT install libraries or qualify BASIC outlining.
"""
import argparse
import gzip
import importlib.util
import json
import os
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('audit_c',ROOT/'build_scripts/audit-c-emitter.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

FAULT_HEADER = '''/*
 * Project: FreeBASIC runtime qualification
 * File: fault-allocator.h
 * Purpose: Inject allocation failure into the error-frame module alone.
 * Responsibilities: Read libc declarations before renaming the module's calloc.
 * This file intentionally does NOT contain: an installed allocator replacement.
 */
#include "fb.h"
void *fb_error_frame_test_calloc(size_t count, size_t bytes);
#define calloc fb_error_frame_test_calloc
/* end of fault-allocator.h */
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--resume',action='store_true')
    for name in ('gcc','clang','runtime-dir','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--configuration',action='append',choices=('standard','width32','threaded','allocation'))
    parser.add_argument('--backend',action='append',choices=('gcc','clang'))
    parser.add_argument('--optimization',action='append',choices=('O0','O2'))
    options = parser.parse_args()
    output = options.output.resolve()
    if os.name!='nt' or not output.is_relative_to(ROOT/'build'):
        parser.error('use a Windows host and an output directory below build/')
    output.mkdir(parents=True,exist_ok=options.resume)
    sources = [ROOT/'src/rtlib/errors/error_frame.c',ROOT/'src/rtlib/errors/error.c',
        ROOT/'src/rtlib/threads/thread_ctx.c',ROOT/'tests/rtlib/error-frame.c']
    tools = {name:dict(path=str(getattr(options,name).resolve()),sha256=audit.digest(getattr(options,name).resolve())) for name in ('gcc','clang')}
    runtime_files = [options.runtime_dir.resolve()/name for name in ('fbrt0.o','libfb.a','libfbmt.a')]
    headers = list((ROOT/'src/rtlib').rglob('*.h'))
    pins = {str(path):audit.digest(path) for path in [*sources,*headers,*runtime_files,
        Path(__file__).resolve(),ROOT/'build_scripts/audit-c-emitter.py']}
    pins.update({tool['path']:tool['sha256'] for tool in tools.values()})
    configurations = [
        ('standard','64',[],'libfb.a'),('width32','32',[],None),
        ('threaded','64',['-DENABLE_MT','-DTEST_MT'],'libfbmt.a'),
        ('allocation','64',['-DTEST_ALLOCATOR'],'libfb.a')]
    requested = [[configuration,backend,optimization]
        for configuration,_,_,_ in configurations
        if not options.configuration or configuration in options.configuration
        for backend in dict.fromkeys(options.backend or ('gcc','clang'))
        for optimization in dict.fromkeys(options.optimization or ('O0','O2'))]
    if options.resume:
        result = json.loads((output/'results.json').read_text())
        assert result['pins']==pins
        assert result['requested_cases']==requested
    else:
        result = dict(pins=pins,requested_cases=requested,records=[],complete=False)
    existing = {(row['configuration'],row['backend'],row['optimization']):row for row in result['records']}
    fault_header = output/'fault-allocator.h'
    if fault_header.exists():
        assert fault_header.read_text()==FAULT_HEADER
    else:
        fault_header.write_text(FAULT_HEADER)
    os.environ['PATH'] = os.pathsep.join(dict.fromkeys([
        str(Path(tools['gcc']['path']).parent),str(Path(tools['clang']['path']).parent),
        os.environ['PATH']]))
    semantics = [flag for flag in audit.SEMANTICS if flag!='-nostdinc']

    def publish():
        audit.write_json(output/'results.json',result)

    for configuration,width,defines,archive in configurations:
        if options.configuration and configuration not in options.configuration:
            continue
        for backend in dict.fromkeys(options.backend or ('gcc','clang')):
            for optimization in dict.fromkeys(options.optimization or ('O0','O2')):
                key = configuration,backend,optimization
                folder = output/('-'.join(key))
                folder.mkdir(exist_ok=key in existing)
                row = existing.get(key)
                if row is None:
                    row = dict(configuration=configuration,backend=backend,optimization=optimization,objects=[],runs=[])
                    result['records'].append(row)
                if row.get('complete'):
                    continue
                objects = []
                for index,source in enumerate(sources):
                    obj = folder/(source.stem+'.o')
                    flags = audit.GCC_WARNINGS if backend=='gcc' else ['-Weverything']
                    extra = ['-include',str(fault_header)] if configuration=='allocation' and index==0 else []
                    command = [tools[backend]['path'],*semantics,'-'+optimization,*flags,'-m'+width,
                        *defines,*extra,'-I'+str(ROOT/'src/rtlib'),'-c',str(source),'-o',str(obj)]
                    if index<len(row['objects']) and row['objects'][index].get('sha256'):
                        assert row['objects'][index]['stage']['command']==command
                        assert audit.digest(obj)==row['objects'][index]['sha256']
                    else:
                        stage = audit.invoke(command,ROOT,folder/(source.stem+'.log'),45)
                        proof = dict(source=str(source),stage=stage)
                        if index<len(row['objects']):row['objects'][index]=proof
                        else:row['objects'].append(proof)
                        publish()
                        assert stage['exit']==0,(configuration,source,stage['exit'])
                        assert int.from_bytes(obj.read_bytes()[:2],'little')=={'32':0x14c,'64':0x8664}[width]
                        # Keep inherited header diagnostics in the full log. A
                        # new warning in the bridge/header/fixture fails this gate.
                        diagnostics = gzip.decompress(Path(stage['log']).read_bytes()).decode(errors='replace')
                        own_names = {'error_frame.c','fb_error_frame.h','error-frame.c'}
                        proof['own_source_warnings'] = sum(
                            name.replace('\\','/').rsplit('/',1)[-1] in own_names
                            for name in re.findall(r'^(.*?):\d+(?::\d+)?: warning:',diagnostics,re.M))
                        assert proof['own_source_warnings']==0,(source,'new source warning')
                        proof['sha256'] = audit.digest(obj)
                        publish()
                    objects.append(obj)
                if archive:
                    exe = folder/'control.exe'
                    runtime = [options.runtime_dir.resolve()/'fbrt0.o',options.runtime_dir.resolve()/archive]
                    row['link'] = audit.invoke([tools['gcc']['path'],*map(str,objects),*map(str,runtime),
                        '-o',str(exe),'-lm','-mthreads'],ROOT,folder/'link.log',30)
                    publish()
                    assert row['link']['exit']==0
                    row['executable_sha256'] = audit.digest(exe)
                    for mode,expected in [('',0),('u',3),('r',3),('n',5)]:
                        os.environ['FBC_ERROR_FRAME_TEST_MODE'] = mode
                        stage = audit.invoke([str(exe)],ROOT,folder/('run-'+(mode or 'positive')+'.log'),8)
                        row['runs'].append(dict(mode=mode,expected=expected,stage=stage))
                        publish()
                        print('ERROR_FRAME',configuration,backend,optimization,mode or 'positive',stage['exit'],flush=True)
                        assert stage['exit']==expected
                        if not mode:
                            assert gzip.decompress(Path(stage['log']).read_bytes()).decode().strip()=='error-frame-runtime-ok'
                    exe.unlink()
                for obj in objects:
                    obj.unlink()
                row['complete'] = True
                publish()
    os.environ.pop('FBC_ERROR_FRAME_TEST_MODE',None)
    assert all(audit.digest(Path(path))==pin for path,pin in pins.items())
    result['complete'] = True
    publish()
    print('ERROR_FRAME_RUNTIME_PASS objects='+str(sum(len(row['objects']) for row in result['records']))+
        ' native_runs='+str(sum(len(row['runs']) for row in result['records'])),flush=True)


if __name__=='__main__':
    main()

# end of test-error-frame-runtime.py
