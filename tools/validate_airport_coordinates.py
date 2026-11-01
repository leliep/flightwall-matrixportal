#!/usr/bin/env python3
"""Validate the generated airport-coordinate source data."""

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
names = json.loads((ROOT / "data/airport_names.de.json").read_text(encoding="utf-8"))
coordinates = json.loads(
    (ROOT / "data/airport_coordinates.json").read_text(encoding="utf-8")
)

assert list(coordinates) == sorted(coordinates), "airport coordinates must stay sorted"
assert set(coordinates) == set(names), "coordinate and display-name codes differ"
for code, position in coordinates.items():
    latitude = position["latitude"]
    longitude = position["longitude"]
    elevation = position["elevation_ft"]
    assert isinstance(latitude, (int, float)) and -90 <= latitude <= 90, code
    assert isinstance(longitude, (int, float)) and -180 <= longitude <= 180, code
    assert elevation is None or isinstance(elevation, int), code

expected = {
    "DUS": (51.0, 52.0, 6.0, 7.5),
    "FRA": (49.0, 51.0, 7.5, 9.5),
    "PMI": (39.0, 40.0, 2.0, 3.5),
    "TFS": (27.0, 29.0, -18.0, -15.0),
}
for code, (lat_min, lat_max, lon_min, lon_max) in expected.items():
    position = coordinates[code]
    assert lat_min <= position["latitude"] <= lat_max, code
    assert lon_min <= position["longitude"] <= lon_max, code

print(f"Airport coordinate table is valid: {len(coordinates)} entries")
