#!/usr/bin/env python3
"""FreeBASIC SpiderMonkey 1.6 Win32 native qualification
File: test-spidermonkey.py
Purpose: Compare C layouts and tagged values, then execute the real examples.
Responsibilities: Isolated i386 SDK, two compiler backends and bounded runs.
This file contains no replacement JavaScript engine or system installation.
"""
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sdk-prefix',type=Path,required=True)
parser.add_argument('--fbc',type=Path,required=True)
parser.add_argument('--tool-dir',type=Path,required=True)
parser.add_argument('--runtime-prefix',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
if os.name!='nt': parser.error('This qualification requires the Windows i386 SDK and toolchain')
root=Path(__file__).resolve().parents[1]
prefix=args.sdk_prefix.resolve(); compiler=args.fbc.resolve(); tools=args.tool_dir.resolve()
work=args.output.resolve(); work.mkdir(parents=True,exist_ok=True)
env=os.environ.copy(); env['PATH']=str(prefix/'bin')+';'+str(tools)+';'+env['PATH']
for label in ('gcc','as','ld','ar'): env[label.upper()]=str(tools/(label+'.exe'))
names=['build_scripts/test-spidermonkey.py','tests/bindings/spidermonkey-native.c',
       'tests/bindings/spidermonkey-native.bas','examples/manual/libraries/spidermonkey1.bas',
       'examples/manual/libraries/spidermonkey2.bas','examples/other-languages/JavaScript/SpiderMonkey/callbacks.bas']
names.extend(path.relative_to(root).as_posix() for path in sorted((root/'inc/spidermonkey').glob('*.bi')))
hashes={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in names}
sdk_names=[path for part in ('include','bin','lib') for path in sorted((prefix/part).glob('*')) if path.is_file()]
sdk_hashes={str(path.relative_to(prefix)):hashlib.sha256(path.read_bytes()).hexdigest() for path in sdk_names}
records=[]
receipt={'status':'pending','bits':32,'sdk_prefix':str(prefix),'source_sha256':hashes,'sdk_sha256':sdk_hashes,
         'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),'records':records}
target=work/'results.json'; target.write_text(json.dumps(receipt,indent=2)+'\n')

def run(command,label):
    result=subprocess.run([str(part) for part in command],cwd=work,env=env,capture_output=True,text=True,timeout=60)
    (work/(label+'.log')).write_text(repr([str(part) for part in command])+'\n'+result.stdout+result.stderr)
    if result.returncode: raise RuntimeError(label+' exit '+str(result.returncode)+': '+result.stdout+result.stderr)
    return result.stdout.strip()

def check_machine(executable):
    data=executable.read_bytes(); offset=struct.unpack_from('<I',data,0x3c)[0]
    assert struct.unpack_from('<H',data,offset+4)[0]==0x14c,executable

try:
    native=work/'native-c.exe'
    run([tools/'gcc.exe','-Wall','-Wextra','-Werror','-DXP_WIN','-DWIN32','-I'+str(prefix/'include'),
         root/'tests/bindings/spidermonkey-native.c','-L'+str(prefix/'lib'),'-ljs','-o',native],'native-c-compile')
    check_machine(native)
    output=run([native],'native-c-run')
    abi=list(map(int,output.splitlines()[0].removeprefix('ABI').split()))
    assert len(abi)==12 and abi[:5]==[4,4,4,4,4],abi
    marker='SPIDERMONKEY_NATIVE=PASS factorial=120 callback=17 root=1 rejection=1 tags=7'
    assert marker in output,output
    records.append({'backend':'C','check':'native','abi':abi,'passed':True,'output':output})
    for backend in ('gcc','gas'):
        for label,relative,expected in (
                ('native','tests/bindings/spidermonkey-native.bas',None),
                ('factorial','examples/manual/libraries/spidermonkey1.bas','result: 120'),
                ('callback','examples/manual/libraries/spidermonkey2.bas','HELLO'),
                ('callback-copy','examples/other-languages/JavaScript/SpiderMonkey/callbacks.bas','HELLO')):
            executable=work/(label+'-'+backend+'.exe'); executable.unlink(missing_ok=True)
            run([compiler,'-target','win32','-prefix',args.runtime_prefix.resolve(),'-buildprefix',str(tools)+'/',
                 '-i',root/'inc','-gen',backend,'-w','all',root/relative,'-p',prefix/'lib','-x',executable],label+'-'+backend+'-compile')
            check_machine(executable)
            output=run([executable],label+'-'+backend+'-run')
            record={'backend':backend,'check':label,'passed':True,'output':output}
            if label=='native':
                observed=list(map(int,output.splitlines()[0].removeprefix('ABI').split()))
                assert observed==abi and marker in output,(observed,abi,output)
                record['abi']=observed
            else: assert output==expected,(label,output,expected)
            records.append(record)
    for name,digest in hashes.items(): assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
    for name,digest in sdk_hashes.items(): assert hashlib.sha256((prefix/name).read_bytes()).hexdigest()==digest,name
    receipt['status']='pass'
    print('SPIDERMONKEY_NATIVE=PASS ABI=12 backends=2 engine_checks=3 examples=6',flush=True)
except Exception as error:
    receipt.update(status='fail',error=str(error))
    raise
finally:
    target.write_text(json.dumps(receipt,indent=2)+'\n')
# end of test-spidermonkey.py
