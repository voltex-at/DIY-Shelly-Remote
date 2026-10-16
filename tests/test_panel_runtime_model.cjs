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
async function fixture(cover=false){
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
 const devices=[makeDevice('shelly-a',0),makeDevice('shelly-b',1)];if(cover===2)Object.assign(devices[0],{endpoint_kind:2,has_switch:false,has_brightness:true,brightness:38,can_light:true,transitioning:false,has_brightness_target:false,brightness_target:0,light_errors:0,light_conditions:0});else if(cover)Object.assign(devices[0],{endpoint_kind:1,has_switch:false,cover_state:5,has_position:true,position:38,has_target:false,target:0,can_move:true,can_position:true,can_stop:true,stop_pending:false,cover_errors:0});
 const state={boot:17,devices,gets:0,posts:[],dropPost:false,stale:null,hold:false,release:null};
 const context={window:{},document,page:'devices',config:null,queueMicrotask,performance:{now:()=>clock},
  MutationObserver:class{constructor(fn){observer=fn;}observe(){}},
  clearTimeout:id=>timers.delete(id),setTimeout:(fn,delay)=>{const id=++timerSerial;timers.set(id,{fn,at:clock+delay});return id;},
  request:async(route,options)=>{
   if(options?.method==='POST'){
    const body=JSON.parse(options.body);state.posts.push(body);
    const d=state.devices.find(item=>item.device_id===body.device_id&&item.generation===body.generation&&item.channel===body.channel&&(item.endpoint_kind??0)===(body.endpoint_kind??0));assert(d);
    eq(route,body.endpoint_kind===1?'devices/cover':body.endpoint_kind===2?'devices/light':'devices/output');d.operation_id=(d.operation_id+1)>>>0;if(!d.operation_id)d.operation_id=1;d.pending=true;if(state.dropPost)throw Error('accepted but response lost');
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
 {
  const f=await fixture();f.state.devices[0].operation_id=2;await f.refresh();f.state.stale=clone(f.state.devices);f.state.stale[0].operation_id=1;f.state.dropPost=true;await f.button(0).listeners.click();ok(f.button(0).disabled);await f.button(0).listeners.click();eq(f.state.posts.length,1);f.state.stale=null;f.state.devices[0].pending=false;await f.refresh();ok(!f.button(0).disabled);
 }
 {
  const f=await fixture();f.state.devices[0].operation_id=0xffffffff;await f.refresh();f.state.stale=clone(f.state.devices);await f.button(0).listeners.click();eq(f.state.devices[0].operation_id,1);ok(f.button(0).disabled);await f.button(0).listeners.click();eq(f.state.posts.length,1);f.state.stale=null;f.state.devices[0].pending=false;await f.refresh();ok(!f.button(0).disabled);
 }
 {
  const f=await fixture(true),card=f.cards()[0],actions=card.children[5].children,range=actions[3],preview=card.children[6];
  ok(!actions[0].disabled&&!actions[1].disabled&&!actions[2].disabled&&!range.disabled);ok(preview.textContent.includes('38%'));
  range.value='90';range.listeners.input();range.listeners.pointercancel();await range.listeners.change();eq(f.state.posts.length,0);eq(range.value,'38');
  range.value='67';range.listeners.input();eq(f.state.posts.length,0);ok(preview.textContent.includes('67%'));eq(f.state.devices[0].position,38);
  const gets=f.state.gets;f.locale('de');eq(f.state.gets,gets);eq(f.cards()[0],card);ok(preview.textContent.includes('67%'));
  await range.listeners.change();eq(f.state.posts[0],{device_id:'shelly-a',generation:2,channel:0,endpoint_kind:1,command:'position',position:67});ok(actions[0].disabled&&range.disabled&&!actions[2].disabled);
  await actions[2].listeners.click();eq(f.state.posts[1],{device_id:'shelly-a',generation:2,channel:0,endpoint_kind:1,command:'stop'});ok(actions[2].disabled);await actions[2].listeners.click();eq(f.state.posts.length,2);
  f.state.devices[0].pending=false;await f.refresh();ok(!actions[0].disabled&&!actions[2].disabled);eq(f.state.devices[0].position,38);
  f.state.devices[0].can_position=false;f.state.devices[0].has_position=false;await f.refresh();ok(range.disabled&&!actions[0].disabled);range.value='50';await range.listeners.change();eq(f.state.posts.length,2);
  f.state.devices[0].cover_errors=32;f.state.devices[0].can_move=false;await f.refresh();ok(actions[0].disabled&&actions[1].disabled&&!actions[2].disabled);
 }
 {
  const f=await fixture(true),actions=f.cards()[0].children[5].children;await actions[0].listeners.click();eq(f.state.posts[0].command,'open');ok(actions[0].disabled&&!actions[2].disabled);
  f.state.dropPost=true;await actions[2].listeners.click();eq(f.state.posts.length,2);ok(actions[2].disabled);
  f.state.devices[0].pending=false;await f.refresh();ok(actions[2].disabled);await actions[2].listeners.click();eq(f.state.posts.length,2); /* Unknown STOP cannot be cleared by a delayed older command. */
  f.state.boot++;f.state.devices[0].operation_id=0;await f.refresh();ok(!actions[2].disabled);
  f.state.devices[0].sample_age_ms=14950;await f.refresh();f.advance(60);ok(actions[0].disabled&&!actions[2].disabled);await actions[0].listeners.click();eq(f.state.posts.length,2);
 }
 {
  const f=await fixture(true),actions=f.cards()[0].children[5].children,range=actions[3];
  range.value='67';range.listeners.input();eq(f.state.posts.length,0);
  f.state.devices[0].can_position=false;await f.refresh();ok(range.disabled);
  f.state.devices[0].can_position=true;await f.refresh();ok(!range.disabled);
  await range.listeners.change();eq(f.state.posts.length,0);
 }
 for(const invalidate of [d=>d.online=false,d=>d.pending=true,d=>d.host='192.0.2.99',d=>{d.has_target=true;d.target=12;},d=>d.sample_age_ms=15001]){
  const f=await fixture(true),range=f.cards()[0].children[5].children[3];range.value='67';range.listeners.input();invalidate(f.state.devices[0]);await f.refresh();Object.assign(f.state.devices[0],{online:true,pending:false,sample_age_ms:0});await f.refresh();await range.listeners.change();eq(f.state.posts.length,0);eq(range.value,'38');
  range.value='71';range.listeners.input();await range.listeners.change();eq(f.state.posts.length,1);eq(f.state.posts[0].position,71);
 }

 {
  const f=await fixture(2),card=f.cards()[0],actions=card.children[5].children,range=actions[2],preview=card.children[6];
  eq(actions.length,3);ok(!range.disabled);ok(preview.textContent.includes('38%'));
  range.value='67';range.listeners.input();eq(f.state.posts.length,0);await f.refresh();eq(f.cards()[0],card);eq(range.value,'67');f.locale('de');eq(range.value,'67');ok(preview.textContent.includes('67%'));
  await range.listeners.change();eq(f.state.posts[0],{device_id:'shelly-a',generation:2,channel:0,endpoint_kind:2,command:'brightness',brightness:67});ok(range.disabled);eq(f.state.devices[0].brightness,38);
  f.state.devices[0].pending=false;f.state.devices[0].brightness=67;await f.refresh();ok(!range.disabled);eq(range.value,'67');
  await actions[1].listeners.click();eq(f.state.posts[1].command,'off');ok(!('brightness' in f.state.posts[1]));f.state.devices[0].pending=false;f.state.devices[0].output=false;await f.refresh();
  f.state.devices[0].brightness=0;f.state.devices[0].output=true;await f.refresh();ok(preview.textContent.includes('0%'));ok(card.children[1].textContent.includes('EIN')); // zero brightness is not OFF
 }
 for(const invalidate of [d=>d.online=false,d=>d.pending=true,d=>d.host='192.0.2.99',d=>{d.transitioning=true;d.has_brightness_target=true;d.brightness_target=12;},d=>d.sample_age_ms=15001,d=>{d.can_light=false;d.light_conditions=2;}]){
  const f=await fixture(2),range=f.cards()[0].children[5].children[2];range.value='67';range.listeners.input();invalidate(f.state.devices[0]);await f.refresh();Object.assign(f.state.devices[0],{online:true,pending:false,sample_age_ms:0,can_light:true,light_conditions:0});await f.refresh();await range.listeners.change();eq(f.state.posts.length,0);eq(range.value,'38');
  range.value='0';range.listeners.input();await range.listeners.change();eq(f.state.posts[0].brightness,0);eq(f.state.posts[0].command,'brightness');
 }
 {
  const f=await fixture(2),range=f.cards()[0].children[5].children[2];range.value='90';range.listeners.input();range.listeners.pointercancel();await range.listeners.change();eq(f.state.posts.length,0);eq(range.value,'38');
  f.state.dropPost=true;range.value='50';range.listeners.input();await range.listeners.change();eq(f.state.posts.length,1);ok(range.disabled);await f.refresh();await range.listeners.change();eq(f.state.posts.length,1);ok(range.disabled);f.state.boot++;f.state.devices[0].pending=false;await f.refresh();ok(!range.disabled);
 }
 for(const bad of [{brightness:null},{brightness:-1},{brightness:101},{brightness:0.5},{can_light:'true'},{light_conditions:-1},{transitioning:null}]){
  const f=await fixture(2);Object.assign(f.state.devices[0],bad);await f.refresh();for(const card of f.cards())for(const button of card.children[5].children)ok(button.disabled);
 }
 for(const blocked of [{has_brightness:false},{light_errors:1},{light_conditions:2}]){
  const f=await fixture(2),actions=f.cards()[0].children[5].children;Object.assign(f.state.devices[0],blocked);await f.refresh();for(const control of actions)ok(control.disabled);await actions[0].listeners.click();eq(f.state.posts.length,0);
 }
 console.log(`PASS: ${checks} deterministic actual-runtime checks; delayed GET/POST loss, independent controls, freshness, epoch, locale; DOM/HTTP MODEL`);
})().catch(error=>{console.error(error);process.exitCode=1;});
