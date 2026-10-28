#!/usr/bin/env python3
"""Validate the static airport display-name translation table."""

import json
import re
from pathlib import Path


SOURCE = Path(__file__).resolve().parents[1] / "data" / "airport_names.de.json"

with SOURCE.open(encoding="utf-8") as handle:
    airports = json.load(handle)

assert isinstance(airports, dict), "airport table must be a JSON object"
assert len(airports) >= 320, "airport table unexpectedly incomplete"
assert list(airports) == sorted(airports), "airport codes must stay sorted"

for code, name in airports.items():
    # Route providers normally return three-letter IATA codes, while local
    # and military movements can legitimately expose four-letter ICAO codes.
    assert re.fullmatch(r"[A-Z]{3,4}", code), f"invalid airport code: {code!r}"
    assert isinstance(name, str) and name.strip() == name and name, (
        f"invalid display name for {code}"
    )
    assert len(name) <= 28, f"display name too long for matrix: {code}={name!r}"

expected = {
    "DUS": "Düsseldorf",
    "CGN": "Köln/Bonn",
    "PMI": "Palma de Mallorca",
    "LHR": "London-Heathrow",
    "SAW": "Istanbul-Sabiha Gökçen",
    "TFS": "Teneriffa Süd",
    "ORD": "Chicago-O’Hare",
    "YYZ": "Toronto",
    "ICN": "Seoul-Incheon",
    "HKG": "Hongkong",
    "SIN": "Singapur",
}
for code, name in expected.items():
    assert airports.get(code) == name, f"unexpected translation: {code}"

print(f"Airport translation table is valid: {len(airports)} entries")
