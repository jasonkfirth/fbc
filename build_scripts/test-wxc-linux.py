#!/usr/bin/env python3
"""FreeBASIC Linux wx-c qualification
File: test-wxc-linux.py
Purpose: Check the genuine i386 wrapper and its original GTK2 windows.
Responsibilities: C/FB values, owned private display, captures and normal close.
This file contains no widget substitute, global desktop input or SDK build.
"""
import argparse
import importlib.util
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time
from sdl3_xvfb import private_display


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('fbc','sdk-prefix','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    if os.name!='posix':parser.error('Linux i386 SDK required')
    root=Path(__file__).resolve().parents[1];output=args.output.resolve();output.mkdir(parents=True,exist_ok=True)
    sources=[Path(__file__).resolve(),root/'tests/bindings/wxc-linux-values.c',
             root/'tests/bindings/wxc-values.bas',*sorted((root/'examples/GUI/wx-c').glob('*.bas'))]
    source_hashes={path.relative_to(root).as_posix():hashlib.sha256(path.read_bytes()).hexdigest() for path in sources}
    # Replace an earlier success before starting, so an interrupted run cannot
    # leave its old receipt looking like acceptance of the current sources.
    (output/'results.json').write_text(json.dumps({'status':'pending','source_sha256':source_hashes},indent=2)+'\n')
    prefix=args.sdk_prefix.resolve();fbc=args.fbc.resolve();library=prefix/'lib/libwx-c-0-9-0-2.so'
    data=library.read_bytes();assert data[:5]==b'\x7fELF\x01' and int.from_bytes(data[18:20],'little')==3
    spec=importlib.util.spec_from_file_location('sdl12_checks',root/'build_scripts/test-sdl12-examples.py')
    gui=importlib.util.module_from_spec(spec);spec.loader.exec_module(gui)
    environment=os.environ.copy();environment.pop('LD_PRELOAD',None)
    environment['LD_LIBRARY_PATH']=str(prefix/'lib')+':'+environment.get('LD_LIBRARY_PATH','')
    records=[]
    def run(command,tag):
        result=subprocess.run([str(x) for x in command],cwd=output,env=environment,capture_output=True,text=True,timeout=40)
        (output/(tag+'.log')).write_text(result.stdout+result.stderr)
        if result.returncode:raise RuntimeError(tag+'\n'+result.stdout+result.stderr)
        return result.stdout
    native=output/'values-c'
    run(['gcc','-m32','-Wall','-Wextra','-Werror',root/'tests/bindings/wxc-linux-values.c','-ldl','-o',native],'compile-c')
    reference=run([native,library],'values-c');assert 'WXC_VALUES=PASS' in reference
    facts=re.search(r'WXC_ABI=([\d,]+)',reference).group(1);assert facts=='4,4,4,4,4',facts
    with private_display(output,environment) as (session,display):
        environment=session
        for backend in ('gcc','gas'):
            flags=[fbc,'-target','linux','-arch','686','-prefix',root,'-i',root/'inc','-p',prefix/'lib','-gen',backend,'-w','all']
            probe=output/('values-'+backend)
            run(flags+[root/'tests/bindings/wxc-values.bas','-x',probe],'compile-values-'+backend)
            observed=run([probe],'values-'+backend)
            assert 'WXC_VALUES=PASS' in observed and re.search(r'WXC_ABI=([\d,]+)',observed).group(1)==facts
            for source in sorted((root/'examples/GUI/wx-c').glob('*.bas')):
                binary=output/(source.stem+'-'+backend);tag=source.stem+'-'+backend
                run(flags+[source,'-x',binary],'compile-'+tag)
                with (output/(tag+'.log')).open('w') as log:
                    child=subprocess.Popen([str(binary)],cwd=output,env=environment,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
                    try:
                        deadline=time.monotonic()+12;window=None
                        while child.poll() is None and time.monotonic()<deadline:
                            found=gui.command(['xdotool','search','--onlyvisible','--pid',str(child.pid)],environment)
                            if found.returncode==0 and found.stdout.strip():window=int(found.stdout.splitlines()[0]);break
                            time.sleep(.05)
                        if window is None:raise RuntimeError(tag+' did not create a visible window')
                        time.sleep(.3)
                        capture=gui.capture(window,output/(tag+'.png'),environment)
                        assert capture['width']>50 and capture['height']>50 and capture['colors']!=1,capture
                        gui.request_close(window,environment)
                        if source.stem=='wx-c_demo':
                            deadline=time.monotonic()+8;dialog=None
                            while child.poll() is None and time.monotonic()<deadline:
                                found=gui.command(['xdotool','search','--onlyvisible','--pid',str(child.pid),'--name','Window Closed!'],environment)
                                if found.returncode==0 and found.stdout.strip():dialog=int(found.stdout.splitlines()[0]);break
                                time.sleep(.05)
                            if dialog is None:raise RuntimeError('exit confirmation not shown')
                            gui.capture(dialog,output/(tag+'-exit.png'),environment)
                            gui.input_command(['windowfocus',str(dialog)],environment)
                            gui.input_command(['key','Return'],environment)
                        assert child.wait(timeout=10)==0,(tag,output/(tag+'.log'))
                        records.append({'source':source.relative_to(root).as_posix(),'backend':backend,'status':'pass','capture':capture,'normal_close':True})
                    finally:gui.stop(child)
            print('WXC_LINUX_NATIVE=PASS '+backend+' values=pass examples=4',flush=True)
    receipt={'status':'pass','platform':'Linux i386 GTK2/Xvfb','native_C_ABI':facts,'records':records,'display':display,
             'source_sha256':source_hashes,
             'compiler_sha256':hashlib.sha256(fbc.read_bytes()).hexdigest(),'library_sha256':hashlib.sha256(data).hexdigest()}
    (output/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('WXC_LINUX_QUALIFICATION=PASS',flush=True)


if __name__=='__main__':main()
# end of test-wxc-linux.py
