/* Actual shipped DOM/JS/CSS; HTTP/auth/storage/reboot/radio below are MODEL.
 * No ESP32, TLS, private session, flash or device restart is involved. Periodic
 * polling is controlled, so a locale switch can prove exactly zero requests. */
'use strict';
const {chromium}=require('playwright');
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'..'),out=path.join(root,'build-ui/panel-locale');
fs.mkdirSync(out,{recursive:true});
// Chrome creates a UNIX SingletonSocket under TMPDIR; the artifact directory
// is too long on Actions. Keep its own temp directory short and separate.
const browserTemp=fs.mkdtempSync(path.join(process.env.RUNNER_TEMP||path.dirname(root),'dsr-web-'));
process.env.TMPDIR=browserTemp;
let checks=0,active;
const ok=(v,message)=>{assert(v,message);checks++;},eq=(a,b)=>{assert.deepEqual(a,b);checks++;};
async function shot(page,file){await page.evaluate(()=>{if(document.querySelector('#test-model-badge'))return;const n=document.createElement('div');n.id='test-model-badge';n.textContent='HOST TEST / HTTP MODEL';n.style.cssText='position:fixed;top:0;left:50%;transform:translateX(-50%);z-index:1000;font:10px sans-serif;background:#f6c45b;color:#111;padding:2px 5px;pointer-events:none';document.body.append(n);});await page.screenshot({path:file,fullPage:true});}
const initial=()=>({schema:2,groups:[{id:1,name:'User group',members:[1]}],revision:1,language:'pl',theme:'graphite',accent:3978995,active_dashboard:1,dashboards:[{id:1,name:'Pulpity',tiles:[]}],rooms:[{id:1,name:'User room',icon:0,photo:''}],devices:[{id:1,name:'User device',kind:'light',host:'192.0.2.8',channel:0,room:1}],scenes:[{id:1,name:'User scene',actions:[{device:1,command:'on'}]}]});
async function fixture(browser,options={}){
 const context=await browser.newContext({locale:options.navigator||'pl-PL',viewport:{width:1440,height:1050}});
 await context.addInitScript(({stored,storageFault,setFault,languages})=>{
  // MODEL clock: periodic status and 800ms autonomous radio callbacks are
  // controlled. Explicit poll()/pollBle() still execute genuine application
  // callbacks. Save's 250ms acknowledgement waits and initial 0ms run normally.
  window.setInterval=()=>1;
  const timeout=window.setTimeout.bind(window);window.setTimeout=(fn,ms,...args)=>ms===800?1:timeout(fn,ms,...args);
  if(languages)Object.defineProperty(navigator,'languages',{get:()=>languages});
  if(stored!==undefined&&!sessionStorage.getItem('__test_seeded'))localStorage.setItem('dsr.web.locale',stored);
  sessionStorage.setItem('__test_seeded','1');
  if(storageFault)Object.defineProperty(window,'localStorage',{get(){throw new DOMException('MODEL denied','SecurityError');}});
  if(setFault)Storage.prototype.setItem=function(){throw new DOMException('MODEL full','QuotaExceededError');};
 },options);
 const state={config:initial(),paired:!!options.paired,requests:[],fail:false,pairFail:false,putFail:false,saveGate:null,bleBusy:false};
 const page=await context.newPage();active=page;state.errors=[];state.pendingReads=new Set();page.on('pageerror',e=>state.errors.push(e.message));
 // Model writes can deliberately stay pending (saveGate). Track only view
 // reads, including activity's read-only POST query, without blocking a save.
 page.on('request',r=>{const u=new URL(r.url());if(u.pathname.startsWith('/api/')&&(r.method()==='GET'||u.pathname==='/api/v1/actions/log/query'))state.pendingReads.add(r);});
 for(const event of ['requestfinished','requestfailed'])page.on(event,r=>state.pendingReads.delete(r));
 await page.route('**/*',async route=>{
  const req=route.request(),u=new URL(req.url());assert.equal(u.origin,'http://127.0.0.1:8765');
  if(!u.pathname.startsWith('/api/')){
   if(u.pathname==='/favicon.ico')return route.fulfill({status:404,body:''});
   const asset={'/':'panel.html','/panel.js':'panel.js','/runtime.js':'runtime.js','/history.js':'history.js','/actions.js':'actions.js','/panel.css':'panel.css'}[u.pathname];assert(asset,'unexpected asset '+u.pathname);
   return route.fulfill({body:fs.readFileSync(path.join(root,'components/dsr_platform/web',asset)),contentType:asset.endsWith('js')?'text/javascript':asset.endsWith('css')?'text/css':'text/html'});
  }
  state.requests.push({path:u.pathname,method:req.method()});
  const send=(status,obj)=>route.fulfill({status,contentType:'application/json',body:JSON.stringify(obj)});
  if(state.fail)return route.abort('failed');
  if(u.pathname.endsWith('/pair')){if(state.pairFail)return send(403,{error:'Pairing fixture refused'});state.paired=true;return send(200,{paired:true});}
  if(!state.paired)return send(401,{});
  if(u.pathname.endsWith('/config')){if(req.method()==='PUT'){if(state.putFail)return send(409,{error:'MODEL conflict'});const next=req.postDataJSON();if(state.onPut){state.onPut();state.onPut=null;}if(state.saveGate)await state.saveGate;if(next.revision!==state.config.revision)return send(409,{error:'MODEL conflict'});state.config={...next,revision:next.revision+1};return send(202,{saving:true});}return send(200,state.config);}
  if(u.pathname.endsWith('/devices/runtime'))return send(200,{boot_id:1,devices:[]});
  if(u.pathname.endsWith('/actions/log/query')){assert.deepEqual(req.postDataJSON(),{epoch:0,offset:0,limit:16});return send(200,{epoch:1,newest:0,total:0,timezone:'UTC',entries:[]});}
  if(u.pathname.endsWith('/actions/status'))return send(200,{boot_id:1,admission_generation:0,revision:state.config.revision,run_id:0,object_id:0,nonce:'',kind:0,available:true,active:false,cancelled:false,current:0,steps:[]});
  if(u.pathname.endsWith('/status'))return send(200,{revision:state.config.revision,busy:false,firmware:'HOST TEST / DEMO'});
  if(u.pathname.endsWith('/preview.raw'))return route.fulfill({body:Buffer.alloc(320*480*2),contentType:'application/octet-stream'});
  if(u.pathname.endsWith('/discovery/scan'))return send(202,{});
  if(u.pathname.endsWith('/ble/scan')){state.bleBusy=true;return send(202,{});}
  if(u.pathname.endsWith('/ble/cancel')){state.bleBusy=false;return send(202,{});}
  if(u.pathname.endsWith('/ble'))return send(200,{busy:state.bleBusy,phase:state.bleBusy?2:4,elapsed_ms:3000,devices:state.bleBusy?[]:[{kind:1,model:'TEST / DEMO BLU',rssi:-48}]});
  if(u.pathname.endsWith('/discovery'))return send(200,{phase:2,devices:[{host:'192.0.2.9',id:'TEST-ONLY',model:'TEST / DEMO',generation:2,auth_required:true}],selected:{},capabilities:{}});
  throw Error('Unexpected endpoint '+u.pathname);
 });
 await page.goto('http://127.0.0.1:8765/');await page.locator(options.paired?'#workspace':'#pairing').waitFor({state:'visible'});
 return {context,page,state,options};
}
async function settleViewReads(f){
 // A just-mounted actor performs status -> persisted config (-> activity
 // query) asynchronously. Drain that preceding view work before measuring
 // the locale event. Two animation frames also drain queued JSON callbacks.
 // This is bounded and does not ignore any request caused by locale changes.
 for(let attempt=0;attempt<60;attempt++){
  const n=f.state.requests.length;
  await f.page.evaluate(()=>new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve))));
  if(!f.state.pendingReads.size&&f.state.requests.length===n)return;
 }
 throw Error('Previous view reads did not settle');
}
async function switchTo(f,locale){
 await settleViewReads(f);
 const before=await f.page.evaluate(()=>({config:JSON.stringify(config),dirty,conflict,saving,editSerial,page,lanMode,lanGeneration,bleGeneration,lanSelected:JSON.stringify(lanSelected),lanCredentials:JSON.stringify(lanCredentials)}));
 const n=f.state.requests.length;
 await f.page.locator('#web-locale').selectOption(locale);
 eq(await f.page.locator('html').getAttribute('lang'),locale);
 await f.page.evaluate(()=>new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve))));
 eq(await f.page.evaluate(()=>({config:JSON.stringify(config),dirty,conflict,saving,editSerial,page,lanMode,lanGeneration,bleGeneration,lanSelected:JSON.stringify(lanSelected),lanCredentials:JSON.stringify(lanCredentials)})),before);
 eq(f.state.requests.length,n);
 if(!f.options.storageFault&&!f.options.setFault)eq(await f.page.evaluate(()=>localStorage.getItem('dsr.web.locale')),locale);
}
async function geometry(page){ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'horizontal overflow');const a=await page.locator('#web-locale').boundingBox();ok(a&&a.x>=0&&a.y>=0&&a.x+a.width<=await page.evaluate(()=>innerWidth),'locale clipped');for(const selector of ['#title','#connection']){const b=await page.locator(selector).boundingBox();ok(!b||a.x+a.width<=b.x||b.x+b.width<=a.x||a.y+a.height<=b.y||b.y+b.height<=a.y,'locale overlaps '+selector);}await page.locator('#web-locale').click();await page.keyboard.press('Escape');}
async function preserve(f,selector){
 await settleViewReads(f);
 await f.page.locator(selector).first().evaluate(n=>{window.__kept=n;n.focus();if(n.type==='text'||n.type==='password'){n.setSelectionRange(1,3);}});
 const before=await f.page.evaluate(()=>({value:__kept.value,start:__kept.selectionStart,end:__kept.selectionEnd,files:__kept.files&&Array.from(__kept.files,x=>[x.name,x.size]),active:document.activeElement===__kept}));
 // Dispatch the actual select's event without deliberately moving focus to it.
 const n=f.state.requests.length;await f.page.evaluate(()=>{const s=document.querySelector('#web-locale');s.value=s.value==='en'?'de':'en';s.dispatchEvent(new Event('change',{bubbles:true}));});
 eq(await f.page.locator(selector).first().evaluate(n=>n===window.__kept),true);
 eq(await f.page.evaluate(()=>({value:__kept.value,start:__kept.selectionStart,end:__kept.selectionEnd,files:__kept.files&&Array.from(__kept.files,x=>[x.name,x.size]),active:document.activeElement===__kept})),before);eq(f.state.requests.length,n);
}
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:process.env.DSR_TEST_CHROME||undefined,args:['--no-sandbox']});
 console.log('Browser:',await browser.version(),'Playwright:',require('playwright/package.json').version);
 for(const [options,expected]of [[{navigator:'en-US'},'en'],[{navigator:'de-AT'},'de'],[{navigator:'fr-FR'},'pl'],[{navigator:'en-US',stored:'de'},'de'],[{navigator:'de-DE',stored:'xx'},'de'],[{navigator:'de-DE',stored:'__proto__'},'de'],[{navigator:'en-US',stored:'constructor'},'en'],[{navigator:'fr-FR',languages:['fr','DE-at','en']},'de'],[{navigator:'en-US',storageFault:true},'en'],[{navigator:'de-DE',setFault:true},'de']]){
  const f=await fixture(browser,options);eq(await f.page.locator('html').getAttribute('lang'),expected);await switchTo(f,'pl');eq(f.state.errors,[]);await f.context.close();
 }
 for(const locale of ['pl','en','de']){
  const f=await fixture(browser,{navigator:'pl-PL',stored:locale});const {page,state}=f;
  eq(await page.locator('html').getAttribute('lang'),locale);await page.locator('#pair-code').fill('0123456789abcdef');await preserve(f,'#pair-code');eq(await page.locator('#pair-code').inputValue(),'0123456789abcdef');await switchTo(f,locale);
  eq(await page.locator('#pairing h2').textContent(),{pl:'Połącz tę przeglądarkę',en:'Pair this browser',de:'Diesen Browser koppeln'}[locale]);
  for(const width of [1440,390]){await page.setViewportSize({width,height:900});await geometry(page);await shot(page,path.join(out,`${locale}-unpaired-${width}.png`));}
  state.pairFail=true;await Promise.all([page.waitForResponse(r=>new URL(r.url()).pathname==='/api/v1/pair'&&r.status()===403),page.locator('#pair-form button').click()]);await page.waitForFunction(()=>document.querySelector('#notice').textContent==='Pairing fixture refused');await switchTo(f,'en');eq(await page.locator('#notice').textContent(),'Pairing fixture refused');eq(await page.locator('#pair-code').inputValue(),'0123456789abcdef');state.pairFail=false;
  await page.locator('#pair-form button').click();await page.locator('#workspace').waitFor({state:'visible'});await switchTo(f,'pl');
  for(const l of ['pl','en','de']){await switchTo(f,l);eq(await page.locator('#editor h3').first().textContent(),'Pulpity');}
  await switchTo(f,'pl');
  await page.locator('nav [data-page="devices"]').click();await page.getByRole('button',{name:'Edytuj',exact:true}).click();await page.getByLabel('Nazwa',{exact:true}).fill('Unsaved user text Pomieszczenia');await page.locator('input[type=password]').fill('TEST-ONLY-password');await preserve(f,'input[type=password]');await preserve(f,'#editor input[type=text]');
  eq(await page.evaluate(()=>config.devices[0].name),'Unsaved user text Pomieszczenia');eq(await page.evaluate(()=>config.language),'pl');ok(await page.evaluate(()=>dirty&&!conflict));
  let release;const putStarted=new Promise(resolve=>state.onPut=resolve);state.saveGate=new Promise(resolve=>release=resolve);await page.locator('#save').click();await putStarted;await page.waitForFunction(()=>saving);await switchTo(f,locale);await page.locator('#editor input[type=text]').first().fill('Later draft B');await switchTo(f,'de');release();await page.waitForFunction(()=>!saving);state.saveGate=null;
  eq(state.config.devices[0].name,'Unsaved user text Pomieszczenia');eq(await page.evaluate(()=>config.devices[0].name),'Later draft B');ok(await page.evaluate(()=>dirty&&!conflict));await page.locator('#save').click();await page.waitForFunction(()=>!saving&&!dirty);eq(state.config.devices[0].name,'Later draft B');await switchTo(f,locale);
  // All eight current navigation entries and the new actor views translate
  // without configuration writes or commands. Opening a view may read first.
  const navigation=['dashboards','rooms','devices','groups','activity','scenes','appearance','system'];
  const navLabels={pl:['Pulpity','Pomieszczenia','Urządzenia','Grupy','Aktywność','Sceny','Wygląd i język','System i OTA'],en:['Dashboards','Rooms','Devices','Groups','Activity','Scenes','Appearance and language','System and OTA'],de:['Dashboards','Räume','Geräte','Gruppen','Aktivität','Szenen','Darstellung und Sprache','System und OTA']};
  const writesBefore=state.requests.filter(r=>r.method!=='GET').length;
  for(const view of ['groups','scenes','activity']){
   await page.locator(`nav [data-page="${view}"]`).click();await settleViewReads(f);
   const viewRequests=state.requests.length;
   for(const l of ['pl','en','de']){
    await switchTo(f,l);eq(await page.locator('nav button').evaluateAll(buttons=>buttons.map(b=>b.dataset.page)),navigation);eq(await page.locator('nav button').allTextContents(),navLabels[l]);
    const heading={groups:{pl:'Grupy · cały dom',en:'Groups · whole home',de:'Gruppen · ganzes Zuhause'},scenes:{pl:'Sceny · cały dom',en:'Scenes · whole home',de:'Szenen · ganzes Zuhause'},activity:{pl:'Aktywność · bieżące uruchomienie',en:'Activity · current boot',de:'Aktivität · aktueller Start'}};
    eq(await page.locator('.actions-section h2').textContent(),heading[view][l]);eq(state.requests.length,viewRequests);
   }
  }
  eq(state.requests.filter(r=>r.method!=='GET'&&r.path!=='/api/v1/actions/log/query').length,writesBefore);await switchTo(f,'pl');
  await page.locator('nav [data-page="rooms"]').click();await page.locator('#editor .row button').first().click();await page.locator('input[type=file]').setInputFiles({name:'fixture.bin',mimeType:'application/octet-stream',buffer:Buffer.from('HOST ONLY')});await preserve(f,'input[type=file]');
  for(const l of ['pl','en','de']){await switchTo(f,l);eq(await page.locator('#notice').textContent(),{pl:'Wybierz JPG, PNG lub WebP do 15 MB.',en:'Choose JPG, PNG or WebP up to 15 MB.',de:'JPG, PNG oder WebP bis 15 MB wählen.'}[l]);}
  await switchTo(f,'pl');await page.getByLabel('Nazwa',{exact:true}).fill('Draft survives conflict');state.config.revision++;await page.evaluate(()=>poll());ok(await page.evaluate(()=>dirty&&conflict));for(const l of ['pl','en','de']){await switchTo(f,l);ok(await page.locator('#save').isDisabled());eq(await page.locator('#notice').textContent(),{pl:'Konfiguracja zmieniła się na pilocie. Wczytaj aktualną wersję, aby uniknąć nadpisania.',en:'Configuration changed on the remote. Reload to avoid overwriting it.',de:'Konfiguration auf der Fernbedienung geändert. Neu laden, um Überschreiben zu vermeiden.'}[l]);if(locale===l)await shot(page,path.join(out,`${locale}-dirty-conflict-mobile.png`));}
  eq(await page.locator('#editor input[type=text]').first().inputValue(),'Draft survives conflict');await page.locator('#reload').click();await page.waitForFunction(()=>!dirty&&!conflict);await switchTo(f,'pl');
  await page.locator('nav [data-page="devices"]').click();await page.getByRole('button',{name:'Wyszukaj Shelly w LAN'}).click();await page.getByRole('button',{name:'Skanuj sieć Wi-Fi',exact:true}).click();await page.getByRole('button',{name:'Sprawdź urządzenie',exact:true}).click();await page.locator('input[type=password]').fill('TEST-LAN-secret');await preserve(f,'input[type=password]');ok(await page.evaluate(()=>page==='discovery'&&lanMode==='credentials'));
  await page.locator('nav [data-page="appearance"]').click();await switchTo(f,'pl');ok((await page.locator('#editor').innerText()).includes('Język pilota'));ok((await page.locator('#editor').innerText()).includes('Język Web Panelu'));await switchTo(f,locale);await shot(page,path.join(out,`${locale}-appearance-mobile.png`));
  await page.locator('nav [data-page="system"]').click();await page.locator('input[type=file]').setInputFiles({name:'TEST-ONLY.bin',mimeType:'application/octet-stream',buffer:Buffer.from('not firmware')});await preserve(f,'input[type=file]');
  await switchTo(f,locale);for(const width of [1440,390]){await page.setViewportSize({width,height:900});await geometry(page);await shot(page,path.join(out,`${locale}-paired-${width}.png`));}
  await page.reload();await page.locator('#workspace').waitFor({state:'visible'});eq(await page.locator('html').getAttribute('lang'),locale);eq(state.config.language,'pl');
  state.fail=true;await page.evaluate(()=>poll());state.fail=false;await page.evaluate(()=>poll());await switchTo(f,locale);eq(state.config.language,'pl');
  // MODEL reboot: backend retains committed config and paired browser, while
  // all browser JS state is recreated by a fresh document.
  state.config=JSON.parse(JSON.stringify(state.config));await page.reload();await page.locator('#workspace').waitFor({state:'visible'});eq(await page.locator('html').getAttribute('lang'),locale);
  await switchTo(f,'pl');await page.locator('nav [data-page="devices"]').click();await page.getByRole('button',{name:'Wyszukaj Shelly w LAN'}).click();await page.getByRole('button',{name:'Bluetooth LE • skan'}).click();await page.waitForFunction(()=>document.querySelector('#ble-status')?.textContent==='Skan przerwany');await Promise.all([page.waitForResponse(r=>r.url().endsWith('/ble/scan')),page.locator('#ble-start').click()]);await page.evaluate(()=>pollBle(bleGeneration));ok(await page.locator('#ble-start').isDisabled());for(const l of ['pl','en','de']){await switchTo(f,l);ok(await page.locator('#ble-start').isDisabled());eq(await page.locator('#ble-status').textContent(),{pl:'Odbieranie reklam: 3 / 20 s',en:'Receiving advertisements: 3 / 20 s',de:'Advertisements empfangen: 3 / 20 s'}[l]);eq(await page.locator('#ble-cancel').textContent(),{pl:'Anuluj skan BLE',en:'Cancel BLE scan',de:'BLE-Scan abbrechen'}[l]);}
  await Promise.all([page.waitForResponse(r=>r.url().endsWith('/ble/cancel')),page.locator('#ble-cancel').click()]);await page.evaluate(()=>pollBle(bleGeneration));ok(await page.locator('#ble-cancel').isDisabled());for(const l of ['pl','en','de']){await switchTo(f,l);eq(await page.locator('#ble-status').textContent(),{pl:'Skan przerwany',en:'Scan cancelled',de:'Scan abgebrochen'}[l]);}
  state.paired=false;await page.evaluate(()=>poll());await page.locator('#pairing').waitFor({state:'visible'});for(const l of ['pl','en','de'])await switchTo(f,l);eq(state.errors,[]);await f.context.close();
 }
 await browser.close();fs.rmSync(browserTemp,{recursive:true,force:true});console.log(`PASS: ${checks} locale checks; actual shipped assets; HTTP/session/radio/reboot MODEL, no device/TLS proof`);
})().catch(async e=>{console.error(e);if(active)await active.screenshot({path:path.join(out,'failure.png'),fullPage:true}).catch(()=>{});process.exit(1);});
