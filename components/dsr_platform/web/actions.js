/* One shared RAM actor. This client never replays a command after an unknown
 * response, timeout, reconnect or boot change. Refresh/status are read-only. */
'use strict';
(()=>{
 const words={title:['Wykonanie i aktywność','Execution and activity','Ausführung und Aktivität'],groups:['Grupy · cały dom','Groups · whole home','Gruppen · ganzes Zuhause'],scenes:['Sceny · cały dom','Scenes · whole home','Szenen · ganzes Zuhause'],activity:['Aktywność · bieżące uruchomienie','Activity · current boot','Aktivität · aktueller Start'],sequential:['Polecenia wykonywane kolejno. Wynik może być częściowy.','Commands run sequentially. Results may be partial.','Befehle laufen nacheinander. Teilergebnisse sind möglich.'],dirty:['Najpierw zapisz zmiany albo wczytaj konfigurację.','Save changes or reload configuration first.','Änderungen zuerst speichern oder Konfiguration neu laden.'],run:['Uruchom scenę','Run scene','Szene starten'],on:['Włącz grupę','Turn group on','Gruppe einschalten'],off:['Wyłącz grupę','Turn group off','Gruppe ausschalten'],refresh:['Sprawdź wynik','Check result','Ergebnis prüfen'],cancel:['Anuluj pozostałe kroki','Cancel remaining steps','Übrige Schritte abbrechen'],clear:['Wyczyść dziennik RAM','Clear RAM activity log','RAM-Aktivität löschen'],newer:['Nowsze','Newer','Neuere'],older:['Starsze','Older','Ältere'],empty:['Brak wpisów.','No entries.','Keine Einträge.'],missing:['Brak połączenia lub wykonawca niedostępny.','Disconnected or executor unavailable.','Keine Verbindung oder Ausführung nicht verfügbar.'],unknown:['Nie potwierdzono przyjęcia. Sprawdzaj wynik; polecenie nie będzie ponowione.','Admission is unconfirmed. Check the result; the command will not be replayed.','Annahme unbestätigt. Ergebnis prüfen; Befehl wird nicht wiederholt.'],restarted:['Pilot uruchomiono ponownie. Żadne polecenie nie jest wznawiane.','The remote restarted. No command is resumed.','Fernbedienung neu gestartet. Kein Befehl wird fortgesetzt.'],waiting:['Oczekuje','Waiting','Wartet'],success:['Potwierdzono','Confirmed','Bestätigt'],failed:['Błąd kroku','Step failed','Schritt fehlgeschlagen'],uncertain:['Wynik niepewny · sprawdź stan','Uncertain result · check state','Ergebnis unsicher · Status prüfen'],cancelled:['Anulowano','Cancelled','Abgebrochen'],unset:['Brak UTC','UTC unset','UTC fehlt'],active:['Wykonywanie','Running','Wird ausgeführt'],done:['Zakończono','Completed','Abgeschlossen'],admission:['Uruchomienie','Run','Ausführung'],state:['Zmiana stanu','State change','Statusänderung'],observed:['Odczyt stanu','State observation','Statusablesung'],observationError:['Błąd lub brak odpowiedzi','Error or no response','Fehler oder keine Antwort'],onCmd:['Włącz','On','Ein'],offCmd:['Wyłącz','Off','Aus'],open:['Otwórz','Open','Öffnen'],close:['Zamknij','Close','Schließen'],stop:['Stop','Stop','Stopp']};
 words.edit=['Edytuj','Edit','Bearbeiten'];words.whole=['Cały dom','Whole home','Ganzes Zuhause'];
 const tr=k=>words[k]?.[document.documentElement.lang==='en'?1:document.documentElement.lang==='de'?2:0]||k;
 const el=(tag,text,cls)=>{const n=document.createElement(tag);if(text!==undefined)n.textContent=text;if(cls)n.className=cls;return n;};
 const button=(label,fn)=>{const n=el('button',tr(label));n.type='button';n.addEventListener('click',fn);return n;};
 let query='',roomFilter=0,root=null,mode='scenes',status=null,published=null,message='',fetching=false,sending=false,serial=0,readSerial=0,offset=0,epoch=0,log=null,pending=null,unknown=false;
 const current=()=>root?.isConnected&&!document.querySelector('#workspace').hidden&&['groups','scenes','activity','dashboards'].includes(page);
 const json=async(path,body)=>{const response=await request(path,body===undefined?{}:{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});return response.json();};
 const valid=s=>s&&Number.isInteger(s.boot_id)&&s.boot_id>0&&Number.isInteger(s.admission_generation)&&s.admission_generation>=0&&Number.isInteger(s.revision)&&s.revision>0&&Number.isInteger(s.run_id)&&s.run_id>=0&&typeof s.active==='boolean'&&typeof s.available==='boolean'&&typeof s.nonce==='string'&&/^(?:[0-9a-f]{16})?$/.test(s.nonce)&&Array.isArray(s.steps)&&s.steps.length<=16&&s.steps.every(v=>Number.isInteger(v.layout_id)&&v.layout_id>0&&Number.isInteger(v.command)&&v.command>=1&&v.command<=5&&Number.isInteger(v.outcome)&&v.outcome>=0&&v.outcome<=4&&Number.isInteger(v.result));
 const outcome=n=>tr(['waiting','success','failed','uncertain','cancelled'][n]||'uncertain');
 const command=n=>tr(['admission','onCmd','offCmd','open','close','stop'][n]||'admission');
 function adopt(next){
  if(!valid(next))throw Error('Invalid action status');
  if(status&&status.boot_id!==next.boot_id){message='restarted';pending=null;unknown=false;}
  if(pending&&next.boot_id===pending.boot_id&&next.nonce===pending.nonce){pending=null;unknown=false;message='';}
  if(status&&status.boot_id===next.boot_id&&next.admission_generation<status.admission_generation)return;
  status=next;
 }
 async function run(kind,id,on){
  if(!current()||sending||unknown||dirty||conflict)return;const owner={mount:serial,read:++readSerial};sending=owner;message='';draw();
  const owned=()=>current()&&serial===owner.mount&&sending===owner;
  try{
   const next=await json('actions/status');if(!owned()||owner.read!==readSerial)return;adopt(next);
   if(dirty||conflict||!status.available||status.active||!published||published.revision!==status.revision)throw Error('Busy');
   const random=new Uint32Array(2);crypto.getRandomValues(random);const nonce=[...random].map(v=>v.toString(16).padStart(8,'0')).join('');
   pending={id,revision:status.revision,boot_id:status.boot_id,admission_generation:status.admission_generation,nonce,...(kind==='groups'?{on}: {})};
   /* No retry of this POST, including on a lost response or an unknown body. */
   unknown=true;const result=await json(kind+'/run',pending);if(!owned())return;if(!result||!Number.isInteger(result.run_id)||result.run_id<=0||result.result!==0)throw Error('Unknown admission');
   await refresh(true);
  }catch(error){if(owned()){if([400,409,503].includes(error.httpStatus)){unknown=false;pending=null;}message=unknown?'unknown':'missing';}}finally{if(sending===owner){sending=false;if(current()&&serial===owner.mount)draw();}}
 }
 async function refresh(force=false){
  if(!current()||fetching&&!force)return;const generation=serial,read=++readSerial;fetching=read;
  const owned=()=>generation===serial&&read===readSerial&&current();
  try{
   const next=await json('actions/status');if(!owned())return;adopt(next);
   if(!published||published.revision!==status.revision){const nextConfig=await json('config');if(!owned())return;if(nextConfig.schema!==2||nextConfig.revision!==status.revision||!Array.isArray(nextConfig.groups)||!Array.isArray(nextConfig.scenes))throw Error('Invalid configuration');published=nextConfig;}
   if(mode==='activity'){let nextLog=await json('actions/log/query',{epoch,offset,limit:16});if(!owned())return;if(!nextLog||!Array.isArray(nextLog.entries)||nextLog.entries.length>16||nextLog.timezone!=='UTC')throw Error('Invalid log');log=nextLog;epoch=nextLog.epoch;}
   if(!unknown&&message!=='restarted')message='';draw();
  }catch{if(owned()){message=unknown?'unknown':'missing';draw();}}finally{if(fetching===read)fetching=false;}
 }
 function draw(){
  if(!current())return;const scroll=root.scrollTop;root.replaceChildren(el('h2',tr(mode==='groups'?'groups':mode==='activity'?'activity':mode==='scenes'?'scenes':'title')),el('p',tr('sequential'),'muted'));
  root.append(el('p',tr(dirty||conflict?'dirty':message||(!status?'missing':status.active?'active':'done')),'muted'));
  const controls=el('div',undefined,'row');controls.append(button('refresh',()=>refresh()));
  if(status?.active)controls.append(button('cancel',async()=>{try{await json('actions/cancel',{boot_id:status.boot_id,run_id:status.run_id});await refresh();}catch{message='missing';draw();}}));root.append(controls);
  if(mode==='activity'){
   for(const e of log?.entries||[]){const row=el('div',undefined,'row');const when=e.utc?new Date(e.utc*1000).toISOString():`${tr('unset')} · ${(e.monotonic_ms/1000).toFixed(0)} s`;row.append(el('span',`${when} · #${e.layout_id||e.run_id} · ${e.kind===2?`${tr('state')}${e.command?' · '+command(e.command):''}`:command(e.command)} · ${e.kind===2?tr(e.result===0?'observed':'observationError'):outcome(e.outcome)} · ${e.result}`));root.append(row);}
   if(!log?.entries?.length)root.append(el('p',tr('empty')));const nav=el('div',undefined,'row'),newer=button('newer',()=>{offset=Math.max(0,offset-16);refresh();}),older=button('older',()=>{offset+=16;refresh();});newer.disabled=!offset;older.disabled=!log||offset+(log.entries?.length||0)>=log.total;nav.append(newer,older,button('clear',async()=>{try{await request('actions/log/clear',{method:'POST'});serial++;fetching=false;offset=epoch=0;log=null;await refresh();}catch{message='missing';draw();}}));root.append(nav);
  }else{
   let objects=mode==='groups'?(published?.groups||[]):mode==='scenes'?(published?.scenes||[]):[];
   if(mode==='dashboards'){const dashboard=published?.dashboards?.find(d=>d.id===published.active_dashboard);objects=(dashboard?.tiles||[]).filter(t=>t.type==='scene'||t.type==='group').map(t=>({...published[t.type==='scene'?'scenes':'groups'].find(x=>x.id===t.ref),actorKind:t.type==='scene'?'scenes':'groups'})).filter(o=>o.id);}
   objects=objects.filter(o=>(!roomFilter||o.room===roomFilter)&&o.name.toLocaleLowerCase().includes(query));if(!objects.length)root.append(el('p',tr('empty'),'empty'));
   for(const o of objects){const kind=o.actorKind||(mode==='groups'?'groups':'scenes'),row=el('div',undefined,'row action-card');const details=el('div',undefined,'grow');details.append(el('h3',o.name),el('p',(published?.rooms?.find(r=>r.id===o.room)?.name||tr('whole'))+' · '+(kind==='groups'?o.members.length:o.actions.length)));row.append(details);if(window.dsrEditAction)row.append(button('edit',()=>window.dsrEditAction(kind,o.id)));const runButton=button(kind==='groups'?'on':'run',()=>run(kind,o.id,true));runButton.disabled=!status?.available||sending||unknown||dirty||conflict||!(kind==='groups'?o.members.length:o.actions.length);row.append(runButton);if(kind==='groups'){const off=button('off',()=>run(kind,o.id,false));off.disabled=runButton.disabled;row.append(off);}root.append(row);}
   for(const step of status?.steps||[])root.append(el('p',`#${step.layout_id} · ${command(step.command)} · ${outcome(step.outcome)} · ${step.result}`));
  }root.scrollTop=scroll;
 }
 window.dsrActionsFilter=(text,room)=>{query=text.toLocaleLowerCase();roomFilter=room;draw();};
 window.dsrActionsMount=(parent,nextMode)=>{query='';roomFilter=0;serial++;readSerial++;fetching=false;sending=false;root=el('section',undefined,'box actions-section');mode=nextMode;offset=epoch=0;log=null;parent.append(root);queueMicrotask(()=>{draw();refresh();});};
 window.dsrActionsRefresh=()=>refresh();window.dsrActionsLocale=()=>draw();
 if(config&&['groups','scenes','activity','dashboards'].includes(page))window.dsrActionsMount(document.querySelector('#editor').firstElementChild,page);
})();
