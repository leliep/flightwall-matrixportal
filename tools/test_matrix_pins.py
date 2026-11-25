#!/usr/bin/env python3
"""Compile the real target-specific pin declarations, checking physical RGB lanes."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
s = (root / 'src/main.cpp').read_text()
a = s.index('#if FLIGHTWALL_MATRIX_HEIGHT == 64\n// This 128x64')
declarations = s[a:s.index('constexpr uint8_t clockPin', a)]
code = '#include <cstdint>\n#include <cassert>\n' + declarations + r'''
int main() {
#if FLIGHTWALL_MATRIX_HEIGHT == 64
 const uint8_t expectedRGB[] = {40,41,42,37,39,38};
 const uint8_t expectedAddress[] = {45,36,48,35,21};
#else
 const uint8_t expectedRGB[] = {42,41,40,38,39,37};
 const uint8_t expectedAddress[] = {45,36,48,35};
#endif
 static_assert(sizeof(rgbPins)==sizeof(expectedRGB), "RGB lane count");
 static_assert(sizeof(addrPins)==sizeof(expectedAddress), "Address lane count");
 for(unsigned i=0;i<sizeof(rgbPins);++i)assert(rgbPins[i]==expectedRGB[i]);
 for(unsigned i=0;i<sizeof(addrPins);++i)assert(addrPins[i]==expectedAddress[i]);
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-panel-pins-') as d:
 p = Path(d);(p/'test.cpp').write_text(code)
 for height in (32,64):
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic',
                  '-DFLIGHTWALL_MATRIX_HEIGHT='+str(height),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
  subprocess.run([str(p/'test')],check=True)
print('Physical RGB and address pins: 128x64 swaps only red/blue; 128x32 unchanged')
