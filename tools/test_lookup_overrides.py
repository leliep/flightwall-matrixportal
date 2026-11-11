#!/usr/bin/env python3
"""Compile real parser/lookups with fault-injected PSRAM and persistence boundaries."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='flightwall-lookups-') as folder:
 for variant in (0,1):
  binary=Path(folder)/f'test-{variant}'
  command=['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',
   f'-DFLIGHTWALL_HAS_MATRIX={variant}','-DARDUINOJSON_ENABLE_ARDUINO_STRING=0',
   '-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0',
   '-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),
   '-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/ArduinoJson/src')]
  sources=['tools/test_lookup_overrides.cpp','src/LookupOverrides.cpp','src/UnknownCodes.cpp',
   'src/AirportNames.generated.cpp','src/AirlineNames.generated.cpp','src/AirportCoordinates.generated.cpp']
  subprocess.run(command+[str(root/file) for file in sources]+['-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)

with tempfile.TemporaryDirectory(prefix='flightwall-lookup-storage-') as folder:
 for variant in (0,1):
  binary=Path(folder)/f'storage-{variant}'
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',
   f'-DFLIGHTWALL_HAS_MATRIX={variant}','-I'+str(root/'tools/test_support/lookup_storage'),
   '-I'+str(root/'include'),str(root/'tools/test_lookup_storage.cpp'),
   str(root/'src/LookupStorage.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
