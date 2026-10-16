/* Shipped HTML/JS/CSS; transport, authentication and device registry are MODEL.
 * Tests correlation/routing/independent controls, never physical Shelly proof. */
'use strict';
const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'..'),out=path.join(root,'build-ui/panel-runtime');fs.mkdirSync(out,{recursive:true});
let checks=0,page;const eq=(a,b)=>{assert.deepEqual(a,b);checks++;},ok=v=>{assert(v);checks++;};
const config={revision:1,language:'pl',theme:'graphite',accent:3978995,active_dashboard:1,dashboards:[{id:1,name:'Home',tiles:[]}],rooms:[],scenes:[],devices:[]};
const device=(id,ch,name)=>({device_id:id,generation:2,channel:ch,name,host:'192.0.2.'+(ch+8),online:true,output:false,has_switch:true,has_power:true,power_w:3.25,has_voltage:true,voltage_v:230,has_current:true,current_a:.1,pending:false,error:0,sample_age_ms:1234,operation_id:0});
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:process.env.DSR_TEST_CHROME||undefined,args:['--no-sandbox']});
 const context=await browser.newContext({viewport:{width:1440,height:1000},locale:'pl-PL'});
 await context.addInitScript(()=>{window.setInterval=()=>1;localStorage.setItem('dsr.web.locale','pl');});
 const state={boot:17,devices:[device('shelly-test-a',0,'<img src=x onerror=alert(1)>'),device('shelly-test-b',1,'Second fixture')],posts:[],gets:0,fail:false,stale:null,dropPost:false};
 page=await context.newPage();const errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.route('**/*',async route=>{
  const req=route.request(),u=new URL(req.url());assert.equal(u.origin,'http://127.0.0.1:8765');
  const send=(status,body)=>route.fulfill({status,contentType:'application/json',body:JSON.stringify(body)});
  if(!u.pathname.startsWith('/api/')){const asset={'/':'panel.html','/panel.js':'panel.js','/runtime.js':'runtime.js','/panel.css':'panel.css'}[u.pathname];assert(asset,u.pathname);return route.fulfill({body:fs.readFileSync(path.join(root,'components/dsr_platform/web',asset)),contentType:asset.endsWith('js')?'text/javascript':asset.endsWith('css')?'text/css':'text/html'});}
  if(u.pathname.endsWith('/config'))return send(200,config);
  if(u.pathname.endsWith('/devices/runtime')){state.gets++;if(state.fail)return route.abort('failed');return send(200,{boot_id:state.boot,devices:state.stale||state.devices});}
  if(u.pathname.endsWith('/devices/output')){
   const body=req.postDataJSON();state.posts.push(body);const d=state.devices.find(d=>d.device_id===body.device_id&&d.generation===body.generation&&d.channel===body.channel);assert(d);assert.deepEqual(Object.keys(body).sort(),['channel','device_id','generation','on']);assert.equal(typeof body.on,'boolean');d.operation_id++;d.pending=true;if(state.dropPost)return route.abort('failed');return send(202,{accepted:true,operation_id:d.operation_id,boot_id:state.boot});
  }
  if(u.pathname.endsWith('/status'))return send(200,{revision:1,firmware:'HOST TEST / DEMO',busy:false});
  if(u.pathname.endsWith('/preview.raw'))return route.fulfill({body:Buffer.alloc(320*480*2),contentType:'application/octet-stream'});
  throw Error('Unexpected path '+u.pathname);
 });
 await page.goto('http://127.0.0.1:8765/');await page.locator('#workspace').waitFor({state:'visible'});await page.locator('nav [data-page=devices]').click();
 const cards=page.locator('.runtime-card');await cards.nth(1).getByRole('button',{name:'Włącz: Second fixture',exact:true}).waitFor();await page.waitForFunction(()=>!document.querySelectorAll('.runtime-card button')[0].disabled);
 eq(await cards.count(),2);eq(await cards.nth(0).locator('h3').innerText(),'<img src=x onerror=alert(1)>');eq(await cards.locator('img').count(),0);ok((await cards.nth(0).innerText()).includes('3,25 W'));ok((await cards.nth(1).innerText()).includes('WYŁ. — potwierdzone'));
 state.stale=structuredClone(state.devices);await cards.nth(0).locator('button').first().click();await page.waitForFunction(()=>document.querySelectorAll('.runtime-card button')[0].disabled);await page.waitForTimeout(80);
 eq(state.posts.length,1);eq(state.posts[0],{device_id:'shelly-test-a',generation:2,channel:0,on:true});ok(await cards.nth(0).locator('button').first().isDisabled());ok(await cards.nth(1).locator('button').first().isEnabled());ok(!(await cards.nth(0).innerText()).includes('WŁ. — potwierdzone'));
 await cards.nth(1).locator('button').first().click();await page.waitForTimeout(80);eq(state.posts.length,2);eq(state.posts[1].device_id,'shelly-test-b');
 state.stale=null;state.devices[1].pending=false;state.devices[1].output=true;await page.evaluate(()=>dsrRuntimeRefresh());ok(await cards.nth(1).locator('button').first().isEnabled());ok(await cards.nth(0).locator('button').first().isDisabled());ok((await cards.nth(1).innerText()).includes('WŁ. — potwierdzone'));
 state.devices[0].pending=false;state.devices[0].error=5;state.devices[0].online=false;await page.evaluate(()=>dsrRuntimeRefresh());ok(await cards.nth(0).locator('button').first().isDisabled());ok(await cards.nth(1).locator('button').first().isEnabled());ok((await cards.nth(0).innerText()).includes('Błąd urządzenia: 5'));eq(await cards.nth(0).locator('p').allTextContents().then(x=>x.some(s=>s.includes('3,25 W'))),false);
 // External wall-switch state changes arrive by readback, with no POST.
 state.devices[1].output=false;state.devices[1].power_w=0;await page.evaluate(()=>dsrRuntimeRefresh());ok((await cards.nth(1).innerText()).includes('WYŁ. — potwierdzone'));eq(state.posts.length,2);
 // A dropped POST response may still mean acceptance: never auto replay.
 state.stale=structuredClone(state.devices);state.dropPost=true;await cards.nth(1).locator('button').first().click();await page.waitForTimeout(80);eq(state.posts.length,3);ok(await cards.nth(1).locator('button').first().isDisabled());ok((await cards.nth(1).innerText()).includes('Nie potwierdzono przyjęcia'));
 await cards.nth(1).locator('button').first().dispatchEvent('click');eq(state.posts.length,3);state.stale=null;
 state.dropPost=false;state.devices[1].pending=false;state.devices[1].output=true;await page.evaluate(()=>dsrRuntimeRefresh());eq(state.posts.length,3);ok(await cards.nth(1).locator('button').first().isEnabled());
 // Locale changes update text in place, preserve focus and generate no I/O.
 await cards.nth(1).locator('button').first().focus();const before=state.gets;await page.evaluate(()=>{window.kept=document.activeElement;const picker=document.querySelector('#web-locale');picker.value='de';picker.dispatchEvent(new Event('change',{bubbles:true}));});await page.waitForTimeout(30);eq(state.gets,before);eq(await page.evaluate(()=>document.activeElement===window.kept),true);ok((await cards.nth(1).innerText()).includes('EIN — durch Abfrage bestätigt'));await page.locator('#web-locale').selectOption('en');await page.waitForTimeout(30);ok((await cards.nth(1).innerText()).includes('ON — confirmed by readback'));
 for(const locale of ['pl','en','de']){await page.locator('#web-locale').selectOption(locale);for(const width of [1440,390]){await page.setViewportSize({width,height:900});ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));await page.screenshot({path:path.join(out,`runtime-${locale}-${width}.png`),fullPage:true});}}await page.locator('#web-locale').selectOption('en');
 state.fail=true;await page.evaluate(()=>dsrRuntimeRefresh());for(const b of await cards.locator('button').all())ok(await b.isDisabled());state.fail=false;await page.evaluate(()=>dsrRuntimeRefresh());ok(await cards.nth(1).locator('button').first().isEnabled());
 state.devices[1].sample_age_ms=14950;await page.evaluate(()=>dsrRuntimeRefresh());await page.waitForTimeout(120);ok(await cards.nth(1).locator('button').first().isDisabled());const posts=state.posts.length;await cards.nth(1).locator('button').first().dispatchEvent('click');eq(state.posts.length,posts);state.devices[1].sample_age_ms=1;await page.evaluate(()=>dsrRuntimeRefresh());
 // Restart does not turn an old accepted command into a success; its epoch ends.
 await cards.nth(1).locator('button').first().click();await page.waitForTimeout(80);ok(await cards.nth(1).locator('button').first().isDisabled());state.boot++;state.devices[1].pending=false;state.devices[1].operation_id=0;state.devices[1].output=false;await page.evaluate(()=>dsrRuntimeRefresh());ok(await cards.nth(1).locator('button').first().isEnabled());ok((await cards.nth(1).innerText()).includes('OFF — confirmed by readback'));
 // Duplicate identity or malformed state fails the entire snapshot closed.
 const good=structuredClone(state.devices);state.devices=[good[1],good[1]];await page.evaluate(()=>dsrRuntimeRefresh());for(const b of await cards.locator('button').all())ok(await b.isDisabled());state.devices=good;await page.evaluate(()=>dsrRuntimeRefresh());
 state.devices=[];await page.evaluate(()=>dsrRuntimeRefresh());eq(await cards.count(),0);ok((await page.locator('.runtime-section').innerText()).includes('No saved connections'));
 eq(errors,[]);await browser.close();console.log(`PASS: ${checks} actual runtime UI checks; independent identity/commands/readback; backend/auth/HTTP MODEL`);
})().catch(async e=>{console.error(e);if(page)await page.screenshot({path:path.join(out,'failure.png'),fullPage:true}).catch(()=>{});process.exit(1);});
