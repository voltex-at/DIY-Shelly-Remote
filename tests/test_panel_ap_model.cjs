/* Actual shipped apView, with explicit DOM/HTTP/clock MODEL providers. */
'use strict';
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(require('node:path').join(__dirname,'../components/dsr_platform/web/panel.js'),'utf8');
const begin=source.indexOf('let apGeneration=0;'),end=source.indexOf('const lanError=code=>',begin);assert(begin>=0&&end>begin);
const flush=()=>new Promise(resolve=>setImmediate(resolve));let checks=0;const ok=x=>{assert(x);checks++;},eq=(x,y)=>{assert.deepEqual(x,y);checks++;};
(async()=>{
 let clock=0,next=0,state=1,posts=0,gets=0,holdGet=false,holdPost=false,dropPost=false,failGet=false,releaseGet,releasePost;
 const timers=new Map();
 class Node{constructor(tag,text){this.tag=tag;this.textContent=text||'';this.children=[];this.disabled=false;}append(...children){this.children.push(...children);}}
 const context={page:'discovery',lanMode:'ap',AbortController,m:x=>x,box:text=>new Node('box',text),el:(tag,text)=>new Node(tag,text),setText:(n,text)=>n.textContent=text,render:()=>{},
  button:(text,fn)=>Object.assign(new Node('button',text),{click:fn}),
  setTimeout:(fn,delay)=>{const id=++next;timers.set(id,{fn,at:clock+delay});return id;},clearTimeout:id=>timers.delete(id),
  request:async(route,options)=>{
   if(route==='ap/open'){
    eq(options.method,'POST');eq(options.body,undefined);posts++;
    if(holdPost)await new Promise(resolve=>releasePost=resolve);
    if(dropPost)throw Error('accepted but response lost');return{json:async()=>({queued:true})};
   }
   eq(route,'ap');gets++;const captured=state;
   if(holdGet)await new Promise(resolve=>releaseGet=resolve);
   if(failGet)throw Error('offline');return{json:async()=>({handoff:captured})};
  }};
 vm.runInNewContext(source.slice(source.indexOf('function onboardingSteps('),source.indexOf('function updateOnboardingChoice('))+source.slice(begin,end)+';globalThis.makeApView=apView;',context);
 const view=context.makeApView(),open=view.children.find(n=>n.id==='ap-open'),status=view.children.find(n=>n.id==='ap-status');
 async function advance(ms){clock+=ms;const due=[...timers].filter(([,t])=>t.at<=clock);for(const [id,t]of due){timers.delete(id);t.fn();}await flush();}
 eq(posts,0);ok(open.disabled);await advance(0);ok(!open.disabled);eq(gets,1);
 holdGet=true;await advance(800);holdPost=true;const pending=open.click();await flush();eq(posts,1);ok(open.disabled);await open.click();eq(posts,1);
 holdGet=false;releaseGet();await flush();ok(open.disabled);holdPost=false;state=2;releasePost();await pending;await advance(800);ok(open.disabled);ok(status.textContent.startsWith('Żądanie oczekuje'));
 state=3;await advance(800);ok(open.disabled);eq(posts,1);ok(status.textContent.startsWith('Kreator działa'));
 state=4;await advance(800);ok(!open.disabled);eq(posts,1);
 // A GET begun inside POST could read READY before admission and arrive after
 // 202. Do not launch that GET; completion also fences older fetch epochs.
 holdPost=true;const during=open.click();await flush();const beforePostGet=gets;await advance(800);eq(gets,beforePostGet);ok(open.disabled);eq(posts,2);
 holdPost=false;state=2;releasePost();await during;await advance(800);ok(open.disabled);ok(status.textContent.startsWith('Żądanie oczekuje'));
 state=4;await advance(800);ok(!open.disabled);
 dropPost=true;await open.click();eq(posts,3);ok(open.disabled);ok(status.textContent.startsWith('Nie potwierdzono otwarcia'));
 state=3;await advance(800);ok(open.disabled);eq(posts,3);failGet=true;await advance(800);ok(open.disabled);ok(status.textContent.startsWith('Brak połączenia'));
 const before=gets;view.children[0].click();await advance(800);eq(gets,before);eq(posts,3);eq(context.lanMode,'start');
 console.log(`PASS: ${checks} actual AP Web handoff checks; single explicit POST, late GET fence, lost response, offline, active/expiry, navigation; DOM/HTTP MODEL`);
})().catch(error=>{console.error(error);process.exitCode=1;});
