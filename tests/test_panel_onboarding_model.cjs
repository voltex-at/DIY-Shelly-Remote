/* Execute shipped choice/manual/transport functions. DOM and HTTP are MODEL;
 * real browser geometry and locale preservation run separately in CI. */
'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const source=fs.readFileSync(path.join(__dirname,'../components/dsr_platform/web/panel.js'),'utf8');
const slice=(start,end)=>{const a=source.indexOf(start),b=source.indexOf(end,a);assert(a>=0&&b>a);return source.slice(a,b);};
let checks=0;const ok=x=>{assert(x);checks++;},eq=(a,b)=>{assert.deepEqual(a,b);checks++;};
class Node{
 constructor(tag,text,cls=''){this.tag=tag;this.textContent=text??'';this.children=[];this.disabled=false;this.attrs={};this.handlers={};this.classes=new Set(cls.split(' '));this.classList={toggle:(key,on)=>on?this.classes.add(key):this.classes.delete(key)};}
 append(...children){this.children.push(...children);}setAttribute(k,v){this.attrs[k]=v;}addEventListener(type,fn){this.handlers[type]=fn;}
 click(){if(!this.disabled)return this.handlers.click?.();}replaceChildren(...c){this.children=c;}set(value){this.value=value;this.handlers.input?.();}
}
async function lifecycle(){
 const flush=()=>new Promise(resolve=>setImmediate(resolve));
 function fixture(){
  const calls=[],notices=[],timers=[],holds=new Map();let renders=0,shown=0;
  const device={host:'192.0.2.20',id:'TEST-ONLY-A',generation:2,name:'USER A',auth_required:false};
  const replies={'devices/runtime':{devices:[]},discovery:{phase:1,devices:[],selected:device,capabilities:{switches:1}},status:{device_busy:false,busy:false,device_result:0},config:{devices:[{host:device.host,channel:0,room:7,name:device.name}]}};
  const context={dirty:false,saving:false,page:'discovery',lanMode:'start',lanGeneration:0,lanCredentials:null,lanSelected:null,lanSnapshot:null,lanBound:[],config:{rooms:[]},
   m:(key,...args)=>key.replace(/\{(\d+)\}/g,(_,i)=>args[i]),el:(...args)=>new Node(...args),setText:(n,t)=>n.textContent=t,setUIAttribute:(n,k,v)=>n.setAttribute(k,v),
   button:(text,fn,cls)=>{const n=new Node('button',text,cls);n.addEventListener('click',fn);return n;},notice:value=>notices.push(value),serverMessage:x=>x,lanError:code=>'error '+code,failure:x=>Error(x),JSON,Number,
   $:()=>new Node('p'),render:()=>renders++,showConfig:value=>{context.config=value;shown++;},setTimeout:fn=>{timers.push(fn);return timers.length;},
   request:async(route,options)=>{
    calls.push({route,options});const held=holds.get(route)?.shift(),captured=JSON.parse(JSON.stringify(replies[route]||{}));
    if(held?.stage==='request')await held.promise;
    return{json:async()=>{if(held?.stage==='json')await held.promise;return captured;}};
   }};
  vm.createContext(context);
  vm.runInContext(slice('let onboardingChoice=null','let lanBound=[];')+slice('function openDiscovery(', 'function discoveryView(')+`
   globalThis.api={enter:openDiscovery,methods:onboardingMethods,scan:scanLan,inspect:inspectLan,poll:pollLan,bind:bindLan,state(){return{choice:onboardingChoice,starting:onboardingStarting,next:onboardingNext,generation:lanGeneration};}};`,context);
  function hold(route,stage='request'){
   let release,reject;const promise=new Promise((yes,no)=>{release=yes;reject=no;}),held={stage,promise,release,reject};
   if(!holds.has(route))holds.set(route,[]);holds.get(route).push(held);return held;
  }
  function enter(){context.api.enter();const parent=new Node('div');context.api.methods(parent);return parent;}
  function selectLan(parent){parent.children[1].children[2].click();return context.api.state().next.click();}
  function cancel(parent){parent.children[2].children[0].click();}
  async function inspect(){context.lanMode='results';await context.api.inspect(device,{username:'owner',password:'MODEL-only-password'});}
  return{context,calls,notices,timers,replies,device,hold,enter,selectLan,cancel,inspect,renders:()=>renders,shown:()=>shown,flush};
 }
 // Cancelling while runtime GET/body parsing is pending must also prevent the
 // not-yet-submitted scan POST, not merely hide its eventual result.
 for(const stage of ['request','json']){
  const f=fixture(),parent=f.enter(),held=f.hold('devices/runtime',stage),pending=f.selectLan(parent);await flush();
  f.cancel(parent);f.enter();const renders=f.renders();held.release();await pending;
  eq(f.calls.filter(c=>c.options?.method==='POST').length,0);eq(f.context.lanMode,'start');eq(f.notices.length,0);eq(f.renders(),renders);ok(!f.context.api.state().starting);
 }
 {
  const f=fixture();f.enter();f.context.lanMode='results';await f.context.api.scan({type:'click'});await flush();
  eq(f.calls.filter(c=>c.route==='discovery/scan').length,1);eq(f.context.lanMode,'scan');
 }
 // An old success OR rejection/finally cannot release a newer Next owner.
 for(const reject of [false,true]){
  const f=fixture(),first=f.enter(),old=f.hold('discovery/scan'),a=f.selectLan(first);await flush();
  f.cancel(first);const second=f.enter(),current=f.hold('discovery/scan'),b=f.selectLan(second);await flush();
  const renders=f.renders(),notices=f.notices.length;if(reject)old.reject(Error('old failure'));else old.release();await a;
  eq(f.context.lanMode,'start');ok(f.context.api.state().starting&&f.context.api.state().next.disabled);eq(f.renders(),renders);eq(f.notices.length,notices);
  current.release();await b;await flush();ok(!f.context.api.state().starting);eq(f.context.lanMode,'scan');eq(f.calls.filter(c=>c.route==='discovery/scan').length,2);
 }
 // Late inspect/poll completions may not overwrite the new session's target,
 // snapshot, notices or view. Cover both await boundaries and error paths.
 for(const route of ['discovery/inspect','discovery'])for(const stage of route==='discovery'?['request','json']:['request'])for(const reject of [false,true]){
  const f=fixture();f.enter();const held=f.hold(route,stage);
  const pending=route==='discovery/inspect'?f.inspect():f.context.api.poll(f.context.api.state().generation);await flush();
  f.enter();f.context.lanSnapshot={marker:'new session'};const renders=f.renders(),notices=f.notices.length;
  if(reject)held.reject(Error('old failure'));else held.release();await pending;
  eq(f.context.lanMode,'start');eq(f.context.lanSnapshot,{marker:'new session'});eq(f.renders(),renders);eq(f.notices.length,notices);eq(f.timers.length,0);
 }
 // Same ID alone is insufficient for a shared singleton discovery snapshot.
 for(const mismatch of [{host:'192.0.2.99'},{generation:1}]){
  const f=fixture();f.enter();f.context.lanMode='inspect';f.context.lanSelected=f.device;
  f.replies.discovery={phase:6,selected:{...f.device,...mismatch},capabilities:{switches:1}};
  await f.context.api.poll(f.context.api.state().generation);eq(f.context.lanMode,'credentials');ok(!f.notices.some(v=>v.includes('potwierdzone odpowiedzią')));
 }
 // Accepted bind may continue in the firmware after navigation. Its late UI
 // acknowledgement must not navigate, clear or reload the replacement view.
 for(const route of ['devices/bind','status','config'])for(const stage of route==='devices/bind'?['request']:['request','json']){
  const f=fixture();f.enter();f.context.lanMode='components';f.context.lanSelected=f.device;f.context.lanCredentials={username:'owner',password:'MODEL-only-password'};
  const held=f.hold(route,stage),button=new Node('button'),pending=f.context.api.bind(f.device.name,7,0,button);await flush();
  if(route!=='devices/bind'){eq(f.timers.length,1);f.timers.shift()();await flush();}
  f.enter();const calls=f.calls.length,renders=f.renders(),notices=f.notices.length;held.release();await pending;
  eq(f.context.lanMode,'start');eq(f.context.page,'discovery');eq(f.calls.length,calls);eq(f.renders(),renders);eq(f.notices.length,notices);eq(f.shown(),0);
 }
 {
  const f=fixture();f.enter();f.context.lanMode='components';f.context.lanSelected=f.device;f.context.lanCredentials={};
  const pending=f.context.api.bind(f.device.name,7,0,new Node('button'));await flush();f.enter();const calls=f.calls.length;f.timers.shift()();await pending;
  eq(f.calls.length,calls);eq(f.shown(),0);eq(f.context.lanMode,'start');
 }
 {
  const f=fixture();f.enter();f.context.lanMode='components';f.context.lanSelected=f.device;f.context.lanCredentials={};
  const pending=f.context.api.bind(f.device.name,7,0,new Node('button'));await flush();f.timers.shift()();await pending;
  eq(f.context.page,'devices');eq(f.shown(),1);eq(f.calls.filter(c=>c.route==='config').length,1);ok(f.notices.at(-1).includes('pomieszczenie zapisane'));
 }
 // Integration with typed Cover: discovery may expose only Cover; BIND must
 // retain endpoint_kind=1. A Switch with the same host/channel must yield the
 // existing partial-publication warning rather than the complete-save notice.
 {
  const f=fixture();f.enter();f.context.lanMode='inspect';f.context.lanSelected=f.device;
  f.replies.discovery={phase:6,selected:f.device,capabilities:{switches:0,covers:4}};
  await f.context.api.poll(f.context.api.state().generation);eq(f.context.lanMode,'components');eq(f.context.lanSnapshot.capabilities.covers,4);
 }
 for(const kind of ['cover','relay']){
  const f=fixture();f.enter();f.context.lanMode='components';f.context.lanSelected=f.device;f.context.lanCredentials={};
  f.replies.config={devices:[{host:f.device.host,channel:2,room:7,name:f.device.name,kind}]};
  const button=new Node('button'),pending=f.context.api.bind(f.device.name,7,'cover:2',button);await flush();
  const body=JSON.parse(f.calls.find(c=>c.route==='devices/bind').options.body);
  eq(body,{host:f.device.host,expected_id:f.device.id,name:f.device.name,room:7,channel:2,endpoint_kind:1});
  f.timers.shift()();await pending;
  eq(f.shown(),1);eq(f.context.page,'devices');
  eq(f.calls.filter(c=>c.route==='devices/bind').length,1);
  ok(f.notices.at(-1).includes(kind==='cover'?'pomieszczenie zapisane':'wymaga sprawdzenia'));
 }
}
(async()=>{
 let requests=[],renders=0,polls=0,fail=false,hold=false,release;
 const context={dirty:false,saving:false,page:'discovery',lanMode:'start',lanGeneration:0,lanCredentials:null,lanSelected:null,lanBound:[],config:{rooms:[{id:7,name:'USER room'}]},
  m:(key,...args)=>key.replace(/\{(\d+)\}/g,(_,i)=>args[i]),el:(...args)=>new Node(...args),setText:(n,t)=>n.textContent=t,setUIAttribute:(n,k,v)=>n.setAttribute(k,v),
  button:(text,fn,cls)=>{const n=new Node('button',text,cls);n.addEventListener('click',fn);return n;},notice:()=>{},serverMessage:x=>x,failure:x=>Error(x),JSON,Number,
  render:()=>renders++,pollLan:()=>polls++,setTimeout:fn=>{queueMicrotask(fn);return 1;},load:async()=>renders++,
  request:async(route,opts)=>{requests.push({route,opts});if(opts?.method==='POST'&&hold)await new Promise(r=>release=r);if(fail&&opts?.method==='POST')throw Error('lost response');return{json:async()=>route==='devices/runtime'?{devices:[]}:route==='status'?{device_busy:false,busy:false,device_result:0}:{devices:[]}};}};
 vm.createContext(context);
 vm.runInContext(slice('let onboardingChoice=null','let lanBound=[];')+slice('function lanInput(', 'function openDiscovery(')+slice('function openDiscovery(', 'async function scanLan(')+slice('async function scanLan(', 'async function inspectLan(')+slice('async function bindLan(', 'function discoveryView(')+`
 globalThis.api={enter:openDiscovery,open:onboardingMethods,manual:manualOnboarding,setReachable(v){remoteReachable=v;updateOnboardingChoice();},state(){return{choice:onboardingChoice,starting:onboardingStarting,next:onboardingNext,cards:onboardingCards,draft:manualDraft};},reset(){onboardingChoice=null;onboardingStarting=false;page='discovery';lanMode='start';}};`,context);
 const api=context.api,buttons=node=>[node,...node.children.flatMap(buttons)].filter(n=>n.tag==='button');
 const open=()=>{api.reset();const parent=new Node('div');api.open(parent);return parent;};
 let parent=open(),state=api.state();ok(state.next.disabled);eq(requests.length,0);eq(state.cards.length,4);
 for(const row of state.cards){row.node.click();eq(api.state().choice,row.id);ok(row.node.classes.has('selected'));eq(row.node.attrs['aria-pressed'],'true');eq(requests.length,0);}
 const old=state.cards[1].node;api.setReachable(false);ok(old.disabled);old.click();eq(api.state().choice,'manual');ok(state.next.disabled);state.cards[0].node.click();ok(!state.next.disabled);eq(requests.length,0);
 await state.next.click();eq(context.lanMode,'ble');eq(requests.length,0);await state.next.click();eq(requests.length,0);
 parent=open();api.setReachable(true);state=api.state();state.cards[1].node.click();await state.next.click();eq(context.lanMode,'ap');eq(requests.length,0);
 parent=open();state=api.state();state.cards[2].node.click();hold=true;const pending=state.next.click();await new Promise(r=>setImmediate(r));ok(api.state().starting&&state.next.disabled);await state.next.click();eq(requests.filter(x=>x.opts?.method==='POST').length,1);hold=false;release();await pending;eq(context.lanMode,'scan');eq(polls,1);eq(requests.filter(x=>x.route==='discovery/scan').length,1);
 parent=open();state=api.state();state.cards[2].node.click();fail=true;await state.next.click();const count=requests.length;await new Promise(r=>setImmediate(r));eq(requests.length,count);eq(context.lanMode,'start');ok(!state.next.disabled);fail=false;
 parent=open();const cancel=buttons(parent).find(n=>n.textContent==='Anuluj');cancel.click();eq(context.page,'devices');eq(api.state().choice,null);
 parent=open();state=api.state();state.cards[3].node.click();await state.next.click();eq(context.lanMode,'manual');const form=new Node('div');api.manual(form);
 const flatten=n=>[n,...n.children.flatMap(flatten)],fields=flatten(form).filter(n=>n.tag==='input'||n.tag==='select');eq(fields.length,7);fields[0].set('192.0.2.20');fields[1].set('User manual');fields[2].set('7');fields[3].set('1');fields[4].set('owner');fields[5].set('MODEL-only-password');
 const apply=buttons(form).find(n=>n.textContent==='Sprawdź i zapisz połączenie');hold=true;const before=requests.length;apply.click();await new Promise(r=>setImmediate(r));ok(apply.disabled);apply.click();eq(requests.length,before+1);const body=JSON.parse(requests.at(-1).opts.body);eq(body,{host:'192.0.2.20',name:'User manual',room:7,channel:1,username:'owner',password:'MODEL-only-password'});eq(requests.at(-1).route,'devices/bind');hold=false;fail=true;release();await new Promise(r=>setImmediate(r));ok(!apply.disabled);const last=requests.length;await new Promise(r=>setImmediate(r));eq(requests.length,last); // no uncertain POST replay
 for(const family of ['cover','light']){
  fail=true;const target=new Node('div');api.manual(target);const inputs=flatten(target).filter(n=>n.tag==='input'||n.tag==='select');inputs[6].set(family);
  const saved=buttons(target).find(n=>n.textContent==='Sprawdź i zapisz połączenie'),count=requests.length;saved.click();await new Promise(r=>setImmediate(r));eq(requests.length,count+1);
  eq(JSON.parse(requests.at(-1).opts.body),{host:'192.0.2.20',name:'User manual',room:7,channel:1,username:'owner',password:'MODEL-only-password',endpoint_kind:family==='cover'?1:2});ok(!saved.disabled);
  const reopened=new Node('div');api.manual(reopened);eq(flatten(reopened).filter(n=>n.tag==='select').at(-1).value,family);eq(requests.length,count+1);
 }
 fail=false;parent=open();state=api.state();state.cards[2].node.click();hold=true;
 const cancelledStart=state.next.click();await new Promise(r=>setImmediate(r));
 buttons(parent).find(n=>n.textContent==='Anuluj').click();eq(context.page,'devices');
 api.enter();const reopened=new Node('div');api.open(reopened);eq(context.lanMode,'start');eq(api.state().choice,null);
 hold=false;release();await cancelledStart;eq(context.lanMode,'start');
 await lifecycle();ok(renders>0);console.log(`PASS: ${checks} shipped onboarding checks; choice zero requests, single Next/POST, cancel/reopen epochs, delayed request/body/error/finally, exact inspect target, late bind ownership; DOM/HTTP MODEL`);
})().catch(e=>{console.error(e);process.exitCode=1;});
