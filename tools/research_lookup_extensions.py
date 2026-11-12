#!/usr/bin/env python3
"""Read controller lookups, research unknown codes and prepare a reviewed package.

GET requests only. Passwords are prompted, never persisted. No upload/restart API.
"""
import argparse
import base64
import csv
from datetime import datetime
import getpass
import hashlib
from html.parser import HTMLParser
import io
import json
import math
from pathlib import Path
import re
import tempfile
import urllib.error
import urllib.parse
import urllib.request
import warnings
from zoneinfo import ZoneInfo

ROOT = Path(__file__).resolve().parents[1]
AIRPORT_URL = 'https://davidmegginson.github.io/ourairports-data/airports.csv'
AIRLINE_URL = 'https://www.faa.gov/air_traffic/publications/atpubs/cnt_html/chap3_section_3.html'
DEFAULT_CONTROLLERS = ['http://192.168.10.105', 'http://192.168.10.189']


def now():
    return datetime.now(ZoneInfo('Europe/Berlin')).isoformat(timespec='seconds')


def decode_json(raw):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError('duplicate JSON field: ' + key)
            result[key] = value
        return result
    return json.loads(raw, object_pairs_hook=unique,
                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError('nonfinite number')))


def valid_text(value, maximum):
    return (isinstance(value, str) and 1 <= len(value) <= maximum
            and value.strip() == value
            and not any(ord(c) < 32 or 127 <= ord(c) <= 159 or 0xD800 <= ord(c) <= 0xDFFF for c in value))


def validate_package(data):
    if (not isinstance(data, dict) or set(data) != {'format', 'version', 'airports', 'airlines'}
            or data['format'] != 'flightwall-lookups-v1' or not valid_text(data['version'], 64)):
        raise ValueError('invalid lookup format/version')
    for table in ('airports', 'airlines'):
        rows = data[table]
        if not isinstance(rows, list) or len(rows) > 256:
            raise ValueError('invalid/oversize table: ' + table)
        codes = set()
        for row in rows:
            fields = {'code', 'name', 'latitude', 'longitude', 'elevation_ft'} if table == 'airports' else {'code', 'name'}
            minimum, length = (3, 28) if table == 'airports' else (2, 64)
            if (not isinstance(row, dict) or set(row) != fields
                    or not isinstance(row['code'], str)
                    or not re.fullmatch('[A-Z0-9]{%d,4}' % minimum, row['code'])
                    or row['code'] in codes or not valid_text(row['name'], length)):
                raise ValueError('invalid/duplicate entry in ' + table)
            codes.add(row['code'])
            if table == 'airports':
                for field, bound in [('latitude', 90), ('longitude', 180)]:
                    value = row[field]
                    if type(value) not in (int, float) or not math.isfinite(value) or abs(value) > bound:
                        raise ValueError('invalid coordinate: ' + row['code'])
                elevation = row['elevation_ft']
                if elevation is not None and (type(elevation) is not int or not -1500 <= elevation <= 30000):
                    raise ValueError('invalid elevation: ' + row['code'])
    if len(package_bytes(data)) > 65536:
        raise ValueError('lookup file exceeds 65536 bytes')
    return data


def package_bytes(data):
    return (json.dumps(data, ensure_ascii=False, indent=2) + '\n').encode('utf-8')


def controller_url(value):
    parsed = urllib.parse.urlsplit(value if '://' in value else 'http://' + value)
    if (parsed.scheme not in ('http', 'https') or not parsed.hostname or parsed.username is not None
            or parsed.password is not None or parsed.query or parsed.fragment or parsed.path not in ('', '/')):
        raise ValueError('controller must be a host URL without credentials, query or path')
    return urllib.parse.urlunsplit((parsed.scheme, parsed.netloc, '', '', ''))


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None  # Never forward controller credentials to a different URL.


def fetch(url, password=None, controller=False):
    headers = {'User-Agent': 'FlightWall-Lookup-Research/1'}
    if password is not None:
        headers['Authorization'] = 'Basic ' + base64.b64encode(('admin:' + password).encode()).decode()
    request = urllib.request.Request(url, headers=headers, method='GET')
    opener = urllib.request.build_opener(NoRedirect()) if controller else urllib.request.build_opener()
    limit = (65536 if url.endswith('/lookups.json') else 262144) if controller else 25 * 1024 * 1024
    with opener.open(request, timeout=30) as response:
        data = response.read(limit + 1)
    if len(data) > limit:
        raise ValueError('response exceeds size limit')
    return data


def safe_error(exc):
    if isinstance(exc, urllib.error.HTTPError):
        return 'HTTP ' + str(exc.code)
    return type(exc).__name__  # Do not echo authentication/transport exception details.


class FAARows(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.rows, self.row, self.cell = [], [], None

    def handle_starttag(self, tag, attrs):
        if tag == 'tr':
            self.row = []
        if tag in ('td', 'th'):
            self.cell = []

    def handle_data(self, data):
        if self.cell is not None:
            self.cell.append(data)

    def handle_endtag(self, tag):
        if tag in ('td', 'th') and self.cell is not None:
            self.row.append(' '.join(''.join(self.cell).split()))
            self.cell = None
        if tag == 'tr' and len(self.row) == 4 and re.fullmatch('[A-Z]{3}', self.row[0]):
            self.rows.append(dict(zip(('code', 'company', 'country', 'telephony'), self.row)))


class Sources:
    def __init__(self, airport_file=None, airline_file=None):
        self.files = {'airports': airport_file, 'airlines': airline_file}
        self.loaded, self.provenance = {}, {}

    def load(self, table):
        if table in self.loaded:
            return self.loaded[table]
        path = self.files[table]
        url = AIRPORT_URL if table == 'airports' else AIRLINE_URL
        raw = Path(path).read_bytes() if path else fetch(url)
        if len(raw) > 25 * 1024 * 1024:
            raise ValueError('source exceeds size limit')
        self.provenance[table] = {'url': url, 'retrieved_at': now(),
                                  'sha256': hashlib.sha256(raw).hexdigest(),
                                  'input': 'local snapshot' if path else 'live download'}
        if table == 'airports':
            reader = csv.DictReader(io.StringIO(raw.decode('utf-8-sig')))
            fields = {'id', 'ident', 'iata_code', 'icao_code', 'gps_code', 'type', 'name',
                      'latitude_deg', 'longitude_deg', 'elevation_ft'}
            if not fields <= set(reader.fieldnames or []):
                raise ValueError('OurAirports schema changed')
            rows = list(reader)
        else:
            parser = FAARows()
            parser.feed(raw.decode('utf-8'))
            rows = parser.rows
        if not rows or (not path and len(rows) < 100):
            raise ValueError('source empty or structure changed')
        self.loaded[table] = rows
        return rows

    def resolve(self, table, code):
        rows = self.load(table)
        if table == 'airports':
            matches = {json.dumps(row, sort_keys=True): row for row in rows
                       if code in [row[field].strip().upper() for field in ('ident', 'iata_code', 'icao_code', 'gps_code')]}
            candidates = list(matches.values())
        else:
            candidates = [row for row in rows if row['code'] == code]
            candidates = list({json.dumps(row, sort_keys=True): row for row in candidates}.values())
        source = dict(self.provenance[table], records=candidates)
        if len(candidates) != 1:
            return None, source, 'not found' if not candidates else 'ambiguous source records'
        row = candidates[0]
        if table == 'airlines':
            result = {'code': code, 'name': row['company']}
        else:
            if row['type'] == 'closed':
                return None, source, 'airport is marked closed; manual review required'
            name = row['name']
            # Remove only a generic terminal suffix; never truncate identifying text.
            if len(name) > 28:
                name = re.sub(r' (International Airport|Airport|Airfield|Aerodrome)$', '', name)
            try:
                elevation = float(row['elevation_ft']) if row['elevation_ft'] else None
                if elevation is not None and (not math.isfinite(elevation) or not elevation.is_integer()):
                    raise ValueError('nonintegral elevation')
                result = {'code': code, 'name': name, 'latitude': float(row['latitude_deg']),
                          'longitude': float(row['longitude_deg']),
                          'elevation_ft': int(elevation) if elevation is not None else None}
            except (ValueError, OverflowError):
                return None, source, 'invalid source coordinates/elevation'
            if name != row['name']:
                source['display_name_adjustment'] = {'original': row['name'], 'display': name,
                                                      'reason': 'generic airport suffix removed'}
        try:
            validate_package({'format': 'flightwall-lookups-v1', 'version': 'check',
                              'airports': [result] if table == 'airports' else [],
                              'airlines': [result] if table == 'airlines' else []})
        except ValueError as exc:
            return None, source, str(exc)
        return result, source, None


def merge_existing(snapshots, report):
    merged = {'airports': {}, 'airlines': {}}
    for table in merged:
        all_codes = sorted({row['code'] for snapshot in snapshots for row in snapshot['lookups'][table]})
        for code in all_codes:
            versions = []
            for snapshot in snapshots:
                for row in snapshot['lookups'][table]:
                    if row['code'] == code:
                        versions.append({'controller': snapshot['controller'], 'entry': row})
            if any(value['entry'] != versions[0]['entry'] for value in versions):
                report['conflicts'].append({'table': table, 'code': code, 'versions': versions})
            else:
                merged[table][code] = versions[0]['entry']
                report['preserved'].append({'table': table, 'code': code, 'sources': versions})
    return merged


def assemble(snapshots, sources, version, local_root=None):
    local_root = local_root or ROOT
    report = {'created_at': now(), 'status': 'blocked', 'controllers': [], 'preserved': [],
              'additions': [], 'conflicts': [], 'unresolved': [], 'errors': []}
    for item in snapshots:
        validate_package(item['lookups'])
        report['controllers'].append({k: item[k] for k in ('controller', 'firmware', 'unknown_airports', 'unknown_airlines')})
        report['controllers'][-1]['lookup_version'] = item['lookups']['version']
    merged = merge_existing(snapshots, report)
    local = {}
    for table, filename in [('airports', 'airport_names.de.json'), ('airlines', 'airline_names.de.json')]:
        local[table] = decode_json((local_root/'data'/filename).read_bytes())
    coordinates = decode_json((local_root/'data/airport_coordinates.json').read_bytes())
    for table in merged:
        unknown = sorted({code for item in snapshots for code in item['unknown_' + table]})
        for code in unknown:
            minimum = 3 if table == 'airports' else 2
            if not isinstance(code, str) or not re.fullmatch('[A-Z0-9]{%d,4}' % minimum, code):
                report['unresolved'].append({'table': table, 'code': code, 'reason': 'invalid observed code'})
                continue
            if code in merged[table] or any(x['table'] == table and x['code'] == code for x in report['conflicts']):
                continue
            if code in local[table] and (table != 'airports' or code in coordinates):
                row = dict(code=code, name=local[table][code])
                filenames = ['data/' + ('airport_names.de.json' if table == 'airports' else 'airline_names.de.json')]
                if table == 'airports':
                    row.update(coordinates[code])
                    filenames.append('data/airport_coordinates.json')
                provenance = {'files': [{'path': path, 'sha256': hashlib.sha256((local_root/path).read_bytes()).hexdigest()} for path in filenames]}
                reason = None
            else:
                try:
                    row, provenance, reason = sources.resolve(table, code)
                except Exception as exc:
                    row, provenance, reason = None, {}, 'source lookup failed: ' + safe_error(exc)
            if row is None:
                report['unresolved'].append({'table': table, 'code': code, 'reason': reason, 'source': provenance})
            else:
                merged[table][code] = row
                report['additions'].append({'table': table, 'entry': row, 'source': provenance})
    package = {'format': 'flightwall-lookups-v1', 'version': version,
               **{table: [rows[code] for code in sorted(rows)] for table, rows in merged.items()}}
    try:
        validate_package(package)
    except ValueError as exc:
        report['errors'].append(str(exc))
    if not any(report[key] for key in ('errors', 'conflicts', 'unresolved')):
        report['status'] = 'ready'
    return package, report


def atomic_write(path, raw):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as file:
        temporary = Path(file.name)
        file.write(raw)
    try:
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)


def markdown(report):
    lines = ['# Lookup-Prüfbericht', '', 'Status: **' + report['status'] + '**', '',
             'Erstellt: ' + report['created_at'], '',
             'Nur lesende Abfragen; kein Upload und kein Neustart.', '',
             '## Controller', '']
    for item in report['controllers']:
        lines.append('- ' + item['controller'] + ': Firmware ' + item['firmware'] +
                     ', Lookup-Version ' + item['lookup_version'])
    lines += ['', '## Erhaltene Ergänzungen', '']
    for item in report['preserved']:
        lines.append('- ' + item['table'] + ' / ' + item['code'] + ': ' + ', '.join(x['controller'] for x in item['sources']))
    lines += ['', '## Recherchierte Ergänzungen und Quellen', '']
    for item in report['additions']:
        row, source = item['entry'], item['source']
        lines.append('- ' + item['table'] + ' / ' + row['code'] + ': **' + row['name'] + '**')
        if 'url' in source:
            lines.append('  Quelle: ' + source['url'])
        else:
            lines.append('  Quelle: ' + ', '.join(x['path'] for x in source['files']))
        if 'latitude' in row:
            lines.append('  Koordinaten: ' + str(row['latitude']) + ', ' + str(row['longitude']) + '; Höhe: ' + str(row['elevation_ft']) + ' ft')
        if 'display_name_adjustment' in source:
            lines.append('  Anzeigename gekürzt: ' + json.dumps(source['display_name_adjustment'], ensure_ascii=False))
    lines += ['', '## Konflikte', '']
    for item in report['conflicts']:
        lines.append('- ' + item['table'] + ' / ' + item['code'] + ': ' + json.dumps(item['versions'], ensure_ascii=False))
    if not report['conflicts']:
        lines.append('Keine Konflikte.')
    lines += ['', '## Offene Punkte', '']
    lines += ['- ' + json.dumps(item, ensure_ascii=False) for item in report['unresolved']]
    lines += ['- ' + item for item in report['errors']]
    if not report['unresolved'] and not report['errors']:
        lines.append('Keine offenen Punkte.')
    lines += ['', 'Upload-Datei erzeugt: ' + str(report.get('output_written', False)), '',
              'Die JSON-Fassung enthält vollständige Einträge, Quellzeilen und Prüfsummen.', '']
    return '\n'.join(lines)


def main(argv=None, password_provider=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--controller', action='append', help='host URL; repeat for both devices')
    parser.add_argument('--version', default=datetime.now(ZoneInfo('Europe/Berlin')).strftime('%Y-%m-%d-%H%M%S'))
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True, help='JSON report; companion .md is also written')
    parser.add_argument('--airports-csv', type=Path, help='optional OurAirports snapshot for offline tests')
    parser.add_argument('--airlines-html', type=Path, help='optional FAA section 3 HTML snapshot')
    args = parser.parse_args(argv)
    if args.report.suffix != '.json' or len({args.output.resolve(), args.report.resolve(), args.report.with_suffix('.md').resolve()}) != 3:
        parser.error('output/report paths must differ; report must end in .json')
    if not valid_text(args.version, 64):
        parser.error('invalid dataset version')
    try:
        controllers = [controller_url(value) for value in (args.controller or DEFAULT_CONTROLLERS)]
        if len(set(controllers)) != len(controllers):
            raise ValueError('duplicate controller URL')
    except ValueError as exc:
        parser.error(str(exc))
    snapshots, errors = [], []
    for controller in controllers:
        try:
            status = decode_json(fetch(controller + '/api/status', controller=True))
            prompt = 'OTA-Passwort für ' + controller + ': '
            if password_provider:
                password = password_provider(prompt)
            else:
                with warnings.catch_warnings():
                    warnings.simplefilter('error', getpass.GetPassWarning)
                    password = getpass.getpass(prompt)
            lookups = decode_json(fetch(controller + '/lookups.json', password=password, controller=True))
            del password
            validate_package(lookups)
            for field in ('unknown_airports', 'unknown_airlines'):
                if not isinstance(status[field], list) or not all(isinstance(x, str) for x in status[field]):
                    raise ValueError('invalid unknown-code status')
            if status.get('dropped_unknown_airports', 0) or status.get('dropped_unknown_airlines', 0):
                raise ValueError('unknown-code list overflow; incomplete inventory')
            snapshots.append(dict(controller=controller, firmware=status['version'], lookups=lookups,
                                  unknown_airports=status['unknown_airports'], unknown_airlines=status['unknown_airlines']))
        except Exception as exc:
            errors.append(controller + ': ' + safe_error(exc))
    if errors:
        report = {'created_at': now(), 'status': 'blocked', 'controllers': [{k: item[k] for k in ('controller', 'firmware', 'unknown_airports', 'unknown_airlines')}
                                  | {'lookup_version': item['lookups']['version']} for item in snapshots],
                  'preserved': [], 'additions': [], 'conflicts': [], 'unresolved': [], 'errors': errors}
        merge_existing(snapshots, report)
        package = None
    else:
        try:
            package, report = assemble(snapshots, Sources(args.airports_csv, args.airlines_html), args.version)
        except Exception as exc:
            package = None
            report = {'created_at': now(), 'status': 'blocked', 'controllers': [], 'preserved': [],
                      'additions': [], 'conflicts': [], 'unresolved': [], 'errors': ['assembly failed: ' + safe_error(exc)]}
    report['output_written'] = report['status'] == 'ready'
    if report['output_written']:
        raw = package_bytes(package)
        atomic_write(args.output, raw)
        report['output'] = {'file': str(args.output), 'sha256': hashlib.sha256(raw).hexdigest(), 'bytes': len(raw)}
    atomic_write(args.report, package_bytes(report))
    atomic_write(args.report.with_suffix('.md'), markdown(report).encode())
    print('Lookup research: ' + report['status'] + '; preserved ' + str(len(report['preserved'])) +
          ', added ' + str(len(report['additions'])) + ', conflicts ' + str(len(report['conflicts'])) +
          ', unresolved ' + str(len(report['unresolved'])) + '. No upload performed.')
    return 0 if report['status'] == 'ready' else 2


if __name__ == '__main__':
    raise SystemExit(main())
