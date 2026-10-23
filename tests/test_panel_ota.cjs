/* Actual panel assets in Chromium; all HTTP, flash and restart are MODELS. */
'use strict';
const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const root=path.resolve(__dirname,'..'),out=path.join(root,'build-ui/panel-ota');fs.mkdirSync(out,{recursive:true});
let browser,page,checks=0;const ok=(v)=>{assert(v);checks++;},eq=(a,b)=>{assert.deepEqual(a,b);checks++;};
const image=Buffer.alloc(512,0x42);
async function shot(name){await page.evaluate(()=>{if(document.querySelector('#model-badge'))return;const b=document.createElement('div');b.id='model-badge';b.textContent='HOST TEST / HTTP MODEL';b.style.cssText='position:fixed;top:0;right:0;z-index:99;background:#ffd166;color:#111;padding:2px 6px;font:12px sans-serif';document.body.append(b);});await page.screenshot({path:path.join(out,name+'.png'),fullPage:true});}
(async()=>{
 browser=await chromium.launch({headless:true,executablePath:process.env.DSR_TEST_CHROME||undefined});
 for(const [lang,outcome] of [['pl','ok'],['en','ok'],['de','ok'],['de','uncertain'],['pl','lost'],['en','server']]){
  const context=await browser.newContext({viewport:{width:1280,height:1000}});await context.addInitScript(lang=>{localStorage.setItem('dsr.web.locale',lang);window.setInterval=()=>1;},lang);
  page=await context.newPage();const errors=[];page.on('pageerror',e=>errors.push(e.message));let uploads=0,restarts=0,mode='ok',release=null,hold=false;
  const config={schema:3,revision:1,language:'pl',theme:'cyber_blue',accent:3978995,active_dashboard:1,dashboards:[{id:1,name:'Test',tiles:[]}],rooms:[],devices:[],groups:[],scenes:[],automations:[]};
  await page.route('**/*',async route=>{
   const req=route.request(),u=new URL(req.url());eq(u.origin,'http://127.0.0.1:8765');
   const send=(status,data)=>route.fulfill({status,contentType:'application/json',body:JSON.stringify(data)});
   if(!u.pathname.startsWith('/api/')){const a={'/':'panel.html','/panel.js':'panel.js','/panel.css':'panel.css','/runtime.js':'runtime.js','/actions.js':'actions.js'}[u.pathname];if(!a)return route.fulfill({status:404,body:''});return route.fulfill({body:fs.readFileSync(path.join(root,'components/dsr_platform/web',a)),contentType:a.endsWith('.js')?'text/javascript':a.endsWith('.css')?'text/css':'text/html'});}
   if(u.pathname.endsWith('/config')){if(req.method()==='PUT'){const revision=config.revision;Object.assign(config,req.postDataJSON(),{revision:revision+1});return send(202,{saving:true});}return send(200,config);}
   if(u.pathname.endsWith('/status'))return send(200,{revision:config.revision,busy:false,firmware:'HOST TEST / MODEL'});
   if(u.pathname.endsWith('/preview.raw'))return route.fulfill({body:Buffer.alloc(320*480*2)});
   if(u.pathname.endsWith('/actions/status'))return send(200,{revision:1,available:true,active:false,steps:[]});
   if(u.pathname.endsWith('/actions/log/query'))return send(200,{epoch:1,total:0,newest:0,entries:[]});
   if(u.pathname.endsWith('/restart')){restarts++;return send(200,{restarting:true});}
   if(u.pathname.endsWith('/ota')){
    uploads++;eq(req.method(),'POST');eq(req.postDataBuffer(),image);eq(req.headers()['x-sha256'],crypto.createHash('sha256').update(image).digest('hex'));
    if(hold)await new Promise(r=>release=r);
    if(mode==='lost')return route.abort('failed');
    if(mode==='low')return send(422,{error:'Battery below 20%; recharge the remote and renew OTA consent'});
    if(mode==='busy')return send(409,{error:'Battery check unavailable; renew OTA consent and retry'});
    if(mode==='denied')return send(403,{error:'Allow OTA on the remote first; approval expires after 2 minutes'});
    if(mode==='bad')return send(400,{error:'Image, board marker or checksum rejected; boot slot unchanged'});
    if(mode==='uncertain')return send(500,{error:'Boot selection unconfirmed; OTA blocked until restart',restart_required:true});
    if(mode==='server')return send(500,{});
    return send(200,{installed:true,restart_required:true});
   }
   throw Error('Unexpected endpoint '+u.pathname);
  });
  await page.goto('http://127.0.0.1:8765/');await page.locator('#workspace').waitFor({state:'visible'});await page.locator('nav [data-page=system]').click();
  const install=page.locator('#editor button.primary'),file=page.locator('#editor input[type=file]');
  await shot(lang+'-initial');ok(await install.isDisabled());ok(await page.locator('.savebar').isHidden());
  if(lang==='pl'&&outcome==='ok'){
   for(const [name,size] of [['small.bin',511],['large.bin',0x600001],['wrong.txt',512]]){
    await file.setInputFiles({name,mimeType:'application/octet-stream',buffer:Buffer.alloc(size)});await install.click();
    await page.locator('.ota-message[data-state=error]').waitFor();eq(uploads,0);
   }
  }
  await file.setInputFiles({name:'app.bin',mimeType:'application/octet-stream',buffer:image});ok(await install.isEnabled());
  if(outcome==='ok'){
   mode='low';await install.click();await page.waitForFunction(()=>document.querySelector('.ota-message')?.textContent.includes('20%'));eq(uploads,1);ok(await install.isEnabled());await shot(lang+'-battery-low');
   mode='busy';await install.click();await page.waitForFunction(()=>document.querySelector('.ota-message')?.textContent.match(/nie jest dostępny|unavailable|nicht verfügbar/));eq(uploads,2);ok(await install.isEnabled());await shot(lang+'-battery-busy');
   mode='denied';await install.click();await page.locator('.ota-message[data-state=error]').waitFor();eq(uploads,3);ok(await install.isEnabled());await shot(lang+'-denied');
   mode='bad';await install.click();await page.waitForFunction(()=>document.querySelector('.ota-message')?.textContent.match(/Odrzucono|rejected|abgelehnt/));eq(uploads,4);ok(await install.isEnabled());
  }
  const before=uploads;mode=outcome;hold=true;await install.click();while(!release)await new Promise(r=>setTimeout(r,5));ok(await install.isDisabled());ok(await file.isDisabled());ok(await page.locator('.ota-progress').isVisible());
  await page.evaluate(()=>document.querySelector('.ota-install').dispatchEvent(new Event('click')));eq(uploads,before+1);await shot(lang+'-upload');
  await page.locator('nav [data-page=rooms]').click();release();hold=false;
  // The late response is kept for the OTA view, not written over another page.
  await page.locator('nav [data-page=system]').click();
  await page.locator('.ota-message[data-state='+({ok:'staged',uncertain:'uncertain',lost:'unconfirmed',server:'unconfirmed'}[outcome])+']').waitFor();
  ok((await page.locator('.ota-file-summary').innerText()).includes('app.bin'));ok(await file.isHidden());
  ok(await install.isDisabled());await shot(lang+'-'+outcome);
  if(['ok','uncertain'].includes(outcome)){
   ok(await page.locator('.ota-restart').isEnabled());await page.locator('.ota-restart').click();await page.locator('.ota-restart:disabled').waitFor();eq(restarts,1);
  }else {ok(await page.locator('.ota-restart').isHidden());eq(restarts,0);}
  eq(uploads,before+1);eq(errors,[]);
  await page.setViewportSize({width:320,height:900});ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));await shot(lang+'-'+outcome+'-mobile');
  if(outcome==='lost'){
   await page.locator('nav [data-page=appearance]').click();await page.locator('#editor select').first().selectOption('en');
   await page.locator('nav [data-page=system]').click();ok(await page.locator('.savebar').isVisible());
   eq(uploads,before+1);eq(restarts,0); /* Unsaved config remains explicit; never auto-save it for OTA. */
   await page.locator('#save').click();await page.locator('.savebar').waitFor({state:'hidden'});eq(config.language,'en');eq(uploads,before+1);eq(restarts,0);
  }
  await context.close();
 }
 await browser.close();console.log(`PASS: ${checks} local OTA browser checks; actual assets, HTTP/flash/restart MODEL`);
})().catch(async e=>{console.error(e);if(page)await page.screenshot({path:path.join(out,'failure.png'),fullPage:true}).catch(()=>{});if(browser)await browser.close();process.exit(1);});
