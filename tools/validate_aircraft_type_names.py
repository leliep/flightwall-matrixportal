#!/usr/bin/env python3
import json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((root/'data/aircraft_type_names.json').read_text())
assert data and list(data)==sorted(data)
for code,name in data.items():
    assert re.fullmatch('[A-Z0-9]{2,4}',code),code
    assert isinstance(name,str) and name.strip() and len(name)<=128,code
assert data['R44']=='Robinson R44'
print(f'Aircraft type fallback table is valid: {len(data)} entries')
