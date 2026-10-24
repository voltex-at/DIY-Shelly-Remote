/* Actual shipped assets and Chromium Web Crypto; HTTP/physical consent are
 * explicit MODELS. Never points at a remote or sends device commands. */
'use strict';
const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),{webcrypto}=require('node:crypto');
const {decryptBackup}=require('../components/dsr_platform/web/backup.js');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.local/ui-review/sys02/backup-web');fs.mkdirSync(out,{recursive:true});
let checks=0,browser,page;const eq=(a,b)=>{assert.deepEqual(a,b);checks++;},ok=x=>{assert(x);checks++;};
(async()=>{
 const meta=fs.readFileSync(path.join(root,'tests/system_data/manifest-v1.json')),raw=Buffer.alloc(24+meta.length);raw.write('DSRSNP1',0);raw.writeUInt16LE(1,8);raw.writeUInt16LE(24,10);raw.writeUInt32LE(meta.length,12);raw.writeUInt32LE(raw.length,20);meta.copy(raw,24);
 browser=await chromium.launch({headless:true,executablePath:process.env.DSR_TEST_CHROME||undefined,args:['--no-sandbox']});
 const context=await browser.newContext({viewport:{width:1200,height:1000},acceptDownloads:true,locale:'pl-PL'});await context.addInitScript(()=>localStorage.setItem('dsr.web.locale','pl'));
 const config={schema:3,revision:7,language:'pl',theme:'cyber_blue',accent:5020661,active_dashboard:1,rooms:[],devices:[],groups:[],scenes:[],automations:[],dashboards:[{id:1,name:'Home',tiles:[]}]};
 let pending=null,seq=0,exported=0,imports=0,cancelled=0,requests=0,downloads=0,hold=false,release,holdExport=false,releaseExport;const password='  Backup Zażółć 🔐  ';const errors=[];
 await context.route('**/*',async route=>{
  const req=route.request(),u=new URL(req.url()),send=(status,value)=>route.fulfill({status,contentType:'application/json',body:JSON.stringify(value)});
  if(!u.pathname.startsWith('/api/')){const file=u.pathname==='/'?'panel.html':path.basename(u.pathname);if(['panel.html','panel.js','panel.css','runtime.js','actions.js','backup.js'].includes(file))return route.fulfill({path:path.join(root,'components/dsr_platform/web',file),contentType:file.endsWith('.js')?'text/javascript':file.endsWith('.css')?'text/css':'text/html'});return route.fulfill({status:404,body:''});}
  if(req.method()!=='GET'){eq(req.headers()['x-dsr-request'],'1');ok(!req.postDataBuffer()?.includes(Buffer.from(password)));ok(!u.pathname.includes('confirm'));}
  if(u.pathname.endsWith('/backup/request')||u.pathname.endsWith('/reset/request')){requests++;if(pending&&pending.phase!==9)return send(409,{error:'Busy'});const op=u.pathname.endsWith('/reset/request')?4:req.postDataJSON().op==='export'?1:2;pending={owned:true,request_id:(++seq).toString(16).padStart(32,'0'),op,phase:5,result:0};return send(202,{request_id:pending.request_id});}
  if(u.pathname.endsWith('/maintenance/status')){const value=pending?{...pending,phase:pending.op<3?10:pending.phase}:{owned:false,phase:0};if(hold)await new Promise(resolve=>release=resolve);return send(200,value).catch(()=>{});}
  if(u.pathname.endsWith('/maintenance/cancel')){cancelled++;if(pending&&req.headers()['x-dsr-maintenance']===pending.request_id)pending.phase=9;return send(200,{cancelled:true});}
  if(u.pathname.endsWith('/backup/export')){eq(req.headers()['x-dsr-maintenance'],pending.request_id);exported++;if(holdExport)await new Promise(resolve=>releaseExport=resolve);if(pending)pending.phase=9;return route.fulfill({status:200,contentType:'application/octet-stream',headers:{'X-DSR-Backup-Bytes':String(raw.length)},body:raw}).catch(()=>{});}
  if(u.pathname.endsWith('/backup/import')){imports++;eq(req.headers()['x-dsr-maintenance'],pending.request_id);eq(req.postDataBuffer(),raw);pending={...pending,op:3,phase:5,request_id:(++seq).toString(16).padStart(32,'0')};return send(202,{candidate:true,request_id:pending.request_id});}
  if(u.pathname.endsWith('/config'))return send(200,config);
  if(u.pathname.endsWith('/status'))return send(200,{revision:7,busy:false,firmware:'HOST / MODEL'});
  if(u.pathname.endsWith('/actions/status'))return send(200,{revision:7,boot_id:1,admission_generation:0,run_id:0,nonce:'',available:true,active:false,steps:[]});
  return send(200,{});
 });
 page=await context.newPage();page.on('pageerror',e=>errors.push(e.message));page.on('download',()=>downloads++);page.setDefaultTimeout(8000);await page.goto('http://127.0.0.1:8765/');await page.locator('#workspace').waitFor({state:'visible'});await page.evaluate(()=>{config.dashboards[0].name='Unsaved draft';dirty=true;});await page.locator('nav [data-page=system]').click();
 const panel=page.locator('.backup-panel'),pass=panel.locator('input[type=password]').nth(0),repeat=panel.locator('input[type=password]').nth(1);await panel.waitFor();await pass.fill(password);await repeat.fill(password);
 const downloadPromise=page.waitForEvent('download');await panel.getByRole('button',{name:'Utwórz szyfrowaną kopię',exact:true}).click();const download=await downloadPromise;const target=path.join(out,'synthetic.dsrbak');await download.saveAs(target);const encrypted=fs.readFileSync(target);eq(encrypted.subarray(0,7).toString(),'DSRBKP1');eq(Buffer.from(await decryptBackup(encrypted,password,{crypto:webcrypto,secure:true})),raw);eq(await pass.inputValue(),'');eq(await repeat.inputValue(),'');eq(exported,1);
 await pass.fill('Wrong backup password');await panel.locator('input[type=file]').setInputFiles(target);const before=requests;await panel.getByRole('button',{name:'Przywróć kopię',exact:true}).click();await panel.getByText('Błędne hasło lub uszkodzony plik. Żadne dane nie zostały wysłane do pilota.',{exact:true}).waitFor();eq(requests,before);eq(imports,0);
 await pass.fill(password);await panel.getByRole('button',{name:'Przywróć kopię',exact:true}).click();await panel.getByText(/Kopia sprawdzona/).waitFor();eq(imports,1);ok(await panel.getByRole('button',{name:'Anuluj',exact:true}).isEnabled());await panel.getByRole('button',{name:'Anuluj',exact:true}).click();await page.waitForTimeout(50);ok(cancelled>=1);
 await page.setViewportSize({width:390,height:900});await page.evaluate(()=>updateWebLocale('de'));await pass.fill(password);await repeat.fill(password);eq(await pass.inputValue(),password);ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));await panel.screenshot({path:path.join(out,'backup-mobile-de.png')});
 hold=true;release=undefined;await panel.getByRole('button',{name:'Verschlüsselte Sicherung erstellen',exact:true}).click();while(!release)await page.waitForTimeout(20);
 const second=await context.newPage();await second.goto('http://127.0.0.1:8765/');await second.locator('#workspace').waitFor({state:'visible'});await second.locator('nav [data-page=system]').click();const panel2=second.locator('.backup-panel');await panel2.locator('input[type=password]').nth(0).fill(password);await panel2.locator('input[type=password]').nth(1).fill(password);await panel2.getByRole('button',{name:'Utwórz szyfrowaną kopię',exact:true}).click();await panel2.getByText(/Nie potwierdzono wyniku/).waitFor();eq(pending.phase,5);await second.close();
 const exportBefore=exported;await page.locator('nav [data-page=rooms]').click();release();hold=false;await page.waitForTimeout(100);eq(exported,exportBefore);eq(await page.locator('.backup-panel').count(),0);
 await page.locator('nav [data-page=system]').click();await page.evaluate(()=>updateWebLocale('en'));let current=page.locator('.backup-panel');await current.locator('input[type=password]').nth(0).fill(password);await current.locator('input[type=password]').nth(1).fill(password);holdExport=true;releaseExport=undefined;await current.getByRole('button',{name:'Create encrypted backup',exact:true}).click();while(!releaseExport)await page.waitForTimeout(20);await page.locator('nav [data-page=rooms]').click();releaseExport();holdExport=false;await page.waitForTimeout(100);eq(downloads,1);
 await page.locator('nav [data-page=system]').click();current=page.locator('.backup-panel');await current.screenshot({path:path.join(out,'backup-mobile-en.png')});eq(await page.evaluate(()=>({name:config.dashboards[0].name,dirty})),{name:'Unsaved draft',dirty:true});
 await page.evaluate(()=>{const original=crypto.subtle.decrypt.bind(crypto.subtle);window.__latePlain=null;window.__originalDecrypt=original;crypto.subtle.decrypt=async(...args)=>{const plain=await original(...args);window.__latePlain=plain;await new Promise(resolve=>window.__releaseDecrypt=resolve);return plain;};});
 await current.locator('input[type=password]').nth(0).fill(password);await current.locator('input[type=file]').setInputFiles(target);const beforeLate=requests;await current.getByRole('button',{name:'Restore backup',exact:true}).click();await page.waitForFunction(()=>!!window.__releaseDecrypt);await page.locator('nav [data-page=rooms]').click();await page.evaluate(()=>window.__releaseDecrypt());await page.waitForTimeout(50);ok(await page.evaluate(()=>new Uint8Array(window.__latePlain).every(x=>x===0)));eq(requests,beforeLate);
 await page.evaluate(async()=>{
  crypto.subtle.decrypt=window.__originalDecrypt;
  const ids=['a'.repeat(32),'b'.repeat(32)],cancelled=[];let releaseUpload,entered;
  const entering=new Promise(r=>entered=r);const password='test backup password';
  const encrypted=await encryptBackup(new Uint8Array(24),password);
  const p=createBackupPanel({request:async(path,options={})=>{
   if(path==='backup/request')return {json:async()=>({request_id:ids[0]})};
   if(path==='maintenance/status')return {json:async()=>({owned:true,request_id:ids[0],phase:10})};
   if(path==='maintenance/cancel'){cancelled.push(options.headers['X-DSR-Maintenance']);return {};}
   if(path==='backup/import'){entered();await new Promise(r=>releaseUpload=r);return {json:async()=>({candidate:true,request_id:ids[1]})};}
   throw Error(path);
  },translate:a=>a[1]});document.body.append(p.root);p.root.querySelector('input[type=password]').value=password;
  const dt=new DataTransfer();dt.items.add(new File([encrypted],'synthetic.dsrbak'));p.root.querySelector('input[type=file]').files=dt.files;
  [...p.root.querySelectorAll('button')].find(b=>b.textContent==='Restore backup').click();await entering;p.dispose();releaseUpload();await new Promise(r=>setTimeout(r,20));
  if(!ids.every(id=>cancelled.includes(id)))throw Error('Late candidate was not cancelled');p.root.remove();
 });checks++;
 await page.locator('nav [data-page=system]').click();current=page.locator('.backup-panel');
 await page.evaluate(()=>Object.defineProperty(globalThis,'isSecureContext',{value:false}));const requestBefore=requests;await current.locator('input[type=password]').nth(0).fill(password);await current.locator('input[type=password]').nth(1).fill(password);await current.getByRole('button',{name:'Create encrypted backup',exact:true}).click();await current.getByText('HTTPS and Web Crypto are required in this browser.',{exact:true}).waitFor();eq(requests,requestBefore);eq(await current.locator('input[type=password]').nth(0).inputValue(),'');eq(errors,[]);
 await browser.close();fs.rmSync(target,{force:true});console.log(`PASS: ${checks} Chromium backup flow checks; actual Web Crypto/assets, HTTP and LCD consent MODEL, no device actions`);
})().catch(async e=>{console.error(e);if(page)await page.screenshot({path:path.join(out,'failure.png'),fullPage:true}).catch(()=>{});if(browser)await browser.close();process.exit(1);});
