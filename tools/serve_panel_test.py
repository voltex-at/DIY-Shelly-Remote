#!/usr/bin/env python3
"""Loopback-only test harness. Uses actual C schema/checkpoint code and LVGL.

The pairing code here is a TEST FIXTURE, never part of ESP32 firmware.
"""
import argparse
import ctypes
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import subprocess
import threading
import tempfile
import time

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--port',type=int,default=8765)
args=p.parse_args()
(ROOT/'build-ui').mkdir(exist_ok=True)
work=Path(tempfile.mkdtemp(prefix='panel-fixture-',dir=ROOT/'build-ui'))
include=Path('/usr/include/cjson') if Path('/usr/include/cjson/cJSON.h').exists() else ROOT/'.test-deps'
layout_probe=work/'layout_probe.c';layout_probe.write_text('#include <stddef.h>\n#include "dsr_ble.h"\nsize_t test_ble_device_size(void){return sizeof(dsr_ble_device_t);}\nsize_t test_ble_kind_offset(void){return offsetof(dsr_ble_device_t,kind);}\n')
subprocess.run(['cc','-std=c11','-D_POSIX_C_SOURCE=200809L','-shared','-fPIC','-I',str(include),'-I',str(ROOT/'tests/support'),'-I',str(ROOT/'components/dsr_platform/include'),'-I',str(ROOT/'components/dsr_config/include'),'-I',str(ROOT/'components/dsr_core/include'),*[str(ROOT/f) for f in ['components/dsr_config/dsr_config.c','components/dsr_config/dsr_config_store.c','components/dsr_config/dsr_photo_name.c','components/dsr_core/dsr_core.c','components/dsr_platform/dsr_discovery_model.c','components/dsr_platform/dsr_ble_model.c']],str(layout_probe),'-l:libcjson.so.1','-lm','-o',str(work/'config.so')],check=True)
lib=ctypes.CDLL(str(work/'config.so'));libc=ctypes.CDLL(None);libc.free.argtypes=[ctypes.c_void_p]
lib.dsr_config_default.restype=ctypes.c_void_p
lib.dsr_config_replace.argtypes=[ctypes.c_char_p,ctypes.c_char_p,ctypes.c_uint32,ctypes.c_char_p];lib.dsr_config_replace.restype=ctypes.c_void_p
lib.dsr_config_store_save.argtypes=[ctypes.c_char_p,ctypes.c_char_p,ctypes.c_char_p];lib.dsr_config_store_save.restype=ctypes.c_bool
lib.dsr_config_store_load.argtypes=[ctypes.c_char_p,ctypes.POINTER(ctypes.c_bool),ctypes.c_char_p];lib.dsr_config_store_load.restype=ctypes.c_void_p
def take(pointer):
    if not pointer:return None
    result=ctypes.string_at(pointer);libc.free(pointer);return result
config=take(lib.dsr_config_default())
error=ctypes.create_string_buffer(96)
assert lib.dsr_config_store_save(str(work).encode(),config,error)
mutex=threading.Lock()
class LanDevice(ctypes.Structure):
    _fields_=[('host',ctypes.c_char*16),('id',ctypes.c_char*64),('mac',ctypes.c_char*13),('model',ctypes.c_char*40),('name',ctypes.c_char*48),('generation',ctypes.c_uint8),('auth_required',ctypes.c_bool)]
class LanCaps(ctypes.Structure):
    _fields_=[('switches',ctypes.c_uint16),('covers',ctypes.c_uint16),('lights',ctypes.c_uint16),('sensors',ctypes.c_bool)]
lib.cJSON_Parse.argtypes=[ctypes.c_char_p];lib.cJSON_Parse.restype=ctypes.c_void_p;lib.cJSON_Delete.argtypes=[ctypes.c_void_p]
lib.dsr_lan_parse_identity.argtypes=[ctypes.c_void_p,ctypes.c_char_p,ctypes.POINTER(LanDevice)]
lib.dsr_lan_parse_capabilities.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.POINTER(LanCaps)]
# Radio provider below is explicitly a host fixture; actual C parsers still run.
identity=b'{"gen":2,"id":"shellyplus2pm-aabbccddeeff","mac":"aabbccddeeff","model":"SNSW-002P16EU","name":"Shelly LAN fixture","auth_en":true}'
raw=lib.cJSON_Parse(identity);device=LanDevice();assert lib.dsr_lan_parse_identity(raw,b'192.168.1.88',ctypes.byref(device))==0;lib.cJSON_Delete(raw)
lan_device={key:(bytes(getattr(device,key)).decode() if key not in ['generation','auth_required'] else getattr(device,key)) for key,_ in LanDevice._fields_}
raw=lib.cJSON_Parse(b'{"switch:1":{"id":1,"output":false},"switch:4":{"id":4,"output":true}}');caps=LanCaps();assert lib.dsr_lan_parse_capabilities(raw,2,ctypes.byref(caps))==0;lib.cJSON_Delete(raw)
lan={'phase':0,'serial':0,'result':0,'devices':[],'selected':{},'capabilities':{}}
lan_ready_at=0
class BleDevice(ctypes.Structure):
    _fields_=[('address',ctypes.c_uint8*6),('address_type',ctypes.c_uint8),('evidence',ctypes.c_uint8),('bthome_version',ctypes.c_uint8),('rssi',ctypes.c_int8),('name',ctypes.c_char*32),('model_id',ctypes.c_uint16),('flags',ctypes.c_uint16),('mac',ctypes.c_uint8*6),('has_model',ctypes.c_bool),('encrypted',ctypes.c_bool),('has_flags',ctypes.c_bool),('has_mac',ctypes.c_bool),('kind',ctypes.c_int),('seen_ms',ctypes.c_uint32)]
# The ctypes mirror must track the C struct exactly (a stale mirror broke
# the BLE fixture once); fail loudly instead of reading shifted fields.
lib.test_ble_device_size.restype=ctypes.c_size_t;lib.test_ble_kind_offset.restype=ctypes.c_size_t
assert ctypes.sizeof(BleDevice)==lib.test_ble_device_size() and BleDevice.kind.offset==lib.test_ble_kind_offset(),'BleDevice ctypes mirror out of date'
lib.dsr_ble_parse.argtypes=[ctypes.c_char_p,ctypes.c_size_t,ctypes.POINTER(BleDevice)];lib.dsr_ble_parse.restype=ctypes.c_bool
lib.dsr_ble_model_name.argtypes=[ctypes.POINTER(BleDevice)];lib.dsr_ble_model_name.restype=ctypes.c_char_p
ble_devices=[]
for data in [bytes.fromhex('06ffa90b0b0200'),bytes.fromhex('11075f44495f4356535f4350525f534f6d5f')]:
    d=BleDevice();assert lib.dsr_ble_parse(data,len(data),ctypes.byref(d))
    ble_devices.append(dict(model=lib.dsr_ble_model_name(ctypes.byref(d)).decode(),name='TEST / DEMO radio fixture',kind=d.kind,rssi=-48,paired=False,bthome=False,encrypted=False,age_ms=0))
ble=dict(phase=0,busy=False,devices=[],elapsed_ms=0)
ble_ready_at=0
save_delay=0
rendered=None
def frame():
    global rendered
    with mutex:
        payload=config
        if rendered and rendered[0]==payload:return rendered[1]
        source=work/'config.json';source.write_bytes(payload)
        subprocess.run([str(ROOT/'build-ui/review-native/test_ui'),str(work),str(source)],check=True,stdout=subprocess.DEVNULL)
        rgb=(work/'01-home.ppm').read_bytes().split(b'\n',3)[3]
        out=bytearray(320*480*2)
        for i in range(320*480):
            r,g,b=rgb[i*3:i*3+3];v=((r*31+127)//255)<<11|((g*63+127)//255)<<5|((b*31+127)//255);out[i*2]=v&255;out[i*2+1]=v>>8
        rendered=(payload,bytes(out));return rendered[1]
class Handler(BaseHTTPRequestHandler):
    def log_message(self,*a):pass
    def send(self,status,data,kind='application/json',cookie=False):
        if isinstance(data,dict):data=json.dumps(data).encode()
        self.send_response(status);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(data)));self.send_header('Cache-Control','no-store')
        if cookie:self.send_header('Set-Cookie','dsr_session=TEST_ONLY; Path=/; HttpOnly; SameSite=Strict')
        self.end_headers();self.wfile.write(data)
    def route(self):
        global config,lan_ready_at,save_delay,ble_ready_at
        if not self.path.startswith('/api/'):
            files={'/':'panel.html','/panel.js':'panel.js','/runtime.js':'runtime.js','/actions.js':'actions.js','/backup.js':'backup.js','/panel.css':'panel.css'}
            if self.path not in files:return self.send(404,{})
            f=ROOT/'components/dsr_platform/web'/files[self.path]
            return self.send(200,f.read_bytes(),{'html':'text/html; charset=utf-8','js':'text/javascript; charset=utf-8','css':'text/css'}[f.suffix[1:]])
        body=self.rfile.read(int(self.headers.get('Content-Length',0)))
        if self.path=='/api/v1/pair' and body==b'0123456789abcdef':return self.send(200,{'paired':True},cookie=True)
        if 'dsr_session=TEST_ONLY' not in self.headers.get('Cookie',''):return self.send(401,{'error':'TEST fixture: pair using code 0123456789abcdef'})
        if self.path=='/api/v1/config' and self.command=='GET':return self.send(200,config)
        if self.path=='/api/v1/config' and self.command=='PUT':
            # Test-only delayed acknowledgement exercises edits made in flight.
            if save_delay:time.sleep(save_delay)
            with mutex:
                revision=json.loads(config)['revision'];e=ctypes.create_string_buffer(96);next_config=take(lib.dsr_config_replace(config,body,revision,e))
                if not next_config:return self.send(409,{'error':e.value.decode()})
                if not lib.dsr_config_store_save(str(work).encode(),next_config,e):return self.send(500,{'error':e.value.decode()})
                config=next_config
            return self.send(202,{'saving':True})
        if self.path=='/api/v1/status':return self.send(200,{'revision':json.loads(config)['revision'],'busy':False,'message':'Saved','device_busy':False,'device_result':0,'firmware':'HOST TEST · DEMO'})
        if self.path=='/api/v1/test/save-delay':save_delay=float(body or b'0');return self.send(200,{})
        if self.path=='/api/v1/ble/scan':
            if ble['busy']:return self.send(409,{'error':'scan busy'})
            ble.update(phase=2,busy=True,devices=[],elapsed_ms=0);ble_ready_at=time.monotonic()+1.2;return self.send(202,{})
        if self.path=='/api/v1/ble/cancel':ble.update(phase=4,busy=False);return self.send(202,{})
        if self.path=='/api/v1/ble':
            if ble['busy'] and time.monotonic()>=ble_ready_at:ble.update(phase=3,busy=False,devices=ble_devices,elapsed_ms=1200)
            return self.send(200,ble)
        if self.path=='/api/v1/discovery/scan':
            lan.update(phase=1,devices=[],serial=lan['serial']+1,probed=0,total=1);lan_ready_at=time.monotonic()+.2
            return self.send(202,{'scanning':True})
        if self.path=='/api/v1/discovery/cancel':lan.update(phase=3,serial=lan['serial']+1);return self.send(202,{})
        if self.path=='/api/v1/discovery':
            if lan['phase']==1 and time.monotonic()>=lan_ready_at:lan.update(phase=2,devices=[lan_device],probed=1,serial=lan['serial']+1)
            return self.send(200,lan)
        if self.path=='/api/v1/discovery/inspect':
            data=json.loads(body);assert data['id']==lan_device['id'] and data['host']==lan_device['host']
            correct=data['username']=='admin' and data['password']=='test-lan-password'
            lan.update(phase=6 if correct else 7,result=0 if correct else 4,selected=lan_device,serial=lan['serial']+1,capabilities={key:getattr(caps,key) for key,_ in LanCaps._fields_})
            return self.send(202,{'checking':True})
        if self.path=='/api/v1/devices/bind':
            data=json.loads(body)
            # Manual IP intentionally omits optional expected_id. The radio
            # provider still represents the same verified identity/host; this
            # harness does not claim that a physical probe took place.
            assert data.get('expected_id',lan_device['id'])==lan_device['id'] and data['host']==lan_device['host'] and data['password']=='test-lan-password' and data['channel'] in [1,4]
            with mutex:
                draft=json.loads(config);ident=max([d['id'] for d in draft['devices']]+[0])+1
                draft['devices'].append({'id':ident,'name':data['name'],'host':data['host'],'channel':data['channel'],'room':data['room'],'kind':'relay'})
                e=ctypes.create_string_buffer(96);next_config=take(lib.dsr_config_replace(config,json.dumps(draft).encode(),draft['revision'],e));assert next_config
                assert lib.dsr_config_store_save(str(work).encode(),next_config,e);config=next_config
            return self.send(202,{'checking':True})
        if self.path=='/api/v1/devices/runtime':return self.send(200,{'boot_id':1,'devices':[]})
        if self.path=='/api/v1/preview.raw':return self.send(200,frame(),'application/octet-stream')
        if self.path=='/api/v1/test/reboot':
            found=ctypes.c_bool();e=ctypes.create_string_buffer(96)
            with mutex:config=take(lib.dsr_config_store_load(str(work).encode(),ctypes.byref(found),e))
            return self.send(200,{'restored':bool(found)})
        if self.path=='/api/v1/test/touch-edit':
            with mutex:
                draft=json.loads(config);draft['dashboards'][0]['name']='Zmieniono na ekranie';e=ctypes.create_string_buffer(96);next_config=take(lib.dsr_config_replace(config,json.dumps(draft).encode(),draft['revision'],e));assert next_config
                assert lib.dsr_config_store_save(str(work).encode(),next_config,e);config=next_config
            return self.send(200,{'changed':True})
        return self.send(404,{'error':'Test harness route unavailable'})
    do_GET=do_POST=do_PUT=route
print(f'HOST TEST ONLY http://127.0.0.1:{args.port} · pairing fixture 0123456789abcdef',flush=True)
ThreadingHTTPServer(('127.0.0.1',args.port),Handler).serve_forever()
