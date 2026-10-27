"""Project: FreeBASIC C backend regression tests
File: test-c-emitter-warning-policy.py
Purpose: Keep incompatible-pointer diagnostics visible through the normal driver.
Responsibilities: Build valid qualifier fixtures with both C backends/widths,
    inject one incompatible C declaration and compare an optional old compiler.
This file intentionally does NOT enable unrelated warnings or run invalid C.
"""
import argparse
import gzip
import importlib.util
import json
import os
from pathlib import Path
import re


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc',type=Path,required=True)
    parser.add_argument('--baseline',type=Path)
    parser.add_argument('--gcc',type=Path,required=True)
    parser.add_argument('--clang',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--runtime-dir',type=Path)
    parser.add_argument('--asm',choices=('intel','att'),default='intel')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(root/'build'):
        parser.error('Output must be below build/')
    output.mkdir(parents=True,exist_ok=False)
    spec = importlib.util.spec_from_file_location('audit_c',root/'build_scripts/audit-c-emitter.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    source_original = root/'tests/pointers/const-nested-arg-casts.bas'
    source_pin = audit.digest(source_original)
    source = output/source_original.name
    source.write_bytes(source_original.read_bytes())
    probe = output/'incompatible-pointer-probe.h'
    probe.write_text('/* Project: C driver regression. File: incompatible-pointer-probe.h.\n'
        '   Purpose: Exercise pointer diagnostics. Responsibilities: Supply one bad\n'
        '   C initializer. This file intentionally does NOT execute any code. */\n'
        'static void pointer_warning_probe(void)\n{\n'
        '    int value = 0;\n    double *pointer = &value;\n    (void)pointer;\n}\n'
        '/* end of incompatible-pointer-probe.h */\n')
    compilers = [('candidate',args.fbc.resolve())]
    if args.baseline:
        compilers.insert(0,('baseline',args.baseline.resolve()))
    c_tools = dict(gcc=args.gcc.resolve(),clang=args.clang.resolve())
    assembler = c_tools['gcc'].parent/'as.exe'
    pins = {str(path):audit.digest(path) for path in [source,probe,assembler,*c_tools.values(),
            *(path for _,path in compilers)]}
    runtime = []
    if args.runtime_dir:
        runtime = [(args.runtime_dir/name).resolve() for name in ('fbrt0.o','libfb.a')]
        if not all(path.is_file() for path in runtime):
            parser.error('Native checks require the Win64 runtime startup and archive')
        pins.update({str(path):audit.digest(path) for path in runtime})
    rows = []
    result = dict(source=str(source_original),source_sha256=source_pin,pins=pins,records=rows,
        assembly_syntax=args.asm,
        runtime_scope='Win64 only; both widths have real normal-driver object checks')
    old_environment = {key:os.environ.get(key) for key in ('GCC','CLANG','AS','PATH')}
    os.environ.update(GCC=str(c_tools['gcc']),CLANG=str(c_tools['clang']),AS=str(assembler))
    os.environ['PATH'] = str(assembler.parent)+os.pathsep+os.environ['PATH']
    try:
        for version,fbc in compilers:
            for target in ('win32','win64'):
                for backend in ('gcc','clang'):
                    for injected in (False,True):
                        folder = output/(version+'-'+target+'-'+backend+('-probe' if injected else '-valid'))
                        folder.mkdir()
                        obj = folder/'unit.o'
                        command = [str(fbc),'-prefix',str(root),'-gen',backend,'-target',target,
                            '-asm',args.asm,'-O','2','-v','-R',
                            '-rr' if version == 'baseline' else '-c','-m',source.stem]
                        if injected:
                            command += ['-Wc','-include "'+str(probe)+'"']
                        command += [str(source),'-o',str(obj)]
                        stage = audit.invoke(command,root,folder/'driver.log',60)
                        row = dict(version=version,target=target,backend=backend,
                                   build_mode='C-to-assembly' if version == 'baseline' else 'normal object',
                                   injected_incompatible_pointer=injected,driver=stage)
                        rows.append(row)
                        audit.write_json(output/'results.json',result)
                        with gzip.open(stage['log'],'rt',errors='replace') as stream:
                            log = stream.read()
                        masked = '-Wno-incompatible-pointer-types' in log
                        assert masked == (version == 'baseline'),(version,target,backend,log[-1500:])
                        pointer_diagnostic = any(item['flag'] == '-Wincompatible-pointer-types'
                                                 for item in stage['counts'])
                        if injected and version == 'candidate':
                            assert pointer_diagnostic,(target,backend,stage)
                        else:
                            assert stage['exit'] == 0,stage
                            if version == 'candidate':
                                assert obj.is_file()
                                # PE/COFF machine values distinguish a real
                                # target-width object from a host-default one.
                                expected_machine = b'\x4c\x01' if target == 'win32' else b'\x64\x86'
                                assert obj.read_bytes()[:2] == expected_machine
                            else:
                                # Some existing target/backend paths already
                                # produce an object during the C stage.
                                assert obj.is_file() or (folder/'unit.asm').is_file()
                            assert not pointer_diagnostic,(version,target,backend,stage)
                            if version == 'candidate' and not injected:
                                assert not any(item['severity'] in ('warning','error','fatal error')
                                               for item in stage['counts']),stage
                                assert not re.search(r'(?m)(?:^|\s)(?:warning|error|fatal error):',log),log
                        cfile = folder/'unit.c'
                        assert cfile.is_file(),'Normal driver did not retain C under its owned object directory'
                        row['c_sha256'] = audit.digest(cfile)
                        row['pointer_diagnostic_visible'] = pointer_diagnostic
                        if not injected and runtime and target == 'win64' and version == 'candidate':
                            exe = folder/'control.exe'
                            row['link'] = audit.invoke([str(c_tools['gcc']),str(obj),*map(str,runtime),
                                '-o',str(exe),'-lm','-mthreads'],folder,folder/'link.log',30)
                            assert row['link']['exit'] == 0
                            row['executable_sha256'] = audit.digest(exe)
                            row['run'] = audit.invoke([str(exe)],folder,folder/'run.log',10)
                            assert row['run']['exit'] == 0
                        audit.write_json(output/'results.json',result)
                        print('DRIVER_POINTER_POLICY',version,target,backend,injected,
                              stage['exit'],pointer_diagnostic,flush=True)
        if args.baseline:
            for row in rows:
                if row['version'] != 'candidate':
                    continue
                previous = next(old for old in rows if old['version'] == 'baseline'
                    and (old['target'],old['backend'],old['injected_incompatible_pointer']) ==
                        (row['target'],row['backend'],row['injected_incompatible_pointer']))
                # Stacked emitter repairs can legitimately change the C. Keep
                # the comparison visible while checking diagnostics and native
                # behavior independently of that spelling.
                row['baseline_C_identical'] = previous['c_sha256'] == row['c_sha256']
        assert audit.digest(source_original) == source_pin
        assert all(audit.digest(Path(path)) == pin for path,pin in pins.items())
        result['complete'] = True
        audit.write_json(output/'results.json',result)
        print('C_EMITTER_WARNING_POLICY_PASS',len(rows),'driver checks',
              sum('run' in row for row in rows),'native runs',flush=True)
    finally:
        for key,value in old_environment.items():
            if value is None:
                os.environ.pop(key,None)
            else:
                os.environ[key] = value
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of test-c-emitter-warning-policy.py
