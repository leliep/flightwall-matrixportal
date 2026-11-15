#!/usr/bin/env python3
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/main.cpp').read_text()
start=source.index('void sendPsramResponse(')
response=source[start:source.index('\n}\n',start)+3]
with tempfile.TemporaryDirectory(prefix='flightwall-temporary-psram-') as folder:
 (Path(folder)/'ResponseUnderTest.hpp').write_text(response)
 for variant in (0,1):
  binary=Path(folder)/str(variant)
  subprocess.run(['c++','-std=c++11','-pthread','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',
   f'-DFLIGHTWALL_HAS_MATRIX={variant}','-DARDUINOJSON_ENABLE_ARDUINO_STRING=0',
   '-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0',
   '-I'+folder,'-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),
   '-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/ArduinoJson/src'),
   str(root/'tools/test_temporary_psram.cpp'),str(root/'src/TemporaryPsram.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
