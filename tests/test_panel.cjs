/* Test actual shipped browser assets against the C-model/LVGL host harness.
 * This is not a replacement for ESP32 radio/HTTP/flash hardware testing. */
const {chromium}=require('playwright');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:process.env.DSR_TEST_CHROME||undefined,args:['--no-sandbox']});
 const context=await browser.newContext({viewport:{width:1440,height:1050}}),page=await context.newPage();let checks=0;
 const errors=[];page.on('pageerror',e=>errors.push(e.message));page.on('request',r=>{assert(r.url().startsWith('http://127.0.0.1:8765/'));});
 await page.goto('http://127.0.0.1:8765/');await page.locator('#pairing').waitFor({state:'visible'});checks++;
 await page.locator('#pair-code').fill('0123456789abcdef');await page.locator('#pair-form button').click();await page.locator('#workspace').waitFor({state:'visible'});checks++;
 await page.getByRole('button',{name:'Pomieszczenia',exact:true}).click();await page.getByRole('button',{name:'+ Dodaj',exact:true}).click();await page.getByLabel('Nazwa',{exact:true}).fill('Salon');await page.getByRole('button',{name:'← Wróć do listy'}).click();
 await page.getByRole('button',{name:'Urządzenia',exact:true}).click();await page.getByRole('button',{name:'+ Dodaj',exact:true}).click();await page.getByLabel('Nazwa',{exact:true}).fill('Światło w salonie');await page.getByLabel('Adres IPv4 Shelly').fill('192.168.1.77');await page.getByLabel('Pomieszczenie',{exact:true}).selectOption('1');
 await page.getByRole('button',{name:'Pulpity',exact:true}).click();await page.getByRole('button',{name:'Edytuj',exact:true}).click();await page.getByRole('button',{name:'+ Dodaj kartę'}).click();await page.getByRole('button',{name:'← Wróć do pulpitu'}).click();
 await page.getByRole('button',{name:'+ Dodaj kartę'}).click();await page.getByLabel('Typ karty').selectOption('room');await page.getByLabel('Szerokość').selectOption('1');await page.getByRole('button',{name:'← Wróć do pulpitu'}).click();
 const tiles=page.locator('.tile');assert.equal(await tiles.count(),2);checks++;
 await tiles.nth(0).dragTo(tiles.nth(1));checks++;await page.locator('#save').click();await page.waitForFunction(()=>document.querySelector('#revision').textContent==='2');checks++;
 const config=await page.evaluate(async()=>await(await fetch('/api/v1/config')).json());assert.equal(config.rooms[0].name,'Salon');assert.equal(config.devices[0].host,'192.168.1.77');assert.equal(config.dashboards[0].tiles[0].type,'room');checks+=3;
 await page.waitForFunction(()=>document.querySelector('#preview-status').textContent.startsWith('Obraz pobrany'));checks++;
 fs.mkdirSync(path.join('build-ui','panel-review'),{recursive:true});await page.screenshot({path:'build-ui/panel-review/panel-desktop.png',fullPage:true});
 await page.evaluate(()=>fetch('/api/v1/test/reboot',{method:'POST'}));await page.reload();await page.locator('#workspace').waitFor({state:'visible'});assert.equal(await page.locator('#revision').textContent(),'2');checks++;
 await page.evaluate(()=>fetch('/api/v1/test/touch-edit',{method:'POST'}));await page.waitForFunction(()=>document.querySelector('#revision').textContent==='3');checks++;
 await page.getByRole('button',{name:'Pulpity',exact:true}).click();await page.getByRole('button',{name:'Edytuj',exact:true}).click();await page.getByLabel('Nazwa',{exact:true}).fill('Zmiana w przeglądarce');
 await page.evaluate(()=>fetch('/api/v1/test/touch-edit',{method:'POST'}));await page.waitForFunction(()=>document.querySelector('#notice').textContent.includes('Konfiguracja zmieniła się'));assert(await page.locator('#save').isDisabled());checks++;
 const sha=await page.evaluate(()=>sha256(new TextEncoder().encode('abc')));assert.equal(sha,'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad');checks++;
 await page.locator('#reload').click();await page.setViewportSize({width:390,height:844});await page.screenshot({path:'build-ui/panel-review/panel-mobile.png',fullPage:true});assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));checks++;
 assert.deepEqual(errors,[]);checks++;await browser.close();console.log(`PASS: ${checks} browser checks; shipped assets, pairing UI, CRUD, drag order, C persistence/reboot, sync/conflict and real LVGL preview`);
})().catch(e=>{console.error(e);process.exit(1)});
