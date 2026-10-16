/* Shipped BLE view/poll functions. DOM/HTTP/radio providers are MODEL. */
'use strict';
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(require('node:path').join(__dirname,'../components/dsr_platform/web/panel.js'),'utf8');
const begin=source.indexOf('let bleGeneration=0'),end=source.indexOf('let apGeneration=0;',begin);assert(begin>=0&&end>begin);
let checks=0;const ok=x=>{assert(x);checks++;},eq=(a,b)=>{assert.deepEqual(a,b);checks++;};
const flush=()=>new Promise(resolve=>setImmediate(resolve));
(async()=>{
 let clock=0,next=0,posts=0,state=1,holdGet=false,holdPost=false,dropPost=false,releaseGet,releasePost;
 const timers=new Map(),nodes=new Map();
 class Node{constructor(tag,text){this.tag=tag;this.textContent=text||'';this.children=[];this.disabled=false;}append(...c){this.children.push(...c);for(const n of c)if(n.id)nodes.set('#'+n.id,n);}replaceChildren(...c){this.children=c;}}
 const context={page:'discovery',lanMode:'ble',m:x=>x,box:t=>new Node('box',t),el:(tag,t)=>new Node(tag,t),setText:(n,t)=>n.textContent=t,render:()=>{},notice:()=>{},serverMessage:x=>x,JSON,
  $:id=>nodes.get(id),button:(t,fn)=>Object.assign(new Node('button',t),{click:fn}),
  setTimeout:(fn,delay)=>{const id=++next;timers.set(id,{fn,at:clock+delay});return id;},clearTimeout:id=>timers.delete(id),
  request:async(route,opts)=>{
   if(route==='ble/open'){posts++;eq(opts.method,'POST');eq(JSON.parse(opts.body),{serial:42,index:1});if(holdPost)await new Promise(r=>releasePost=r);if(dropPost)throw Error('response lost');return{json:async()=>({queued:true})};}
   eq(route,'ble');const captured=state;if(holdGet)await new Promise(r=>releaseGet=r);
   return{json:async()=>({serial:42,handoff:captured,phase:3,busy:false,elapsed_ms:0,devices:[{kind:1,name:'BLU',rssi:-30,age_ms:0},{kind:0,name:'Own RPC',rssi:-40,age_ms:0}]})};
  }};
 vm.runInNewContext(source.slice(source.indexOf('function onboardingSteps('),source.indexOf('function updateOnboardingChoice('))+source.slice(begin,end)+';globalThis.makeBleView=bleView;',context);
 const view=context.makeBleView(),list=nodes.get('#ble-results'),start=nodes.get('#ble-start');
 async function advance(ms){clock+=ms;const due=[...timers].filter(([,t])=>t.at<=clock);for(const [id,t]of due){timers.delete(id);t.fn();}await flush();}
 const findOpen=()=>list.children.flatMap(n=>n.children||[]).find(n=>n.tag==='button');
 eq(posts,0);await advance(0);let open=findOpen();ok(open&&!open.disabled);eq(list.children.filter(n=>n.tag==='div').length,2);
 holdGet=true;await advance(800);holdPost=true;let pending=open.click();await flush();eq(posts,1);ok(open.disabled);await open.click();eq(posts,1);
 holdGet=false;releaseGet();await flush();ok(open.disabled);state=2;holdPost=false;releasePost();await pending;await advance(0);open=findOpen();ok(open.disabled&&start.disabled);eq(posts,1);
 state=3;await advance(800);ok(findOpen().disabled);eq(posts,1);state=4;await advance(800);ok(!findOpen().disabled);
 dropPost=true;await findOpen().click();eq(posts,2);state=3;await advance(0);ok(findOpen().disabled);await advance(800);eq(posts,2);
 const before=posts;view.children[0].click();await advance(800);eq(posts,before);eq(context.lanMode,'start');
 console.log(`PASS: ${checks} actual BLE Web view checks; scan serial/index, no MAC/PSK, explicit single POST, stale GET fence, active/expiry/navigation; DOM/HTTP MODEL`);
})().catch(e=>{console.error(e);process.exitCode=1;});
