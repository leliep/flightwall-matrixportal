#!/usr/bin/env python3
import json
import re
from pathlib import Path

path = Path(__file__).resolve().parents[1] / "data/airline_names.de.json"
names = json.loads(path.read_text())
assert isinstance(names, dict) and names, "Airline-Tabelle ist leer"
for code, name in names.items():
    assert re.fullmatch(r"[A-Z0-9]{2,4}", code), f"Ungültiger Code: {code}"
    assert isinstance(name, str) and name.strip(), f"Name fehlt: {code}"
assert names.get("LH") == "Lufthansa" and names.get("DLH") == "Lufthansa"
print(f"Airline translation table is valid: {len(names)} entries")
