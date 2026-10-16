#!/usr/bin/env python3
"""Native release selection fixtures; existing strict parser oracle also runs."""
import argparse,copy,hashlib,json,os,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true');p.add_argument('--cjson-source',type=Path);a=p.parse_args()
ASSET='DIY-Shelly-Remote-es3c35p-manifest.json';cases=[]
def release(tag='v1.2.4',dev=False):
 return dict(id=123,url='https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/123',tag_name=tag,draft=False,prerelease=dev,assets=[dict(name=ASSET,state='uploaded',size=700,browser_download_url='https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/'+tag+'/'+ASSET)],body='Zmiany\n\tąć <b>plain</b>\r')
def raw(obj):return json.dumps(obj,ensure_ascii=False,separators=(',',':')).encode()
def add(obj,channel=0,result=0,tag='v1.2.4',notes='Zmiany\n ąć <b>plain</b>'):
 cases.append((obj if isinstance(obj,bytes) else raw(obj),channel,result,tag.encode() if result==0 else b'',notes.encode() if result==0 else b''))
base=release();add(base);add([release(dev=True)],1)
add([release()],1,4);add([],1,4);add(base,1,2);add([base],0,2);add(base,7,1)
for change in [{'draft':True},{'prerelease':True},{'tag_name':'../x'},{'url':'https://evil.test/releases/123'},{'id':124}]:
 obj=copy.deepcopy(base);obj.update(change);add(obj,result=-1)
for key,value in [('draft',1),('prerelease','false'),('id','123'),('tag_name',None),('assets',{}),('body',7)]:
 obj=copy.deepcopy(base);obj[key]=value;add(obj,result=3 if key=='tag_name' else 2)
for key in ['draft','prerelease','id','tag_name','url','assets']:
 obj=copy.deepcopy(base);del obj[key];add(obj,result=3 if key=='tag_name' else 2)
for change in [{'state':'pending'},{'size':0},{'size':2049},{'size':True},{'browser_download_url':'https://evil.test/file'},{'name':'other.json'}]:
 obj=copy.deepcopy(base);obj['assets'][0].update(change);add(obj,result=-1)
obj=copy.deepcopy(base);obj['assets']*=2;add(obj,result=5)
obj=copy.deepcopy(base);obj['assets']=[];add(obj,result=4)
for notes,expected in [(None,''),('', ''),('ą'*200,'ą'*160),('x'+'ą'*200,'x'+'ą'*159),('a\t\r\n\x01\x7fb','a \nb')]:
 obj=copy.deepcopy(base);obj['body']=notes;add(obj,notes=expected)
obj=copy.deepcopy(base);del obj['body'];add(obj,notes='')
first=release('v1.3.0',True);later=release('v9.0.0',True);draft=release('v0.1.0',True);draft['draft']=True
add([draft,base,first,later],1,tag='v1.3.0');add([base]*4+[first],1,tag='v1.3.0');add([first]*6,1,2)
for entry in [None,1,[],{},dict(draft=False,prerelease='true')]:add([first,entry],1,2)
first_bad=copy.deepcopy(first);first_bad['assets']=[];add([first_bad,later],1,4)
for data in [raw(base)+b'{}',b'\xff',b'{"a":"\\u0000"}',raw(base).replace(b'"draft":false',b'"draft":false,"dr\\u0061ft":false'),raw(base).replace(b'"id":123',b'"id":1e2'),b' '*17000,b'']:
 add(data,result=-1)
obj=copy.deepcopy(base);obj['extra']={'x':'\0'};add(obj,result=2)
with tempfile.TemporaryDirectory(prefix='dsr-discovery-') as tmp:
 tmp=Path(tmp);corpus=tmp/'cases.bin'
 with corpus.open('wb') as f:
  f.write(struct.pack('>I',len(cases)))
  for data,ch,res,tag,notes in cases:
   f.write(struct.pack('>iIIII',res,ch,len(data),len(tag),len(notes)));f.write(data);f.write(tag);f.write(notes)
 flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
 if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 elif a.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
 json_include=Path('/usr/include/cjson')
 if not(json_include/'cJSON.h').exists():json_include=ROOT/'.test-deps'
 dependency=['-l:libcjson.so.1']
 if a.cjson_source:
  source=a.cjson_source.resolve();pin='c859b25da02955fef659d658b8f324b5cde87be3'
  if subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()!=pin:raise SystemExit('Unexpected cJSON commit')
  hashes={'cJSON.c':'298581a04a36c0165da4b0aade235c23088cb2faa58651d720ea2f3706ed0b0d','cJSON.h':'25b0145150d500498e4d209cec69c18c42cf818bffcc54690be3b895a2a16dee'}
  json_include=tmp/'cjson';json_include.mkdir()
  for name,digest in hashes.items():
   data=(source/name).read_bytes()
   if hashlib.sha256(data).hexdigest()!=digest:raise SystemExit('Unexpected cJSON hash: '+name)
   (json_include/name).write_bytes(data)
  dependency=[str(json_include/'cJSON.c'),'-lm']
 includes=[json_include,ROOT/'components/dsr_ota/include']
 subprocess.run([os.environ.get('CC','cc'),*flags,*[v for d in includes for v in ['-I',str(d)]],str(ROOT/'components/dsr_ota/dsr_ota_release.c'),str(ROOT/'tests/test_ota_discovery.c'),*dependency,'-o',str(tmp/'test')],check=True)
 subprocess.run([str(tmp/'test'),str(corpus)],check=True)
