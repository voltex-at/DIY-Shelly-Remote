/* Passwords and plaintext snapshots exist only in this authenticated HTTPS
 * browser session. No storage/cache/CDN or plaintext-download fallback. */
'use strict';
(()=>{
 const MAX=2097152,HEADER=64,ITERATIONS=600000,MAGIC=new Uint8Array([68,83,82,66,75,80,49,0]);
 function environment(env){const e=env||{crypto:globalThis.crypto,secure:globalThis.isSecureContext===true};if(!e.secure||!e.crypto?.subtle||!e.crypto?.getRandomValues)throw Error('secure');return e.crypto;}
 function bytes(data){if(data instanceof ArrayBuffer)return new Uint8Array(data);if(ArrayBuffer.isView(data))return new Uint8Array(data.buffer,data.byteOffset,data.byteLength);throw Error('file');}
 function passwordBytes(password){
  if(typeof password!=='string'||Array.from(password).length<12)throw Error('password');
  for(const ch of password){const n=ch.codePointAt(0);if(n>=0xd800&&n<=0xdfff)throw Error('password');}
  const p=new TextEncoder().encode(password);if(p.length>256){p.fill(0);throw Error('password');}return p;
 }
 async function keyFor(crypto,password,salt,usage){
  const p=passwordBytes(password);try{const base=await crypto.subtle.importKey('raw',p,'PBKDF2',false,['deriveKey']);return await crypto.subtle.deriveKey({name:'PBKDF2',salt,iterations:ITERATIONS,hash:'SHA-256'},base,{name:'AES-GCM',length:256},false,[usage]);}finally{p.fill(0);}
 }
 function validateBackupHeader(data){
  const b=bytes(data);if(b.length<HEADER+16+24||b.length>MAX+HEADER+16)throw Error('file');
  const v=new DataView(b.buffer,b.byteOffset,b.byteLength);if(!MAGIC.every((x,i)=>b[i]===x)||v.getUint16(8,true)!==1||v.getUint16(10,true)!==HEADER||v.getUint32(12,true)!==ITERATIONS||v.getUint32(44,true)!==b.length-HEADER||b.subarray(48,64).some(x=>x!==0))throw Error('file');
  return {length:b.length-HEADER};
 }
 async function encryptBackup(data,password,env){
  const crypto=environment(env),plain=bytes(data);if(plain.length<24||plain.length>MAX)throw Error('file');
  const header=new Uint8Array(HEADER);header.set(MAGIC);const v=new DataView(header.buffer);v.setUint16(8,1,true);v.setUint16(10,HEADER,true);v.setUint32(12,ITERATIONS,true);crypto.getRandomValues(header.subarray(16,32));crypto.getRandomValues(header.subarray(32,44));v.setUint32(44,plain.length+16,true);
  const key=await keyFor(crypto,password,header.subarray(16,32),'encrypt');const ciphertext=await crypto.subtle.encrypt({name:'AES-GCM',iv:header.subarray(32,44),additionalData:header,tagLength:128},key,plain);
  const out=new Uint8Array(HEADER+ciphertext.byteLength);out.set(header);out.set(new Uint8Array(ciphertext),HEADER);new Uint8Array(ciphertext).fill(0);return out.buffer;
 }
 async function decryptBackup(data,password,env){
  const crypto=environment(env);validateBackupHeader(data);const b=bytes(data),header=b.subarray(0,HEADER),key=await keyFor(crypto,password,header.subarray(16,32),'decrypt');
  try{return await crypto.subtle.decrypt({name:'AES-GCM',iv:header.subarray(32,44),additionalData:header,tagLength:128},key,b.subarray(HEADER));}catch{throw Error('decrypt');}
 }
 const words={
  title:['Kopia i przywracanie','Backup and restore','Sichern und Wiederherstellen'],export:['Utwórz szyfrowaną kopię','Create encrypted backup','Verschlüsselte Sicherung erstellen'],import:['Przywróć kopię','Restore backup','Sicherung wiederherstellen'],reset:['Poproś o pełny reset','Request full reset','Vollständigen Reset anfordern'],
  info:['Pełna kopia zawiera konfigurację, zdjęcia i połączenia. Odtwarzanie działa tylko na tym samym pilocie. Hasło pozostaje w przeglądarce; zachowaj je, bo bez niego nie odzyskasz kopii.','A full backup contains configuration, photos and connections. Restore works only on the same remote. The password stays in your browser; keep it, as the backup cannot be recovered without it.','Die vollständige Sicherung enthält Konfiguration, Fotos und Verbindungen. Wiederherstellung nur auf derselben Fernbedienung. Das Passwort bleibt im Browser; ohne dieses Passwort ist keine Wiederherstellung möglich.'],
  password:['Hasło kopii (min. 12 znaków)','Backup password (at least 12 characters)','Sicherungspasswort (mindestens 12 Zeichen)'],repeat:['Powtórz hasło','Repeat password','Passwort wiederholen'],file:['Zaszyfrowany plik .dsrbak','Encrypted .dsrbak file','Verschlüsselte Datei .dsrbak'],cancel:['Anuluj','Cancel','Abbrechen'],refresh:['Sprawdź stan','Check status','Status prüfen'],
  waiting:['Potwierdź tę operację na ekranie pilota. Zgoda wygasa po 120 s.','Confirm this operation on the remote screen. Consent expires after 120 s.','Diesen Vorgang am Bildschirm der Fernbedienung bestätigen. Die Zustimmung gilt 120 s.'],crypto:['Szyfrowanie lub odszyfrowywanie pliku…','Encrypting or decrypting file…','Datei wird verschlüsselt oder entschlüsselt…'],working:['Pilot przygotowuje i sprawdza dane…','The remote is preparing and checking data…','Die Fernbedienung bereitet Daten vor und prüft sie…'],
  candidate:['Kopia sprawdzona. Potwierdź na LCD odtworzenie danych i zapisanych zgód automatyzacji. Po restarcie sparuj przeglądarkę ponownie.','Backup checked. Confirm restoring data and saved automation approvals on the LCD. Pair the browser again after restart.','Sicherung geprüft. Wiederherstellung der Daten und gespeicherten Automatisierungszustimmungen am LCD bestätigen. Browser nach dem Neustart neu koppeln.'],
  done:['Zaszyfrowana kopia została przygotowana do pobrania.','The encrypted backup is ready to download.','Die verschlüsselte Sicherung steht zum Herunterladen bereit.'],committed:['Operacja zatwierdzona na pilocie. Trwa restart i odtwarzanie danych. Po zakończeniu sparuj panel ponownie.','The operation was committed on the remote. Restart and data recovery are in progress. Pair the panel again when finished.','Vorgang auf der Fernbedienung bestätigt. Neustart und Wiederherstellung laufen. Panel anschließend neu koppeln.'],
  resetinfo:['Reset usuwa wszystkie dane użytkownika pilota i wymaga przytrzymania przycisku na LCD przez 3 s. Urządzenia Shelly zachowują swój stan.','Reset removes all user data from the remote and requires holding the LCD button for 3 s. Shelly devices keep their current state.','Reset löscht alle Benutzerdaten der Fernbedienung und erfordert 3 s Halten am LCD. Shelly-Geräte behalten ihren aktuellen Zustand.'],
  secure:['Wymagane HTTPS i Web Crypto w tej przeglądarce.','HTTPS and Web Crypto are required in this browser.','HTTPS und Web Crypto sind in diesem Browser erforderlich.'],passwordError:['Hasło musi mieć co najmniej 12 znaków, nie więcej niż 256 bajtów; oba hasła muszą być identyczne.','Use at least 12 characters, up to 256 bytes; both passwords must match.','Mindestens 12 Zeichen, höchstens 256 Bytes; beide Passwörter müssen übereinstimmen.'],
  decrypt:['Błędne hasło lub uszkodzony plik. Żadne dane nie zostały wysłane do pilota.','Wrong password or damaged file. No data were sent to the remote.','Falsches Passwort oder beschädigte Datei. Es wurden keine Daten an die Fernbedienung gesendet.'],fileError:['Wybierz poprawny plik kopii do 2 MiB danych.','Select a valid backup file containing up to 2 MiB of data.','Gültige Sicherungsdatei mit bis zu 2 MiB Daten auswählen.'],expired:['Operacja anulowana, zgoda wygasła lub dane pilota się zmieniły. Rozpocznij ponownie.','The operation was cancelled, consent expired or remote data changed. Start again.','Vorgang abgebrochen, Zustimmung abgelaufen oder Daten geändert. Erneut starten.'],unavailable:['Nie potwierdzono wyniku. Sprawdź ekran pilota przed ponowieniem.','The result is unconfirmed. Check the remote screen before retrying.','Ergebnis unbestätigt. Vor erneutem Versuch den Bildschirm der Fernbedienung prüfen.'],low:['Bateria poniżej 20%. Doładuj pilot przed odtwarzaniem lub resetem.','Battery below 20%. Recharge before restoring or resetting.','Akku unter 20 %. Vor Wiederherstellung oder Reset aufladen.']
 };
 function createBackupPanel({request,translate,onNotice=()=>{}}){
  const tr=k=>translate?translate(words[k]||[k,k,k]):(words[k]||[k,k,k])[document.documentElement.lang==='de'?2:document.documentElement.lang==='en'?1:0];
  const root=document.createElement('section');root.className='backup-panel';const bindings=[];
  const el=(tag,key,cls)=>{const n=document.createElement(tag);if(key){bindings.push([n,key]);n.textContent=tr(key);}if(cls)n.className=cls;return n;};
  const title=el('h2','title'),info=el('p','info','muted'),status=el('p',null,'automation-warning');status.hidden=true;status.setAttribute('role','status');status.setAttribute('aria-live','polite');
  const progress=el('progress');progress.hidden=true;progress.setAttribute('aria-label',tr('working'));
  const form=el('div',null,'backup-fields');
  const input=(key,type)=>{const label=el('label',key),n=el('input');n.type=type;if(type==='password'){n.autocomplete='new-password';n.spellcheck=false;}label.append(n);form.append(label);return n;};
  const pass=input('password','password'),repeat=input('repeat','password'),file=input('file','file');file.accept='.dsrbak';
  const actions=el('div',null,'actions');let disposed=false,busy=false,serial=0,controller=null,owned=null,message='',buffers=[];
  const alive=s=>!disposed&&s===serial&&!controller?.signal.aborted;
  const clearBuffers=()=>{for(const b of buffers)bytes(b).fill(0);buffers=[];pass.value=repeat.value='';};
  const show=key=>{message=key;status.hidden=!key;status.textContent=tr(key);onNotice('');};
  const refreshLocale=()=>{for(const [n,k]of bindings)n.firstChild?n.firstChild.nodeValue=tr(k):n.textContent=tr(k);if(message)status.textContent=tr(message);};
  const update=()=>{for(const b of [exportButton,importButton,resetButton])b.disabled=busy||!!owned;pass.disabled=repeat.disabled=file.disabled=busy;cancelButton.disabled=!busy&&!owned;progress.hidden=!busy;};
  const post=(path,body,signal=controller?.signal)=>request(path,{method:'POST',headers:body===undefined?{}:{'Content-Type':'application/json'},body:body===undefined?undefined:JSON.stringify(body),signal});
  const headers=id=>({'Content-Type':'application/octet-stream','X-DSR-Maintenance':id});
  const cancelOwned=id=>{if(id)request('maintenance/cancel',{method:'POST',headers:headers(id)}).catch(()=>{});};
  async function statusRead(s){const result=await request('maintenance/status',{signal:controller?.signal});const value=await result.json();if(!alive(s))throw Error('aborted');return value;}
  async function waitGrant(s,id){
   const began=performance.now();while(alive(s)&&performance.now()-began<120000){const value=await statusRead(s);if(value.request_id===id&&value.owned===true){if(value.phase===11)return value;if(value.phase===8||value.phase===9)throw Error('expired');}await new Promise(resolve=>setTimeout(resolve,500));}throw Error('expired');
  }
  async function begin(op,s){show('waiting');const response=await post(op==='reset'?'reset/request':'backup/request',op==='reset'?undefined:{op});const value=await response.json();if(!alive(s)){cancelOwned(value.request_id);throw Error('aborted');}if(!/^[0-9a-f]{32}$/.test(value.request_id))throw Error('unavailable');owned=value.request_id;return owned;}
  async function perform(kind){
   if(disposed||busy)return;busy=true;serial++;const s=serial;controller=new AbortController();const ownBuffers=[];buffers=ownBuffers;update();
   try{
    if(kind==='reset'){await begin(kind,s);return;}
    environment();const password=pass.value;if(kind==='export'){passwordBytes(password).fill(0);if(password!==repeat.value)throw Error('password');const id=await begin(kind,s);await waitGrant(s,id);show('working');
     const response=await request('backup/export',{method:'POST',headers:headers(id),signal:controller.signal});const size=Number(response.headers.get('X-DSR-Backup-Bytes')||response.headers.get('Content-Length'));if(!Number.isInteger(size)||size<24||size>MAX)throw Error('file');
     const reader=response.body?.getReader();if(!reader)throw Error('file');const chunks=[];let received=0;let raw;
     try{while(true){const {done,value}=await reader.read();if(done)break;received+=value.byteLength;if(received>size||!alive(s)){value.fill(0);throw Error('file');}chunks.push(value);}if(received!==size)throw Error('file');raw=new ArrayBuffer(size);const target=new Uint8Array(raw);let at=0;for(const chunk of chunks){target.set(chunk,at);at+=chunk.length;}}
     finally{await reader.cancel().catch(()=>{});for(const chunk of chunks)chunk.fill(0);}
     ownBuffers.push(raw);if(!alive(s))return;show('crypto');const encrypted=await encryptBackup(raw,password);ownBuffers.push(encrypted);if(!alive(s))return;
     const blob=new Blob([encrypted],{type:'application/octet-stream'}),url=URL.createObjectURL(blob),anchor=document.createElement('a');anchor.href=url;anchor.download='DIY-Shelly-Remote.dsrbak';anchor.click();setTimeout(()=>URL.revokeObjectURL(url),0);owned=null;show('done');
    }else{
     const selected=file.files[0];if(!selected||selected.size<104||selected.size>MAX+80)throw Error('file');const encrypted=await selected.arrayBuffer();ownBuffers.push(encrypted);if(!alive(s))return;show('crypto');const raw=await decryptBackup(encrypted,password);ownBuffers.push(raw);if(!alive(s))return;const id=await begin('upload',s);await waitGrant(s,id);show('working');
     const response=await request('backup/import',{method:'POST',headers:headers(id),body:raw,signal:controller.signal});const value=await response.json();if(!alive(s))return;if(!value.candidate||!/^[0-9a-f]{32}$/.test(value.request_id))throw Error('unavailable');owned=value.request_id;show('candidate');
    }
   }catch(e){if(alive(s)){show(({secure:'secure',password:'passwordError',file:'fileError',decrypt:'decrypt',expired:'expired'})[e.message]||'unavailable');cancelOwned(owned);owned=null;}}
   finally{for(const buffer of ownBuffers)bytes(buffer).fill(0);if(s===serial){clearBuffers();busy=false;update();}}
  }
  const button=(key,fn,cls)=>{const n=el('button',key,cls);n.type='button';n.addEventListener('click',fn);return n;};
  const exportButton=button('export',()=>perform('export'),'primary'),importButton=button('import',()=>perform('import'),'primary'),resetButton=button('reset',()=>perform('reset'));
  const cancelButton=button('cancel',()=>{const id=owned;owned=null;serial++;controller?.abort();clearBuffers();busy=false;show('expired');update();cancelOwned(id);});
  const checkButton=button('refresh',async()=>{if(disposed)return;try{const response=await request('maintenance/status'),value=await response.json();if(disposed)return;if(value.owned&&value.request_id===owned){if(value.phase===6){owned=null;show('committed');}else if(value.result===5)show('low');else if(value.phase===9){owned=null;show('expired');}else show(value.op===3?'candidate':'waiting');update();}}catch{if(!disposed)show('unavailable');}});
  actions.append(exportButton,importButton,checkButton,cancelButton);root.append(title,info,form,progress,status,actions,el('p','resetinfo','automation-warning'),resetButton);update();
  return {root,refreshLocale,dispose(){if(disposed)return;disposed=true;serial++;controller?.abort();clearBuffers();const id=owned;owned=null;cancelOwned(id);}};
 }
 const api={encryptBackup,decryptBackup,validateBackupHeader,createBackupPanel};if(typeof module!=='undefined'&&module.exports)module.exports=api;else Object.assign(globalThis,api);
})();
