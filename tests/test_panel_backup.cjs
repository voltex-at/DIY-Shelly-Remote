'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),{webcrypto}=require('node:crypto');
const {encryptBackup,decryptBackup,validateBackupHeader}=require('../components/dsr_platform/web/backup.js');
let checks=0;const eq=(a,b)=>{assert.deepEqual(a,b);checks++;},reject=async fn=>{await assert.rejects(fn);checks++;};
(async()=>{
 const env={crypto:webcrypto,secure:true},fixture=Buffer.from(fs.readFileSync('tests/system_data/manifest-v1.json')),password='  Zażółć 🔐 backup  ';
 const encoded=await encryptBackup(fixture,password,env);eq(new TextDecoder().decode(new Uint8Array(encoded).slice(0,7)),'DSRBKP1');eq(encoded.byteLength,fixture.length+80);
 eq(Buffer.from(await decryptBackup(encoded,password,env)),fixture);eq(validateBackupHeader(encoded).length,fixture.length+16);
 await reject(()=>decryptBackup(encoded,password.trim(),env));await reject(()=>decryptBackup(encoded,'Other password twelve',env));
 for(const at of [0,8,10,12,32,44,48,63,64,encoded.byteLength-1]){const bad=encoded.slice(0);new Uint8Array(bad)[at]^=1;await reject(()=>decryptBackup(bad,password,env));}
 await reject(()=>encryptBackup(fixture,'too short',env));await reject(()=>encryptBackup(fixture,'🔥'.repeat(100),env));await reject(()=>encryptBackup(fixture,'long password\ud800',env));
 await reject(()=>encryptBackup(new Uint8Array(2097153),password,env));await reject(()=>decryptBackup(encoded.slice(0,-1),password,env));
 await reject(()=>encryptBackup(fixture,password,{crypto:webcrypto,secure:false}));await reject(()=>decryptBackup(encoded,password,{secure:true}));
 const second=await encryptBackup(fixture,password,env);assert.notDeepEqual(Buffer.from(second),Buffer.from(encoded));checks++;
 eq(Buffer.from(await decryptBackup(second,password,env)),fixture);
 console.log(`PASS: ${checks} backup file crypto checks; genuine Web Crypto, synthetic fixture, no network or device`);
})().catch(e=>{console.error(e);process.exit(1);});
