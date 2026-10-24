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
  color:['Kolor RGB','RGB color','RGB-Farbe'],
  applyColor:['Zastosuj kolor','Apply color','Farbe anwenden'],
  colorPreview:['Podgląd koloru • bez wysyłania','Color preview • not sent','Farbvorschau • nicht gesendet'],
  colorConfirmed:['Potwierdzony RGB: {color}','Confirmed RGB: {color}','Bestätigtes RGB: {color}'],
  rgbTransition:['Przejście • poczekaj na odczyt','Transition • wait for readback','Übergang • Rückmeldung abwarten'],
  rgbError:['Sterowanie RGB zablokowane','RGB controls blocked','RGB-Steuerung gesperrt'],
  brightness:['Jasność potwierdzona: {position}%','Confirmed brightness: {position}%','Bestätigte Helligkeit: {position}%'],
  noBrightness:['Jasność niedostępna','Brightness unavailable','Helligkeit nicht verfügbar'],
  lightError:['Sterowanie Light zablokowane: {code}','Light controls blocked: {code}','Light-Steuerung gesperrt: {code}'],
  transition:['Przejście do {position}%','Transition to {position}%','Übergang zu {position}%'],
  zeroBrightness:['0% jasności nie oznacza OFF','0% brightness does not mean OFF','0% Helligkeit bedeutet nicht AUS'],
  refresh:['Odśwież status','Refresh status','Status aktualisieren'],
  sensor:['H&T Gen3 • tylko odczyt','H&T Gen3 • read only','H&T Gen3 • nur lesen'],
  sensorNote:['Czas pomiaru nieznany. Czujnik usypia także na USB. Wybudź przyciskiem, jeśli nie odpowiada.','Measurement time unknown. The sensor sleeps even on USB. Wake it with its button if it does not respond.','Messzeit unbekannt. Der Sensor schläft auch mit USB. Bei fehlender Antwort per Taste wecken.'],
  responseAge:['Odpowiedź odebrana {seconds} s temu','Response received {seconds} s ago','Antwort vor {seconds} s empfangen'],
  sensorMissing:['Brak odpowiedzi • możliwe uśpienie','No response • may be asleep','Keine Antwort • Schlafmodus möglich'],
  external:['Zasilanie zewnętrzne','External power','Externe Versorgung'],
  yes:['tak','yes','ja'],no:['nie','no','nein'],
  sensorError:['Błędy pomiarów: {code}','Measurement errors: {code}','Messfehler: {code}'],
  coverError:['Błąd rolety: {code}','Cover error: {code}','Rollladenfehler: {code}'],
  batteryShort:['baterii','battery','Batterie'],
  trv:['Termostat BLU TRV','BLU TRV thermostat','BLU-TRV-Thermostat'],
  trvTarget:['cel','target','Ziel'],valve:['zawór','valve','Ventil'],
  trvFault:['Termostat zgłasza błąd (np. brak kalibracji)','Thermostat reports an error (e.g. not calibrated)','Thermostat meldet einen Fehler (z. B. nicht kalibriert)'],
  bluSensor:['Czujnik BLU • przez bramkę','BLU sensor • via gateway','BLU-Sensor • über Gateway'],
  bluMissing:['Bramka nie odpowiada','Gateway not responding','Gateway antwortet nicht'],
  bluNoData:['Bramka nie zna jeszcze danych','Gateway has no data yet','Gateway hat noch keine Daten'],
  motion:['Ruch wykryty','Motion detected','Bewegung erkannt'],noMotion:['Brak ruchu','No motion','Keine Bewegung'],
  openState:['Otwarte','Open','Offen'],closedState:['Zamknięte','Closed','Geschlossen'],
  bluPacket:['Ostatni pakiet BLU: {time}','Last BLU packet: {time}','Letztes BLU-Paket: {time}'],
  bluError:['Błąd odczytu BLU (klucz/szyfrowanie): {code}','BLU read error (key/encryption): {code}','BLU-Lesefehler (Schlüssel/Verschlüsselung): {code}'],
  bluNote:['Tylko odczyt. Bramka Shelly odbiera pakiety BLU; pilot odpytuje ją co 15 s.','Read only. The Shelly gateway receives BLU packets; the remote polls it every 15 s.','Nur lesen. Das Shelly-Gateway empfängt BLU-Pakete; die Fernbedienung fragt es alle 15 s ab.'],
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
 function sensorValid(d){return d.channel===0&&d.output===null&&d.has_switch===false&&d.has_power===false&&typeof d.can_refresh==='boolean'&&typeof d.has_sample_time==='boolean'&&d.measurement_time_known===false&&['temperature','humidity','battery'].every((n,i)=>typeof d['has_'+n]==='boolean'&&(d['has_'+n]?Number.isFinite(d[['temperature_c','humidity_pct','battery_pct'][i]]):d[['temperature_c','humidity_pct','battery_pct'][i]]===null))&&(d.external_present===null||typeof d.external_present==='boolean')&&Number.isInteger(d.sensor_errors)&&d.sensor_errors>=0&&d.sensor_errors<=7;}
 const nullBool=v=>v===null||typeof v==='boolean',nullNumber=v=>v===null||Number.isFinite(v);
 function bluValid(d){const b=d.blu;return d.output===null&&d.has_switch===false&&d.has_power===false&&!!b&&typeof b==='object'&&typeof b.valid==='boolean'&&nullBool(b.motion)&&nullBool(b.open)&&['illuminance_lux','battery_pct','temperature_c','humidity_pct','button','rssi','updated_s'].every(n=>nullNumber(b[n]))&&Number.isInteger(b.errors)&&b.errors>=0&&b.errors<=15&&(b.trv===undefined||b.trv===null||(typeof b.trv==='object'&&['target_c','current_c','valve_pct'].every(n=>nullNumber(b.trv[n]))&&typeof b.trv.fault==='boolean'));}
 function valid(d){return d&&[0,1,2,3,4,5].includes(kind(d))&&typeof d.device_id==='string'&&d.device_id.length>0&&d.device_id.length<64&&Number.isInteger(d.generation)&&[1,2].includes(d.generation)&&Number.isInteger(d.channel)&&d.channel>=0&&d.channel<16&&typeof d.name==='string'&&typeof d.host==='string'&&typeof d.online==='boolean'&&(kind(d)>=4?d.output===null:typeof d.output==='boolean')&&typeof d.has_switch==='boolean'&&typeof d.pending==='boolean'&&Number.isInteger(d.operation_id)&&d.operation_id>=0&&d.operation_id<=0xffffffff&&Number.isInteger(d.error)&&Number.isFinite(d.sample_age_ms)&&d.sample_age_ms>=0&&(kind(d)===0||(d.generation===2&&(kind(d)===4?sensorValid(d):kind(d)===5?bluValid(d):kind(d)===3?d.channel===0&&typeof d.has_rgb==='boolean'&&Array.isArray(d.rgb)&&d.rgb.length===3&&d.rgb.every(v=>Number.isInteger(v)&&v>=0&&v<=255)&&Number.isInteger(d.brightness)&&d.brightness>=0&&d.brightness<=100&&typeof d.can_rgb==='boolean'&&typeof d.transitioning==='boolean'&&typeof d.rgb_error==='boolean':kind(d)===2?typeof d.has_brightness==='boolean'&&Number.isInteger(d.brightness)&&d.brightness>=0&&d.brightness<=100&&typeof d.can_light==='boolean'&&typeof d.transitioning==='boolean'&&typeof d.has_brightness_target==='boolean'&&Number.isInteger(d.brightness_target)&&d.brightness_target>=0&&d.brightness_target<=100&&Number.isInteger(d.light_errors)&&d.light_errors>=0&&d.light_errors<=255&&Number.isInteger(d.light_conditions)&&d.light_conditions>=0&&d.light_conditions<=255:typeof d.has_position==='boolean'&&Number.isInteger(d.position)&&d.position>=0&&d.position<=100&&Number.isInteger(d.cover_state)&&d.cover_state>=0&&d.cover_state<=6&&typeof d.can_move==='boolean'&&typeof d.can_position==='boolean'&&typeof d.can_stop==='boolean'&&typeof d.stop_pending==='boolean'&&Number.isInteger(d.cover_errors)&&d.cover_errors>=0&&d.cover_errors<=255)));}
 const colorHex=d=>'#'+d.rgb.map(v=>v.toString(16).padStart(2,'0')).join('');
 const canRgb=d=>d.has_rgb&&d.can_rgb&&!d.rgb_error&&!d.transitioning;
 const canLight=d=>d.can_light&&d.has_brightness&&!d.light_errors&&!d.light_conditions;
 const age=card=>card.data.sample_age_ms+Math.max(0,performance.now()-card.received);
 function fresh(card){return card.data.online&&age(card)<=15000;}
 function cancelRange(card){if(card.range){card.dragging=false;card.rangeCanceled=true;}card.colorDirty=false;}
 function paint(card){
  const d=card.data,l=local.get(key(d));
  const title=d.name;card.name.textContent=title;
  card.identity.textContent=[d.model,d.firmware,d.device_id].filter(Boolean).join(' · ');
  if(Number.isInteger(d.layout_id))for(const node of document.querySelectorAll('[data-device-name-id]'))if(node.dataset.deviceNameId===String(d.layout_id))node.textContent=title;
  card.address.textContent=text('channel',d);
  const stop=stops.get(key(d)),waiting=d.pending||!!l?.waiting||!!stop?.waiting,sensor=kind(d)===4,cover=kind(d)===1,light=kind(d)===2,rgb=kind(d)===3;
  if(kind(d)===5){
   const b=d.blu,known=d.online&&b.valid,unit=(v,u,digits=1)=>`${v.toLocaleString(document.documentElement.lang,{maximumFractionDigits:digits})} ${u}`;
   card.state.textContent=text(!d.online?'bluMissing':!b.valid?'bluNoData':b.trv?'trv':b.motion!==null?(b.motion?'motion':'noMotion'):b.open!==null?(b.open?'openState':'closedState'):'bluSensor');
   card.measure.textContent=known&&b.trv?[[b.trv.current_c,'°C'],[b.trv.target_c,'°C '+text('trvTarget')],[b.trv.valve_pct,'% '+text('valve'),0],[b.battery_pct,'% '+text('batteryShort'),0],[b.rssi,'dBm',0]].filter(([v])=>v!==null).map(([v,u,digits])=>unit(v,u,digits)).join(' · '):known?[[b.illuminance_lux,'lx',0],[b.temperature_c,'°C'],[b.humidity_pct,'% RH',0],[b.battery_pct,'% '+text('batteryShort'),0],[b.rssi,'dBm',0]].filter(([v])=>v!==null).map(([v,u,digits])=>unit(v,u,digits)).join(' · '):'';
   card.age.textContent=known&&b.updated_s?text('bluPacket',{time:new Date(b.updated_s*1000).toLocaleString(document.documentElement.lang)}):d.has_sample_time?text('responseAge',{seconds:Math.floor(age(card)/1000)}):text('unknown');
   card.error.textContent=d.error?text('error',{code:d.error}):b.errors?text('bluError',{code:b.errors}):b.trv&&b.trv.fault?text('trvFault'):'';
   card.on.hidden=card.off.hidden=true;card.note.textContent=text('bluNote');return;
  }
  if(kind(d)===4){
   card.state.textContent=text(waiting?'pending':d.online?'sensor':'sensorMissing');
   card.age.textContent=d.has_sample_time?text('responseAge',{seconds:Math.floor(age(card)/1000)}):text('unknown');
   card.measure.textContent=[['temperature','temperature_c','°C'],['humidity','humidity_pct','% RH'],['battery','battery_pct','%']].map(([has,value,unit])=>`${d['has_'+has]?d[value].toLocaleString(document.documentElement.lang,{maximumFractionDigits:2}):'—'} ${unit}`).join(' · ')+` · ${text('external')}: ${d.external_present===null?'—':text(d.external_present?'yes':'no')}`;
   card.error.textContent=l?.message?text(l.message):d.error?text('error',{code:d.error}):d.sensor_errors?text('sensorError',{code:d.sensor_errors}):'';
   card.on.textContent=text('refresh');card.on.disabled=!d.can_refresh||waiting;card.on.setAttribute('aria-label',`${text('refresh')}: ${d.name}`);card.off.hidden=true;card.note.textContent=text('sensorNote');return;
  }
  card.state.textContent=text(waiting?'pending':fresh(card)?(cover?(['unknown','opened','closed','opening','closing','stopped','calibrating'][d.cover_state]):(d.output?'on':'off')):'offline');
  card.age.textContent=fresh(card)?text('age',{seconds:Math.floor(age(card)/1000)}):text('unknown');
  card.measure.textContent=fresh(card)?[['has_power','power_w','W'],['has_voltage','voltage_v','V'],['has_current','current_a','A'],['has_energy','energy_wh','Wh']].filter(([has,value])=>d[has]===true&&Number.isFinite(d[value])).map(([,value,unit])=>`${d[value].toLocaleString(document.documentElement.lang,{maximumFractionDigits:2})} ${unit}`).join(' · '):'';
  card.error.textContent=stop?.message?text(stop.message):l?.message?text(l.message):d.error?text('error',{code:d.error}):cover&&d.cover_errors?text('coverError',{code:d.cover_errors}):light&&(d.light_errors||d.light_conditions)?text('lightError',{code:d.light_errors||d.light_conditions}):rgb&&d.rgb_error?text('rgbError'):'';
  for(const [button,on]of [[card.on,true],[card.off,false]]){const label=cover?(on?'open':'close'):(on?'turnOn':'turnOff');button.textContent=text(label);button.disabled=!fresh(card)||waiting||(cover?!d.can_move:light?!canLight(d):rgb?!canRgb(d):sensor?false:!d.has_switch);button.setAttribute('aria-label',`${text(label)}: ${d.name}`);}
  if(cover){card.stop.textContent=text('stop');card.stop.disabled=!d.can_stop||d.stop_pending||!!stop?.waiting;card.stop.setAttribute('aria-label',`${text('stop')}: ${d.name}`);
   card.range.disabled=!fresh(card)||waiting||!d.can_position;if(card.range.disabled)cancelRange(card);card.range.setAttribute('aria-label',`${text('position',{position:d.has_position?d.position:'?'})}: ${d.name}`);if(!card.dragging)card.range.value=String(d.has_position?d.position:0);
   const confirmed=fresh(card)&&d.has_position?text('position',{position:d.position}):text('noPosition');card.preview.textContent=confirmed+(card.dragging?` · ${text('preview',{position:card.range.value})}`:'');
  }
  if(light||rgb){
   card.range.disabled=!fresh(card)||waiting||(rgb?!canRgb(d):!canLight(d));if(card.range.disabled)cancelRange(card);card.range.setAttribute('aria-label',`${text('brightness',{position:(rgb?d.has_rgb:d.has_brightness)?d.brightness:'?'})}: ${d.name}`);if(!card.dragging)card.range.value=String((rgb?d.has_rgb:d.has_brightness)?d.brightness:0);
   const confirmed=fresh(card)&&(rgb?d.has_rgb:d.has_brightness)?text('brightness',{position:d.brightness}):text('noBrightness');card.preview.textContent=confirmed+(card.dragging?` · ${text('preview',{position:card.range.value})}`:d.transitioning?` · ${rgb?text('rgbTransition'):text('transition',{position:d.brightness_target})}`:` · ${text('zeroBrightness')}`);
  }
  if(rgb){
   card.colorToggle.textContent=text('color');card.colorPanel.hidden=!card.colorView;
   card.color.disabled=!fresh(card)||waiting||!canRgb(d);if(card.color.disabled)card.colorDirty=false;
   if(!card.colorDirty)card.color.value=colorHex(d);
   card.color.setAttribute('aria-label',`${text('color')}: ${d.name}`);
   card.colorStatus.textContent=card.colorDirty?text('colorPreview'):fresh(card)&&d.has_rgb?text('colorConfirmed',{color:colorHex(d)}):text('unknown');
   card.colorApply.textContent=text('applyColor');card.colorApply.disabled=card.color.disabled||!card.colorDirty;
  }
 }
 function expire(){
  clearTimeout(expiryTimer);expiryTimer=null;if(!current())return;
  let delay=Infinity;for(const card of cards.values()){paint(card);if(kind(card.data)===4&&card.data.has_sample_time)delay=Math.min(delay,1000);if(fresh(card))delay=Math.min(delay,15001-age(card));}
  if(Number.isFinite(delay))expiryTimer=setTimeout(expire,Math.max(1,delay));
 }
 function translate(){if(!current())return;section.querySelector('h2').textContent=text('title');if(status.dataset.message)status.textContent=text(status.dataset.message);for(const card of cards.values())paint(card);}
 function fail(){if(!current())return;status.dataset.message='unavailable';status.textContent=text('unavailable');for(const card of cards.values()){card.data={...card.data,online:false,can_stop:false,can_refresh:false};paint(card);}}
 function create(d){
  const root=node('article','runtime-card'),head=node('div','runtime-heading'),name=node('h3'),identity=node('p','muted'),address=node('p','muted'),state=node('p'),measure=node('p'),age=node('p','muted'),error=node('p','runtime-error'),actions=node('div','actions'),on=node('button'),off=node('button');
  on.type=off.type='button';error.setAttribute('role','status');head.append(name,identity,address);actions.append(on,off);root.append(head,state,measure,age,error,actions);
  const card={root,name,identity,address,state,measure,age,error,on,off,data:d,received:performance.now(),rangeCanceled:true};
  if(kind(d)>=4){card.note=node('p','muted');root.append(card.note);}
  if([1,2,3].includes(kind(d))){const light=kind(d)===2,rgb=kind(d)===3;if(!light&&!rgb){card.stop=node('button');card.stop.type='button';actions.append(card.stop);card.stop.addEventListener('click',()=>command(card,false,{command:'stop'}));}card.range=node('input');card.range.type='range';card.range.min='0';card.range.max='100';card.range.step='1';card.preview=node('p','muted');actions.append(card.range);root.append(card.preview);
   card.range.addEventListener('input',()=>{if(card.range.disabled)return;card.rangeCanceled=false;card.dragging=true;paint(card);});
   card.range.addEventListener('change',()=>{const position=Number(card.range.value),gesture=card.dragging&&!card.rangeCanceled;card.dragging=false;card.rangeCanceled=true;if(!gesture){paint(card);return;}return command(card,false,(light||rgb)?{command:'brightness',brightness:position}:{command:'position',position});});
   card.range.addEventListener('pointercancel',()=>{card.dragging=false;card.rangeCanceled=true;paint(card);});
  }
  if(kind(d)===3){
   card.colorToggle=node('button');card.colorToggle.type='button';card.colorPanel=node('div','rgb-color-view');card.color=node('input');card.color.type='color';card.colorStatus=node('p','muted');card.colorApply=node('button');card.colorApply.type='button';
   card.colorPanel.append(card.color,card.colorStatus,card.colorApply);root.append(card.colorToggle,card.colorPanel);
   card.colorToggle.addEventListener('click',()=>{card.colorView=!card.colorView;paint(card);});
   card.color.addEventListener('input',()=>{if(card.color.disabled)return;card.colorDirty=true;paint(card);});
   card.color.addEventListener('pointercancel',()=>{card.colorDirty=false;paint(card);});
   card.colorApply.addEventListener('click',()=>{if(!card.colorDirty||card.color.disabled||!/^#[0-9a-f]{6}$/i.test(card.color.value))return;const rgb=[1,3,5].map(i=>parseInt(card.color.value.slice(i,i+2),16));card.colorDirty=false;return command(card,false,{command:'rgb',rgb});});
  }
  on.addEventListener('click',()=>command(card,true));off.addEventListener('click',()=>command(card,false));return card;
 }
 function apply(snapshot,started,serial){
  if(!Number.isInteger(snapshot.boot_id)||snapshot.boot_id<=0||snapshot.boot_id>0xffffffff||!Array.isArray(snapshot.devices)||snapshot.devices.length>16||snapshot.devices.some(d=>!valid(d))||new Set(snapshot.devices.map(key)).size!==snapshot.devices.length){fail();return;}
  // A firmware restart ends the old command epoch. Read the new process state;
  // it is no acknowledgement that a command in the previous process succeeded.
  if(boot!==null&&boot!==snapshot.boot_id){local.clear();stops.clear();for(const c of cards.values())cancelRange(c);}boot=snapshot.boot_id;
  const keep=new Set();
  for(const d of snapshot.devices){const k=key(d);keep.add(k);let c=cards.get(k);if(!c){c=create(d);cards.set(k,c);holder.append(c.root);}else if(c.data.host!==d.host||c.data.has_target!==d.has_target||c.data.target!==d.target||c.data.brightness_target!==d.brightness_target||c.data.transitioning!==d.transitioning||(kind(d)===3&&(c.data.brightness!==d.brightness||c.data.rgb.some((v,i)=>v!==d.rgb[i]))))cancelRange(c);c.data=d;c.received=started;
   for(const map of [local,stops]){const l=map.get(k);if(l?.waiting&&!d.pending&&((l.operation>0&&serialReached(d.operation_id,l.operation))||(l.unknown&&serial>l.afterSerial&&serialReached(d.operation_id,l.baseline)))){l.waiting=false;if(!l.unknown)l.message='';}}paint(c);
  }
  for(const [k,c]of cards)if(!keep.has(k)){c.root.remove();cards.delete(k);local.delete(k);stops.delete(k);}
  status.dataset.message=snapshot.devices.length?'':'empty';status.textContent=snapshot.devices.length?'':text('empty');
  expire();
 }
 async function refresh(){
  if(!current()||fetching)return;fetching=true;const serial=++fetchSerial,started=performance.now();
  try{const r=await request('devices/runtime');const snapshot=await r.json();if(current()&&serial===fetchSerial)apply(snapshot,started,serial);}
  catch{if(serial===fetchSerial)fail();}finally{fetching=false;}
 }
 async function command(card,on,intent){
  const d=card.data,k=key(d),cover=kind(d)===1,light=kind(d)===2,rgb=kind(d)===3,sensor=kind(d)===4,stop=cover&&intent?.command==='stop';
  if(!current()||!card.root.isConnected||(stop?(!d.can_stop||d.stop_pending||stops.get(k)?.waiting):((sensor?!d.can_refresh:!fresh(card))||d.pending||local.get(k)?.waiting||stops.get(k)?.waiting||(cover?!(intent?.command==='position'?d.can_position:d.can_move):light?!canLight(d):rgb?!canRgb(d):sensor?false:!d.has_switch))))return;
  if(cover&&!stop&&!intent)intent={command:on?'open':'close'};
  if((light||rgb)&&!intent)intent={command:on?'on':'off'};
  if(light&&(!['on','off','brightness'].includes(intent.command)||(intent.command==='brightness'&&(!Number.isInteger(intent.brightness)||intent.brightness<0||intent.brightness>100))))return;
  if(rgb&&(!['on','off','brightness','rgb'].includes(intent.command)||(intent.command==='brightness'&&(!Number.isInteger(intent.brightness)||intent.brightness<0||intent.brightness>100))||(intent.command==='rgb'&&(!Array.isArray(intent.rgb)||intent.rgb.length!==3||intent.rgb.some(v=>!Number.isInteger(v)||v<0||v>255)))))return;
  const l={waiting:true,operation:0,baseline:d.operation_id,unknown:false,message:''};(stop?stops:local).set(k,l);paint(card);
  try{
   // Explicit identity tuple only. The firmware resolves its private locator
   // and credential; this API cannot accept a browser-supplied target address.
   const body={device_id:d.device_id,generation:d.generation,channel:d.channel};if(sensor)body.endpoint_kind=4;else if(cover||light||rgb)Object.assign(body,{endpoint_kind:cover?1:rgb?3:2},intent);else body.on=on;
   const r=await request(sensor?'devices/refresh':cover?'devices/cover':light?'devices/light':rgb?'devices/rgb':'devices/output',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
   const result=await r.json();if(result.accepted!==true||result.boot_id!==boot||!Number.isInteger(result.operation_id)||result.operation_id<=0||result.operation_id>0xffffffff)throw Error('invalid acknowledgement');
   l.operation=result.operation_id;await refresh();
  }catch(e){
   // A transport failure may follow acceptance. Never replay POST. Fetch only
   // readback, keep a visible uncertainty message until an explicit next try.
   if([400,401,403,404,409,422,429].includes(e.httpStatus)){l.waiting=false;l.message='rejected';}
   else{l.unknown=true;l.afterSerial=fetchSerial;l.message='uncertain';}
   await refresh();
  }finally{if(card.root.isConnected)paint(card);}
 }
 window.dsrRuntimeMount=parent=>{
  ++fetchSerial;cards.clear();section=node('section','box runtime-section');const title=node('h2');status=node('p','muted');status.dataset.message='loading';holder=node('div','runtime-grid');section.append(title,status,holder);parent.prepend(section);
  // render() constructs its view off-DOM. Run after the view has been attached.
  queueMicrotask(()=>{translate();refresh();});
 };
 window.dsrRuntimeRefresh=refresh;
 new MutationObserver(translate).observe(document.documentElement,{attributes:true,attributeFilter:['lang']});
 window.dsrRuntimeDeviceName=id=>{for(const card of cards.values())if(card.data.layout_id===id)return card.data.name;return '';};
 // panel.js may finish its initial config fetch before this deferred asset.
 if(config&&page==='devices')window.dsrRuntimeMount(document.querySelector('#editor').firstElementChild);
})();
