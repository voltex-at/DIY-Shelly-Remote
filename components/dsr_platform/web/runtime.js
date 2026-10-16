/* Live transport registry, separate from the workspace editor. No credentials,
 * optimistic switch state, automatic SET retry or host-based command routing. */
'use strict';
(()=>{
 const words={
  title:['Połączone urządzenia — stan rzeczywisty','Connected devices — live state','Verbundene Geräte — aktueller Zustand'],
  loading:['Odczyt stanu urządzeń…','Reading device state…','Gerätestatus wird gelesen…'],
  empty:['Brak zapisanych połączeń. Dodaj Shelly przez wyszukiwanie LAN.','No saved connections. Add a Shelly using LAN discovery.','Keine gespeicherten Verbindungen. Füge einen Shelly über die LAN-Suche hinzu.'],
  unavailable:['Stan niedostępny. Sterowanie zablokowane do następnego odczytu.','State unavailable. Controls are disabled until the next reading.','Status nicht verfügbar. Steuerung bis zur nächsten Abfrage gesperrt.'],
  pending:['Polecenie w kolejce lub odczyt potwierdzający…','Command queued or awaiting readback…','Befehl eingereiht oder Rückmeldung ausstehend…'],
  offline:['Offline / brak aktualnego odczytu','Offline / no current reading','Offline / kein aktueller Messwert'],
  on:['WŁ. — potwierdzone odczytem','ON — confirmed by readback','EIN — durch Abfrage bestätigt'],
  off:['WYŁ. — potwierdzone odczytem','OFF — confirmed by readback','AUS — durch Abfrage bestätigt'],
  turnOn:['Włącz','Turn on','Einschalten'],turnOff:['Wyłącz','Turn off','Ausschalten'],
  error:['Błąd urządzenia: {code}. Polecenia nie ponowiono.','Device error: {code}. The command was not retried.','Gerätefehler: {code}. Der Befehl wurde nicht wiederholt.'],
  rejected:['Polecenie nie zostało przyjęte. Sprawdź stan i spróbuj świadomie ponownie.','The command was not accepted. Check the state before retrying.','Der Befehl wurde nicht angenommen. Prüfe den Status vor einem erneuten Versuch.'],
  uncertain:['Nie potwierdzono przyjęcia polecenia. Sprawdź odczyt; jeśli blokada pozostaje, odśwież stronę. Polecenia nie ponowiono.','Command acceptance is unconfirmed. Check readback; reload the page if controls stay locked. The command was not retried.','Befehlsannahme unbestätigt. Prüfe die Rückmeldung; bei anhaltender Sperre die Seite neu laden. Keine automatische Wiederholung.'],
  channel:['Kanał {channel} · {host}','Channel {channel} · {host}','Kanal {channel} · {host}'],
  age:['Wiek odczytu: {seconds} s','Reading age: {seconds} s','Messwertalter: {seconds} s'],
  unknown:['Brak odczytu','No reading','Kein Messwert'],
  open:['Otwórz','Open','Öffnen'],close:['Zamknij','Close','Schließen'],stop:['STOP','STOP','STOP'],
  opened:['Otwarte','Open','Geöffnet'],closed:['Zamknięte','Closed','Geschlossen'],opening:['Otwieranie','Opening','Öffnet'],closing:['Zamykanie','Closing','Schließt'],stopped:['Zatrzymano','Stopped','Gestoppt'],calibrating:['Kalibracja','Calibrating','Kalibrierung'],
  position:['Pozycja potwierdzona: {position}%','Confirmed position: {position}%','Bestätigte Position: {position}%'],
  preview:['Podgląd celu: {position}% — puść, aby wysłać','Target preview: {position}% — release to send','Zielvorschau: {position}% — zum Senden loslassen'],
  noPosition:['Pozycjonowanie niedostępne','Position control unavailable','Positionierung nicht verfügbar'],
  coverError:['Błąd rolety: {code}','Cover error: {code}','Rollladenfehler: {code}'],
 };
 let section=null,holder=null,status=null,fetching=false,fetchSerial=0,expiryTimer=null,boot=null;
 const cards=new Map(),local=new Map(),stops=new Map();
 const locale=()=>({en:1,de:2}[document.documentElement.lang]||0);
 function text(key,args={}){return words[key][locale()].replace(/\{(\w+)\}/g,(_,name)=>String(args[name]??''));}
 function node(tag,cls){const n=document.createElement(tag);if(cls)n.className=cls;return n;}
 // Operation IDs are uint32 serials; an older GET must stay older at wrap.
 const serialReached=(current,expected)=>((current-expected)>>>0)<0x80000000;
 const kind=d=>d.endpoint_kind??0;
 const key=d=>JSON.stringify([d.device_id,d.generation,kind(d),d.channel]);
 const current=()=>section?.isConnected&&page==='devices'&&!document.querySelector('#workspace').hidden;
 function valid(d){return d&&[0,1].includes(kind(d))&&typeof d.device_id==='string'&&d.device_id.length>0&&d.device_id.length<64&&Number.isInteger(d.generation)&&[1,2].includes(d.generation)&&Number.isInteger(d.channel)&&d.channel>=0&&d.channel<16&&typeof d.name==='string'&&typeof d.host==='string'&&typeof d.online==='boolean'&&typeof d.output==='boolean'&&typeof d.has_switch==='boolean'&&typeof d.pending==='boolean'&&Number.isInteger(d.operation_id)&&d.operation_id>=0&&d.operation_id<=0xffffffff&&Number.isInteger(d.error)&&Number.isFinite(d.sample_age_ms)&&d.sample_age_ms>=0&&(kind(d)===0||(d.generation===2&&typeof d.has_position==='boolean'&&Number.isInteger(d.position)&&d.position>=0&&d.position<=100&&Number.isInteger(d.cover_state)&&d.cover_state>=0&&d.cover_state<=6&&typeof d.can_move==='boolean'&&typeof d.can_position==='boolean'&&typeof d.can_stop==='boolean'&&typeof d.stop_pending==='boolean'&&Number.isInteger(d.cover_errors)&&d.cover_errors>=0&&d.cover_errors<=255));}
 const age=card=>card.data.sample_age_ms+Math.max(0,performance.now()-card.received);
 function fresh(card){return card.data.online&&age(card)<=15000;}
 function cancelRange(card){if(card.range){card.dragging=false;card.rangeCanceled=true;}}
 function paint(card){
  const d=card.data,l=local.get(key(d));
  card.name.textContent=d.name;card.identity.textContent=d.device_id;
  card.address.textContent=text('channel',d);
  const stop=stops.get(key(d)),waiting=d.pending||!!l?.waiting||!!stop?.waiting,cover=kind(d)===1;
  card.state.textContent=text(waiting?'pending':fresh(card)?(cover?(['unknown','opened','closed','opening','closing','stopped','calibrating'][d.cover_state]):(d.output?'on':'off')):'offline');
  card.age.textContent=fresh(card)?text('age',{seconds:Math.floor(age(card)/1000)}):text('unknown');
  card.measure.textContent=fresh(card)?[['has_power','power_w','W'],['has_voltage','voltage_v','V'],['has_current','current_a','A'],['has_energy','energy_wh','Wh']].filter(([has,value])=>d[has]===true&&Number.isFinite(d[value])).map(([,value,unit])=>`${d[value].toLocaleString(document.documentElement.lang,{maximumFractionDigits:2})} ${unit}`).join(' · '):'';
  card.error.textContent=stop?.message?text(stop.message):l?.message?text(l.message):d.error?text('error',{code:d.error}):cover&&d.cover_errors?text('coverError',{code:d.cover_errors}):'';
  for(const [button,on]of [[card.on,true],[card.off,false]]){const label=cover?(on?'open':'close'):(on?'turnOn':'turnOff');button.textContent=text(label);button.disabled=!fresh(card)||waiting||(cover?!d.can_move:!d.has_switch);button.setAttribute('aria-label',`${text(label)}: ${d.name}`);}
  if(cover){card.stop.textContent=text('stop');card.stop.disabled=!d.can_stop||d.stop_pending||!!stop?.waiting;card.stop.setAttribute('aria-label',`${text('stop')}: ${d.name}`);
   card.range.disabled=!fresh(card)||waiting||!d.can_position;if(card.range.disabled)cancelRange(card);card.range.setAttribute('aria-label',`${text('position',{position:d.has_position?d.position:'?'})}: ${d.name}`);if(!card.dragging)card.range.value=String(d.has_position?d.position:0);
   const confirmed=fresh(card)&&d.has_position?text('position',{position:d.position}):text('noPosition');card.preview.textContent=confirmed+(card.dragging?` · ${text('preview',{position:card.range.value})}`:'');
  }
 }
 function expire(){
  clearTimeout(expiryTimer);expiryTimer=null;if(!current())return;
  let delay=Infinity;for(const card of cards.values()){paint(card);if(fresh(card))delay=Math.min(delay,15001-age(card));}
  if(Number.isFinite(delay))expiryTimer=setTimeout(expire,Math.max(1,delay));
 }
 function translate(){if(!current())return;section.querySelector('h2').textContent=text('title');if(status.dataset.message)status.textContent=text(status.dataset.message);for(const card of cards.values())paint(card);}
 function fail(){window.dsrHistorySnapshot?.(null);if(!current())return;status.dataset.message='unavailable';status.textContent=text('unavailable');for(const card of cards.values()){card.data={...card.data,online:false,can_stop:false};paint(card);}}
 function create(d){
  const root=node('article','runtime-card'),head=node('div','runtime-heading'),name=node('h3'),identity=node('p','muted'),address=node('p','muted'),state=node('p'),measure=node('p'),age=node('p','muted'),error=node('p','runtime-error'),actions=node('div','actions'),on=node('button'),off=node('button');
  on.type=off.type='button';error.setAttribute('role','status');head.append(name,identity,address);actions.append(on,off);root.append(head,state,measure,age,error,actions);
  const card={root,name,identity,address,state,measure,age,error,on,off,data:d,received:performance.now(),rangeCanceled:true};
  if(kind(d)===1){card.stop=node('button');card.stop.type='button';card.range=node('input');card.range.type='range';card.range.min='0';card.range.max='100';card.range.step='1';card.preview=node('p','muted');actions.append(card.stop,card.range);root.append(card.preview);
   card.stop.addEventListener('click',()=>command(card,false,{command:'stop'}));
   card.range.addEventListener('input',()=>{if(card.range.disabled)return;card.rangeCanceled=false;card.dragging=true;paint(card);});
   card.range.addEventListener('change',()=>{const position=Number(card.range.value),gesture=card.dragging&&!card.rangeCanceled;card.dragging=false;card.rangeCanceled=true;if(!gesture){paint(card);return;}return command(card,false,{command:'position',position});});
   card.range.addEventListener('pointercancel',()=>{card.dragging=false;card.rangeCanceled=true;paint(card);});
  }
  on.addEventListener('click',()=>command(card,true));off.addEventListener('click',()=>command(card,false));return card;
 }
 function apply(snapshot,started){
  if(!Number.isInteger(snapshot.boot_id)||snapshot.boot_id<=0||snapshot.boot_id>0xffffffff||!Array.isArray(snapshot.devices)||snapshot.devices.length>16||snapshot.devices.some(d=>!valid(d))||new Set(snapshot.devices.map(key)).size!==snapshot.devices.length){fail();return;}
  // A firmware restart ends the old command epoch. Read the new process state;
  // it is no acknowledgement that a command in the previous process succeeded.
  if(boot!==null&&boot!==snapshot.boot_id){local.clear();stops.clear();for(const c of cards.values())cancelRange(c);}boot=snapshot.boot_id;
  const keep=new Set();
  for(const d of snapshot.devices){const k=key(d);keep.add(k);let c=cards.get(k);if(!c){c=create(d);cards.set(k,c);holder.append(c.root);}else if(c.data.host!==d.host||c.data.has_target!==d.has_target||c.data.target!==d.target)cancelRange(c);c.data=d;c.received=started;
   for(const map of [local,stops]){const l=map.get(k);if(l?.waiting&&!d.pending&&((l.operation>0&&serialReached(d.operation_id,l.operation))||(l.unknown&&kind(d)===0&&d.operation_id!==l.baseline&&serialReached(d.operation_id,l.baseline)))){l.waiting=false;if(!l.unknown)l.message='';}}paint(c);
  }
  for(const [k,c]of cards)if(!keep.has(k)){c.root.remove();cards.delete(k);local.delete(k);stops.delete(k);}
  status.dataset.message=snapshot.devices.length?'':'empty';status.textContent=snapshot.devices.length?'':text('empty');
  window.dsrHistorySnapshot?.(snapshot,started);expire();
 }
 async function refresh(){
  if(!current()||fetching)return;fetching=true;const serial=++fetchSerial,started=performance.now();
  try{const r=await request('devices/runtime');const snapshot=await r.json();if(current()&&serial===fetchSerial)apply(snapshot,started);}
  catch{if(serial===fetchSerial)fail();}finally{fetching=false;}
 }
 async function command(card,on,intent){
  const d=card.data,k=key(d),cover=kind(d)===1,stop=cover&&intent?.command==='stop';
  if(!current()||!card.root.isConnected||(stop?(!d.can_stop||d.stop_pending||stops.get(k)?.waiting):(!fresh(card)||d.pending||local.get(k)?.waiting||stops.get(k)?.waiting||(cover?!(intent?.command==='position'?d.can_position:d.can_move):!d.has_switch))))return;
  if(cover&&!stop&&!intent)intent={command:on?'open':'close'};
  const l={waiting:true,operation:0,baseline:d.operation_id,unknown:false,message:''};(stop?stops:local).set(k,l);paint(card);
  try{
   // Explicit identity tuple only. The firmware resolves its private locator
   // and credential; this API cannot accept a browser-supplied target address.
   const body={device_id:d.device_id,generation:d.generation,channel:d.channel};if(cover)Object.assign(body,{endpoint_kind:1},intent);else body.on=on;
   const r=await request(cover?'devices/cover':'devices/output',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
   const result=await r.json();if(result.accepted!==true||result.boot_id!==boot||!Number.isInteger(result.operation_id)||result.operation_id<=0||result.operation_id>0xffffffff)throw Error('invalid acknowledgement');
   l.operation=result.operation_id;await refresh();
  }catch{
   // A transport failure may follow acceptance. Never replay POST. Fetch only
   // readback, keep a visible uncertainty message until an explicit next try.
   l.unknown=true;l.message='uncertain';await refresh();
  }finally{if(card.root.isConnected)paint(card);}
 }
 window.dsrRuntimeMount=parent=>{
  ++fetchSerial;cards.clear();section=node('section','box runtime-section');const title=node('h2');status=node('p','muted');status.dataset.message='loading';holder=node('div','runtime-grid');section.append(title,status,holder);parent.prepend(section);
  // render() constructs its view off-DOM. Run after the view has been attached.
  queueMicrotask(()=>{translate();refresh();});
 };
 window.dsrRuntimeRefresh=refresh;
 new MutationObserver(translate).observe(document.documentElement,{attributes:true,attributeFilter:['lang']});
 // panel.js may finish its initial config fetch before this deferred asset.
 if(config&&page==='devices')window.dsrRuntimeMount(document.querySelector('#editor').firstElementChild);
})();
