/* Actual shipped actions.js, asynchronous DOM/HTTP providers are MODEL.
 * Optional argv[2] selects an exact review snapshot before integration.
 * No radio, browser, firmware execution or backend-admission proof. */
'use strict';
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict'),path=require('node:path');
const source=fs.readFileSync(process.argv[2]||path.join(__dirname,'../components/dsr_platform/web/actions.js'),'utf8');
const flush=()=>new Promise(resolve=>setImmediate(resolve));
let checks=0;
const eq=(a,b,message)=>{assert.deepEqual(a,b,message);checks++;},ok=(value,message)=>{assert(value,message);checks++;};
class Node{
 constructor(tag){this.tag=tag;this.children=[];this.listeners={};this.parent=null;this.textContent='';this.scrollTop=0;}
 get isConnected(){return this.tag==='root'||!!this.parent?.isConnected;}
 append(...nodes){for(const n of nodes){n.parent=this;this.children.push(n);}}
 replaceChildren(...nodes){for(const n of this.children)n.parent=null;this.children=[];this.append(...nodes);}
 addEventListener(type,fn){this.listeners[type]=fn;}
}
async function fixture(){
 const base=new Node('root'),workspace=new Node('workspace'),parent=new Node('div');base.append(workspace);workspace.append(parent);
 const document={documentElement:{lang:'en'},createElement:tag=>new Node(tag),querySelector:()=>workspace};
 const config={schema:3,automations:[],revision:7,groups:[{id:1,name:'Group A',members:[42]}],scenes:[{id:2,name:'Scene B',actions:[{device:42,command:'off'}]}],dashboards:[],active_dashboard:1};
 let status={boot_id:17,admission_generation:0,revision:7,run_id:0,object_id:0,kind:0,nonce:'',available:true,active:false,cancelled:false,current:0,steps:[]};
 const requests=[],holds=new Map();
 const c={window:{},document,config:null,page:'groups',dirty:false,conflict:false,queueMicrotask,Uint32Array,
  crypto:{getRandomValues:arr=>{arr[0]=12;arr[1]=requests.length;return arr;}},
  request:async(route,options)=>{
   const body=options?.body===undefined?undefined:JSON.parse(options.body);requests.push({route,method:options?.method||'GET',body});
   const held=holds.get(route)?.shift();let captured;
   if(route==='actions/status')captured=structuredClone(status);
   else if(route==='config')captured=structuredClone(config);
   else if(route.endsWith('/run')){
    // Match the real backend's independent boot/generation/revision admission.
    const admitted=body.boot_id===status.boot_id&&body.admission_generation===status.admission_generation&&body.revision===config.revision&&!status.active;
    requests.at(-1).admitted=admitted;
    if(admitted)status={...status,admission_generation:status.admission_generation+1,run_id:status.run_id+1,object_id:body.id,kind:route.startsWith('groups')?0:1,nonce:body.nonce,active:true,available:false,steps:[{layout_id:42,command:body.on?1:2,outcome:0,result:0}]};
    captured={run_id:admitted?status.run_id:0,result:admitted?0:11};
   }else throw Error('Unexpected '+route);
   if(held?.stage==='request')await held.promise;
   return{json:async()=>{if(held?.stage==='json')await held.promise;return captured;}};
  }};
 vm.runInNewContext(source,c);
 function mount(mode){parent.replaceChildren();c.page=mode;c.window.dsrActionsMount(parent,mode);}
 mount('groups');await flush();
 function hold(route,stage='request'){
  let release,reject;const promise=new Promise((yes,no)=>{release=yes;reject=no;});
  const held={stage,promise,release,reject};if(!holds.has(route))holds.set(route,[]);holds.get(route).push(held);return held;
 }
 const all=(tag,node=parent)=>node.children.flatMap(child=>[...(child.tag===tag?[child]:[]),...all(tag,child)]);
 const text=(node=parent)=>node.textContent+' '+node.children.map(text).join(' ');
 const button=label=>{const found=all('button').find(n=>n.textContent===label);assert(found,'Missing '+label);return found;};
 return{c,requests,hold,mount,text,button,refresh:()=>c.window.dsrActionsRefresh(),set:patch=>status={...status,...patch},status:()=>status};
}
const tests=[];
for(const reboot of [false,true])tests.push([reboot?'stale GET cannot restore previous boot':'same-generation stale GET cannot regress completed progress',async()=>{
 const f=await fixture(),post=f.hold('groups/run'),running=f.button('Turn group on').listeners.click();await flush();
 eq(f.requests.filter(r=>r.route==='groups/run').length,1);ok(f.status().active);
 const old=f.hold('actions/status'),oldRefresh=f.refresh();await flush();
 f.set(reboot?{boot_id:18,admission_generation:0,run_id:0,nonce:'',active:false,available:true,current:0,steps:[]}:{active:false,available:true,current:1,steps:[{layout_id:42,command:1,outcome:1,result:0}]});
 post.release();await running;await flush();
 const completed=f.text();ok(completed.includes(reboot?'remote restarted':'Confirmed'));ok(!f.button('Turn group on').disabled);
 old.release();await oldRefresh;
 eq(f.text(),completed,'late GET replaced the newer completed/boot snapshot');ok(!f.button('Turn group on').disabled);
 eq(f.requests.filter(r=>r.route.endsWith('/run')).length,1,'GET reconciliation must not replay a command');
}]);
for(const stage of ['request','json'])tests.push(['leave/reopen cancels unsubmitted run preflight at '+stage,async()=>{
 const f=await fixture(),old=f.hold('actions/status',stage),running=f.button('Turn group on').listeners.click();await flush();
 eq(f.requests.filter(r=>r.route.endsWith('/run')).length,0);
 f.c.page='rooms';f.mount('scenes');await flush();ok(f.text().includes('Scene B'));
 old.release();await running;await flush();
 const runs=f.requests.filter(r=>r.route.endsWith('/run'));
 eq(runs.length,0,'old group preflight submitted after mounting a new scene view: '+JSON.stringify(runs));
 ok(!f.status().active,'backend admitted the old command because nonce/generation were still valid');
 ok(!f.button('Run scene').disabled);
}]);
for(const stage of ['request','json'])tests.push(['async editor completion invalidates unsubmitted run preflight at '+stage,async()=>{
 const f=await fixture(),old=f.hold('actions/status',stage),running=f.button('Turn group on').listeners.click();await flush();
 eq(f.requests.filter(r=>r.route.endsWith('/run')).length,0);
 // Shipped panel.js uploadPhoto() can finish after navigating to groups;
 // its room.photo assignment calls changed() without replacing this view.
 f.c.dirty=true;
 old.release();await running;await flush();
 const runs=f.requests.filter(r=>r.route.endsWith('/run'));
 eq(runs.length,0,'preflight submitted after an asynchronous editor completion made the draft dirty: '+JSON.stringify(runs));
 ok(!f.status().active);ok(f.button('Turn group on').disabled);
}]);
(async()=>{
 let failed=0;
 for(const [name,test] of tests){try{await test();console.log('PASS: '+name);}catch(error){failed++;console.error('FAIL: '+name+'\n'+error.message);}}
 console.log(`${checks} actual actions async checks; ${failed} failing cases; DOM/HTTP/admission MODEL`);
 if(failed)process.exitCode=1;
})().catch(error=>{console.error(error);process.exitCode=1;});
