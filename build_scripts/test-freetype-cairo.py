#!/usr/bin/env python3
"""FreeBASIC native text/2D qualification
File: test-freetype-cairo.py
Purpose: Check FreeType/Cairo layouts and original window rendering.
Responsibilities: Native C references, exact coverage/clipping, fonts and cleanup.
This file contains no substitute engine, user-font modification or desktop capture.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import os
import struct
import subprocess


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc',type=Path,required=True)
    parser.add_argument('--bits',type=int,choices=(32,64),default=64)
    parser.add_argument('--tool-dir',type=Path,required=True)
    parser.add_argument('--runtime-prefix',type=Path,required=True)
    parser.add_argument('--font',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    work=args.output.resolve();work.mkdir(parents=True,exist_ok=True)
    tools=args.tool_dir.resolve();sdk=tools.parent;compiler=args.fbc.resolve()
    env=os.environ.copy();env['PATH']=str(tools)+';'+env['PATH'];env['FB_GFX_DRIVER']='GDI'
    for name in ('gcc','as','ld','ar'):env[name.upper()]=str(tools/(name+'.exe'))
    examples={'char':'examples/graphics/FreeType/char.bas','text':'examples/graphics/FreeType/drawstr.bas',
              'manual':'examples/manual/libraries/freetype2.bas','cairo':'examples/manual/libraries/cairo.bas'}
    sources=list(examples.values())+['examples/graphics/FreeType/bitmap.bi',
        'tests/bindings/freetype-cairo.c','tests/bindings/freetype-cairo.bas',
        'build_scripts/test-freetype-cairo.py','build_scripts/windows-test-images.py',
        'doc/manual/cache/ExtLibfreetype2.wakka','doc/manual/cache/ExtLibcairo.wakka',
        'inc/freetype2/freetype.bi','inc/freetype2/ftimage.bi','inc/cairo/cairo.bi']
    fingerprint=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    sdk_files=[sdk/'include/freetype2/freetype/freetype.h',sdk/'include/freetype2/freetype/ftimage.h',
               sdk/'include/cairo/cairo.h',sdk/'bin/libfreetype-6.dll',sdk/'bin/libcairo-2.dll',
               sdk/'lib/libfreetype.dll.a',sdk/'lib/libcairo.dll.a',args.font.resolve()]
    records=[]
    receipt={'status':'pending','bits':args.bits,'source_sha256':{p:fingerprint(root/p) for p in sources},
        'SDK_sha256':{str(p):fingerprint(p) for p in sdk_files},'compiler_sha256':fingerprint(compiler),'records':records}
    def save(): (work/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    def record(backend,name,case,**evidence):
        records.append({'backend':backend,'example':examples.get(name,name),'case':case,'passed':True,**evidence});save()
    def run(command,directory,label,expected=0):
        r=subprocess.run(list(map(str,command)),cwd=directory,env=env,stdin=subprocess.DEVNULL,capture_output=True,timeout=30)
        (directory/(label+'.log')).write_bytes(repr(list(map(str,command))).encode()+b'\n'+r.stdout+r.stderr)
        assert r.returncode==expected,(label,r.returncode,r.stdout[:1200],r.stderr[:1200])
        return r.stdout
    def abi(data):
        return {line.split()[0]:list(map(int,line.split()[1:])) for line in data.decode().splitlines()
                if line.split() and line.split()[0] in ('FT','CAIRO')}
    def raw(path):
        data=path.read_bytes();w,h=struct.unpack_from('<II',data);assert len(data)==8+w*h*4
        return w,h,data[8:]
    def compile_fb(source,backend,directory,label):
        binary=directory/(label+'.exe')
        flags=[compiler,'-i',root/'inc','-p',sdk/'lib','-gen',backend,'-w','all']
        if args.bits==32:flags+=['-target','win32','-prefix',args.runtime_prefix.resolve(),'-buildprefix',str(tools)+'/']
        run(flags+[root/source,'-x',binary],directory,label+'-compile')
        data=binary.read_bytes();offset=struct.unpack_from('<I',data,0x3c)[0]
        assert struct.unpack_from('<H',data,offset+4)[0]==(0x14c if args.bits==32 else 0x8664)
        return binary
    save()
    native=work/'reference.exe'
    run([tools/'gcc.exe','-Wall','-Wextra','-Werror','-I'+str(sdk/'include/freetype2'),
         '-I'+str(sdk/'include/cairo'),root/'tests/bindings/freetype-cairo.c','-lfreetype','-lcairo','-o',native],work,'c-compile')
    reference=run([native,args.font.resolve(),work],work,'c-run')
    receipt['native_reference']=reference.decode().strip()
    glyph,cairo=raw(work/'glyph.bgra'),raw(work/'cairo.bgra')
    spec=importlib.util.spec_from_file_location('private_images',root/'build_scripts/windows-test-images.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    class TextImages(module.Images):
        def frame(self,process,hwnd):
            result=super().frame(process,hwnd);self.last_frame=result;return result
    images=TextImages()
    try:
        for backend in ('gcc','gas' if args.bits==32 else 'gas64'):
            directory=work/backend;directory.mkdir(exist_ok=True)
            binary=compile_fb('tests/bindings/freetype-cairo.bas',backend,directory,'bounds')
            output=directory/'bounds.bin'
            actual=run([binary,output],directory,'bounds-run')
            assert abi(actual)==abi(reference),(actual,reference)
            assert b'GLYPH_BOUNDS=PASS' in actual
            data=output.read_bytes();assert len(data)==216*60
            position=0;case=0
            coverage=[[0,64,128,192,255],[255,192,128,64,0],[17,33,99,177,233]]
            for flow in range(2):
                rows=coverage if flow==0 else coverage[::-1]
                for x in range(-6,6):
                    for y in range(-4,5):
                        expected=bytearray()
                        for dy in range(3):
                            for dx in range(4):
                                alpha=rows[dy-y][dx-x] if 0<=dy-y<3 and 0<=dx-x<5 else 0
                                colors=[(src*alpha+dst*(255-alpha)+127)//255 for src,dst in zip((10,130,220),(60,40,20))]
                                expected+=bytes(colors)+b'\xff'
                            expected+=b'\xa5'*4
                        assert data[position:position+60]==expected,(backend,flow,x,y)
                        position+=60;case+=1
            record(backend,'renderer','native layouts and exact bitmap bounds',cases=case,scalar_comparisons=16,pixels=case*12)
            for name,source in examples.items():
                binary=compile_fb(source,backend,directory,name)
                if name in ('char','manual'):
                    outcome=images.run_graphics([binary,args.font.resolve()],directory,env,
                        [(0,0,*glyph,0)],directory/(name+'-window.log'))
                elif name=='cairo':
                    outcome=images.run_graphics([binary],directory,env,[(0,0,*cairo,0)],directory/'cairo-window.log')
                else:
                    background=b''.join(struct.pack('<I',x^239) for x in range(320))
                    outcome=images.run_graphics([binary,args.font.resolve()],directory,env,
                        [(0,239,320,1,background,0)],directory/'text-window.log')
                    pixels=images.last_frame[2]
                    ink=sum(pixels[(y*320+x)*4:(y*320+x)*4+3] != struct.pack('<I',x^y)[:3]
                            for y in range(240) for x in range(320))
                    assert ink>100,('Original random text did not draw colored glyphs',ink)
                    outcome['colored_glyph_pixels']=ink
                record(backend,name,'original window and normal final keypress',**outcome)
                if name!='cairo':
                    run([binary,directory/'missing.ttf'],directory,name+'-missing',1)
                    record(backend,name,'original missing font',exit=1)
        assert all(fingerprint(root/p)==h for p,h in receipt['source_sha256'].items())
        assert all(fingerprint(Path(p))==h for p,h in receipt['SDK_sha256'].items())
        receipt['status']='passed';save()
        print('NATIVE_FREETYPE_CAIRO=PASS bits='+str(args.bits)+' records='+str(len(records)))
    finally:images.close()


if __name__=='__main__':main()
# end of test-freetype-cairo.py
