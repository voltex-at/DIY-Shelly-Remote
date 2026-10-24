/* Shipped assets in Chromium. Auth, config persistence and LCD source are
 * explicit HTTP MODELS; this never contacts or controls a physical remote. */
'use strict';
const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'..'),out=path.join(root,'build-ui/panel-settings');fs.mkdirSync(out,{recursive:true});
let browser,page,checks=0;const ok=(v,message)=>{assert(v,message);checks++;},eq=(a,b)=>{assert.deepEqual(a,b);checks++;};
const initial=()=>({schema:3,revision:1,language:'pl',theme:'graphite',accent:3978995,active_dashboard:1,dashboards:[{id:1,name:'Parter',tiles:[]}],rooms:[{id:1,name:'Salon i jadalnia',icon:0,photo:''}],devices:[],groups:[],scenes:[],automations:[]});
const image=process.env.DSR_PANEL_PREVIEW_RGB565?fs.readFileSync(process.env.DSR_PANEL_PREVIEW_RGB565):Buffer.alloc(320*480*2,0x42);assert.equal(image.length,320*480*2);
async function shot(name){await page.evaluate(genuine=>{if(document.querySelector('#test-model-badge'))return;const n=document.createElement('div');n.id='test-model-badge';n.textContent=genuine?'HOST / HTTP MODEL · LCD: LVGL fixture':'HOST / HTTP + LCD MODEL';n.style.cssText='position:fixed;top:0;right:0;z-index:99;background:#ffd166;color:#111;font:11px sans-serif;padding:2px 5px;pointer-events:none';document.body.append(n);},!!process.env.DSR_PANEL_PREVIEW_RGB565);await page.screenshot({path:path.join(out,name+'.png'),fullPage:true});}
async function gold(selector){const colors=await page.locator(selector).evaluate(n=>{const s=getComputedStyle(n);return[s.color,s.borderTopColor,parseFloat(s.borderTopWidth)];});eq(colors,['rgb(255, 193, 7)','rgb(255, 193, 7)',1]);}
const waiting={pl:'Pilot jeszcze nie udostępnił nowej klatki. Odśwież podgląd ponownie.',en:'The remote has not provided a new frame yet. Refresh the preview again.',de:'Die Fernbedienung hat noch kein neues Bild bereitgestellt. Vorschau erneut aktualisieren.'};
(async()=>{
 browser=await chromium.launch({headless:true,executablePath:process.env.DSR_TEST_CHROME||undefined});
 for(const lang of ['pl','en','de']){
  const context=await browser.newContext({viewport:{width:1280,height:1000}});
  page=await context.newPage();const state={config:initial(),paired:false,pairFail:true,pairs:0,puts:0,previews:0,preview:'ready',gate:null};const errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.addInitScript(lang=>{localStorage.setItem('dsr.web.locale',lang);const timeout=window.setTimeout.bind(window);window.setTimeout=(fn,ms,...args)=>ms===1500?0:timeout(fn,ms,...args);},lang);
  await page.route('**/*',async route=>{
   const r=route.request(),u=new URL(r.url());eq(u.origin,'http://127.0.0.1:8765');
   if(!u.pathname.startsWith('/api/')){const file={'/':'panel.html','/panel.js':'panel.js','/runtime.js':'runtime.js','/actions.js':'actions.js','/backup.js':'backup.js','/panel.css':'panel.css'}[u.pathname];return file?route.fulfill({body:fs.readFileSync(path.join(root,'components/dsr_platform/web',file)),contentType:file.endsWith('.js')?'text/javascript':file.endsWith('.css')?'text/css':'text/html'}):route.fulfill({status:404,body:''});}
   const send=(status,json)=>route.fulfill({status,json});
   if(u.pathname.endsWith('/pair')){state.pairs++;if(state.gate)await state.gate;if(state.pairFail)return send(401,{error:'Pairing expired, locked or invalid; reopen pairing on the remote'});state.paired=true;return send(200,{paired:true});}
   if(!state.paired)return send(401,{error:'Pair this browser on the remote'});
   if(u.pathname.endsWith('/config')){if(r.method()==='PUT'){state.puts++;const next=r.postDataJSON();eq(next.revision,state.config.revision);state.config={...next,revision:next.revision+1};return send(202,{saving:true});}return send(200,state.config);}
   if(u.pathname.endsWith('/actions/status'))return send(200,{boot_id:1,revision:state.config.revision,available:true,active:false,steps:[]});
   if(u.pathname.endsWith('/status'))return send(200,{revision:state.config.revision,busy:false,firmware:'HOST TEST / DEMO'});
   if(u.pathname.endsWith('/preview.raw')){state.previews++;if(state.preview==='waiting')return send(202,{waiting:true});if(state.preview==='short')return route.fulfill({body:Buffer.alloc(2),contentType:'application/octet-stream'});if(state.preview==='offline')return route.abort('failed');return route.fulfill({body:image,contentType:'application/octet-stream'});}
   throw Error('Unexpected endpoint '+u.pathname);
  });
  await page.goto('http://127.0.0.1:8765/');await page.locator('#pairing').waitFor({state:'visible'});await page.locator('#pair-code').fill('0123456789abcdef');
  await Promise.all([page.waitForResponse(r=>r.url().endsWith('/pair')&&r.status()===401),page.locator('#pair-form button').click()]);
  await shot(lang+'-pair-error');eq(await page.locator('#pair-code').inputValue(),'0123456789abcdef');
  state.pairFail=false;await page.locator('#pair-form button').click();await page.locator('#workspace').waitFor({state:'visible'});await page.evaluate(()=>poll());eq(state.previews,0);
  await page.locator('#preview-refresh').click();await page.waitForFunction(()=>document.querySelector('#preview-status').textContent.includes('LVGL'));
  state.preview='waiting';await page.locator('#preview-refresh').click();await page.waitForFunction(()=>!document.querySelector('#preview-refresh').disabled);
  eq(await page.locator('#preview-status').textContent(),waiting[lang]);
  ok(await page.locator('#preview').evaluate(n=>Array.from(n.getContext('2d').getImageData(0,0,1,1).data).every(v=>v===0)),'old LCD pixels survive an unavailable frame');await gold('#preview-status');await shot(lang+'-preview-waiting');
  state.preview='short';await page.locator('#preview-refresh').click();await page.waitForFunction(()=>!document.querySelector('#preview-refresh').disabled);ok(!(await page.locator('#preview-status').textContent()).includes('LVGL'));await gold('#preview-status');
  state.preview='offline';await page.locator('#preview-refresh').click();await page.waitForFunction(()=>!document.querySelector('#preview-refresh').disabled);await gold('#preview-status');await shot(lang+'-preview-offline');
  state.preview='ready';await page.locator('#preview-refresh').click();await page.waitForFunction(()=>document.querySelector('#preview-status').textContent.includes('LVGL'));
  await page.locator('nav [data-page="appearance"]').click();const choices=page.locator('#editor select').nth(1);eq(await choices.locator('option').evaluateAll(o=>o.map(x=>x.value)),['graphite','light','cyber_blue']);await choices.selectOption('cyber_blue');
  eq(state.config.theme,'graphite');eq(state.puts,0);await page.locator('#save').click();await page.waitForFunction(()=>!saving&&!dirty);eq(state.config.theme,'cyber_blue');eq(state.config.language,'pl');eq(state.puts,1);await shot(lang+'-appearance');
  await page.locator('nav [data-page="rooms"]').click();await page.locator('#editor .row button').first().click();await page.locator('#editor input[type=text]').fill('Salon na parterze');state.config.revision++;await page.evaluate(()=>poll());ok(await page.locator('#save').isDisabled());await gold('#notice');eq(await page.locator('#editor input[type=text]').inputValue(),'Salon na parterze');await page.setViewportSize({width:390,height:900});await shot(lang+'-conflict-mobile');
  ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'mobile overflow');await page.locator('#reload').click();await page.waitForFunction(()=>!dirty&&!conflict);eq(await page.evaluate(()=>config.rooms[0].name),'Salon i jadalnia');eq(state.puts,1);
  state.paired=false;await page.evaluate(()=>poll());await page.locator('#pairing').waitFor({state:'visible'});eq(errors,[]);await context.close();
 }
 await browser.close();console.log(`PASS: ${checks} panel settings checks; actual assets, HTTP/config/LCD provider MODEL`);
})().catch(async e=>{console.error(e);if(page)await page.screenshot({path:path.join(out,'failure.png'),fullPage:true}).catch(()=>{});if(browser)await browser.close();process.exitCode=1;});
