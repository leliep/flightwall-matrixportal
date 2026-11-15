#!/usr/bin/env python3
"""Guard Xtensa handler stack frames in both actual target binaries.

This checks compiler output, not host stack usage. The 4 KiB per-handler limit
leaves most of the 12 KiB network stack for WebServer, formatting and callers;
it does not replace runtime stack high-water measurements.
"""
import json,os,re,shutil,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
core=Path(os.environ.get('PLATFORMIO_CORE_DIR',str(Path.home()/'.platformio')))
name='xtensa-esp32s3-elf-objdump'
objdump=shutil.which(name) or str(core/'packages/toolchain-xtensa-esp32s3/bin'/name)
results=[]
for env in ('adafruit_matrixportal_esp32s3','esp32s3_n16r8_headless'):
 disassembly=subprocess.check_output([objdump,'-d',str(root/'.pio/build'/env/'firmware.elf')],text=True)
 frames={}
 for symbol,label in [('_Z15handleStatusApiv','status_api'),('_Z15handleRouteTestv','route_test_submit'),('_Z15runRouteTestJobv','route_test_worker'),('_Z16handleDisplayApiv','display_api')]:
  match=re.search('<'+symbol+r'>:\s*\n[^\n]*entry\s+a1,\s*(\S+)',disassembly)
  assert match, f'Missing frame for {symbol} in {env}'
  size=int(match.group(1),0);frames[label]=size
  assert size<=4096, f'{env} {label}: {size} bytes exceeds 4096-byte frame budget'
 results.append({'variant':env,'handler_stack_frames_bytes':frames,'network_task_stack_bytes':12288,'web_task_stack_bytes':12288})
print(json.dumps(results,indent=2))
