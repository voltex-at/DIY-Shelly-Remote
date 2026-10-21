/* Actual shipped load/showConfig/save/poll; deterministic DOM/HTTP MODEL.
 * Held request and JSON boundaries reproduce draft clobber without Chromium. */
'use strict';
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict'),path=require('node:path');
const source=fs.readFileSync(process.argv[2]||path.join(__dirname,'../components/dsr_platform/web/panel.js'),'utf8');
const slice=(a,b)=>{const start=source.indexOf(a),end=source.indexOf(b,start);assert(start>=0&&end>start);return source.slice(start,end);};
const start=source.indexOf('\n',source.indexOf('async function request('))+1;
const loadSource=source.slice(start,source.indexOf('function box(',start));
const flush=()=>new Promise(resolve=>setImmediate(resolve));let checks=0;
const ok=(v,m)=>{assert(v,m);checks++;},eq=(a,b,m)=>{assert.deepEqual(a,b,m);checks++;};
function fixture(){
 const nodes=new Map(),holds=new Map(),calls=[],notices=[],frames=[];
 const document={revision:5,dashboards:[{id:1,name:'Old saved name'}]};let server=structuredClone(document),renders=0,refreshes=0;
 const node=id=>{if(!nodes.has(id))nodes.set(id,{hidden:false,disabled:false,textContent:'',firstElementChild:{},getContext:()=>({createImageData:(w,h)=>({data:new Uint8Array(w*h*4)}),putImageData:im=>frames.push(Array.from(im.data.slice(0,8)))})});return nodes.get(id);};
 const c={document:{hidden:false},actionSession:null,config:structuredClone(document),dirty:false,conflict:false,saving:false,editSerial:0,page:'dashboards',lanGeneration:0,remoteReachable:true,window:{dsrActionsRefresh:()=>refreshes++},
  $:node,setText:(n,t)=>n.textContent=t,m:x=>x,notice:x=>notices.push(x),failure:x=>Error(x),serverMessage:x=>x,structuredClone,Uint8Array,JSON,
  render:()=>{renders++;node('#editor').firstElementChild={};},updateOnboardingChoice:()=>{},setTimeout:fn=>{queueMicrotask(fn);return 1;},
  request:async(route,options)=>{
   calls.push({route,method:options?.method||'GET'});
   if(options?.method==='PUT'){if(c.deferPut)await c.deferPut;if(c.failPut)throw Error('MODEL Save failure');const body=JSON.parse(options.body);assert.equal(body.revision,server.revision);server={...body,revision:body.revision+1};}
   const held=holds.get(route)?.shift(),captured=structuredClone(route==='config'?server:{revision:server.revision,busy:false,firmware:'MODEL'});
   if(held?.stage==='request')await held.promise;
   return{status:200,json:async()=>{if(held?.stage==='json')await held.promise;return captured;},arrayBuffer:async()=>{if(held?.stage==='body')await held.promise;return c.previewBytes||new ArrayBuffer(0);}};
  }};
 vm.createContext(c);vm.runInContext(loadSource+slice('function canonical(', 'async function poll(){if(!config')+slice('async function poll(){if(!config','async function uploadPhoto(')+'\nglobalThis.api={load,showConfig,save,poll,refreshPreview:typeof refreshPreview===\'function\'?refreshPreview:null};',c);
 function hold(route,stage){let release,reject;const promise=new Promise((yes,no)=>{release=yes;reject=no;});const h={stage,promise,release,reject};if(!holds.has(route))holds.set(route,[]);holds.get(route).push(h);return h;}
 function edit(name){c.config.dashboards[0].name=name;c.dirty=true;c.editSerial++;}
 return{c,calls,notices,frames,hold,edit,node,setServer:value=>server=structuredClone(value),renders:()=>renders,refreshes:()=>refreshes};
}
const tests=[];
for(const stage of ['request','json'])for(const change of ['edit','save','bind','reload'])tests.push([`late config ${stage} preserves newer ${change}`,async()=>{
 const f=fixture(),held=f.hold('config',stage),old=f.c.api.load();await flush();
 if(change==='edit')f.edit('Later draft B');
 else if(change==='save'){f.edit('Saved version A');await f.c.api.save();}
 else{f.setServer({revision:6,dashboards:[{id:1,name:'New binding document'}]});if(change==='bind')f.c.api.showConfig({revision:6,dashboards:[{id:1,name:'New binding document'}]});else await f.c.api.load();}
 const expected=JSON.stringify(f.c.config),dirty=f.c.dirty,renders=f.renders(),notes=f.notices.length;
 held.release();await old;eq(JSON.stringify(f.c.config),expected);eq(f.c.dirty,dirty);eq(f.renders(),renders);eq(f.notices.length,notes);
}]);
for(const stage of ['request','json'])tests.push([`stale status ${stage} cannot conflict with newer BIND and draft`,async()=>{
 const f=fixture(),held=f.hold('status',stage),old=f.c.api.poll();await flush();
 f.c.api.showConfig({revision:6,dashboards:[{id:1,name:'New bound document'}]});f.edit('Draft after binding');const n=f.calls.length,notes=f.notices.length;
 held.release();await old;ok(!f.c.conflict);eq(f.c.config.revision,6);eq(f.c.config.dashboards[0].name,'Draft after binding');eq(f.calls.length,n);eq(f.notices.length,notes);ok(f.c.remoteReachable);
}]);
tests.push(['fresh lower revision recovery still protects a dirty draft',async()=>{const f=fixture();f.c.api.showConfig({revision:6,dashboards:[{id:1,name:'New'}]});f.edit('Draft');await f.c.api.poll();ok(f.c.conflict&&f.node('#save').disabled);eq(f.calls.filter(x=>x.route==='config').length,0);eq(f.c.config.dashboards[0].name,'Draft');}]);
for(const stage of ['request','json'])tests.push([`Save begin/failure ABA invalidates config ${stage}`,async()=>{
 const f=fixture();f.edit('Keep after failed Save');const held=f.hold('config',stage),old=f.c.api.load();await flush();f.c.failPut=true;await f.c.api.save();ok(!f.c.saving&&f.c.dirty&&f.c.config.revision===5);const notes=f.notices.length;held.release();await old;eq(f.c.config.dashboards[0].name,'Keep after failed Save');ok(f.c.dirty);eq(f.notices.length,notes);
}]);
for(const stage of ['request','json'])tests.push([`navigation invalidates config ${stage}`,async()=>{
 const f=fixture(),held=f.hold('config',stage),old=f.c.api.load();await flush();f.c.page='rooms';f.c.render();const renders=f.renders();held.release();await old;eq(f.renders(),renders);eq(f.c.page,'rooms');
}]);
tests.push(['status polling never overlaps an unfinished request and recovers after failure',async()=>{const f=fixture(),held=f.hold('status','request'),old=f.c.api.poll();await flush();await f.c.api.poll();eq(f.calls.filter(x=>x.route==='status').length,1);held.reject(Error('Held response failed'));await old;ok(!f.c.remoteReachable);await f.c.api.poll();ok(f.c.remoteReachable);eq(f.calls.filter(x=>x.route==='status').length,2);}]);

tests.push(['routine status polls never request a full LCD snapshot',async()=>{const f=fixture();await f.c.api.poll();await f.c.api.poll();eq(f.calls.filter(x=>x.route==='preview.raw').length,0);eq(f.calls.filter(x=>x.route==='status').length,2);}]);
tests.push(['hidden browser tab pauses status and manual preview I/O',async()=>{const f=fixture();f.c.document.hidden=true;await f.c.api.poll();eq(f.calls.length,0);ok(typeof f.c.api.refreshPreview==='function');await f.c.api.refreshPreview();eq(f.calls.length,0);}]);
tests.push(['explicit LCD preview refresh is single-flight and releases after failure',async()=>{const f=fixture();ok(typeof f.c.api.refreshPreview==='function');const held=f.hold('preview.raw','request'),first=f.c.api.refreshPreview();await flush();await f.c.api.refreshPreview();eq(f.calls.filter(x=>x.route==='preview.raw').length,1);held.reject(Error('Snapshot transport failed'));await first;await f.c.api.refreshPreview();eq(f.calls.filter(x=>x.route==='preview.raw').length,2);ok(!f.node('#preview-refresh').disabled);}]);
tests.push(['manual snapshot still decodes genuine RGB565 red and green pixels',async()=>{const f=fixture(),bytes=new Uint8Array(320*480*2);bytes.set([0,248,224,7]);f.c.previewBytes=bytes.buffer;await f.c.api.refreshPreview();eq(f.frames,[[255,0,0,255,0,255,0,255]]);ok(f.node('#preview-status').textContent.includes('Obraz pobrany'));}]);
tests.push(['preview body arriving after the tab is hidden cannot paint',async()=>{const f=fixture();f.c.previewBytes=new ArrayBuffer(320*480*2);const held=f.hold('preview.raw','body'),old=f.c.api.refreshPreview();await flush();f.c.document.hidden=true;held.release();await old;eq(f.frames.length,0);ok(!f.node('#preview-refresh').disabled);}]);
tests.push(['fresh external revision still raises conflict and disables save',async()=>{const f=fixture();f.edit('Draft');f.setServer({revision:6,dashboards:[{id:1,name:'External'}]});await f.c.api.poll();ok(f.c.conflict&&f.node('#save').disabled);ok(f.notices.some(n=>n.includes('Konfiguracja zmieniła się')));eq(f.c.config.dashboards[0].name,'Draft');}]);
tests.push(['explicit current Reload still discards the old dirty draft',async()=>{const f=fixture();f.edit('Discard this draft');await f.c.api.load();ok(!f.c.dirty&&!f.c.conflict);eq(f.c.config.dashboards[0].name,'Old saved name');}]);
tests.push(['Reload during pending PUT cannot replace the submitted document',async()=>{
 const f=fixture();f.edit('Version A');let release;f.c.deferPut=new Promise(resolve=>release=resolve);const saving=f.c.api.save();await flush();await f.c.api.load();release();await saving;
 const durable=await(await f.c.request('config')).json();eq(f.c.config.revision,durable.revision);eq(f.c.config.dashboards[0].name,'Version A');eq(f.c.config.dashboards[0].name,durable.dashboards[0].name);ok(!f.c.dirty&&!f.c.conflict&&!f.c.saving);
}]);
tests.push(['ordinary Save retains later draft B and exact durable version A',async()=>{const f=fixture();f.edit('Version A');const held=f.hold('config','request'),saving=f.c.api.save();await flush();f.edit('Version B');held.release();await saving;ok(f.c.dirty&&!f.c.conflict&&!f.c.saving);eq(f.c.config.revision,6);eq(f.c.config.dashboards[0].name,'Version B');ok(f.notices.some(n=>n.includes('Późniejsze zmiany pozostają niezapisane.')));}]);
tests.push(['explicit Reload during delayed PUT cannot counterfeit the saved document',async()=>{
 const f=fixture();f.edit('Version A');let release;f.c.deferPut=new Promise(resolve=>release=resolve);
 const saving=f.c.api.save();await flush();ok(f.c.saving);
 await f.c.api.load();release();await saving;
 const durable=await(await f.c.request('config')).json();
 eq(f.c.config.revision,durable.revision);
 eq(f.c.config.dashboards[0].name,durable.dashboards[0].name,'local document labelled saved diverges from exact durable ACK');
 ok(!f.c.dirty&&!f.c.conflict&&!f.c.saving);
 // The ordinary, explicit discard remains available after Save settles.
 f.edit('Discard after settled Save');await f.c.api.load();
 eq(f.c.config.dashboards[0].name,'Version A');ok(!f.c.dirty);
}]);
(async()=>{let failures=0;for(const[name,test]of tests){try{await test();console.log('PASS: '+name);}catch(e){failures++;console.error('FAIL: '+name+'\n'+e.message);}}console.log(`${checks} actual config read ownership checks; ${failures} failing cases; DOM/HTTP MODEL`);if(failures)process.exitCode=1;})().catch(e=>{console.error(e);process.exitCode=1;});
