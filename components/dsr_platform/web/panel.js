'use strict';
const $=s=>document.querySelector(s), el=(tag,text,cls)=>{const n=document.createElement(tag);if(text!==undefined)n.textContent=text;if(cls)n.className=cls;return n;};
let config=null,page='dashboards',dirty=false,saving=false,conflict=false,dragId=null,editSerial=0;
const titles={dashboards:'Pulpity',rooms:'Pomieszczenia',devices:'Urządzenia Shelly',discovery:'Dodaj urządzenie Shelly',scenes:'Sceny',appearance:'Wygląd i język',system:'System i aktualizacje'};
const kinds={light:'Oświetlenie',relay:'Przekaźnik',plug:'Gniazdko',cover:'Roleta',contact:'Czujnik otwarcia',sensor:'Czujnik'};
function notice(s){$('#notice').textContent=s;}
function changed(){editSerial++;dirty=true;$('#dirty').textContent='Niezapisane zmiany';$('#save').disabled=saving||conflict;}
function button(text,fn,cls){const b=el('button',text,cls);b.type='button';b.addEventListener('click',fn);return b;}
function field(parent,label,value,fn,type='text'){const l=el('label',label),i=el('input');i.type=type;i.value=value;i.addEventListener('input',()=>{fn(type==='number'?Number(i.value):i.value);changed();});l.append(i);parent.append(l);return i;}
function select(parent,label,value,options,fn){const l=el('label',label),s=el('select');s.setAttribute('aria-label',label);for(const [v,t]of options){const o=el('option',t);o.value=v;s.append(o);}s.value=String(value);s.addEventListener('change',()=>{fn(s.value);changed();});l.append(s);parent.append(l);return s;}
function nextId(items){return Math.max(0,...items.map(x=>x.id))+1;}
async function request(path,options={}){const r=await fetch('/api/v1/'+path,{...options,headers:{'X-DSR-Request':'1',...(options.headers||{})}});if(r.status===401){$('#workspace').hidden=true;$('#pairing').hidden=false;throw Error('Parowanie wymagane');}if(!r.ok){let e;try{e=await r.json();}catch{}throw Error(e?.error||'Błąd '+r.status);}return r;}
async function load(){try{const r=await request('config');config=await r.json();dirty=conflict=false;$('#workspace').hidden=false;$('#pairing').hidden=true;$('#revision').textContent=config.revision;$('#dirty').textContent='Wszystkie zmiany zapisane';$('#save').disabled=true;render();notice('');}catch(e){notice(e.message);}}
function box(title){const n=el('div',undefined,'box');if(title)n.append(el('h2',title));return n;}
function empty(parent,text){parent.append(el('p',text,'empty'));}
function remove(kind,id){const refs=[];for(const d of config.dashboards)for(const t of d.tiles)if(t.type===kind.slice(0,-1)&&t.ref===id)refs.push(t);if(kind==='devices'&&config.scenes.some(s=>s.actions.some(a=>a.device===id))){notice('Najpierw usuń działania scen odwołujące się do urządzenia.');return;}if(kind==='rooms'&&config.devices.some(d=>d.room===id)){notice('Najpierw przenieś urządzenia z tego pomieszczenia.');return;}if(refs.length){notice('Najpierw usuń powiązane karty z pulpitów.');return;}if(kind==='dashboards'&&config.dashboards.length===1){notice('Zachowaj przynajmniej jeden pulpit.');return;}config[kind]=config[kind].filter(x=>x.id!==id);if(config.active_dashboard===id&&kind==='dashboards')config.active_dashboard=config.dashboards[0].id;changed();render();}
function list(kind,edit){const n=box(titles[kind]);for(const item of config[kind]){const row=el('div',undefined,'row'),txt=el('div',undefined,'grow');txt.append(el('h3',item.name));txt.append(el('p',kind==='devices'?(kinds[item.kind]+' · '+(item.host||'Bez adresu')):'#'+item.id));row.append(txt,button('Edytuj',()=>edit(item)),button('Usuń',()=>remove(kind,item.id),'danger'));n.append(row);}if(!config[kind].length)empty(n,'Brak wpisów. Dodaj pierwszy.');n.append(button('+ Dodaj',()=>{const id=nextId(config[kind]);if(config[kind].length>=({dashboards:4,rooms:8,devices:16,scenes:8}[kind]))return notice('Osiągnięto limit wpisów.');const item={id,name:kind==='dashboards'?'Nowy pulpit':kind==='rooms'?'Nowe pomieszczenie':kind==='devices'?'Nowe urządzenie':'Nowa scena',...(kind==='dashboards'?{tiles:[]}:kind==='rooms'?{icon:0,photo:''}:kind==='devices'?{host:'',channel:0,room:0,kind:'light'}:{actions:[]})};config[kind].push(item);changed();edit(item);},'primary'));return n;}
function editBase(kind,item){const target=$('#editor');target.replaceChildren();const n=box('Edycja: '+item.name);const form=el('div',undefined,'fields');field(form,'Nazwa',item.name,v=>item.name=v).maxLength=47;n.append(form);target.append(n);n.append(button('← Wróć do listy',render));return {n,form};}
function editRoom(item){
 const {n,form}=editBase('rooms',item);
 select(form,'Ikona',item.icon,[[0,'Salon'],[1,'Kuchnia'],[2,'Sypialnia'],[3,'Łazienka']],v=>item.icon=Number(v));
 n.append(el('p',item.photo?'Zdjęcie jest zapisane w pilocie.':'Możesz użyć własnego zdjęcia.','muted'));
 const f=field(form,'Zdjęcie pomieszczenia','',()=>{},'file');f.accept='image/jpeg,image/png,image/webp';
 f.addEventListener('change',async()=>{if(!f.files[0])return;try{await uploadPhoto(f.files[0],item);notice('Zdjęcie zoptymalizowane do 320 × 180. Zapisz konfigurację.');}catch(e){notice(e.message);}});
 n.append(button('Usuń zdjęcie',()=>{item.photo='';changed();editRoom(item);}));
}
function editDevice(item){
 const {form,n}=editBase('devices',item);
 select(form,'Typ urządzenia',item.kind,Object.entries(kinds),v=>item.kind=v);
 field(form,'Adres IPv4 Shelly',item.host,v=>item.host=v).maxLength=15;
 field(form,'Kanał (0–15)',item.channel,v=>item.channel=v,'number');
 select(form,'Pomieszczenie',item.room,[[0,'Nieprzypisane'],...config.rooms.map(r=>[r.id,r.name])],v=>item.room=Number(v));
 n.append(el('p','Karta i połączenie są zapisywane osobno. Obecny moduł komunikacji obsługuje jedno zweryfikowane połączenie z przekaźnikiem. Sprawdzenie poniżej zastąpi poprzednie połączenie dopiero po sukcesie. Czujniki i rolety wymagają dalszej obsługi protokołu.','muted'));
 const loginLabel=el('label','Login Shelly'),login=el('input');login.value='admin';login.autocomplete='off';login.maxLength=32;loginLabel.append(login);
 const passLabel=el('label','Hasło Shelly'),password=el('input');password.type='password';password.autocomplete='off';password.maxLength=64;passLabel.append(password);form.append(loginLabel,passLabel);
 n.append(button('Sprawdź i powiąż połączenie',async()=>{try{notice('Sprawdzanie tożsamości Shelly…');await request('devices/bind',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({host:item.host,name:item.name,channel:item.channel,username:login.value,password:password.value})});password.value='';for(let i=0;i<60;i++){await new Promise(r=>setTimeout(r,500));const status=await(await request('status')).json();if(!status.device_busy){notice(status.device_message||'Sprawdź wynik na ekranie pilota.');return;}}notice('Sprawdzanie trwa. Wynik pojawi się na pilocie.');}catch(error){password.value='';notice(error.message);}},'primary'));
}
function editScene(item){const {n}=editBase('scenes',item);n.append(el('p','Jawne działania sceny. Wykonanie wymaga obsługi typu urządzenia przez moduł komunikacji.','muted'));for(const [index,a]of item.actions.entries()){const row=el('div',undefined,'row');select(row,'Urządzenie',a.device,config.devices.map(d=>[d.id,d.name]),v=>a.device=Number(v));select(row,'Działanie',a.command,[['on','Włącz'],['off','Wyłącz'],['open','Otwórz'],['close','Zamknij'],['stop','Stop']],v=>a.command=v);row.append(button('×',()=>{item.actions.splice(index,1);changed();editScene(item);}));n.append(row);}n.append(button('+ Działanie',()=>{if(!config.devices.length||item.actions.length>=8)return notice('Dodaj urządzenie; limit sceny to 8 działań.');item.actions.push({device:config.devices[0].id,command:'on'});changed();editScene(item);}));}
function move(d,id,delta){const at=d.tiles.findIndex(t=>t.id===id),to=at+delta;if(to<0||to>=d.tiles.length)return;d.tiles.splice(to,0,d.tiles.splice(at,1)[0]);changed();editDashboard(d);}
function editDashboard(d){const {n,form}=editBase('dashboards',d);select(form,'Pulpit startowy',config.active_dashboard===d.id?'yes':'no',[['yes','Tak'],['no','Nie']],v=>{if(v==='yes')config.active_dashboard=d.id;});n.append(el('p','Przeciągaj karty albo użyj strzałek. Karty urządzeń zawsze zajmują pełną szerokość.','muted'));for(const t of d.tiles){const row=el('div',undefined,'tile');row.draggable=true;row.dataset.id=t.id;row.addEventListener('dragstart',()=>{dragId=t.id;row.classList.add('dragging');});row.addEventListener('dragend',()=>row.classList.remove('dragging'));row.addEventListener('dragover',e=>e.preventDefault());row.addEventListener('drop',e=>{e.preventDefault();const a=d.tiles.findIndex(x=>x.id===dragId),b=d.tiles.findIndex(x=>x.id===t.id);if(a>=0&&b>=0){d.tiles.splice(b,0,d.tiles.splice(a,1)[0]);changed();editDashboard(d);}});row.append(el('span','⠿','drag'));const target=config[t.type==='device'?'devices':t.type==='room'?'rooms':'scenes'].find(x=>x.id===t.ref);const text=el('div',undefined,'grow');text.append(el('h3',target?.name||'Brak obiektu'),el('p',t.type+' · '+(t.span===2?'pełna szerokość':'½ szerokości'),'muted'));row.append(text,button('↑',()=>move(d,t.id,-1)),button('↓',()=>move(d,t.id,1)),button('Edytuj',()=>editTile(d,t)),button('×',()=>{d.tiles=d.tiles.filter(x=>x.id!==t.id);changed();editDashboard(d);}));n.append(row);}if(!d.tiles.length)empty(n,'Ten pulpit jest pusty.');n.append(button('+ Dodaj kartę',()=>{if(d.tiles.length>=24)return notice('Limit: 24 karty na pulpit.');if(!config.devices.length&&!config.rooms.length&&!config.scenes.length)return notice('Najpierw dodaj pomieszczenie lub urządzenie.');const type=config.devices.length?'device':config.rooms.length?'room':'scene',a=config[type==='device'?'devices':type==='room'?'rooms':'scenes'];const tile={id:nextId(d.tiles),type,ref:a[0].id,span:2,height:1,fields:255};d.tiles.push(tile);changed();editTile(d,tile);},'primary'));}
function editTile(d,t){
 const target=$('#editor');target.replaceChildren();const n=box('Karta pulpitu'),form=el('div',undefined,'fields');n.append(form);target.append(n);
 select(form,'Typ karty',t.type,[['device','Urządzenie'],['room','Pomieszczenie'],['scene','Scena']],v=>{
  const a=config[v==='device'?'devices':v==='room'?'rooms':'scenes'];
  if(!a.length){notice('Najpierw dodaj wpis tego typu.');editTile(d,t);return;}
  t.type=v;t.ref=a[0].id;if(v!=='room')t.span=2;editTile(d,t);
 });
 const a=config[t.type==='device'?'devices':t.type==='room'?'rooms':'scenes'];
 select(form,'Wyświetlany obiekt',t.ref,a.map(x=>[x.id,x.name]),v=>t.ref=Number(v));
 select(form,'Szerokość',t.span,t.type==='room'?[[1,'Połowa'],[2,'Pełna szerokość']]:[[2,'Pełna szerokość']],v=>t.span=Number(v));
 select(form,'Wysokość',t.height,[[1,'Standardowa'],[2,'Duża']],v=>t.height=Number(v));
 const c=el('div',undefined,'checks');
 for(const [bit,name]of [[1,'Moc (W)'],[2,'Napięcie (V)'],[4,'Bateria'],[8,'Temperatura'],[16,'Wilgotność'],[32,'Pozycja'],[64,'Stan otwarcia'],[128,'Dodatkowe parametry']]){
  const l=el('label',name),i=el('input');i.type='checkbox';i.checked=!!(t.fields&bit);
  i.addEventListener('change',()=>{t.fields=i.checked?t.fields|bit:t.fields&~bit;changed();});l.prepend(i);c.append(l);
 }
 n.append(c,button('← Wróć do pulpitu',()=>editDashboard(d)));
}
function appearance(){const n=box('Wygląd i język'),f=el('div',undefined,'fields');n.append(f);select(f,'Język interfejsu',config.language,[['pl','Polski'],['en','English'],['de','Deutsch']],v=>config.language=v);select(f,'Motyw',config.theme,[['graphite','Grafit'],['midnight','Nocny']],v=>config.theme=v);field(f,'Kolor akcentu','#'+config.accent.toString(16).padStart(6,'0'),v=>config.accent=parseInt(v.slice(1),16),'color');return n;}
function system(){const n=box('Aktualizacja OTA');n.append(el('p','Wybierz plik aplikacji BIN dla LCDWiki ES3C35P. Obraz USB łączący bootloader i partycje zostanie odrzucony. Konfiguracja i zdjęcia pozostają w oddzielnej partycji.'));const f=el('input');f.type='file';f.accept='.bin';n.append(f,button('Sprawdź i zainstaluj OTA',async()=>{if(!f.files[0])return;try{const data=await f.files[0].arrayBuffer();notice('Wysyłanie obrazu do nieaktywnej partycji…');const r=await request('ota',{method:'POST',headers:{'Content-Type':'application/octet-stream','X-SHA256':sha256(new Uint8Array(data))},body:data});const result=await r.json();if(result.installed){notice('Obraz zweryfikowany. Uruchom ponownie pilot, aby go zastosować.');n.append(button('Uruchom ponownie',async()=>{await request('restart',{method:'POST'});notice('Pilot uruchamia się ponownie…');}));}}catch(e){notice(e.message);}},'primary'));n.append(el('p','Najpierw zweryfikuj źródło pliku. Suma SHA256 sprawdza przesyłanie, a nie podpis wydawcy. To rozwojowy instalator lokalny.','muted'));return n;}
let lanMode='start',lanSelected=null,lanCredentials=null,lanSnapshot=null,lanGeneration=0;
let bleGeneration=0;
async function pollBle(generation){
 if(page!=='discovery'||lanMode!=='ble'||generation!==bleGeneration)return;
 try{
  const s=await(await request('ble')).json();if(generation!==bleGeneration)return;
  const status=$('#ble-status'),list=$('#ble-results');if(!status||!list)return;
  status.textContent=s.busy?`Odbieranie reklam: ${Math.floor(s.elapsed_ms/1000)} / 20 s` : s.phase===5?'Bluetooth niedostępny':s.phase===4?'Skan przerwany':s.devices.length?'Wykryto — nie sparowano':'Nie znaleziono urządzeń BLE';
  $('#ble-start').disabled=s.busy;$('#ble-cancel').disabled=!s.busy;list.replaceChildren();
  for(const [kind,heading] of [[0,'Shelly • RPC / konfiguracja'],[1,'Shelly BLU'],[2,'Pozostałe reklamy']]){
   const rows=s.devices.filter(d=>d.kind===kind);if(!rows.length)continue;list.append(el('h3',heading));
   for(const d of rows){const card=el('div',undefined,'row');card.append(el('strong',d.model||d.name||'Nieznany model'),el('span',`${d.rssi} dBm · Wykryto, nie sparowano`));list.append(card);}
  }
 }catch(e){notice(e.message);}
 if(generation===bleGeneration)setTimeout(()=>pollBle(generation),800);
}
function bleView(){
 const n=box('Bluetooth LE • skan'),generation=++bleGeneration;
 n.append(button('← Metody dodawania',()=>{bleGeneration++;lanMode='start';render();}),el('p','Skan reklam BLE działa na pilocie. Nie paruje, nie przekazuje danych Wi-Fi i nie zapisuje fikcyjnych urządzeń. Nazwa i typ reklamy nie są uwierzytelnioną tożsamością.'));
 const start=button('Szukaj BLE',async()=>{try{await request('ble/scan',{method:'POST'});}catch(e){notice(e.message);}},'primary');start.id='ble-start';
 const cancel=button('Anuluj skan BLE',async()=>{try{await request('ble/cancel',{method:'POST'});}catch(e){notice(e.message);}});cancel.id='ble-cancel';
 const status=el('p','Odczyt stanu skanera…');status.id='ble-status';const list=el('div');list.id='ble-results';n.append(start,cancel,status,list,el('p','BLU może wymagać krótkiego naciśnięcia przycisku. Urządzenie Wi-Fi musi reklamować BLE; konfiguracja RPC może wymagać pairing i włączonego okna konfiguracji.','muted'));setTimeout(()=>pollBle(generation),0);return n;
}
const lanError=code=>({4:'Błąd logowania. Sprawdź dane.',5:'Shelly offline lub brak Wi-Fi.',9:'Tożsamość urządzenia się zmieniła. Wyszukaj ponownie.',10:'Limit logowań Shelly. Poczekaj przed ponowieniem.'}[code]||'Nie udało się zweryfikować urządzenia.');
function lanInput(parent,label,value='',type='text'){
 const l=el('label',label),i=el('input');i.type=type;i.value=value;i.setAttribute('aria-label',label);i.maxLength=type==='password'?64:47;l.append(i);parent.append(l);return i;
}
function lanSelect(parent,label,items){const l=el('label',label),s=el('select');s.setAttribute('aria-label',label);for(const [value,name]of items){const o=el('option',name);o.value=value;s.append(o);}l.append(s);parent.append(l);return s;}
function openDiscovery(){if(dirty||saving)return notice('Zapisz szkic konfiguracji przed dodaniem połączenia.');page='discovery';lanMode='start';lanCredentials=null;lanGeneration++;render();}
async function scanLan(){
 try{await request('discovery/scan',{method:'POST'});notice('Wyszukiwanie urządzeń…');lanMode='scan';render();pollLan(++lanGeneration);}catch(e){notice(e.message);}
}
async function inspectLan(device,credentials){
 lanSelected=device;
 if(device.auth_required&&!credentials){lanMode='credentials';render();return;}
 lanCredentials=credentials||{username:'',password:''};
 try{await request('discovery/inspect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({host:device.host,id:device.id,...lanCredentials})});notice('Sprawdzanie urządzenia…');lanMode='inspect';render();pollLan(++lanGeneration);}catch(e){notice(e.message);}
}
async function pollLan(generation){
 if(page!=='discovery'||generation!==lanGeneration)return;
 try{
  lanSnapshot=await(await request('discovery')).json();const s=lanSnapshot;
  if(s.phase===1||s.phase===5){const progress=$('#lan-progress');if(progress)progress.textContent=s.phase===1?`Weryfikacja API: ${s.probed} / ${s.total}`:'Odczyt możliwości urządzenia…';setTimeout(()=>pollLan(generation),800);return;}
  if(lanMode==='scan'){lanMode='results';render();if(s.phase===4)notice('Najpierw połącz Wi-Fi.');}
  else if(lanMode==='inspect'){
   if(s.phase===6&&s.selected.id===lanSelected.id){lanMode='components';render();notice('Możliwości urządzenia potwierdzone odpowiedzią API.');}
   else{lanMode='credentials';render();notice(lanError(s.result));}
  }
 }catch(e){notice(e.message);}
}
async function bindLan(name,room,channel,button){
 button.disabled=true;const body={host:lanSelected.host,expected_id:lanSelected.id,name,room:Number(room),channel:Number(channel),...lanCredentials};
 try{
  await request('devices/bind',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});notice('Sprawdzanie tożsamości i zapisywanie połączenia…');
  for(let i=0;i<100;i++){
   await new Promise(r=>setTimeout(r,250));const s=await(await request('status')).json();
   if(s.device_busy||s.busy)continue;
   if(s.device_result!==0){if(s.device_result===4){lanMode='credentials';render();}throw Error(lanError(s.device_result));}
   const current=await(await request('config')).json();const saved=current.devices.find(d=>d.host===body.host&&d.channel===body.channel&&d.room===body.room&&d.name===body.name);
   page='devices';lanCredentials=null;lanGeneration++;await load();notice(saved?'Połączenie, nazwa i pomieszczenie zapisane.':'Połączenie zapisano. Konfiguracja pulpitu wymaga sprawdzenia.');return;
  }
  throw Error('Nie potwierdzono zakończenia zapisu. Sprawdź stan na pilocie.');
 }catch(e){notice(e.message);}finally{body.password='';button.disabled=false;}
}
function discoveryView(){
 if(lanMode==='ble')return bleView();
 const n=box('Dodaj urządzenie Shelly');n.append(button('← Urządzenia',()=>{lanCredentials=null;lanGeneration++;page='devices';render();}));
 if(lanMode==='scan'||lanMode==='inspect'){
  const p=el('p',lanMode==='scan'?'Wyszukiwanie mDNS i weryfikacja API…':'Sprawdzanie wybranego urządzenia…');p.id='lan-progress';n.append(p,button('Przerwij',async()=>{try{await request('discovery/cancel',{method:'POST'});}catch(e){notice(e.message);}}));return n;
 }
 if(lanMode==='start'){
  n.append(el('p','Wyszukaj urządzenia w tej samej sieci lokalnej. Lista zawiera wyłącznie tożsamości potwierdzone odpowiedzią API.'),button('Skanuj sieć Wi-Fi',scanLan,'primary'));
  const ap=button('Wi-Fi AP — niewdrożone',()=>{});ap.disabled=true;n.append(ap,button('Bluetooth LE • skan',()=>{lanCredentials=null;lanGeneration++;lanMode='ble';render();}));return n;
 }
 if(lanMode==='results'){
  n.append(button('Skanuj ponownie',scanLan));if(!lanSnapshot?.devices.length)empty(n,'Nie znaleziono Shelly. Sprawdź Wi-Fi, mDNS i izolację klientów.');
  for(const d of lanSnapshot?.devices||[]){const row=el('div',undefined,'row'),text=el('div',undefined,'grow');text.append(el('h3',d.model),el('p',`${d.host} · Gen ${d.generation} · ${d.auth_required?'Wymaga hasła':'Bez hasła'}`),el('p',d.id,'muted'));row.append(text,button('Sprawdź urządzenie',()=>inspectLan(d)));n.append(row);}return n;
 }
 n.append(el('h3',lanSelected.model),el('p',`${lanSelected.host} · Gen ${lanSelected.generation} · ${lanSelected.id}`,'muted'));
 const form=el('div',undefined,'fields');n.append(form);
 if(lanMode==='credentials'){
  const u=lanInput(form,'Login','admin'),p=lanInput(form,'Hasło Shelly','','password');n.append(button('Sprawdź urządzenie',()=>inspectLan(lanSelected,{username:u.value,password:p.value}),'primary'));return n;
 }
 const name=lanInput(form,'Nazwa urządzenia',lanSelected.name),room=lanSelect(form,'Pomieszczenie',[[0,'Nieprzypisane'],...config.rooms.map(r=>[r.id,r.name])]);
 const channels=[];for(let i=0;i<16;i++)if(lanSnapshot.capabilities.switches&(1<<i))channels.push([i,String(i)]);
 const channel=lanSelect(form,'Kanał z odpowiedzi API',channels.length?channels:[['','Brak obsługiwanego kanału']]);
 n.append(el('p','Obecny transport obsługuje jedno aktywne połączenie przekaźnika. Zapis zastąpi wcześniejsze powiązanie dopiero po poprawnej weryfikacji.'));
 const save=button('Sprawdź i zapisz połączenie',()=>bindLan(name.value,room.value,channel.value,save),'primary');save.disabled=!channels.length;n.append(save);
 if(!channels.length)n.append(el('p','Urządzenie rozpoznane. Sterowanie tym typem nie jest wdrożone.','muted'));return n;
}
function render(){if(!config)return;if(page!=='discovery'){lanCredentials=null;lanGeneration++;}$('#title').textContent=titles[page];document.querySelectorAll('nav button').forEach(b=>b.setAttribute('aria-current',b.dataset.page===(page==='discovery'?'devices':page)?'page':'false'));const target=$('#editor');if(page==='discovery'){target.replaceChildren(discoveryView());return;}const view=page==='appearance'?appearance():page==='system'?system():list(page,({dashboards:editDashboard,rooms:editRoom,devices:editDevice,scenes:editScene})[page]);if(page==='devices')view.prepend(button('Wyszukaj Shelly w LAN',openDiscovery,'primary'));target.replaceChildren(view);}
function canonical(value){if(Array.isArray(value))return value.map(canonical);if(value&&typeof value==='object')return Object.fromEntries(Object.keys(value).sort().map(k=>[k,canonical(value[k])]));return value;}
async function save(){
 if(!dirty||saving||conflict)return;
 const submitted=structuredClone(config),serial=editSerial;saving=true;$('#save').disabled=true;
 try{
  await request('config',{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify(submitted)});notice('Zapisywanie konfiguracji…');let acknowledged=false;
  for(let i=0;i<40;i++){
   await new Promise(r=>setTimeout(r,250));const s=await(await request('status')).json();if(s.busy)continue;
   if(s.revision!==submitted.revision+1)throw Error('Zapis nie powiódł się. Zmiany pozostają w edytorze.');
   const published=await(await request('config')).json(),expected={...submitted,revision:s.revision};
   if(JSON.stringify(canonical(published))!==JSON.stringify(canonical(expected))){conflict=true;throw Error('Konfiguracja zmieniła się na pilocie. Wczytaj aktualną wersję.');}
   config.revision=s.revision;$('#revision').textContent=s.revision;acknowledged=true;
   if(editSerial===serial){dirty=false;$('#dirty').textContent='Wszystkie zmiany zapisane';notice('Zapisano. Ekran pilota korzysta z tej samej konfiguracji.');}
   else{dirty=true;$('#dirty').textContent='Niezapisane zmiany';notice('Zapisano wcześniejszą wersję. Późniejsze zmiany pozostają niezapisane.');}break;
  }
  if(!acknowledged)throw Error('Nie potwierdzono zapisu w wymaganym czasie. Zachowano zmiany w edytorze.');
 }catch(e){notice(e.message);}finally{saving=false;$('#save').disabled=!dirty||conflict;}
}
async function poll(){if(!config||$('#workspace').hidden)return;try{const s=await(await request('status')).json();$('#connection').textContent='● Połączono lokalnie';$('#firmware').textContent=s.firmware||'';if(!saving&&s.revision!==config.revision){if(dirty){conflict=true;$('#save').disabled=true;notice('Konfiguracja zmieniła się na pilocie. Wczytaj aktualną wersję, aby uniknąć nadpisania.');}else await load();}const r=await request('preview.raw');if(r.status===200){const bytes=new Uint8Array(await r.arrayBuffer());if(bytes.length===320*480*2){const ctx=$('#preview').getContext('2d'),im=ctx.createImageData(320,480);for(let i=0;i<320*480;i++){const v=bytes[i*2]|bytes[i*2+1]<<8;im.data[i*4]=(v>>11)*255/31;im.data[i*4+1]=((v>>5)&63)*255/63;im.data[i*4+2]=(v&31)*255/31;im.data[i*4+3]=255;}ctx.putImageData(im,0,0);$('#preview-status').textContent='Obraz pobrany z zadania LVGL pilota.';}}}catch(e){$('#connection').textContent='Brak połączenia';}}
async function uploadPhoto(file,room){
 if(!['image/jpeg','image/png','image/webp'].includes(file.type)||file.size>15*1024*1024)throw Error('Wybierz JPG, PNG lub WebP do 15 MB.');
 const bitmap=await createImageBitmap(file);if(bitmap.width*bitmap.height>40000000){bitmap.close();throw Error('Zbyt duże zdjęcie.');}
 const c=document.createElement('canvas');c.width=320;c.height=180;
 const ctx=c.getContext('2d'),ratio=Math.max(320/bitmap.width,180/bitmap.height);
 ctx.drawImage(bitmap,(320-bitmap.width*ratio)/2,(180-bitmap.height*ratio)/2,bitmap.width*ratio,bitmap.height*ratio);bitmap.close();
 const rgba=ctx.getImageData(0,0,320,180).data,raw=new Uint8Array(115200);
 for(let i=0;i<57600;i++){const value=(rgba[i*4]>>3)<<11|(rgba[i*4+1]>>2)<<5|(rgba[i*4+2]>>3);raw[i*2]=value&255;raw[i*2+1]=value>>8;}
 const id=sha256(raw).slice(0,32);await request('photos/'+id,{method:'POST',headers:{'Content-Type':'application/octet-stream'},body:raw});room.photo=id;changed();
}
/* SHA-256 is included locally because WebCrypto.digest is not available on
 * ordinary HTTP origins. No CDN, telemetry, external fonts or cloud calls. */
function sha256(input){const K=[0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2];let H=[0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19];const length=Math.ceil((input.length+9)/64)*64,p=new Uint8Array(length);p.set(input);p[input.length]=128;new DataView(p.buffer).setUint32(length-4,input.length*8);const w=new Uint32Array(64),ro=(v,n)=>v>>>n|v<<(32-n);for(let at=0;at<length;at+=64){const dv=new DataView(p.buffer,at,64);for(let i=0;i<16;i++)w[i]=dv.getUint32(i*4);for(let i=16;i<64;i++)w[i]=(w[i-16]+(ro(w[i-15],7)^ro(w[i-15],18)^w[i-15]>>>3)+w[i-7]+(ro(w[i-2],17)^ro(w[i-2],19)^w[i-2]>>>10))>>>0;let[a,b,c,d,e,f,g,h]=H;for(let i=0;i<64;i++){const t1=(h+(ro(e,6)^ro(e,11)^ro(e,25))+((e&f)^(~e&g))+K[i]+w[i])>>>0,t2=((ro(a,2)^ro(a,13)^ro(a,22))+((a&b)^(a&c)^(b&c)))>>>0;h=g;g=f;f=e;e=(d+t1)>>>0;d=c;c=b;b=a;a=(t1+t2)>>>0;}H=H.map((v,i)=>(v+[a,b,c,d,e,f,g,h][i])>>>0);}return H.map(v=>v.toString(16).padStart(8,'0')).join('');}
document.querySelectorAll('nav button').forEach(b=>b.addEventListener('click',()=>{page=b.dataset.page;render();}));$('#save').addEventListener('click',save);$('#reload').addEventListener('click',load);$('#pair-form').addEventListener('submit',async e=>{e.preventDefault();try{await request('pair',{method:'POST',body:$('#pair-code').value.trim()});$('#pair-code').value='';await load();}catch(error){notice(error.message);}});window.addEventListener('beforeunload',e=>{if(dirty){e.preventDefault();e.returnValue='';}});load();setInterval(poll,1500);
