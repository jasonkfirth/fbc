#!/usr/bin/env python3
"""FreeBASIC native CGUI ABI and event qualification
File: test-cgui.py
Purpose: Compare independent C and FreeBASIC output against real native SDKs.
Responsibilities: Bounded compilation, private displays and guarded runtime checks.
This file contains no CGUI implementation or installed SDK mutation.
"""
import argparse
from contextlib import nullcontext
import hashlib,json,os
from pathlib import Path
import shutil,subprocess

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
parser.add_argument('--fbc',type=Path,required=True)
parser.add_argument('--sdk-prefix',type=Path,required=True)
parser.add_argument('--allegro-prefix',type=Path)
parser.add_argument('--cc',type=Path,default=Path(shutil.which('gcc') or 'gcc'))
parser.add_argument('--outdir',type=Path)
args=parser.parse_args()
root=args.root.resolve()
prefix=args.sdk_prefix.resolve()
compiler=args.fbc.resolve()
cc=args.cc.resolve()
work=args.outdir.resolve() if args.outdir else root/'out/test-cgui'
work.mkdir(parents=True,exist_ok=True)
windows=os.name=='nt'
env=os.environ.copy()
env['PATH']=str(cc.parent)+os.pathsep+env.get('PATH','')
includes=['-I'+str(prefix/'include')]
libraries=['-L'+str(prefix/'lib'),'-lcgui']
fb_flags=['-i',root/'inc','-p',prefix/'lib']
runtime=[prefix/('bin' if windows else 'lib')]
if args.allegro_prefix:
    allegro=args.allegro_prefix.resolve()
    includes+=['-I'+str(allegro/'include')]
    libraries+=['-L'+str(allegro/'lib'),'-lalleg']
    fb_flags+=['-p',allegro/'lib']
    runtime+=[allegro/('bin' if windows else 'lib')]
else:
    includes+=subprocess.check_output(['pkg-config','--cflags','allegro'],text=True).split()
    libraries+=subprocess.check_output(['pkg-config','--libs','allegro'],text=True).split()
if windows:
    env.update(GCC=str(cc),AS=str(cc.parent/'as.exe'),LD=str(cc.parent/'ld.exe'))
    env['PATH']=os.pathsep.join(map(str,runtime))+os.pathsep+env['PATH']
else:
    env['LD_LIBRARY_PATH']=os.pathsep.join(map(str,runtime))+os.pathsep+env.get('LD_LIBRARY_PATH','')
    from sdl3_xvfb import private_display

def run(command,label,environment=env):
    result=subprocess.run([str(x) for x in command],env=environment,cwd=work,
                          capture_output=True,text=True,timeout=60)
    (work/(label+'.log')).write_text(repr([str(x) for x in command])+'\n'+result.stdout+result.stderr)
    if result.returncode:
        raise RuntimeError(label+' exit '+str(result.returncode)+'\n'+result.stdout+result.stderr)
    return result.stdout

def facts(output):
    rows=output.splitlines()
    return {kind:list(map(int,next(row for row in rows if row.startswith(kind+' ')).split()[1:]))
            for kind in ('ABI','CONSTANTS','VERSION')}

native=work/('c-probe.exe' if windows else 'c-probe')
run([cc,'-Wall','-Wextra','-Werror']+includes+[root/'tests/bindings/cgui-abi.c']+libraries+['-o',native],'c-compile')
expected=facts(run([native,'--abi'],'c-abi'))
display_directory=work/'display'
if not windows:
    display_directory.mkdir(exist_ok=True)
session=nullcontext((env,None)) if windows else private_display(display_directory,env)
records=[]
with session as (environment,display):
    observed=run([native],'c-runtime',environment)
    assert 'CGUI_C_RUNTIME=PASS' in observed
    for backend in ('gcc','gas64'):
        binary=work/('fb-'+backend+('.exe' if windows else ''))
        run([compiler]+fb_flags+['-gen',backend,'-g','-exx',root/'tests/bindings/cgui-abi.bas','-x',binary],backend+'-compile')
        observed=facts(run([binary,'--abi'],backend+'-abi'))
        assert observed==expected,backend
        output=run([binary],backend+'-runtime',environment)
        assert 'CGUI_FB_RUNTIME=PASS' in output
        records.append({'backend':backend,'facts':observed,'passed':True,
                        'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
                        'native_header_sha256':hashlib.sha256((prefix/'include/cgui.h').read_bytes()).hexdigest(),
                        'c_runtime_passed':True,'fb_runtime_passed':True})
(work/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print('CGUI_ABI_EVENT_RUNTIME=PASS backends=2 native_facts=8 constants=4 version_fields=4',flush=True)
# end of test-cgui.py
