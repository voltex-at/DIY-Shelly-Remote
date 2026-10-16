/* Executes the actual runtime.js with a minimal DOM and deterministic clock.
 * DOM/layout, HTTP/auth and registry are MODEL; use Playwright for real DOM QA.
 * Optional argument selects runtime.js from another checkout for review. */
'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(process.argv[2]||path.resolve(__dirname,'../components/dsr_platform/web/runtime.js'),'utf8');
let checks=0;
const eq=(a,b)=>{assert.deepEqual(a,b);checks++;},ok=value=>{assert(value);checks++;};
const clone=value=>JSON.parse(JSON.stringify(value));
const flush=()=>new Promise(resolve=>setImmediate(resolve));
async function fixture(){
 let clock=0,timerSerial=0,observer;
 const timers=new Map();
 class Node {
  constructor(tag){this.tag=tag;this.children=[];this.dataset={};this.listeners={};this.parent=null;this.textContent='';this.disabled=false;this.hidden=false;}
  get isConnected(){return this===root||!!this.parent?.isConnected;}
  append(...nodes){for(const node of nodes){node.parent=this;this.children.push(node);}}
  prepend(...nodes){for(const node of nodes.reverse()){node.parent=this;this.children.unshift(node);}}
  remove(){if(this.parent)this.parent.children=this.parent.children.filter(node=>node!==this);this.parent=null;}
  setAttribute(){}
  addEventListener(type,fn){this.listeners[type]=fn;}
  querySelector(tag){for(const child of this.children){if(child.tag===tag)return child;const found=child.querySelector(tag);if(found)return found;}return null;}
 }
 const root=new Node('root'),workspace=new Node('workspace'),parent=new Node('div');
 root.append(workspace);workspace.append(parent);
 const document={documentElement:{lang:'en'},createElement:tag=>new Node(tag),querySelector:selector=>selector==='#workspace'?workspace:null};
 const makeDevice=(device_id,channel)=>({device_id,generation:2,channel,host:'192.0.2.'+(channel+10),name:device_id,online:true,output:false,has_switch:true,has_power:true,power_w:2.5,has_voltage:false,has_current:false,pending:false,error:0,sample_age_ms:0,operation_id:0});
 const state={boot:17,devices:[makeDevice('shelly-a',0),makeDevice('shelly-b',1)],gets:0,posts:[],dropPost:false,stale:null,hold:false,release:null};
 const context={window:{},document,page:'devices',config:null,queueMicrotask,performance:{now:()=>clock},
  MutationObserver:class{constructor(fn){observer=fn;}observe(){}},
  clearTimeout:id=>timers.delete(id),setTimeout:(fn,delay)=>{const id=++timerSerial;timers.set(id,{fn,at:clock+delay});return id;},
  request:async(route,options)=>{
   if(options?.method==='POST'){
    const body=JSON.parse(options.body);state.posts.push(body);
    const d=state.devices.find(item=>item.device_id===body.device_id&&item.generation===body.generation&&item.channel===body.channel);assert(d);
    d.operation_id++;d.pending=true;if(state.dropPost)throw Error('accepted but response lost');
    return{json:async()=>({accepted:true,operation_id:d.operation_id,boot_id:state.boot})};
   }
   eq(route,'devices/runtime');state.gets++;
   const snapshot={boot_id:state.boot,devices:clone(state.stale||state.devices)};
   if(state.hold)await new Promise(resolve=>{state.release=resolve;});
   return{json:async()=>snapshot};
  }};
 vm.runInNewContext(source,context,{filename:'runtime.js'});
 function advance(ms){
  const end=clock+ms;
  for(;;){let earliest=null;for(const entry of timers)if(entry[1].at<=end&&(!earliest||entry[1].at<earliest[1].at))earliest=entry;if(!earliest)break;clock=earliest[1].at;timers.delete(earliest[0]);earliest[1].fn();}
  clock=end;
 }
 function mount(){const detached=new Node('view');context.window.dsrRuntimeMount(detached);parent.append(detached);return detached;}
 const view=mount();await flush();
 const cards=()=>view.children[0].children[2].children;
 const button=index=>cards()[index].children[5].children[0];
 return{state,context,view,cards,button,advance,mount,locale:lang=>{document.documentElement.lang=lang;observer();},refresh:()=>context.window.dsrRuntimeRefresh()};
}
(async()=>{
 {
  const f=await fixture();eq(f.state.gets,1);eq(f.cards().length,2);ok(!f.button(0).disabled);ok(!f.button(1).disabled);
  f.state.stale=clone(f.state.devices);await f.button(0).listeners.click();eq(f.state.posts.length,1);ok(f.button(0).disabled);ok(!f.button(1).disabled);
  await f.button(0).listeners.click();eq(f.state.posts.length,1);
  await f.button(1).listeners.click();eq(f.state.posts.length,2);eq(f.state.posts[1],{device_id:'shelly-b',generation:2,channel:1,on:true});
  f.state.stale=null;f.state.devices[1].pending=false;f.state.devices[1].output=true;await f.refresh();ok(!f.button(1).disabled);ok(f.button(0).disabled);
 }
 {
  const f=await fixture();f.state.hold=true;const earlierGet=f.refresh();await flush();f.state.dropPost=true;
  await f.button(0).listeners.click();eq(f.state.posts.length,1);ok(f.button(0).disabled);ok(!f.button(1).disabled);
  await f.button(0).listeners.click();eq(f.state.posts.length,1);f.state.hold=false;f.state.release();await earlierGet;ok(f.button(0).disabled);
  f.state.devices[0].pending=false;await f.refresh();ok(!f.button(0).disabled);eq(f.state.posts.length,1);
 }
 {
  const f=await fixture();f.state.devices[0].sample_age_ms=14950;await f.refresh();ok(!f.button(0).disabled);const gets=f.state.gets;
  f.advance(60);ok(f.button(0).disabled);eq(f.state.gets,gets);await f.button(0).listeners.click();eq(f.state.posts.length,0);
  f.state.devices[0].sample_age_ms=2000;f.state.hold=true;const slow=f.refresh();await flush();f.advance(14000);f.state.hold=false;f.state.release();await slow;ok(f.button(0).disabled);
 }
 {
  const f=await fixture();await f.button(0).listeners.click();ok(f.button(0).disabled);f.state.boot++;f.state.devices[0].pending=false;f.state.devices[0].operation_id=0;
  await f.refresh();ok(!f.button(0).disabled);eq(f.state.posts.length,1);
  const card=f.cards()[0],button=f.button(0),gets=f.state.gets;f.locale('de');eq(f.cards()[0],card);eq(f.button(0),button);eq(f.state.gets,gets);ok(button.textContent.includes('Einschalten'));
  f.state.devices=[f.state.devices[0],clone(f.state.devices[0])];await f.refresh();ok(f.button(0).disabled);ok(f.button(1).disabled);
 }
 console.log(`PASS: ${checks} deterministic actual-runtime checks; delayed GET/POST loss, independent controls, freshness, epoch, locale; DOM/HTTP MODEL`);
})().catch(error=>{console.error(error);process.exitCode=1;});
