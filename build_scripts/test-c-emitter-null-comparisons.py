"""Project: FreeBASIC C backend regression tests
File: test-c-emitter-null-comparisons.py
Purpose: Check null equality lowering for nested qualified pointer types.
Responsibilities: Retain specific old Clang failures, build both C backends at
    both widths and run Win64 null/non-null value controls when a runtime is supplied.
This file intentionally does NOT remove argument casts or suppress warnings.
"""
import argparse
import importlib.util
import json
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc',type=Path,required=True)
    parser.add_argument('--baseline',type=Path)
    parser.add_argument('--gcc',type=Path,required=True)
    parser.add_argument('--clang',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--runtime-dir',type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(root/'build'):
        parser.error('Output must be below build/')
    output.mkdir(parents=True,exist_ok=False)
    spec = importlib.util.spec_from_file_location('audit_c',root/'build_scripts/audit-c-emitter.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    source = root/'tests/pointers/const-nested-null-comparisons.bas'
    source_sha = audit.digest(source)
    compilers = [('candidate',args.fbc.resolve())]
    if args.baseline:
        compilers.insert(0,('baseline',args.baseline.resolve()))
    tools = dict(gcc=args.gcc.resolve(),clang=args.clang.resolve())
    pins = {str(path):audit.digest(path) for path in [source,*tools.values(),*(p for _,p in compilers)]}
    runtime = []
    if args.runtime_dir:
        runtime = [(args.runtime_dir/name).resolve() for name in ('fbrt0.o','libfb.a')]
        if not all(path.is_file() for path in runtime):
            parser.error('Win64 native checks require startup and runtime archive files')
        pins.update({str(path):audit.digest(path) for path in runtime})
    rows = []
    result = dict(source=str(source),source_sha256=source_sha,pins=pins,records=rows)
    for version,fbc in compilers:
        for target,width in (('win32','32'),('win64','64')):
            for backend in ('gcc','clang'):
                folder = output/(version+'-'+target+'-'+backend)
                folder.mkdir()
                row = dict(version=version,target=target,backend=backend,objects=[])
                row['emit'] = audit.invoke([str(fbc),'-prefix',str(root),'-gen',backend,
                    '-target',target,'-r','-m',source.stem,str(source),'-o',str(folder/'unit.o')],
                    root,folder/'emit.log',30)
                rows.append(row)
                audit.write_json(output/'results.json',result)
                assert row['emit']['exit'] == 0
                cfile = folder/'unit.c'
                row['c_sha256'] = audit.digest(cfile)
                if version == 'baseline':
                    row['rejection'] = audit.invoke([str(tools['clang']),*audit.SEMANTICS,
                        '-m'+width,'-Weverything','-Werror=compare-distinct-pointer-types',
                        '-fno-caret-diagnostics','-fsyntax-only',str(cfile)],
                        folder,folder/'baseline.log',30)
                    assert row['rejection']['exit'] != 0
                    assert any('compare-distinct-pointer-types' in count['flag'] and count['severity'] == 'error'
                               for count in row['rejection']['counts'])
                else:
                    for tool in ('gcc','clang'):
                        for optimization in ('O0','O2'):
                            stem = tool+'-'+optimization
                            obj = folder/'unit.o'
                            warnings = audit.GCC_WARNINGS if tool == 'gcc' else [
                                '-Weverything','-Werror=compare-distinct-pointer-types','-fno-caret-diagnostics']
                            case = dict(tool=tool,optimization=optimization)
                            case['object'] = audit.invoke([str(tools[tool]),*audit.SEMANTICS,
                                '-m'+width,'-'+optimization,*warnings,'-fdiagnostics-color=never',
                                '-c','unit.c','-o',str(obj)],folder,folder/(stem+'.log'),30)
                            row['objects'].append(case)
                            audit.write_json(output/'results.json',result)
                            assert case['object']['exit'] == 0
                            assert not any('compare-distinct-pointer-types' in count['flag']
                                           for count in case['object']['counts'])
                            case['object_sha256'] = audit.digest(obj)
                            if runtime and target == 'win64':
                                exe = folder/'control.exe'
                                case['link'] = audit.invoke([str(tools['gcc']),str(obj),*map(str,runtime),
                                    '-o',str(exe),'-lm','-mthreads'],folder,folder/(stem+'-link.log'),30)
                                assert case['link']['exit'] == 0
                                case['executable_sha256'] = audit.digest(exe)
                                case['run'] = audit.invoke([str(exe)],folder,folder/(stem+'-run.log'),10)
                                assert case['run']['exit'] == 0
                assert audit.digest(cfile) == row['c_sha256']
                audit.write_json(output/'results.json',result)
                print('NULL_COMPARISON_CHECK',version,target,backend,flush=True)
    assert all(audit.digest(Path(path)) == pin for path,pin in pins.items())
    result['complete'] = True
    audit.write_json(output/'results.json',result)
    print('C_EMITTER_NULL_COMPARISONS_PASS',
        sum(len(row['objects']) for row in rows),'objects',
        sum('run' in case for row in rows for case in row['objects']),'native runs',flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-c-emitter-null-comparisons.py
