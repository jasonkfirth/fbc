#!/usr/bin/env python3
"""FreeBASIC native Aspell qualification
File: test-aspell.py
Purpose: Check genuine SDK layouts and the original stdin-driven example.
Responsibilities: Installed dictionaries, UTF-8 boundaries, statuses and EOF.
This file contains no replacement spelling engine or personal dictionary writes.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fbc',type=Path,required=True)
    parser.add_argument('--cc',type=Path,required=True)
    parser.add_argument('--prefix',type=Path,required=True)
    parser.add_argument('--bits',type=int,choices=(32,64),default=64)
    parser.add_argument('--dictionary-dir',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1];work=args.output.resolve()
    work.mkdir(parents=True,exist_ok=True)
    compiler=args.fbc.resolve();cc=args.cc.resolve();sdk=cc.parent.parent
    windows=os.name=='nt';suffix='.exe' if windows else ''
    if not windows and args.bits!=64:parser.error('The Linux lane currently requires native x86-64')
    environment=os.environ.copy();environment['PATH']=str(cc.parent)+os.pathsep+environment['PATH']
    if windows:
        for variable,tool in (('GCC','gcc'),('AS','as'),('LD','ld'),('AR','ar')):
            environment[variable]=str(cc.parent/(tool+'.exe'))
    sources=['build_scripts/test-aspell.py','examples/manual/libraries/aspell.bas','inc/aspell.bi',
             'tests/bindings/aspell-abi.c','tests/bindings/aspell-abi.bas']
    fingerprint=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    hashes={name:fingerprint(root/name) for name in sources}
    dictionary=args.dictionary_dir.resolve() if args.dictionary_dir else None
    dictionary_hashes={str(path):fingerprint(path) for path in dictionary.rglob('*') if path.is_file()} if dictionary else {}
    if not windows and dictionary is None:
        # Record the real SDK's default dictionary/data roots without copying
        # or rebuilding installed word lists. The example leaves them read-only.
        for option in ('dict-dir','data-dir'):
            directory=Path(subprocess.check_output([cc.parent/'aspell','config',option],env=environment,text=True).strip())
            for path in directory.rglob('*'):
                if path.is_file():dictionary_hashes[str(path)]=fingerprint(path)
    library=sdk/'bin/libaspell-15.dll' if windows else Path(subprocess.check_output([cc,'-print-file-name=libaspell.so'],text=True).strip()).resolve()
    sdk_hashes={str(path):fingerprint(path) for path in (sdk/'include/aspell.h',library)}
    records=[];receipt={'status':'pending','platform':'windows' if windows else 'linux','bits':args.bits,
        'compiler_sha256':fingerprint(compiler),'source_sha256':hashes,'SDK_sha256':sdk_hashes,
        'dictionary_sha256':dictionary_hashes,'records':records}
    target=work/'results.json'
    def save():target.write_text(json.dumps(receipt,indent=2)+'\n')
    def run(command,label,input_bytes=b'',expected=0):
        result=subprocess.run(list(map(str,command)),cwd=work,env=environment,input=input_bytes,capture_output=True,timeout=30)
        (work/(label+'.log')).write_bytes(repr(list(map(str,command))).encode()+b'\n'+result.stdout+result.stderr)
        assert result.returncode==expected,(label,result.returncode,result.stdout,result.stderr)
        return result.stdout
    def abi(output):
        return list(map(int,output.decode().splitlines()[0].removeprefix('ABI').split()))
    save()
    try:
        native=work/('reference'+suffix)
        run([cc,'-Wall','-Wextra','-Werror','-I'+str(sdk/'include'),root/'tests/bindings/aspell-abi.c',
             '-L'+str(sdk/'lib'),'-laspell','-o',native],'reference-compile')
        reference=run([native],'reference-run');receipt['native_reference']=reference.decode().strip()
        cases=[('Canadian',["en_CA"],b'hello\ncolour\n\n',0,[b'Word is Correct']),
               ('American',["en_US"],b'colour\n\n',0,[b'Suggestions:',b'color']),
               ('suggestions',["en_CA"],b'helllo\n\n',0,[b'Suggestions:',b'hello']),
               ('EOF',["en_CA"],b'hello\n',0,[b'Word is Correct']),
               ('empty-EOF',["en_CA"],b'',0,[]),
               ('missing-language',["zz_unavailable"],b'',1,[]),
               ('too-long',["en_CA"],b'a'*257+b'\n',1,[b'too long']),
               ('NUL',["en_CA"],b'hello\x00tail\n',1,[b'zero byte'])]
        malformed=[b'\xff',b'\xc0\x80',b'\xc3',b'\xed\xa0\x80',b'\xf4\x90\x80\x80',b'\xe0\x80\x80',b'\xf0\x80\x80\x80']
        cases += [('utf8-'+str(index),['en_CA'],value+b'\n',1,[b'invalid UTF-8']) for index,value in enumerate(malformed)]
        cases += [('valid-utf8-'+str(index),['en_CA'],value+b'\n\n',0,[]) for index,value in enumerate((b'caf\xc3\xa9',b'\xe4\xb8\xad',b'\xf0\x9f\x99\x82'))]
        for backend in ('gcc','gas' if args.bits==32 else 'gas64'):
            flags=[compiler,'-prefix',args.prefix.resolve(),'-i',root/'inc','-p',sdk/'lib','-gen',backend,'-w','all']
            if windows and args.bits==32:flags+=['-target','win32']
            probe=work/('abi-'+backend+suffix)
            run([*flags,root/'tests/bindings/aspell-abi.bas','-x',probe],backend+'-abi-compile')
            assert abi(run([probe],backend+'-abi-run'))==abi(reference)
            records.append({'backend':backend,'case':'ABI','scalars':12,'passed':True})
            binary=work/('example-'+backend+suffix)
            run([*flags,root/'examples/manual/libraries/aspell.bas','-x',binary],backend+'-compile')
            for label,arguments,data,expected,markers in cases:
                if dictionary:arguments=[*arguments,str(dictionary)]
                output=run([binary,*arguments],backend+'-'+label,data,expected)
                assert all(marker in output for marker in markers),(label,output)
                if label=='Canadian':assert output.count(b'Word is Correct')==2
                records.append({'backend':backend,'case':label,'exit':expected,'passed':True})
                save()
        for name,digest in hashes.items():assert fingerprint(root/name)==digest,name
        for name,digest in {**sdk_hashes,**dictionary_hashes}.items():assert fingerprint(Path(name))==digest,name
        receipt['status']='passed';save()
        print('ASPELL_NATIVE=PASS bits='+str(args.bits)+' records='+str(len(records)),flush=True)
    except Exception as error:
        receipt.update(status='failed',error=repr(error));save();raise


if __name__=='__main__':main()
# end of test-aspell.py
