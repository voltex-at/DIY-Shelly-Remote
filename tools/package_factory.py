#!/usr/bin/env python3
"""Build separate clean FACTORY16MiB and app-only UPDATE packages; never flash."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib

FLASH_SIZE = 0x1000000
DATA_OFFSET, DATA_SIZE = 0xc20000, 0x3e0000
LAYOUT = [("nvs",1,2,0x9000,0x10000), ("otadata",1,0,0x19000,0x2000),
          ("phy_init",1,1,0x1b000,0x1000), ("ota_0",0,0x10,0x20000,0x600000),
          ("ota_1",0,0x11,0x620000,0x600000), ("dsrdata",1,0x82,DATA_OFFSET,DATA_SIZE)]
SEGMENTS = {0: ("bootloader/bootloader.bin",0x8000),
            0x8000: ("partition_table/partition-table.bin",0x1000),
            0x19000: ("ota_data_initial.bin",0x2000),
            0x20000: ("diy_shelly_remote.bin",0x600000)}

def require(value, message):
    if not value:
        raise ValueError(message)

def digest(data):
    return dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),crc32=f"{zlib.crc32(data):08x}")

def partition_layout(data):
    require(len(data)<=0x1000 and len(data)%32==0,"Invalid partition table length")
    entries=[]
    for offset in range(0,len(data),32):
        magic=struct.unpack_from("<H",data,offset)[0]
        if magic==0xebeb:
            require(data[offset+2:offset+16]==b"\xff"*14,"Invalid partition MD5 marker")
            require(hashlib.md5(data[:offset]).digest()==data[offset+16:offset+32],"Partition MD5 mismatch")
            require(data[offset+32:]==b"\xff"*(len(data)-offset-32),"Unexpected partition table tail")
            require(entries==LAYOUT,"Partition boundaries differ from approved 16MiB layout")
            return entries
        require(magic==0x50aa,"Invalid partition entry")
        _,kind,sub,start,size,label,flags=struct.unpack_from("<HBBII16sI",data,offset)
        require(flags==0,"Unexpected partition flags")
        entries.append((label.split(b"\0")[0].decode("ascii"),kind,sub,start,size))
    raise ValueError("Partition MD5 missing")

def validate_app(data):
    require(len(data)>256 and data[0]==0xe9 and struct.unpack_from("<H",data,12)[0]==9 and data[23]==1,
            "Expected ESP32-S3 hashed application image")
    offset=24;checksum=0xef
    for _ in range(data[1]):
        require(offset+8<=len(data),"Truncated image segment header")
        _,size=struct.unpack_from("<II",data,offset);offset+=8
        require(size<=len(data)-offset,"Truncated image segment")
        for byte in data[offset:offset+size]:checksum^=byte
        offset+=size
    check_at=(offset+16)//16*16-1
    require(check_at+33==len(data),"Invalid image final length")
    require(data[offset:check_at]==bytes(check_at-offset) and data[check_at]==checksum,"Image padding/XOR mismatch")
    require(hashlib.sha256(data[:check_at+1]).digest()==data[-32:],"Image appended SHA mismatch")
    require(struct.unpack_from("<I",data,32)[0]==0xabcd5432,"Missing application descriptor")
    return data[48:80].split(b"\0")[0].decode("ascii")

def compose(segments, filesystem):
    require(set(segments)==set(SEGMENTS),"Unexpected or missing flash segment")
    require(len(filesystem)==DATA_SIZE,"SPIFFS must fill exactly 0x3E0000 bytes")
    image=bytearray(b"\xff"*FLASH_SIZE)
    for offset,data in segments.items():
        require(0<len(data)<=SEGMENTS[offset][1],"Flash segment crosses approved boundary")
        image[offset:offset+len(data)]=data
    image[DATA_OFFSET:]=filesystem
    for offset,data in segments.items():require(image[offset:offset+len(data)]==data,"Merged segment mismatch")
    require(len(image)==FLASH_SIZE and DATA_OFFSET+len(filesystem)==FLASH_SIZE,"Factory boundary mismatch")
    # Every byte outside the exact approved input extents must remain erased.
    cursor=0
    for offset,data in sorted([*segments.items(),(DATA_OFFSET,filesystem)]):
        require(image[cursor:offset]==b"\xff"*(offset-cursor),"Unexpected non-FF gap data")
        cursor=offset+len(data)
    require(cursor==FLASH_SIZE,"Incomplete final partition")
    return bytes(image)

def package(build, sdkconfig, idf, out, source, tested):
    require(not out.exists(),"Output already exists; choose a new directory")
    sdk=sdkconfig.read_text()
    for option in ["CONFIG_ESPTOOLPY_FLASHSIZE=\"16MB\"", "CONFIG_SPIFFS_PAGE_SIZE=256",
                   "CONFIG_SPIFFS_OBJ_NAME_LEN=32", "CONFIG_SPIFFS_META_LENGTH=4",
                   "CONFIG_SPIFFS_USE_MAGIC=y", "CONFIG_SPIFFS_USE_MAGIC_LENGTH=y"]:
        require(option in sdk.splitlines(),"Unexpected target configuration: "+option)
    flash=json.loads((build/"flasher_args.json").read_text())
    require(flash["flash_settings"]==dict(flash_mode="dio",flash_size="16MB",flash_freq="80m"),"Unexpected flash settings")
    require(flash["extra_esptool_args"]["chip"]=="esp32s3","Wrong target chip")
    files={int(offset,16):name for offset,name in flash["flash_files"].items()}
    require(len(files)==len(flash["flash_files"]) and files=={k:v[0] for k,v in SEGMENTS.items()},"Unexpected flasher segment routing")
    segments={offset:(build/name).read_bytes() for offset,(name,_) in SEGMENTS.items()}
    partition_layout(segments[0x8000]);version=validate_app(segments[0x20000])
    require(subprocess.check_output(["git","-C",str(idf),"rev-parse","HEAD"],text=True).strip()==
            "735507283d5b2f9fb363a1901172dbd9e847945d","Wrong IDF generator revision")
    with tempfile.TemporaryDirectory(prefix="dsr-factory-") as tmp:
        directory=Path(tmp);empty=directory/"empty";empty.mkdir();fs=directory/"dsrdata.bin"
        subprocess.run([sys.executable,str(idf/"components/spiffs/spiffsgen.py"),hex(DATA_SIZE),str(empty),str(fs),
                        "--page-size","256","--block-size","4096","--obj-name-len","32","--meta-len","4",
                        "--use-magic","--use-magic-len"],check=True)
        filesystem=fs.read_bytes()
    require(len(filesystem)==DATA_SIZE,"Generator output length mismatch")
    # Full empty filesystem is deterministic; magic is checked for all 992 blocks.
    for block in range(DATA_SIZE//4096):
        magic=(0x20140529^256^((DATA_SIZE//4096)-block))&0xffff
        require(struct.unpack_from("<H",filesystem,block*4096+252)[0]==magic,"Missing SPIFFS block magic")
    image=compose(segments,filesystem)
    factory=out/"FACTORY-CLEAN-16MB";update=out/"UPDATE-APP-ONLY";factory.mkdir(parents=True);update.mkdir()
    name="DIY-Shelly-Remote-es3c35p-FACTORY-CLEAN-16MB.bin"
    (factory/name).write_bytes(image);(factory/"dsrdata.bin").write_bytes(filesystem)
    for offset,(path,_) in SEGMENTS.items():
        target=factory/path;target.parent.mkdir(exist_ok=True);target.write_bytes(segments[offset])
    single=dict(flash,flash_files={"0x0":name})
    (factory/"flasher_args.json").write_text(json.dumps(single,indent=2)+"\n")
    alternate=dict(flash,flash_files={**flash["flash_files"],"0xc20000":"dsrdata.bin"})
    (factory/"flasher_args-segments.json").write_text(json.dumps(alternate,indent=2)+"\n")
    shutil.copy2(build/"diy_shelly_remote.bin",update/"diy_shelly_remote.bin")
    provenance=f"Source commit: {source}\nTested commit: {tested}\n"
    for destination in [factory,update]:(destination/"BUILD_COMMIT.txt").write_text(provenance)
    report=dict(version=version,source=source,tested=tested,flash_bytes=FLASH_SIZE,
                factory=digest(image),spiffs=dict(offset=DATA_OFFSET,**digest(filesystem),magic_blocks=992),
                segments={hex(offset):dict(path=SEGMENTS[offset][0],**digest(data)) for offset,data in segments.items()},
                partition_md5_verified=True,all_other_bytes_ff=True,
                empty_spiffs_contains_no_credentials_or_user_files=True,
                mount_proof_file="SPIFFS_MOUNT_PROOF.json",
                physical_acceptance=False)
    (factory/"FACTORY_MANIFEST.json").write_text(json.dumps(report,indent=2)+"\n")
    for destination in [factory,update]:
        lines=[]
        for path in sorted(destination.rglob("*.bin")):
            lines.append(hashlib.sha256(path.read_bytes()).hexdigest()+"  "+path.relative_to(destination).as_posix())
        (destination/"SHA256SUMS.txt").write_text("\n".join(lines)+"\n")
    return report

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    for name in ["build-dir","sdkconfig","idf","out"]:p.add_argument("--"+name,type=Path,required=True)
    p.add_argument("--source",required=True);p.add_argument("--tested",required=True);a=p.parse_args()
    report=package(a.build_dir.resolve(),a.sdkconfig.resolve(),a.idf.resolve(),a.out.resolve(),a.source,a.tested)
    print(json.dumps({"version":report["version"],"factory":report["factory"],"spiffs":report["spiffs"]}))
