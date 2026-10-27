"""Project: FreeBASIC C backend regression tests
File: test-c-emitter-termination.py
Purpose: Verify termination declarations and generated fallthrough cleanup.
Responsibilities: Check END/SYSTEM/STOP, real Win32/Win64 objects, alternate
    label/jump-table entries and native Win64 returning/terminating paths.
This file intentionally does NOT suppress warnings or replace runtime libraries.
"""
import argparse
from collections import Counter
import importlib.util
import json
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc', 'gcc', 'clang', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--baseline-results', type=Path)
    parser.add_argument('--runtime-dir', type=Path)
    parser.add_argument('--resume', action='store_true')
    parser.add_argument('--forward', action='store_true')
    options = parser.parse_args()
    output = options.output.resolve()
    if not output.is_relative_to(root/'build'):
        parser.error('output must be below build/')
    if options.runtime_dir:
        options.runtime_dir = options.runtime_dir.resolve()
        if not all((options.runtime_dir/name).is_file() for name in ('fbrt0.o','libfb.a')):
            parser.error('runtime checks require the Win64 runtime objects')
    output.mkdir(parents=True,exist_ok=options.resume)
    spec = importlib.util.spec_from_file_location('audit_c',root/'build_scripts/audit-c-emitter.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    source = root/'tests/quirk'/('c-termination-forward.bas' if options.forward else 'c-end-noreturn.bas')
    source_pin = audit.digest(source)
    names = ['fbc','gcc','clang']+(['baseline'] if options.baseline else [])
    tools = {name:dict(path=str(getattr(options,name).resolve()),
                       sha256=audit.digest(getattr(options,name).resolve())) for name in names}
    runtime_pins = ({str(options.runtime_dir/name):audit.digest(options.runtime_dir/name)
                    for name in ('fbrt0.o','libfb.a')} if options.runtime_dir else {})
    versions = [('candidate',options.fbc.resolve())]
    if options.baseline:
        versions.insert(0,('baseline',options.baseline.resolve()))
    records, texts = [], {}
    resume_note = None
    if options.resume:
        previous = json.loads((output/'results.json').read_text())
        assert previous['source_sha256']==source_pin and previous['runtime_pins']==runtime_pins
        records = previous['records']
        # A candidate which has not run yet is not an input to the retained
        # baseline stages. Keep its old identity explicit when changing it.
        differences = {key for key in tools if tools[key]!=previous['tools'][key]}
        if differences:
            assert differences=={'fbc'} and all(row['version']=='baseline' for row in records)
        resume_note = dict(previous_tools=previous['tools'],unused_candidate_replaced=bool(differences),
            retained_records=len(records))
    elif options.baseline_results:
        assert options.baseline
        previous = json.loads(options.baseline_results.read_text())
        assert previous['source_sha256']==source_pin and previous['runtime_pins']==runtime_pins
        assert all(previous['tools'][name]==tools[name] for name in ('baseline','gcc','clang'))
        records = [row for row in previous['records'] if row['version']=='baseline']
        assert len(records)==20 and all(len(row['cases'])==2 for row in records)
        resume_note = dict(baseline_results=str(options.baseline_results.resolve()),
            baseline_results_sha256=audit.digest(options.baseline_results),retained_baseline_records=len(records))
    def key(row):
        return tuple(row[name] for name in ('version','kind','implicit','library','target','backend'))
    existing = {key(row):row for row in records}
    assert len(existing)==len(records)
    executions = sum(mode in case for row in records for case in row['cases']
                     for mode in ('return','end','tail','branch'))

    def publish():
        audit.write_json(output/'results.json',dict(records=records,tools=tools,
            source_sha256=source_pin,runtime_pins=runtime_pins,executions=executions,
            resume=resume_note))

    for version,compiler in versions:
        profiles = ([(0,False,False)] if options.forward else
                    [(kind,False,False) for kind in range(3)]+[(0,True,False),(0,False,True)])
        for kind,implicit,library in profiles:
            for target,width in [('win32','32'),('win64','64')]:
                for backend in ('gcc','clang'):
                    suffix = 'library' if library else ('implicit' if implicit else str(kind))
                    work = output/f'{version}-{suffix}-{target}-{backend}'
                    row = dict(version=version,kind=kind,implicit=implicit,library=library,
                        target=target,backend=backend,cases=[])
                    if key(row) in existing:
                        work = Path(existing[key(row)]['emit']['cwd'])
                    work.mkdir(exist_ok=options.resume or key(row) in existing)
                    command = [str(compiler),'-prefix',str(root),'-gen',backend,'-target',target,'-r']
                    if library:
                        command += ['-dll','-d','TERMINATION_LIBRARY']
                    else:
                        command += ['-m',source.stem,'-d',f'TERMINATION_KIND={kind}']
                    if implicit:
                        command += ['-d','TERMINATION_IMPLICIT_MAIN']
                    command += [str(source),'-o',str(work/'unit.o')]
                    if key(row) in existing:
                        row = existing[key(row)]
                        assert row['emit']['command']==command
                        assert audit.digest(work/'unit.c')==row['c_sha256']
                    else:
                        records.append(row)
                        row['emit'] = audit.invoke(command,work,work/'emit.log',30)
                        publish()
                    assert row['emit']['exit']==0
                    assert not any(x['severity'] in ('error','warning') for x in row['emit']['counts'])
                    cfile = work/'unit.c'
                    text = cfile.read_text()
                    texts[version,suffix,target,backend] = text
                    row['c_sha256'] = audit.digest(cfile)
                    declarations = [line for line in text.splitlines()
                                    if line.startswith('void ') and ' fb_End(' in line]
                    if not library:
                        assert declarations
                        assert all(('__attribute__((noreturn))' in line)==(version=='candidate')
                                   for line in declarations)
                    for optimization in ('O0','O2'):
                        flags = (audit.GCC_WARNINGS+['-fno-diagnostics-show-caret'] if backend=='gcc'
                                 else ['-Weverything','-fno-caret-diagnostics'])
                        if version=='candidate':
                            flags += ['-Werror=null-dereference','-Werror=return-type']
                            if backend=='clang':
                                flags += ['-Werror=unreachable-code','-Werror=missing-noreturn']
                        native = options.runtime_dir and target=='win64' and not library
                        obj = work/(optimization+'.o')
                        command = [tools[backend]['path'],*audit.SEMANTICS,'-'+optimization,
                            *flags,'-fdiagnostics-color=never','-m'+width,
                            '-c','unit.c','-o',str(obj)]
                        found = [case for case in row['cases'] if case['optimization']==optimization]
                        assert len(found)<=1
                        case = found[0] if found else dict(optimization=optimization)
                        if not found:
                            row['cases'].append(case)
                        if 'object' in case:
                            assert case['object']['command']==command and case['object']['exit']==0
                            if not native or all(mode in case for mode in ('return','end','tail','branch')):
                                if native:
                                    assert case['link']['exit']==0
                                    assert [case[mode]['exit'] for mode in ('return','end','tail','branch')]==[0,37,38,39]
                                continue
                        else:
                            case['object'] = audit.invoke(command,work,work/(optimization+'.log'),30)
                            publish()
                        assert case['object']['exit']==0
                        machine = int.from_bytes(obj.read_bytes()[:2],'little')
                        assert machine=={'32':0x14c,'64':0x8664}[width]
                        case['object_sha256'] = audit.digest(obj)
                        case['COFF_machine'] = machine
                        if native:
                            exe = work/(optimization+'.exe')
                            command = [tools['gcc']['path'],str(obj),
                                str(options.runtime_dir/'fbrt0.o'),str(options.runtime_dir/'libfb.a'),
                                '-o',str(exe),'-lm','-mthreads']
                            if 'link' not in case:
                                case['link'] = audit.invoke(command,work,work/(optimization+'-link.log'),30)
                                publish()
                            else:
                                assert case['link']['command']==command
                            assert case['link']['exit']==0
                            for mode,arguments,expected in [('return',[],0),('end',['end'],37),
                                                           ('tail',['tail'],38),('branch',['branch'],39)]:
                                if mode not in case:
                                    case[mode] = audit.invoke([str(exe),*arguments],work,
                                        work/(optimization+'-'+mode+'.log'),10)
                                    executions += 1
                                    publish()
                                assert case[mode]['exit']==expected
                            exe.unlink()
                        obj.unlink()
    if options.baseline and not options.forward:
        # Deferred C emission can reorder whole procedure definitions. Keep all
        # DLL declarations and procedure bodies identical, including result loads.
        for target in ('win32','win64'):
            for backend in ('gcc','clang'):
                candidate = Counter(block.strip() for block in texts['candidate','library',target,backend].split('\n\n') if block.strip())
                baseline = Counter(block.strip() for block in texts['baseline','library',target,backend].split('\n\n') if block.strip())
                assert candidate==baseline
    assert audit.digest(source)==source_pin
    assert all(audit.digest(Path(tool['path']))==tool['sha256'] for tool in tools.values())
    assert all(audit.digest(Path(path))==pin for path,pin in runtime_pins.items())
    publish()
    print(f'C_EMITTER_TERMINATION_PASS emissions={len(records)} objects={2*len(records)} executions={executions}')
    return 0


if __name__=='__main__':
    raise SystemExit(main())

# end of test-c-emitter-termination.py
