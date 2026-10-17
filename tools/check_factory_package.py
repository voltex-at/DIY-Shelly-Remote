#!/usr/bin/env python3
"""Verify received or freshly built clean factory and app-only package bytes."""
import argparse
import json
from pathlib import Path
from package_factory import DATA_OFFSET, DATA_SIZE, FLASH_SIZE, SEGMENTS, digest, partition_layout, require, validate_app, validate_esp_image

def verify(directory):
    factory=directory/"FACTORY-CLEAN-16MB";update=directory/"UPDATE-APP-ONLY"
    manifest=json.loads((factory/"FACTORY_MANIFEST.json").read_text())
    proof=json.loads((factory/"SPIFFS_MOUNT_PROOF.json").read_text())
    image=(factory/"DIY-Shelly-Remote-es3c35p-FACTORY-CLEAN-16MB.bin").read_bytes()
    fs=(factory/"dsrdata.bin").read_bytes()
    require(len(image)==FLASH_SIZE and len(fs)==DATA_SIZE,"Wrong full image/partition length")
    require(manifest["flash_bytes"]==FLASH_SIZE and manifest["spiffs"]["offset"]==DATA_OFFSET and
            manifest["spiffs"]["magic_blocks"]==992,"Wrong manifest geometry")
    require(digest(image)==manifest["factory"] and digest(fs)=={k:manifest["spiffs"][k] for k in ["bytes","sha256","crc32"]},"Factory/partition checksum mismatch")
    require(image[DATA_OFFSET:]==fs,"Wrong final SPIFFS offset or bytes")
    require(proof["image_sha256"]==digest(fs)["sha256"] and proof["image_bytes"]==len(fs),"Mount proof belongs to another image")
    require(proof["exact_image_mounted_without_format_or_mutation"] is True and
            proof["actual_default_and_rooms_checkpoint_save_remount"] is True and
            proof["actual_gc_process_exit_cuts"]>=210,"Incomplete original IDF mount proof")
    require(proof["idf"]=="735507283d5b2f9fb363a1901172dbd9e847945d" and
            proof["spiffs"]=="0dbb3f71c5f6fae3747a9d935372773762baf852","Wrong mount verifier revision")
    cursor=0
    for offset,(name,limit) in sorted(SEGMENTS.items()):
        data=(factory/name).read_bytes()
        require(0<len(data)<=limit and image[offset:offset+len(data)]==data,"Segment mismatch")
        require(image[cursor:offset]==b"\xff"*(offset-cursor),"Non-FF reserved gap")
        require(digest(data)=={k:manifest["segments"][hex(offset)][k] for k in ["bytes","sha256","crc32"]},"Segment digest mismatch")
        cursor=offset+len(data)
    require(image[cursor:DATA_OFFSET]==b"\xff"*(DATA_OFFSET-cursor),"Non-FF OTA1/reserved gap")
    partition_layout((factory/"partition_table/partition-table.bin").read_bytes())
    validate_esp_image((factory/"bootloader/bootloader.bin").read_bytes())
    app=(factory/"diy_shelly_remote.bin").read_bytes()
    require(validate_app(app)==manifest["version"],"Application version mismatch")
    require((update/"diy_shelly_remote.bin").read_bytes()==app,"UPDATE app differs from FACTORY")
    require({p.name for p in update.iterdir()}<={"diy_shelly_remote.bin","BUILD_COMMIT.txt","SHA256SUMS.txt","INSTALL_UPDATE_PL.md","licenses"},"UPDATE contains a non-app segment")
    require({p.relative_to(update).as_posix() for p in update.rglob("*") if p.is_file() and p.suffix.lower()==".bin"}=={"diy_shelly_remote.bin"},
            "UPDATE contains a nested non-app binary")
    require((factory/"BUILD_COMMIT.txt").read_bytes()==(update/"BUILD_COMMIT.txt").read_bytes(),"Mixed source provenance")
    require((factory/"BUILD_COMMIT.txt").read_text()==f"Source commit: {manifest['source']}\nTested commit: {manifest['tested']}\n",
            "Manifest/source provenance mismatch")
    for folder in [factory,update]:
        actual=set()
        for line in (folder/"SHA256SUMS.txt").read_text().splitlines():
            checksum,name=line.split();require(name not in actual,"Duplicate checksum filename");actual.add(name)
            path=folder/name;require(path.resolve().is_relative_to(folder.resolve()),"Unsafe checksum path")
            require(digest(path.read_bytes())["sha256"]==checksum,"Listed checksum mismatch")
        require(actual=={p.relative_to(folder).as_posix() for p in folder.rglob("*.bin")},"Missing/unexpected binary checksum")
    primary=json.loads((factory/"flasher_args.json").read_text())
    alternate=json.loads((factory/"flasher_args-segments.json").read_text())
    for routing in [primary,alternate]:
        require(routing["flash_settings"]==dict(flash_mode="dio",flash_size="16MB",flash_freq="80m") and
                routing["extra_esptool_args"]["chip"]=="esp32s3","Wrong factory flasher settings")
    require(primary["flash_files"]=={"0x0":"DIY-Shelly-Remote-es3c35p-FACTORY-CLEAN-16MB.bin"},"FACTORY main routing is not one full image at0")
    require({int(k,16):v for k,v in alternate["flash_files"].items()}=={**{k:v[0] for k,v in SEGMENTS.items()},DATA_OFFSET:"dsrdata.bin"},"Alternate segment map omits/misroutes SPIFFS")
    return {"version":manifest["version"],"factory":digest(image),"spiffs":digest(fs),"original_idf_mount_verified":True,"reserved_gaps_ff":True,"update_app_only":True,"physical_acceptance":False}

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--package-dir",type=Path,required=True);a=p.parse_args()
    print(json.dumps(verify(a.package_dir.resolve()),indent=2))
