#!/usr/bin/env python3
"""Exercise the actual HTML helper used by both journal tables."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/main.cpp').read_text()
def function(signature):
 start=source.index(signature+' {')
 return source[start:source.index('\n}\n',start)+3]
assert source.count('journalFlightHtml(entry.flight, entry.hex)')==2
code='''#include <Arduino.h>
#include <cassert>
'''+function('String htmlEscape(String value)')+function('String journalFlightHtml(const String &flight, const String &hex)')+r'''
int main() {
 assert(journalFlightHtml("DLH072","abc123")==String("<a class=\"flightAwareLink\" href=\"https://www.flightaware.com/live/flight/DLH072\" target=\"_blank\" rel=\"noopener noreferrer\">DLH072</a>"));
 assert(journalFlightHtml("D-ABCD","").indexOf('%')==-1);
 assert(journalFlightHtml("", "abc123")==String("abc123"));
 assert(journalFlightHtml("  ", "<unknown>")==String("&lt;unknown&gt;"));
 String injected=journalFlightHtml("A/\"?&#<", "");
 assert(injected==String("<a class=\"flightAwareLink\" href=\"https://www.flightaware.com/live/flight/A%2F%22%3F%26%23%3C\" target=\"_blank\" rel=\"noopener noreferrer\">A/&quot;?&amp;#&lt;</a>"));
 assert(journalFlightHtml("A/B","")==String("<a class=\"flightAwareLink\" href=\"https://www.flightaware.com/live/flight/A%2FB\" target=\"_blank\" rel=\"noopener noreferrer\">A/B</a>"));
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-journal-links-') as folder:
 cpp=Path(folder)/'test.cpp';cpp.write_text(code)
 for variant in (0,1):
  binary=Path(folder)/f'test-{variant}'
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',f'-DFLIGHTWALL_HAS_MATRIX={variant}','-I'+str(root/'tools/test_support'),str(cpp),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
print('Journal links: both variants passed callsign, registration, missing callsign and HTML/URL escaping')
