/* Shipped actions.js; DOM/session/HTTP providers are explicit MODELS. */
'use strict';
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict'),path=require('node:path');
let checks=0;const ok=x=>{assert(x);checks++;},eq=(a,b)=>{assert.deepEqual(a,b);checks++;},flush=()=>new Promise(r=>setImmediate(r));
class Node{constructor(tag){this.tag=tag;this.children=[];this.listeners={};this.parent=null;this.textContent='';this.scrollTop=0;}get isConnected(){return this.tag==='root'||!!this.parent?.isConnected;}append(...nodes){for(const n of nodes){n.parent=this;this.children.push(n);}}replaceChildren(...nodes){for(const n of this.children)n.parent=null;this.children=[];this.append(...nodes);}addEventListener(key,fn){this.listeners[key]=fn;}}
async function fixture(){
 const base=new Node('root'),workspace=new Node('workspace'),parent=new Node('div');base.append(workspace);workspace.append(parent);const document={documentElement:{lang:'en'},createElement:t=>new Node(t),querySelector:()=>workspace};
 const published={schema:3,automations:[],revision:7,groups:[{id:1,name:'<img src=x onerror=alert(1)>',members:[42,43]}],scenes:[{id:2,name:'Sleep',actions:[{device:42,command:'off'}]}],dashboards:[{id:1,tiles:[{type:'scene',ref:2}]}],active_dashboard:1};
 let status={boot_id:17,admission_generation:0,revision:7,run_id:0,object_id:0,kind:0,nonce:'',available:true,active:false,cancelled:false,current:0,steps:[]},drop=false,admit=true,hold=false,release;
 const requests=[];let log={epoch:1,newest:1,total:1,timezone:'UTC',entries:[{sequence:1,utc:0,monotonic_ms:120000,run_id:1,object_id:1,layout_id:42,source:0,kind:0,command:1,outcome:3,result:5}]};
 const c={window:{},document,config:null,page:'groups',dirty:false,conflict:false,queueMicrotask,crypto:{getRandomValues:arr=>{arr[0]=12;arr[1]=requests.length;return arr;}},Uint32Array,request:async(route,options)=>{
  const body=options?.body===undefined?undefined:JSON.parse(options.body);requests.push({route,method:options?.method||'GET',body});
  if(route==='actions/status'){const captured=structuredClone(status);if(hold)await new Promise(r=>release=r);return{json:async()=>captured};}
  if(route==='config')return{json:async()=>structuredClone(published)};
  if(route.endsWith('/run')){if(admit)status={...status,admission_generation:status.admission_generation+1,run_id:status.run_id+1,object_id:body.id,kind:route.startsWith('groups')?0:1,nonce:body.nonce,active:true,available:false,steps:[{layout_id:42,command:body.on?1:2,outcome:0,result:0}]};if(drop)throw Error('MODEL lost POST response');return{json:async()=>({run_id:status.run_id,result:0})};}
  if(route==='actions/cancel'){status.cancelled=true;return{json:async()=>({cancelled:true})};}
  if(route==='actions/log/query')return{json:async()=>structuredClone(log)};
  if(route==='actions/log/clear'){assert.equal(options.method,'POST');assert.equal(options.body,undefined);log={...log,epoch:log.epoch+1,total:0,entries:[]};return{json:async()=>({cleared:true})};}
  throw Error('Unexpected '+route);
 }};
 vm.runInNewContext(fs.readFileSync(path.resolve(__dirname,'../components/dsr_platform/web/actions.js'),'utf8'),c);c.window.dsrActionsMount(parent,'groups');await flush();
 const all=(tag,n=parent)=>n.children.flatMap(x=>[...(x.tag===tag?[x]:[]),...all(tag,x)]),text=(n=parent)=>n.textContent+' '+n.children.map(text).join(' '),button=label=>{const b=all('button').find(n=>n.textContent===label);assert(b,'Missing '+label);return b;};
 return{c,published,requests,all,text,button,status:()=>status,set:patch=>status={...status,...patch},log:patch=>log={...log,...patch},drop:v=>drop=v,admit:v=>admit=v,hold:()=>hold=true,release:()=>{hold=false;release();},refresh:()=>c.window.dsrActionsRefresh(),mount:mode=>{parent.replaceChildren();c.page=mode;c.window.dsrActionsMount(parent,mode);}};
}
(async()=>{
 const f=await fixture();ok(f.text().includes('<img src=x onerror=alert(1)>'));eq(f.all('img').length,0);eq(f.requests.filter(r=>r.route.endsWith('/run')).length,0);ok(!f.button('Turn group on').disabled);
 const original=JSON.stringify(f.published);f.c.dirty=true;await f.refresh();ok(f.button('Turn group on').disabled);await f.button('Turn group on').listeners.click();eq(f.requests.filter(r=>r.route.endsWith('/run')).length,0);f.c.dirty=false;
 await f.refresh();await f.button('Turn group on').listeners.click();const run=f.requests.find(r=>r.route==='groups/run');eq(Object.keys(run.body).sort(),['admission_generation','boot_id','id','nonce','on','revision']);eq(run.body.id,1);eq(run.body.boot_id,17);eq(run.body.admission_generation,0);eq(run.body.on,true);ok(/^[0-9a-f]{16}$/.test(run.body.nonce));ok(!JSON.stringify(run.body).includes('host'));ok(f.button('Turn group on').disabled);
 f.set({active:false,available:true,current:1,steps:[{layout_id:42,command:1,outcome:3,result:5}]});await f.refresh();ok(f.text().includes('Uncertain result'));eq(f.requests.filter(r=>r.route.endsWith('/run')).length,1);eq(JSON.stringify(f.published),original);
 // Lost response after admission is resolved by GET nonce, never another POST.
 f.drop(true);await f.button('Turn group off').listeners.click();ok(f.text().includes('Admission is unconfirmed'));const count=f.requests.filter(r=>r.route.endsWith('/run')).length;await f.refresh();ok(f.text().includes('Running'));eq(f.requests.filter(r=>r.route.endsWith('/run')).length,count);
 f.set({active:false,available:true});await f.refresh();f.admit(false);await f.button('Turn group on').listeners.click();ok(f.text().includes('Admission is unconfirmed'));await f.refresh();ok(f.button('Turn group on').disabled);f.c.document.documentElement.lang='de';f.c.window.dsrActionsLocale();ok(f.text().includes('Annahme unbestätigt'));eq(f.requests.filter(r=>r.route.endsWith('/run')).length,count+1);
 // Navigation/reconnect preserves uncertainty; a new boot ends the old epoch.
 f.mount('groups');await flush();ok(f.button('Gruppe einschalten').disabled);f.set({boot_id:18,admission_generation:0,run_id:0,nonce:'',steps:[]});await f.refresh();ok(f.text().includes('neu gestartet'));ok(!f.button('Gruppe einschalten').disabled);eq(f.requests.filter(r=>r.route.endsWith('/run')).length,count+1);
 f.c.document.documentElement.lang='en';f.c.window.dsrActionsLocale();f.drop(false);f.admit(true);f.mount('scenes');await flush();await f.button('Run scene').listeners.click();const scene=f.requests.filter(r=>r.route==='scenes/run').at(-1);ok(scene&&!('on' in scene.body));eq(scene.body.id,2);eq(scene.body.boot_id,18);
 await f.button('Cancel remaining steps').listeners.click();eq(f.requests.filter(r=>r.route==='actions/cancel').at(-1).body,{boot_id:18,run_id:f.status().run_id});
 f.mount('activity');await flush();ok(f.text().includes('UTC unset'));ok(f.text().includes('120 s'));const query=f.requests.filter(r=>r.route==='actions/log/query').at(-1);eq(query.body,{epoch:0,offset:0,limit:16});await f.button('Clear RAM activity log').listeners.click();ok(f.text().includes('No entries'));eq(f.requests.filter(r=>r.route.endsWith('/run')).length,count+2);eq(JSON.stringify(f.published),original);
 f.log({epoch:2,total:1,entries:[{utc:0,monotonic_ms:120000,layout_id:43,kind:2,command:0,outcome:2,result:5}]});await f.refresh();ok(f.text().includes('State change'));ok(f.text().includes('Error or no response'));ok(!f.text().includes('Confirmed'));ok(!f.text().includes(' · Off ·'));
 console.log(`PASS: ${checks} shipped actions JS checks; persisted definitions, dirty guard, metadata/nonce/generation, lost POST GET-only, reboot, cancel, redacted activity/UTC/clear, locale; DOM/HTTP MODEL`);
})().catch(e=>{console.error(e);process.exitCode=1;});
