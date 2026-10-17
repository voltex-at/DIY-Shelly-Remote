#!/usr/bin/env python3
"""Independent corpus/oracle using Python's JSON implementation, not C parser."""
import argparse,copy,hashlib,json,os,random,re,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');p.add_argument('--ubsan-only',action='store_true')
p.add_argument('--cjson-source',type=Path,help='Existing pinned IDF cJSON checkout; never downloads source')
args=p.parse_args()
CJSON_COMMIT='c859b25da02955fef659d658b8f324b5cde87be3'
CJSON_HASHES={'cJSON.c':'298581a04a36c0165da4b0aade235c23088cb2faa58651d720ea2f3706ed0b0d',
              'cJSON.h':'25b0145150d500498e4d209cec69c18c42cf818bffcc54690be3b895a2a16dee'}
TAG='v1.2.4';ASSET='manifest.json';PREFIX='https://github.com/voltex-at/DIY-Shelly-Remote/releases/download/'
def selected():return dict(name=ASSET,state='uploaded',size=700,browser_download_url=PREFIX+TAG+'/'+ASSET)
base=dict(id=123456789,url='https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/123456789',tag_name=TAG,draft=False,prerelease=False,assets=[selected()])
class Invalid(ValueError):pass
def integer(s):
    if not re.fullmatch(r'0|[1-9][0-9]*',s) or int(s)>2**53-1:raise Invalid('unsigned canonical number')
    return int(s)
def forbidden(_):raise Invalid('non-integer')
def object_pairs(pairs):
    d={}
    for key,value in pairs:
        if key in d:raise Invalid('decoded duplicate key')
        d[key]=value
    return d
def token(s,limit):return isinstance(s,str) and 1<=len(s)<=limit and re.fullmatch(r'[A-Za-z0-9_.-]+',s) and s!='.' and '..' not in s
def oracle(raw,tag=TAG,asset=ASSET,channel=0):
    if not raw or len(raw)>16384:return 1,0
    if not token(tag,64) or not token(asset,96):return 1,0
    if channel!=0:return 3,0
    try:
        obj=json.loads(raw.decode('utf-8'),parse_int=integer,parse_float=forbidden,parse_constant=forbidden,object_pairs_hook=object_pairs)
        nodes=0
        def walk(value,depth=0):
            nonlocal nodes
            nodes+=1
            if nodes>512:raise Invalid('node cap')
            if isinstance(value,(dict,list)):
                depth+=1
                if depth>8:raise Invalid('depth cap')
                if isinstance(value,dict):
                    for key in value:strings(key)
                for child in value.values() if isinstance(value,dict) else value:walk(child,depth)
            elif isinstance(value,str):strings(value)
        def strings(s):
            if '\0' in s or any(0xd800<=ord(c)<=0xdfff for c in s):raise Invalid('NUL/surrogate')
        walk(obj)
    except (ValueError,UnicodeError,RecursionError):return 2,0
    if not isinstance(obj,dict):return 2,0
    required={'id':int,'url':str,'tag_name':str,'draft':bool,'prerelease':bool,'assets':list}
    if any(k not in obj or type(obj[k]) is not t for k,t in required.items()):return 2,0
    if obj['id']<1:return -1,0
    if obj['url']!='https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/'+str(obj['id']) or obj['tag_name']!=tag or obj['draft'] or obj['prerelease']:return 3,0
    if len(obj['assets'])>32:return 2,0
    if any(not isinstance(a,dict) or type(a.get('name')) is not str for a in obj['assets']):return 2,0
    matches=[a for a in obj['assets'] if a['name']==asset]
    if not matches:return 4,0
    if len(matches)>1:return 5,0
    match=matches[0]
    if any(type(match.get(k)) is not t for k,t in {'state':str,'size':int,'browser_download_url':str}.items()):return 2,0
    if match['state']!='uploaded' or not 1<=match['size']<=2048 or match['browser_download_url']!=PREFIX+tag+'/'+asset:return -1,0
    return 0,match['size']
def wire(obj,ascii=True):return json.dumps(obj,separators=(',',':'),ensure_ascii=ascii).encode('utf-8')
cases=[]
def add(value,expected=None,tag=TAG,asset=ASSET,channel=0):
    raw=value if isinstance(value,bytes) else wire(value)
    inferred,size=oracle(raw,tag,asset,channel)
    cases.append((raw,inferred if expected is None else expected,size,tag.encode(),asset.encode(),channel))
raw=wire(base);add(base);add(dict(reversed(list(base.items()))));add(b' \n'+json.dumps(base,indent=2).encode()+b'\r\t')
add(raw.replace(b'"tag_name"',b'"tag_\\u006eame"'))
add(raw.replace(b'/',b'\\/'))
add(raw.replace(b'"manifest.json"',b'"manifest\\u002ejson"'))
for tag,asset in [('release_build-2026','firmware_manifest.json'),('a'*64,'b'*96)]:
    m=copy.deepcopy(base);m['tag_name']=tag;m['assets'][0]['name']=asset;m['assets'][0]['browser_download_url']=PREFIX+tag+'/'+asset;add(m,tag=tag,asset=asset)
m=copy.deepcopy(base);m['assets'].insert(0,{'name':'Żółć 😀','body':'ignored'});add(wire(m,False))
for key in base:
    m=copy.deepcopy(base);del m[key];add(m)
for key,values in {'id':[None,True,'123',0], 'url':[None,False,123,'https://api.github.com/repos/other/repo/releases/123456789',base['url']+'0',base['url'].replace('123456789','0123456789')], 'tag_name':[None,False,TAG.upper(),'1.2.4'], 'draft':[None,0,'false',True], 'prerelease':[None,0,'false',True], 'assets':[None,{},False,'x']}.items():
    for value in values:m=copy.deepcopy(base);m[key]=value;add(m)
for key,values in {'state':[None,False,1,'new','uploaded '], 'size':[None,True,'700',0,1,2048,2049], 'browser_download_url':[None,123,PREFIX+TAG+'/other.json',PREFIX+TAG+'/'+ASSET+'?x','https://github.com.evil/voltex-at/DIY-Shelly-Remote/releases/download/'+TAG+'/'+ASSET]}.items():
    for value in values:m=copy.deepcopy(base);m['assets'][0][key]=value;add(m)
for key in ('state','size','browser_download_url'):
    m=copy.deepcopy(base);del m['assets'][0][key];add(m)
for assets in [[],[selected(),selected()],[{'name':'other.json'}],[None],[[]],[{}],[{'name':123}],[{'name':'MANIFEST.JSON'}]]:
    m=copy.deepcopy(base);m['assets']=assets;add(m)
for count in (31,32,33):
    m=copy.deepcopy(base);m['assets']=[{'name':'other-'+str(i)} for i in range(count-1)]+[selected()];add(m)
for value in ('Zażółć 😀','\b\f\n\r\t/"\\','',{'é':1},[None,True,False,{},[]]):m=copy.deepcopy(base);m['body']=value;add(wire(m,False))
for number in (1,9007199254740990,9007199254740991):
    m=copy.deepcopy(base);m['id']=number;m['url']='https://api.github.com/repos/voltex-at/DIY-Shelly-Remote/releases/'+str(number);add(m)
for number in (b'-0',b'-1',b'1.0',b'1e3',b'01',b'+1',b'9007199254740992',b'999999999999999999999999999999999999999',b'NaN',b'Infinity'):
    add(raw[:-1]+b',"ignored":'+number+b'}')
for string in (b'\\u0000',b'\\ud800',b'\\udc00',b'\\ud800x',b'\\ud800\\u0041',b'\\udc00\\ud800',b'\0',b'\xc0\x80',b'\xed\xa0\x80',b'\xf4\x90\x80\x80',b'\x80',b'\xf0\x9f'):
    add(raw[:-1]+b',"ignored":"'+string+b'"}')
for text in (b'\\ud83d\\ude00',b'\\u0105',b'\\/\\b\\f\\n\\r\\t\\"\\\\'):
    add(raw[:-1]+b',"ignored":"'+text+b'"}')
add(raw[:-1]+b',"\\u0000":1}')
add(raw[:-1]+b',"tag_name":"'+TAG.encode()+b'"}')
add(raw[:-1]+b',"tag_\\u006eame":"'+TAG.encode()+b'"}')
for duplicate in (b'{"x":1,"x":2}',b'{"x":1,"\\u0078":2}',b'[{"x":1,"x":2}]',b'{"\\ud83d\\ude00":1,"\xf0\x9f\x98\x80":2}'):
    add(raw[:-1]+b',"ignored":'+duplicate+b'}')
def nodes(value):return 1+sum(nodes(c) for c in (value.values() if isinstance(value,dict) else value if isinstance(value,list) else []))
for limit in (511,512,513):
    m=copy.deepcopy(base);m['ignored']=[];m['ignored']=[0]*(limit-nodes(m));add(m)
for depth in (7,8,9):
    ignored='leaf'
    for _ in range(depth-1):ignored=[ignored]
    m=copy.deepcopy(base);m['ignored']=ignored;add(m)
add(raw+b' '*(16384-len(raw)));add(raw+b' '*(16385-len(raw)))
for value in [b'',b'[]',b'null',b'123',raw+b'{}',raw+b'\0',b'\xef\xbb\xbf'+raw]:add(value)
for length in range(len(raw)):add(raw[:length])
add(base,tag='.',expected=1);add(base,tag='v..1',expected=1);add(base,tag='a'*65,expected=1);add(base,asset='a'*97,expected=1);add(base,channel=1,expected=3)
rng=random.Random(0x2424)
for _ in range(700):
    mutated=bytearray(raw);at=rng.randrange(len(mutated));action=rng.randrange(3)
    if action==0:mutated[at]=rng.randrange(256)
    elif action==1:del mutated[at]
    else:mutated[at:at]=bytes([rng.randrange(256)])
    add(bytes(mutated))
with tempfile.TemporaryDirectory(prefix='dsr-release-') as directory:
    tmp=Path(directory);fixture=tmp/'corpus.bin'
    with fixture.open('wb') as f:
        f.write(struct.pack('>I',len(cases)))
        for data,expected,size,tag,asset,channel in cases:
            f.write(struct.pack('>iIIIII',expected,size,len(tag),len(asset),channel,len(data)));f.write(tag);f.write(asset);f.write(data)
    flags=['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-g']
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    elif args.ubsan_only:flags+=['-fsanitize=undefined','-fno-omit-frame-pointer']
    dependency=['-l:libcjson.so.1']
    if args.cjson_source:
        source=args.cjson_source.resolve()
        head=subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()
        if head!=CJSON_COMMIT:raise SystemExit(f'Unexpected cJSON commit: {head}')
        pinned=tmp/'pinned-cjson';pinned.mkdir()
        for name,expected in CJSON_HASHES.items():
            data=(source/name).read_bytes()
            if hashlib.sha256(data).hexdigest()!=expected:raise SystemExit(f'Unexpected cJSON source hash: {name}')
            # Compile exactly the verified bytes, unaffected by later checkout edits.
            (pinned/name).write_bytes(data)
        flags+=['-I',str(pinned)]
        dependency=[str(pinned/'cJSON.c'),'-lm']
        print(f'Instrumented genuine IDF cJSON {CJSON_COMMIT}',flush=True)
    else:flags+=['-I','/usr/include/cjson']
    for folder in ['components/dsr_ota/include','.test-deps']:flags+=['-I',str(ROOT/folder)]
    binary=tmp/'release-test'
    subprocess.run([os.environ.get('CC','cc'),*flags,str(ROOT/'components/dsr_ota/dsr_ota_release.c'),str(ROOT/'tests/test_ota_release.c'),*dependency,'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(fixture)],check=True)
print(f'Independent oracle seed=0x2424: {len(cases)} cases; {sum(expected==0 for _,expected,*_ in cases)} positive candidates; no I/O or publisher trust')
