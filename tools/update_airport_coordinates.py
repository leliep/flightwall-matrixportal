#!/usr/bin/env python3
"""Refresh coordinates for the airports already known to FlightWall.

The complete OurAirports data set is intentionally not committed. This script
downloads (or reads) airports.csv and writes only the records referenced by the
display-name table.
"""

import argparse
import csv
import io
import json
from pathlib import Path
from typing import Dict, List, Optional, Union
from urllib.request import urlopen


ROOT = Path(__file__).resolve().parents[1]
NAMES = ROOT / "data" / "airport_names.de.json"
OUTPUT = ROOT / "data" / "airport_coordinates.json"
SOURCE_URL = "https://davidmegginson.github.io/ourairports-data/airports.csv"

# Metropolitan, historic, or locally-used aliases which are not exposed as a
# current IATA/ICAO code in OurAirports.
ALIASES = {
    "BNJ": "EDKB",     # Bonn-Hangelar
    "EAP": "LFSB",     # Basel metropolitan code -> EuroAirport
    "TXL": "DE-0876",  # closed Berlin-Tegel record
}


def read_rows(source: Optional[str]) -> List[Dict[str, str]]:
    if source:
        with Path(source).open(encoding="utf-8", newline="") as handle:
            return list(csv.DictReader(handle))
    with urlopen(SOURCE_URL, timeout=30) as response:
        text = io.TextIOWrapper(response, encoding="utf-8", newline="")
        return list(csv.DictReader(text))


def index_rows(
    rows: List[Dict[str, str]],
) -> Dict[str, Dict[str, List[Dict[str, str]]]]:
    result: Dict[str, Dict[str, List[Dict[str, str]]]] = {}
    for row in rows:
        for field in ("iata_code", "icao_code", "gps_code", "ident"):
            code = row[field].strip().upper()
            if code:
                result.setdefault(field, {}).setdefault(code, []).append(row)
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "csv_file", nargs="?", help="optional local OurAirports airports.csv"
    )
    args = parser.parse_args()

    names = json.loads(NAMES.read_text(encoding="utf-8"))
    index = index_rows(read_rows(args.csv_file))
    coordinates: Dict[str, Dict[str, Optional[Union[float, int]]]] = {}
    missing: List[str] = []

    for code in names:
        lookup = ALIASES.get(code, code)
        fields = ("iata_code", "icao_code", "gps_code", "ident")
        if len(lookup) == 4:
            fields = ("icao_code", "gps_code", "ident", "iata_code")
        row = None
        for field in fields:
            unique = {item["id"]: item for item in index[field].get(lookup, [])}
            if len(unique) == 1:
                row = next(iter(unique.values()))
                break
        if row is None:
            missing.append(code)
            continue
        elevation = row["elevation_ft"].strip()
        coordinates[code] = {
            "latitude": round(float(row["latitude_deg"]), 6),
            "longitude": round(float(row["longitude_deg"]), 6),
            "elevation_ft": int(round(float(elevation))) if elevation else None,
        }

    if missing:
        raise SystemExit("No unique OurAirports record for: " + ", ".join(missing))

    OUTPUT.write_text(
        json.dumps(coordinates, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"Updated {len(coordinates)} airport coordinates from {SOURCE_URL}")


if __name__ == "__main__":
    main()
