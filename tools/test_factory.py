#!/usr/bin/env python3
"""Meaningful boundary/corruption tests; synthetic bytes, not target proof."""
import hashlib
import struct
from package_factory import DATA_SIZE, DATA_OFFSET, FLASH_SIZE, LAYOUT, SEGMENTS, compose, partition_layout, validate_app

checks=0
def reject(call):
    global checks
    try:call()
    except (ValueError,UnicodeError,struct.error):checks+=1;return
    raise AssertionError("invalid input accepted")

def table(entries=LAYOUT):
    raw=b"".join(struct.pack("<HBBII16sI",0x50aa,t,s,o,n,name.encode(),0) for name,t,s,o,n in entries)
    raw+=b"\xeb\xeb"+b"\xff"*14+hashlib.md5(raw).digest()
    return raw+b"\xff"*(0xc00-len(raw))

partition=table();assert partition_layout(partition)==LAYOUT;checks+=1
reject(lambda:partition_layout(partition[:-1]))
bad=bytearray(partition);bad[10]^=1;reject(lambda:partition_layout(bad))
bad=bytearray(partition);bad[-1]=0;reject(lambda:partition_layout(bad))
entries=list(LAYOUT);entries[-1]=("dsrdata",1,0x82,DATA_OFFSET,DATA_SIZE-4096);reject(lambda:partition_layout(table(entries)))
entries=list(LAYOUT);entries[0]=("nvs",1,2,0x9000,0x11000);reject(lambda:partition_layout(table(entries)))
bad=bytearray(partition);bad[28]=1;bad[208:224]=hashlib.md5(bad[:192]).digest();reject(lambda:partition_layout(bad))
segments={offset:b"test" for offset in SEGMENTS};fs=b"\xff"*DATA_SIZE
full=compose(segments,fs);assert len(full)==FLASH_SIZE and full[DATA_OFFSET:]==fs;checks+=1
reject(lambda:compose({**segments,0x9000:b"secret"},fs))
reject(lambda:compose({k:v for k,v in segments.items() if k!=0x8000},fs))
reject(lambda:compose({**segments,0x20000:b"x"*(0x600000+1)},fs))
reject(lambda:compose(segments,fs[:-1]))
reject(lambda:compose(segments,fs+b"\0"))
header=bytearray(24);header[0]=0xe9;header[1]=1;header[12]=9;header[23]=1
payload=bytearray(256);struct.pack_into("<I",payload,0,0xabcd5432);payload[16:21]=b"1.2.3"
app=header+struct.pack("<II",0x3c000000,len(payload))+payload;checksum=0xef
for value in payload:checksum^=value
check_at=(len(app)+16)//16*16-1;app+=bytes(check_at-len(app))+bytes([checksum]);app+=hashlib.sha256(app).digest()
assert validate_app(app)=="1.2.3";checks+=1
bad=bytearray(app);bad[40]^=1;reject(lambda:validate_app(bad))
bad=bytearray(app);bad[-1]^=1;reject(lambda:validate_app(bad))
bad=bytearray(app);bad[12]=0;reject(lambda:validate_app(bad))
reject(lambda:validate_app(app[:-1]))
print(f"PASS: factory packager {checks} synthetic boundary/corruption checks; real generator/mount/target proof separate")
