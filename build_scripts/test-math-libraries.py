#!/usr/bin/env python3
"""FreeBASIC small native library qualification
File: test-math-libraries.py
Purpose: Compare real GMP, GSL, libffi and Lua ABIs and run their examples.
Responsibilities: SDK isolation, two backends, real private console keypresses.
This file contains no substitute libraries or omitted final waits.
"""
from pathlib import Path
import argparse,hashlib,importlib.util,json,math,os,re,shutil,struct,subprocess,sys

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--fbc',type=Path,required=True)
parser.add_argument('--bits',type=int,choices=(32,64),default=64)
parser.add_argument('--tool-dir',type=Path,required=True)
parser.add_argument('--runtime-prefix',type=Path,required=True)
parser.add_argument('--lua-prefix',type=Path)
parser.add_argument('--lua-include',type=Path)
parser.add_argument('--ffi-static',action='store_true')
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
if os.name!='nt': parser.error('This qualification currently requires Windows SDKs')
root=Path(__file__).resolve().parents[1]; compiler=args.fbc.resolve(); tools=args.tool_dir.resolve()
work=args.output.resolve(); work.mkdir(parents=True,exist_ok=True)
sdk=tools.parent; lua_prefix=args.lua_prefix.resolve() if args.lua_prefix else None
static_lib=None
if args.ffi_static:
    static_lib=work/'static-sdk/lib'; static_lib.mkdir(parents=True,exist_ok=True)
    shutil.copy2(sdk/'lib/libffi.a',static_lib/'libffi.a')
spec=importlib.util.spec_from_file_location('private_console',root/'build_scripts/windows-test-console.py')
console=importlib.util.module_from_spec(spec); spec.loader.exec_module(console)
env=os.environ.copy(); env['PATH']=(str(lua_prefix/'bin')+';' if lua_prefix else '')+str(tools)+';'+env['PATH']
for label in ('gcc','as','ld','ar'): env[label.upper()]=str(tools/(label+'.exe'))
names=['build_scripts/test-math-libraries.py','build_scripts/windows-test-console.py','inc/gmp.bi','inc/ffi.bi',
       'examples/manual/libraries/gmp.bas','examples/manual/libraries/ffi/helloworld.bas',
       'examples/manual/libraries/ffi/closures.bas','examples/misc/libffi/closures.bas']
families=['gmp','ffi']
if args.bits==64:
    families+=['gsl','lua']
    assert lua_prefix and args.lua_include
    names+=['examples/manual/libraries/gsl1.bas','examples/manual/libraries/gsl2.bas','examples/math/GSL/matrix.bas',
            'examples/other-languages/Lua/minmax.bas','inc/Lua/lua.bi','inc/Lua/lauxlib.bi','inc/Lua/lualib.bi',
            'inc/gsl/gsl_matrix_double.bi']
for family in families:
    stem='lua-abi' if family=='lua' else family+'-native'
    names+=['tests/bindings/'+stem+'.c','tests/bindings/'+stem+'.bas']
hashes={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in names}
sdk_paths=[sdk/'include/gmp.h',sdk/'include/ffi.h',sdk/'include/ffitarget.h',
           sdk/'bin/libgmp-10.dll',sdk/'bin/libffi-8.dll',sdk/'lib/libgmp.dll.a',sdk/'lib/libffi.dll.a']
if args.ffi_static: sdk_paths+=[sdk/'lib/libffi.a',static_lib/'libffi.a']
if args.bits==64:
    sdk_paths+=[sdk/'include/gsl/gsl_matrix_double.h',sdk/'include/gsl/gsl_math.h',
                sdk/'bin/libgsl-28.dll',sdk/'bin/libgslcblas-0.dll',
                lua_prefix/'bin/lua53.dll',lua_prefix/'lib/liblua.dll.a',args.lua_include.resolve()/'lua.h',
                args.lua_include.resolve()/'lauxlib.h']
sdk_hashes={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in sdk_paths}
records=[]; receipt={'status':'pending','bits':args.bits,'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
                    'source_sha256':hashes,'sdk_sha256':sdk_hashes,'ffi_linkage':'static' if args.ffi_static else 'DLL','records':records}
target=work/'results.json'; target.write_text(json.dumps(receipt,indent=2)+'\n')

def run(command,label,expected_exit=0):
    result=subprocess.run([str(part) for part in command],cwd=work,env=env,stdin=subprocess.DEVNULL,capture_output=True,text=True,timeout=45)
    (work/(label+'.log')).write_text(repr([str(part) for part in command])+'\n'+result.stdout+result.stderr)
    assert result.returncode==expected_exit,(label,result.returncode,result.stdout,result.stderr)
    return result.stdout.strip()

def compile_fb(relative,backend,label,family):
    exe=work/(label+'-'+backend+'.exe'); exe.unlink(missing_ok=True)
    flags=[compiler,'-i',root/'inc','-gen',backend,'-w','all']
    if args.bits==32: flags+=['-target','win32','-prefix',args.runtime_prefix.resolve(),'-buildprefix',str(tools)+'/']
    if family=='gmp': flags+=['-d','LIBGMP_DLL']
    if family=='ffi' and args.ffi_static: flags+=['-d','FFI_STATIC_BUILD','-p',static_lib]
    if family=='lua': flags+=['-eunwind','-p',lua_prefix/'lib']
    flags+=['-p',sdk/'lib',root/relative,'-x',exe]
    run(flags,label+'-'+backend+'-compile')
    binary=exe.read_bytes(); offset=struct.unpack_from('<I',binary,0x3c)[0]
    assert struct.unpack_from('<H',binary,offset+4)[0]==(0x14c if args.bits==32 else 0x8664)
    if family=='ffi' and args.ffi_static:
        imports=run([tools/'objdump.exe','-p',exe],label+'-'+backend+'-imports')
        assert 'DLL Name: libffi-' not in imports,imports
    return exe

def abi(output): return list(map(int,output.splitlines()[0].removeprefix('ABI').split()))

try:
    for family in families:
        stem='lua-abi' if family=='lua' else family+'-native'
        native=work/(family+'-c.exe')
        flags=[tools/'gcc.exe','-Wall','-Wextra','-Werror','-I'+str(sdk/'include')]
        if family=='lua': flags+=['-I'+str(args.lua_include.resolve())]
        if family=='ffi' and args.ffi_static: flags+=['-DFFI_STATIC_BUILD','-L'+str(static_lib)]
        flags+=[root/('tests/bindings/'+stem+'.c'),'-L'+str(sdk/'lib')]
        if family=='lua': flags+=['-L'+str(lua_prefix/'lib')]
        flags+=['-l'+family]
        if family=='gsl': flags+=['-lgslcblas']
        flags+=['-o',native]
        run(flags,family+'-c-compile')
        if family=='ffi' and args.ffi_static:
            imports=run([tools/'objdump.exe','-p',native],family+'-c-imports')
            assert 'DLL Name: libffi-' not in imports,imports
        reference=run([native,'abi'] if family=='ffi' else [native],family+'-c-abi')
        expected=abi(reference)
        executables=[]; freebasic_abi={}
        for backend in ('gcc','gas' if args.bits==32 else 'gas64'):
            exe=compile_fb('tests/bindings/'+stem+'.bas',backend,family+'-native',family)
            observed=run([exe,'abi'] if family=='ffi' else [exe],family+'-'+backend+'-abi')
            actual=abi(observed)
            freebasic_abi[backend]=actual
            if family=='ffi' and args.bits==32:
                # GCC's aligned typedef has size 28 and alignment 8. FreeBASIC
                # rounds its storage to 32. Field offsets and alignment agree;
                # ffi_closure_alloc accepts this harmless extra tail storage.
                assert actual[:3]+actual[4:]==expected[:3]+expected[4:] and expected[3]<=actual[3]<=expected[3]+7,(family,backend,expected,actual)
            else:
                assert actual==expected,(family,backend,expected,actual)
            executables.append((backend,exe))
        if family=='ffi':
            reference=run([native],family+'-c-run')
            for backend,exe in executables:
                output=run([exe],family+'-'+backend+'-run')
                assert 'LIBFFI_NATIVE=PASS' in output,output
        records.append({'family':family,'check':'C and both FreeBASIC layouts and native execution','abi':expected,
                        'freebasic_abi':freebasic_abi,'ffi_closure_tail_storage':'Win32 FreeBASIC rounds to 32; C size 28, matching fields and alignment' if family=='ffi' and args.bits==32 else None,
                        'passed':True,'C_output':reference})
    examples=[('gmp','examples/manual/libraries/gmp.bas','gmp'),
              ('ffi','examples/manual/libraries/ffi/helloworld.bas','ffi-call'),
              ('ffi','examples/manual/libraries/ffi/closures.bas','ffi-closure'),
              ('ffi','examples/misc/libffi/closures.bas','ffi-closure-copy')]
    if args.bits==64:
        examples+=[('gsl','examples/manual/libraries/gsl1.bas','gsl-math'),
                   ('gsl','examples/manual/libraries/gsl2.bas','gsl-matrix'),
                   ('gsl','examples/math/GSL/matrix.bas','gsl-matrix-copy'),
                   ('lua','examples/other-languages/Lua/minmax.bas','lua-minmax')]
    for backend in ('gcc','gas' if args.bits==32 else 'gas64'):
        for family,relative,label in examples:
            exe=compile_fb(relative,backend,label,family)
            if family=='gmp':
                output=run([exe],label+'-'+backend+'-run')
                sys.set_int_max_str_digits(22000) # The fixed 2^65536 result has 19729 digits.
                assert output=='2^65536 = '+str(2**65536),output[:200]
                record={'digits':19729}
            else:
                if family=='lua':
                    script='''local cases={{-8,-2,-4},{0,5,2},{42,2,17,33,15,1.5},{-1},{3,0,-3}}
for _,v in ipairs(cases) do
 local lo,hi=MinMax(table.unpack(v));local wantlo,wanthi=math.min(table.unpack(v)),math.max(table.unpack(v))
 assert(lo==wantlo and hi==wanthi)
end
assert(not pcall(MinMax))
assert(not pcall(MinMax,1,{}))
print('LUA_MINMAX=PASS cases=5 errors=2')
'''
                    (work/'minmax.lua').write_text(script)
                result=console.run_console([exe],work,env,hold_console=family=='ffi')
                assert result['exit']==0,result
                output=result['output']; record={'private_console':result}
                (work/(label+'-'+backend+'-run.log')).write_text(output+'\n')
                if family=='ffi':
                    assert ('Function returned 42' if label=='ffi-call' else 'Returned 42') in output,output
                    assert ('Hello world' if label=='ffi-call' else 'Hello World!') in output,output
                elif label=='gsl-math':
                    power=re.search(r'3\.141 \^ 4 =\s*([\d.]+)',output)
                    assert power and abs(float(power[1])-3.141**4)<0.0000000001 and 'length 3 and 4 is 5' in output,output
                elif family=='gsl':
                    values=re.findall(r'm\(\s*(\d+)\s*,\s*(\d+)\s*\)\s*=\s*([\d.]+)',output)
                    assert len(values)==18,output
                    for index,(i,j,value) in enumerate(values):
                        expected=0.23+100*(int(i) if index<9 else int(j))+(int(j) if index<9 else int(i))
                        assert abs(float(value)-expected)<0.00001,(index,value,expected)
                else:
                    assert 'LUA_MINMAX=PASS cases=5 errors=2' in output,output
                    (work/'minmax.lua').unlink()
                    failed=console.run_console([exe],work,env)
                    assert failed['exit']==1 and 'Lua script failed:' in failed['output'],failed
                    record['missing_script']=failed
            record.update(family=family,source=relative,backend=backend,passed=True)
            records.append(record)
    for name,digest in hashes.items(): assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
    for name,digest in sdk_hashes.items(): assert hashlib.sha256(Path(name).read_bytes()).hexdigest()==digest,name
    receipt['status']='pass'
    print('NATIVE_MATH_LIBRARIES=PASS bits='+str(args.bits)+' records='+str(len(records)),flush=True)
except Exception as error:
    receipt.update(status='fail',error=str(error))
    raise
finally:
    target.write_text(json.dumps(receipt,indent=2)+'\n')
# end of test-math-libraries.py
