#!/usr/bin/env python3
"""FreeBASIC SQLite 3 example qualification
File: test-sqlite3.py
Purpose: Verify the real SDK, database selection and callback value semantics.
Responsibilities: Private database fixtures, two backends and failing SQL status.
This file contains no database server or access to existing user databases.
"""
from pathlib import Path
import argparse,hashlib,json,os,re,sqlite3,subprocess

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--fbc',type=Path,required=True)
parser.add_argument('--cc',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]; cc=args.cc.resolve(); compiler=args.fbc.resolve()
sdk=cc.parent.parent; work=args.output.resolve(); work.mkdir(parents=True,exist_ok=True)
suffix='.exe' if os.name=='nt' else ''
env=os.environ.copy(); env['PATH']=str(cc.parent)+os.pathsep+env['PATH']
if os.name=='nt':
    for label in ('gcc','as','ld','ar'): env[label.upper()]=str(cc.parent/(label+'.exe'))
names=('build_scripts/test-sqlite3.py','tests/bindings/sqlite3-native.c','examples/database/sqlite3_test.bas','inc/sqlite3.bi')
hashes={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in names}
records=[]; receipt={'status':'pending','compiler_sha256':hashlib.sha256(compiler.read_bytes()).hexdigest(),
                    'source_sha256':hashes,'records':records}
target=work/'results.json'; target.write_text(json.dumps(receipt,indent=2)+'\n')
def run(command,label,expected=0):
    result=subprocess.run([str(part) for part in command],cwd=work,env=env,stdin=subprocess.DEVNULL,capture_output=True,text=True,timeout=30)
    (work/(label+'.log')).write_text(repr([str(part) for part in command])+'\n'+result.stdout+result.stderr)
    assert result.returncode==expected,(label,result.returncode,result.stdout,result.stderr)
    return result.stdout.strip()
try:
    native=work/('native-c'+suffix)
    run([cc,'-Wall','-Wextra','-Werror','-I'+str(sdk/'include'),root/'tests/bindings/sqlite3-native.c',
         '-L'+str(sdk/'lib'),'-lsqlite3','-o',native],'native-c-compile')
    output=run([native],'native-c-run')
    assert 'SQLITE3_NATIVE=PASS' in output,output
    records.append({'backend':'C','passed':True,'output':output})
    for backend in ('gcc','gas64'):
        directory=work/backend; directory.mkdir(exist_ok=True)
        for name,value in (('sqlite3_test.db','default database'),('selected.db','requested database')):
            database=sqlite3.connect(directory/name)
            try:
                database.execute('create table if not exists features(value text)')
                database.execute('delete from features')
                database.execute('insert into features values(?)',(value,))
                database.commit()
            finally:
                database.close()
        exe=directory/('example'+suffix)
        run([compiler,'-i',root/'inc','-gen',backend,'-w','all',root/'examples/database/sqlite3_test.bas',
             '-p',sdk/'lib','-x',exe],backend+'-compile')
        cases=[([],0,'default database'),([str(directory/'selected.db')],0,'requested database'),
               ([str(directory/'selected.db'),'select value from features'],0,'requested database'),
               ([str(directory/'selected.db'),"select NULL as n,'' as e,'alive' as v"],0,None),
               ([str(directory/'selected.db'),'select * from missing_table'],1,'SQL error'),
               ([str(directory/'absent-parent/test.db')],1,"Can't open database"),
               (['one','two','three'],1,'Usage:')]
        for index,(arguments,expected,marker) in enumerate(cases):
            result=subprocess.run([str(exe),*arguments],cwd=directory,env=env,stdin=subprocess.DEVNULL,
                                  capture_output=True,text=True,timeout=10)
            output=result.stdout.strip()
            (directory/('case-'+str(index)+'.log')).write_text(output+'\n'+result.stderr)
            assert result.returncode==expected,(backend,index,result.returncode,output,result.stderr)
            if marker: assert marker in output,(backend,index,output)
            else:
                values=dict(re.findall(r"^(\w+)\s*= '([^']*)'$",output,re.M))
                assert values=={'n':'NULL','e':'','v':'alive'},values
            records.append({'backend':backend,'case':index,'arguments':arguments,'exit':expected,'passed':True,'output':output})
    for name,digest in hashes.items(): assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
    receipt['status']='pass'
    print('SQLITE3_EXAMPLE=PASS native_C=1 original_cases=14 backends=2',flush=True)
except Exception as error:
    receipt.update(status='fail',error=str(error))
    raise
finally:
    target.write_text(json.dumps(receipt,indent=2)+'\n')
# end of test-sqlite3.py
