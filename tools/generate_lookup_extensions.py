#!/usr/bin/env python3
"""Build a manual-upload lookup package from the maintained source tables."""
import argparse
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--version', required=True, help='human-readable dataset version (1–64 characters)')
parser.add_argument('--airports', nargs='*', default=[])
parser.add_argument('--airlines', nargs='*', default=[])
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
if not 1 <= len(args.version) <= 64 or args.version.strip() != args.version or any(ord(c)<32 or 127<=ord(c)<=159 for c in args.version):
    parser.error('invalid version')
names = json.loads((ROOT/'data/airport_names.de.json').read_text())
coordinates = json.loads((ROOT/'data/airport_coordinates.json').read_text())
airlines = json.loads((ROOT/'data/airline_names.de.json').read_text())
airport_codes = sorted(set(code.strip().upper() for code in args.airports))
airline_codes = sorted(set(code.strip().upper() for code in args.airlines))
if len(airport_codes)>256 or len(airline_codes)>256:
    parser.error('maximum 256 entries per table')
if any(code not in names or code not in coordinates for code in airport_codes):
    parser.error('unknown airport; extend and validate the source tables first')
if any(code not in airlines for code in airline_codes):
    parser.error('unknown airline; extend and validate the source table first')
data = {'format':'flightwall-lookups-v1','version':args.version,
        'airports':[dict(code=code,name=names[code],**coordinates[code]) for code in airport_codes],
        'airlines':[dict(code=code,name=airlines[code]) for code in airline_codes]}
text = json.dumps(data,ensure_ascii=False,indent=2)+'\n'
if len(text.encode())>65536:
    parser.error('file exceeds 65536 bytes')
args.output.write_text(text,encoding='utf-8')
print(f'Wrote {len(airport_codes)} airports and {len(airline_codes)} airlines to {args.output}')
