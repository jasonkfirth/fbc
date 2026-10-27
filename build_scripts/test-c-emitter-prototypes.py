"""Project: FreeBASIC C backend regression tests
File: test-c-emitter-prototypes.py
Purpose: Verify declarations for emitted public definitions without BASIC callers.
Responsibilities: Compile real Win32/Win64 objects, retain unsuppressed diagnostics,
    compare unchanged object bytes, and call exports from a separate native C unit.
This file intentionally does NOT install a compiler or replace runtime libraries.
"""
import argparse
import importlib.util
from pathlib import Path
import re


DRIVER = r'''/*
    Project: FreeBASIC C backend regression tests
    File: driver.c
    Purpose: Call public BASIC definitions from a separate C translation unit.
    Responsibilities: Verify scalar, pointer, structure and calling-convention ABI.
    This file intentionally does NOT define the BASIC procedures it calls.
*/
typedef signed int int32_t;
_Static_assert(sizeof(int32_t) == 4, "FreeBASIC LONG is a 32-bit type");

struct FB_AUDIT_PAIR_TAG {
    int32_t LEFT_VALUE;
    int32_t RIGHT_VALUE;
};
_Static_assert(sizeof(struct FB_AUDIT_PAIR_TAG) == 8, "two FreeBASIC LONG fields");

#if defined(__i386__)
#define FB_STDCALL __attribute__((stdcall))
#else
#define FB_STDCALL
#endif

int32_t fb_audit_exported_value(int32_t value);
int32_t fb_audit_declared_value(int32_t value);
int32_t fb_audit_pair_sum(struct FB_AUDIT_PAIR_TAG *value);
const int32_t *fb_audit_pointer(const int32_t *value);
int32_t fb_audit_namespace_value(int32_t value);
int32_t FB_STDCALL fb_audit_stdcall_value(int32_t value);
void fb_audit_end(void);
int32_t fb_audit_check_exports(int32_t terminate);

int32_t fb_audit_check_exports(int32_t terminate)
{
    const int32_t value = 11;
    struct FB_AUDIT_PAIR_TAG pair = {3, 7};

    if (fb_audit_exported_value(value) != 12)
        return 1;
    if (fb_audit_declared_value(value) != 13)
        return 2;
    if (fb_audit_pair_sum(&pair) != 10 || fb_audit_pair_sum(0) != 0)
        return 3;
    if (fb_audit_pointer(&value) != &value)
        return 4;
    if (fb_audit_namespace_value(value) != 14)
        return 5;
    if (fb_audit_stdcall_value(value) != 15)
        return 6;
    if (terminate) {
        fb_audit_end();
        return 98;
    }
    return 0;
}

/* end of driver.c */
'''


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fbc','gcc','clang','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--baseline',type=Path)
    parser.add_argument('--runtime-dir',type=Path)
    options = parser.parse_args()
    output = options.output.resolve()
    if not output.is_relative_to(root/'build'):
        parser.error('output must be below build/')
    output.mkdir(parents=True,exist_ok=False)
    spec = importlib.util.spec_from_file_location('audit_c',root/'build_scripts/audit-c-emitter.py')
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    source = root/'tests/quirk/c-emitted-prototypes.bas'
    source_pin = audit.digest(source)
    names = ['fbc','gcc','clang']+(['baseline'] if options.baseline else [])
    tools = {name:dict(path=str(getattr(options,name).resolve()),
        sha256=audit.digest(getattr(options,name).resolve())) for name in names}
    runtime = options.runtime_dir.resolve() if options.runtime_dir else None
    runtime_pins = ({str(runtime/name):audit.digest(runtime/name)
        for name in ('fbrt0.o','libfb.a')} if runtime else {})
    versions = [('candidate',options.fbc.resolve())]
    if options.baseline:
        versions.insert(0,('baseline',options.baseline.resolve()))
    records, objects = [], {}
    executions = 0

    def publish():
        audit.write_json(output/'results.json',dict(records=records,tools=tools,
            source_sha256=source_pin,runtime_pins=runtime_pins,executions=executions))

    for version,compiler in versions:
        for target,width in [('win32','32'),('win64','64')]:
            for backend in ('gcc','clang'):
                work = output/f'{version}-{target}-{backend}'
                work.mkdir()
                row = dict(version=version,target=target,backend=backend,cases=[])
                records.append(row)
                row['emit'] = audit.invoke([str(compiler),'-prefix',str(root),'-gen',backend,
                    '-target',target,'-r','-m',source.stem,'-d','C_EXPORT_DRIVER',str(source),
                    '-o',str(work/'unit.o')],work,work/'emit.log',30)
                publish()
                assert row['emit']['exit']==0
                cfile = work/'unit.c'
                text = cfile.read_text()
                row['c_sha256'] = audit.digest(cfile)
                # C requires matching tags and member names across translation
                # units. Retain the emitter's tag while independently declaring
                # the two-LONG layout and checking its size in the C caller.
                match = re.search(r'\bfb_audit_pair_sum\(\s*struct\s+([A-Za-z0-9_$]+)\s*\*',text)
                assert match
                for name in ('exported_value','declared_value','pair_sum','pointer',
                             'namespace_value','stdcall_value','end'):
                    spelling = r'\bfb_audit_'+name+r'\s*\('
                    alias = r'\basm\("_?fb_audit_'+name+r'(?:@\d+)?"\)'
                    assert re.search(spelling,text) or re.search(alias,text),name
                assert 'fb_audit_unused_external' not in text and 'fb_audit_unused_private' not in text
                (work/'driver.c').write_text(DRIVER.replace('FB_AUDIT_PAIR_TAG',match[1]))
                row['driver_sha256'] = audit.digest(work/'driver.c')
                for optimization in ('O0','O2'):
                    flags = (audit.GCC_WARNINGS+['-fno-diagnostics-show-caret'] if backend=='gcc'
                             else ['-Weverything','-fno-caret-diagnostics'])
                    flags += ['-fdiagnostics-color=never']
                    gates = ['-Werror=missing-prototypes','-Werror=missing-declarations',
                             '-Werror=strict-prototypes']
                    common = [tools[backend]['path'],*audit.SEMANTICS,'-'+optimization,
                        *flags,'-m'+width]
                    obj = work/(optimization+'.o')
                    case = dict(optimization=optimization)
                    row['cases'].append(case)
                    case['object'] = audit.invoke([*common,*(gates if version=='candidate' else []),
                        '-c','unit.c','-o',str(obj)],work,work/(optimization+'.log'),30)
                    publish()
                    assert case['object']['exit']==0
                    machine = int.from_bytes(obj.read_bytes()[:2],'little')
                    assert machine=={'32':0x14c,'64':0x8664}[width]
                    case['COFF_machine'] = machine
                    case['object_sha256'] = audit.digest(obj)
                    objects[version,target,backend,optimization] = case['object_sha256']
                    if runtime and target=='win64':
                        driver = work/(optimization+'-driver.o')
                        case['driver'] = audit.invoke([*common,*gates,'-c','driver.c','-o',str(driver)],
                            work,work/(optimization+'-driver.log'),30)
                        publish()
                        assert case['driver']['exit']==0
                        case['driver_object_sha256'] = audit.digest(driver)
                        exe = work/(optimization+'.exe')
                        case['link'] = audit.invoke([tools['gcc']['path'],str(obj),str(driver),
                            *runtime_pins,'-o',str(exe),'-lm','-mthreads'],work,
                            work/(optimization+'-link.log'),30)
                        publish()
                        assert case['link']['exit']==0
                        for mode,args,expected in [('return',[],0),('end',['terminate'],37)]:
                            case[mode] = audit.invoke([str(exe),*args],work,
                                work/(optimization+'-'+mode+'.log'),10)
                            executions += 1
                            publish()
                            assert case[mode]['exit']==expected
                        exe.unlink()
                        driver.unlink()
                    obj.unlink()
    comparisons = []
    if options.baseline:
        for target,width in [('win32','32'),('win64','64')]:
            for backend in ('gcc','clang'):
                for optimization in ('O0','O2'):
                    before = objects['baseline',target,backend,optimization]
                    after = objects['candidate',target,backend,optimization]
                    comparisons.append(dict(target=target,backend=backend,optimization=optimization,
                        baseline_sha256=before,candidate_sha256=after,byte_identical=before==after))
        audit.write_json(output/'object-comparison.json',comparisons)
        assert all(row['byte_identical'] for row in comparisons)
    assert audit.digest(source)==source_pin
    assert all(audit.digest(Path(tool['path']))==tool['sha256'] for tool in tools.values())
    assert all(audit.digest(Path(path))==pin for path,pin in runtime_pins.items())
    publish()
    print(f'C_EMITTER_PROTOTYPES_PASS emissions={len(records)} objects={2*len(records)} executions={executions}')
    return 0


if __name__=='__main__':
    raise SystemExit(main())

# end of test-c-emitter-prototypes.py
