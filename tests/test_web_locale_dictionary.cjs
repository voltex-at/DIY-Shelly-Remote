/* Execute the actual dictionary prefix, not a copied test dictionary. This
 * complements browser execution; it does NOT model DOM/session correctness. */
'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const base=path.resolve(__dirname,'../components/dsr_platform/web');
const source=fs.readFileSync(path.join(base,'panel.js'),'utf8');
const html=fs.readFileSync(path.join(base,'panel.html'),'utf8');
const stop=source.indexOf('const uiTextBindings=');assert(stop>0);
class UniqueMap extends Map {
 constructor(rows){super();for(const [key,value]of rows){assert(!this.has(key),'duplicate locale key: '+key);this.set(key,value);}}
}
const context=vm.createContext({Map:UniqueMap});
vm.runInContext(source.slice(0,stop)+'\nglobalThis.messages=WEB_MESSAGES;',context);
let checks=0;
const slots=text=>[...new Set(Array.from(text.matchAll(/\{(\d+)\}/g),m=>m[1]))].sort();
for(const [key,row]of context.messages){
 assert.equal(typeof key,'string');assert(key.length>0);checks++;
 for(const locale of ['pl','en','de']){assert.equal(typeof row[locale],'string',key+'/'+locale);assert(row[locale].trim().length>0,key+'/'+locale);assert.deepEqual(slots(row[locale]),slots(key),'placeholder mismatch: '+key+'/'+locale);checks+=2;}
}
for(const match of source.matchAll(/\bm\(((?:'(?:\\.|[^'\\])*')|(?:"(?:\\.|[^"\\])*"))/g)){
 const key=vm.runInContext(match[1],context);assert(context.messages.has(key),'missing literal message: '+key);checks++;
}
for(const match of html.matchAll(/data-ui(?:-aria)?="([^"]+)"/g)){
 assert(!match[1].includes('&'),'encoded dictionary key requires explicit decoder');
 assert(context.messages.has(match[1]),'missing HTML message: '+match[1]);checks++;
}
console.log(`PASS: ${checks} actual locale dictionary checks; ${context.messages.size} unique complete PL/EN/DE entries; literal JS/HTML keys covered, not browser proof`);
