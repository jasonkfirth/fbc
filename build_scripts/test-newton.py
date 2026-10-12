#!/usr/bin/env python3
"""FreeBASIC Newton 3.13 native qualification
File: test-newton.py
Purpose: Compare actual C layouts and physics with both FreeBASIC backends.
Responsibilities: Private SDK selection, bounded runs and source evidence.
This file contains no substitute physics or system SDK installation.
"""
from pathlib import Path
import argparse,hashlib,json,os,re,struct,subprocess

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sdk-prefix',type=Path,required=True)
parser.add_argument('--fbc',type=Path,required=True)
parser.add_argument('--cc',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
prefix=args.sdk_prefix.resolve(); compiler=args.fbc.resolve(); cc=args.cc.resolve()
work=args.output.resolve(); work.mkdir(parents=True,exist_ok=True)
windows=os.name=='nt'; suffix='.exe' if windows else ''
environment=os.environ.copy()
environment['PATH']=str(prefix/'bin')+os.pathsep+str(cc.parent)+os.pathsep+environment['PATH']
environment['LD_LIBRARY_PATH']=str(prefix/'lib')
if windows:
    for label in ('GCC','AS','LD','AR'):
        environment[label]=str(cc.parent/({'GCC':'gcc'}.get(label,label.lower())+'.exe'))
sources=('build_scripts/test-newton.py','tests/bindings/newton-native.c',
         'tests/bindings/newton-native.bas','examples/math/Newton/test.bas','inc/Newton.bi',
         'build_scripts/patches/newton313-mingw-timer.patch')
hashes={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in sources}
sdk_files=[prefix/'include/Newton.h']
sdk_files.extend(path for part in ('bin','lib') for path in sorted((prefix/part).glob('*')) if path.is_file())
sdk_hashes={str(path.relative_to(prefix)):hashlib.sha256(path.read_bytes()).hexdigest() for path in sdk_files}
records=[]
receipt={'status':'pending','sdk_prefix':str(prefix),'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
         'cc_sha256':hashlib.sha256(cc.read_bytes()).hexdigest(),'source_sha256':hashes,'sdk_sha256':sdk_hashes,'records':records}
receipt_path=work/'results.json'
receipt_path.write_text(json.dumps(receipt,indent=2)+'\n')

def run(command,label):
    result=subprocess.run([str(part) for part in command],cwd=work,env=environment,
                          capture_output=True,text=True,timeout=60)
    (work/(label+'.log')).write_text(repr([str(part) for part in command])+'\n'+result.stdout+result.stderr)
    if result.returncode:
        raise RuntimeError(label+' exit '+str(result.returncode)+': '+result.stdout+result.stderr)
    return result.stdout.strip()

def examine(output):
    lines=output.splitlines()
    abi=list(map(int,lines[0].removeprefix('ABI').split()))
    assert len(abi)==12 and abi[:5]==[4,4,8,8,8],abi
    fall=re.search(r'^FALL\s+(-?[\d.eE+]+)\s+CALLBACKS\s+(\d+)\s*$',output,re.M)
    assert fall and 'NEWTON_NATIVE=PASS version=313 float=4 cleanup=1' in output,output
    return abi,float(fall[1]),int(fall[2])

try:
    native=work/('native-c'+suffix)
    run([cc,'-Wall','-Wextra','-Werror','-I'+str(prefix/'include'),root/'tests/bindings/newton-native.c',
         '-L'+str(prefix/'lib'),'-lNewton','-o',native],'native-c-compile')
    output=run([native],'native-c-run')
    abi,fall,callbacks=examine(output)
    records.append({'backend':'C','check':'native','abi':abi,'fall':fall,'callbacks':callbacks,'passed':True})
    for backend in ('gcc','gas64'):
        for label,relative in (('native','tests/bindings/newton-native.bas'),('example','examples/math/Newton/test.bas')):
            executable=work/(label+'-'+backend+suffix)
            executable.unlink(missing_ok=True)
            command=[compiler,'-i',root/'inc','-gen',backend,'-w','all',root/relative,
                     '-p',prefix/'lib','-x',executable]
            run(command,label+'-'+backend+'-compile')
            if windows:
                binary=executable.read_bytes(); offset=struct.unpack_from('<I',binary,0x3c)[0]
                assert struct.unpack_from('<H',binary,offset+4)[0]==0x8664
            output=run([executable],label+'-'+backend+'-run')
            record={'backend':backend,'check':label,'passed':True,'output':output}
            if label=='native':
                observed,y,count=examine(output)
                assert observed==abi,(observed,abi)
                assert abs(y-fall)<0.00001 and count==callbacks,(y,fall,count,callbacks)
                record.update(abi=observed,fall=y,callbacks=count)
            else:
                assert not output,output
            records.append(record)
    for name,digest in hashes.items():
        assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
    for name,digest in sdk_hashes.items():
        assert hashlib.sha256((prefix/name).read_bytes()).hexdigest()==digest,name
    receipt['status']='pass'
    print('NEWTON_NATIVE=PASS ABI=12 backends=2 physics=3 examples=2 cleanup=3',flush=True)
except Exception as error:
    receipt.update(status='fail',error=str(error))
    raise
finally:
    receipt_path.write_text(json.dumps(receipt,indent=2)+'\n')
# end of test-newton.py
