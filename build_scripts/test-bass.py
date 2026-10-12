#!/usr/bin/env python3
"""FreeBASIC native BASS media and playback qualification
File: test-bass.py
Purpose: Compare native C/FreeBASIC layouts and decoded PCM, then run real examples.
Responsibilities: Genuine SDKs, bounded products, input errors and independent WAV data.
This file contains no replacement decoder or system audio configuration.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import wave

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
parser.add_argument('--fbc',type=Path,required=True)
parser.add_argument('--sdk-prefix',type=Path,required=True)
parser.add_argument('--bits',type=int,choices=(32,64),default=64)
parser.add_argument('--cc',type=Path,default=Path(shutil.which('gcc') or 'gcc'))
parser.add_argument('--prefix32',type=Path)
parser.add_argument('--outdir',type=Path)
args=parser.parse_args()
root=args.root.resolve()
sdk=args.sdk_prefix.resolve()
cc=args.cc.resolve()
compiler=args.fbc.resolve()
output=args.outdir.resolve() if args.outdir else root/'out/test-bass'
output.mkdir(parents=True,exist_ok=True)
windows=os.name=='nt'
env=os.environ.copy()
env['FB_EXAMPLE_AUTOPLAY']='1'
env['PATH']=str(cc.parent)+os.pathsep+env.get('PATH','')
flags=['-i',root/'inc','-p',sdk/'lib']
libraries=['-L'+str(sdk/'lib'),'-lbass']
if windows:
    env.update(GCC=str(cc),AS=str(cc.parent/'as.exe'),LD=str(cc.parent/'ld.exe'))
    env['PATH']=str(sdk/'bin')+';'+env['PATH']
    if args.bits==32:
        assert args.prefix32 is not None,'The native Win32 runtime prefix is required'
        flags+=['-target','win32','-buildprefix',str(cc.parent)+'/', '-prefix',args.prefix32.resolve()]
        # The SDK supplies MSVC/GCC stdcall imports separately from the
        # Windows-MS names used by FreeBASIC's existing binding.
        libraries=[sdk/'lib/bass.lib']
else:
    assert args.bits==64,'This lane qualifies native Linux x86_64'
    env['LD_LIBRARY_PATH']=str(sdk/'lib')
backends=('gcc','gas64' if args.bits==64 else 'gas')

def run(command,working,label,expected=0):
    result=subprocess.run([str(x) for x in command],cwd=working,env=env,
                          capture_output=True,text=True,stdin=subprocess.DEVNULL,timeout=60)
    (output/(label+'.log')).write_text(repr([str(x) for x in command])+'\n'+result.stdout+result.stderr)
    if result.returncode!=expected:
        raise RuntimeError(label+' exit '+str(result.returncode)+'\n'+result.stdout+result.stderr)
    return result.stdout

def facts(text):
    return {row.split()[0]:list(map(int,row.split()[1:])) for row in text.splitlines() if row.strip()}

media=root/'examples/sound/data'
with wave.open(str(media/'prodigy.wav'),'rb') as wav:
    assert wav.getsampwidth()==2 and wav.getcomptype()=='NONE'
    raw=wav.readframes(wav.getnframes())
expected_wav=b''.join(struct.pack('<f',value[0]/32768.0) for value in struct.iter_unpack('<h',raw))
assert len(expected_wav)<4*1024*1024 and any(expected_wav)
record={'bits':args.bits,'compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
        'native_header_sha256':hashlib.sha256((sdk/'include/bass.h').read_bytes()).hexdigest(),
        'native_library_sha256':hashlib.sha256((sdk/('bin/bass.dll' if windows else 'lib/libbass.so')).read_bytes()).hexdigest(),
        'source_sha256':{name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in
                        ('build_scripts/test-bass.py','tests/bindings/bass-abi.c','tests/bindings/bass-abi.bas',
                         'tests/bindings/bass-decode.c','tests/bindings/bass-decode.bas')},
        'backends':[]}
with tempfile.TemporaryDirectory(prefix='bass-native-') as temporary:
    work=Path(temporary)
    native_abi=work/'c-abi.exe'
    native_decode=work/'c-decode.exe'
    for name,binary in (('bass-abi',native_abi),('bass-decode',native_decode)):
        run([cc,'-Wall','-Wextra','-Werror','-I'+str(sdk/'include'),root/'tests/bindings'/(name+'.c')]+
            libraries+['-o',binary],work,name+'-c-compile')
    expected_facts=facts(run([native_abi],work,'native-abi'))
    malformed=work/'malformed.media'
    malformed.write_bytes(b'Invalid audio fixture\0'*8)
    for kind in ('stream','music'):
        rejected=work/('rejected-'+kind+'.pcm')
        run([native_decode,malformed,rejected,kind],work,kind+'-c-malformed',expected=1)
        assert not rejected.exists()
    reference={}
    cases=[('prodigy.wav','stream'),('example1.ogg','stream'),('example2.ogg','stream'),
           ('dne_trtn.mod','music'),('example.mo3','music')]
    for filename,kind in cases:
        target=work/(filename+'.c.pcm')
        text=run([native_decode,media/filename,target,kind],work,filename+'-c-decode')
        assert 'BASS_C_DECODE=PASS' in text
        data=target.read_bytes()
        assert 0<len(data)<=4*1024*1024
        assert all(-16.0<value[0]<16.0 for value in struct.iter_unpack('<f',data))
        assert any(value[0]!=0 for value in struct.iter_unpack('<f',data))
        if filename=='prodigy.wav':
            assert data==expected_wav,'Vendor WAV decode differs from independent signed-16 PCM conversion'
        reference[filename]=data
    for backend in backends:
        lane={'backend':backend,'passed':False,'pcm':[],'examples':[]}
        fb_abi=work/(backend+'-abi.exe')
        fb_decode=work/(backend+'-decode.exe')
        for name,binary in (('bass-abi',fb_abi),('bass-decode',fb_decode)):
            run([compiler]+flags+['-gen',backend,'-w','all',root/'tests/bindings'/(name+'.bas'),'-x',binary],
                work,name+'-'+backend+'-compile')
        observed=facts(run([fb_abi],work,backend+'-abi'))
        assert observed==expected_facts
        lane['facts']=observed
        for kind in ('stream','music'):
            rejected=work/('rejected-'+backend+'-'+kind+'.pcm')
            run([fb_decode,malformed,rejected,kind],work,kind+'-'+backend+'-malformed',expected=1)
            assert not rejected.exists()
        lane['malformed_media_rejected']=True
        for filename,kind in cases:
            target=work/(filename+'.fb.pcm')
            text=run([fb_decode,media/filename,target,kind],work,filename+'-'+backend+'-decode')
            assert 'BASS_FB_DECODE=PASS' in text
            data=target.read_bytes()
            assert data==reference[filename],(backend,filename,'Native C/FB PCM differs')
            lane['pcm'].append({'filename':filename,'bytes':len(data),
                                'sha256':hashlib.sha256(data).hexdigest(),'passed':True})
        for source,argument,marker in (
            ('examples/manual/libraries/bass.bas',media/'dne_trtn.mod','BASS position seconds:'),
            ('examples/sound/BASS/small.bas',media,'BASS stream position seconds:'),
            ('examples/sound/BASS/demo.bas',media,'BASS autoplay controls completed:')):
            binary=work/'example.exe'
            run([compiler]+flags+['-gen',backend,'-w','all',root/source,'-x',binary],work,
                Path(source).stem+'-'+backend+'-compile')
            text=run([binary,argument],work,Path(source).stem+'-'+backend+'-run')
            assert marker in text
            run([binary,work/'missing-asset'],work,Path(source).stem+'-'+backend+'-missing',expected=1)
            lane['examples'].append({'source':source,'passed':True,'output':text,
                                     'source_sha256':hashlib.sha256((root/source).read_bytes()).hexdigest()})
        lane['passed']=True
        record['backends'].append(lane)
        (output/'results.json').write_text(json.dumps(record,indent=2)+'\n')
        print('BASS_MEDIA_PLAYBACK=PASS',args.bits,backend,'media=5 examples=3',flush=True)
print('BASS_NATIVE_QUALIFICATION=PASS',flush=True)
# end of test-bass.py
