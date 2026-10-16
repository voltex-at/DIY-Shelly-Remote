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
 };
 let section=null,holder=null,status=null,fetching=false,fetchSerial=0,expiryTimer=null,boot=null;
 const cards=new Map(),local=new Map();
 const locale=()=>({en:1,de:2}[document.documentElement.lang]||0);
 function text(key,args={}){return words[key][locale()].replace(/\{(\w+)\}/g,(_,name)=>String(args[name]??''));}
 function node(tag,cls){const n=document.createElement(tag);if(cls)n.className=cls;return n;}
 const key=d=>JSON.stringify([d.device_id,d.generation,d.channel]);
 const current=()=>section?.isConnected&&page==='devices'&&!document.querySelector('#workspace').hidden;
 function valid(d){return d&&typeof d.device_id==='string'&&d.device_id.length>0&&d.device_id.length<64&&Number.isInteger(d.generation)&&d.generation>0&&d.generation<=4&&Number.isInteger(d.channel)&&d.channel>=0&&d.channel<16&&typeof d.name==='string'&&typeof d.host==='string'&&typeof d.online==='boolean'&&typeof d.output==='boolean'&&typeof d.has_switch==='boolean'&&typeof d.pending==='boolean'&&Number.isInteger(d.operation_id)&&d.operation_id>=0&&Number.isInteger(d.error)&&Number.isFinite(d.sample_age_ms)&&d.sample_age_ms>=0;}
 const age=card=>card.data.sample_age_ms+Math.max(0,performance.now()-card.received);
 function fresh(card){return card.data.online&&card.data.has_switch&&age(card)<=15000;}
 function paint(card){
  const d=card.data,l=local.get(key(d));
  card.name.textContent=d.name;card.identity.textContent=d.device_id;
  card.address.textContent=text('channel',d);
  const waiting=d.pending||!!l?.waiting;
  card.state.textContent=text(waiting?'pending':fresh(card)?(d.output?'on':'off'):'offline');
  card.age.textContent=fresh(card)?text('age',{seconds:Math.floor(age(card)/1000)}):text('unknown');
  card.measure.textContent=fresh(card)?[['has_power','power_w','W'],['has_voltage','voltage_v','V'],['has_current','current_a','A']].filter(([has,value])=>d[has]===true&&Number.isFinite(d[value])).map(([,value,unit])=>`${d[value].toLocaleString(document.documentElement.lang,{maximumFractionDigits:2})} ${unit}`).join(' · '):'';
  card.error.textContent=l?.message?text(l.message):d.error?text('error',{code:d.error}):'';
  for(const [button,on]of [[card.on,true],[card.off,false]]){button.textContent=text(on?'turnOn':'turnOff');button.disabled=!fresh(card)||waiting;button.setAttribute('aria-label',`${text(on?'turnOn':'turnOff')}: ${d.name}`);}
 }
 function expire(){
  clearTimeout(expiryTimer);expiryTimer=null;if(!current())return;
  let delay=Infinity;for(const card of cards.values()){paint(card);if(fresh(card))delay=Math.min(delay,15001-age(card));}
  if(Number.isFinite(delay))expiryTimer=setTimeout(expire,Math.max(1,delay));
 }
 function translate(){if(!current())return;section.querySelector('h2').textContent=text('title');if(status.dataset.message)status.textContent=text(status.dataset.message);for(const card of cards.values())paint(card);}
 function fail(){if(!current())return;status.dataset.message='unavailable';status.textContent=text('unavailable');for(const card of cards.values()){card.data={...card.data,online:false};paint(card);}}
 function create(d){
  const root=node('article','runtime-card'),head=node('div','runtime-heading'),name=node('h3'),identity=node('p','muted'),address=node('p','muted'),state=node('p'),measure=node('p'),age=node('p','muted'),error=node('p','runtime-error'),actions=node('div','actions'),on=node('button'),off=node('button');
  on.type=off.type='button';error.setAttribute('role','status');head.append(name,identity,address);actions.append(on,off);root.append(head,state,measure,age,error,actions);
  const card={root,name,identity,address,state,measure,age,error,on,off,data:d,received:performance.now()};
  on.addEventListener('click',()=>command(card,true));off.addEventListener('click',()=>command(card,false));return card;
 }
 function apply(snapshot,started){
  if(!Number.isInteger(snapshot.boot_id)||snapshot.boot_id<=0||snapshot.boot_id>0xffffffff||!Array.isArray(snapshot.devices)||snapshot.devices.length>16||snapshot.devices.some(d=>!valid(d))||new Set(snapshot.devices.map(key)).size!==snapshot.devices.length){fail();return;}
  // A firmware restart ends the old command epoch. Read the new process state;
  // it is no acknowledgement that a command in the previous process succeeded.
  if(boot!==null&&boot!==snapshot.boot_id)local.clear();boot=snapshot.boot_id;
  const keep=new Set();
  for(const d of snapshot.devices){const k=key(d);keep.add(k);let c=cards.get(k);if(!c){c=create(d);cards.set(k,c);holder.append(c.root);}c.data=d;c.received=started;
   const l=local.get(k);if(l?.waiting&&!d.pending&&((l.operation>0&&d.operation_id>=l.operation)||(l.unknown&&d.operation_id!==l.baseline))){l.waiting=false;if(!l.unknown)l.message='';}paint(c);
  }
  for(const [k,c]of cards)if(!keep.has(k)){c.root.remove();cards.delete(k);local.delete(k);}
  status.dataset.message=snapshot.devices.length?'':'empty';status.textContent=snapshot.devices.length?'':text('empty');
  expire();
 }
 async function refresh(){
  if(!current()||fetching)return;fetching=true;const serial=++fetchSerial,started=performance.now();
  try{const r=await request('devices/runtime');const snapshot=await r.json();if(current()&&serial===fetchSerial)apply(snapshot,started);}
  catch{if(serial===fetchSerial)fail();}finally{fetching=false;}
 }
 async function command(card,on){
  const d=card.data,k=key(d);if(!current()||!fresh(card)||d.pending||local.get(k)?.waiting)return;
  const l={waiting:true,operation:0,baseline:d.operation_id,unknown:false,message:''};local.set(k,l);paint(card);
  try{
   // Explicit identity tuple only. The firmware resolves its private locator
   // and credential; this API cannot accept a browser-supplied target address.
   const r=await request('devices/output',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({device_id:d.device_id,generation:d.generation,channel:d.channel,on})});
   const result=await r.json();if(result.accepted!==true||result.boot_id!==boot||!Number.isInteger(result.operation_id)||result.operation_id<=0)throw Error('invalid acknowledgement');
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
