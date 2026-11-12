#!/usr/bin/env python3
"""Research/merge and read-only controller integration regression tests."""
import base64
import contextlib
import copy
import csv
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import io
import json
from pathlib import Path
import tempfile
import threading
import unittest
from unittest.mock import patch
import research_lookup_extensions as r


def empty():
    return dict(format='flightwall-lookups-v1', version='old', airports=[], airlines=[])


def snapshot(name='matrix', airports=None, airlines=None, lookups=None):
    return dict(controller=name, firmware='test', unknown_airports=airports or [],
                unknown_airlines=airlines or [], lookups=lookups or empty())


class Tests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        (self.root/'data').mkdir()
        for file in ('airport_names.de.json', 'airport_coordinates.json', 'airline_names.de.json'):
            (self.root/'data'/file).write_text('{}')
        self.csv = self.root/'airports.csv'
        self.html = self.root/'airlines.html'
        self.rows = [dict(id='1', ident='ZLXY', iata_code='XIY', icao_code='ZLXY', gps_code='ZLXY',
                         type='large_airport', name="Xi'an Xianyang International Airport",
                         latitude_deg='34.4', longitude_deg='108.7', elevation_ft='1572')]
        self.write_csv()
        self.html.write_text('<table><tr><th>3-Ltr</th><th>Company</th><th>Country</th><th>Telephony</th></tr>'
                             '<tr><td><p>ELG</p></td><td>ELANGENI GMBH</td><td>AUSTRIA</td><td>ELANGENI</td></tr></table>')
        self.sources = r.Sources(self.csv, self.html)

    def tearDown(self):
        self.temp.cleanup()

    def write_csv(self):
        with self.csv.open('w', newline='') as file:
            writer = csv.DictWriter(file, fieldnames=list(self.rows[0]))
            writer.writeheader()
            writer.writerows(self.rows)

    def test_external_sources_and_preservation(self):
        old = empty()
        old['airlines'] = [dict(code='DLH', name='My Lufthansa')]
        snapshots = [snapshot(airports=['XIY'], lookups=old), snapshot('headless', airlines=['ELG'])]
        before = copy.deepcopy(snapshots)
        package, report = r.assemble(snapshots, self.sources, 'new', self.root)
        self.assertEqual(report['status'], 'ready')
        self.assertEqual(snapshots, before)
        self.assertIn(old['airlines'][0], package['airlines'])
        self.assertEqual(package['airports'][0]['name'], "Xi'an Xianyang")
        self.assertIn('display_name_adjustment', report['additions'][0]['source'])
        self.assertTrue(all(x['source']['sha256'] for x in report['additions']))
        self.assertEqual(r.validate_package(package), package)

    def test_conflict_is_not_overwritten(self):
        a, b = empty(), empty()
        a['airlines'] = [dict(code='DLH', name='First')]
        b['airlines'] = [dict(code='DLH', name='Second')]
        _, report = r.assemble([snapshot(lookups=a), snapshot('headless', lookups=b)], self.sources, 'new', self.root)
        self.assertEqual(report['status'], 'blocked')
        self.assertEqual([x['entry']['name'] for x in report['conflicts'][0]['versions']], ['First', 'Second'])

    def test_identical_existing_entries_merge(self):
        old = empty()
        old['airlines'] = [dict(code='DLH', name='Existing')]
        package, report = r.assemble([snapshot(lookups=old), snapshot('headless', lookups=old)], self.sources, 'new', self.root)
        self.assertEqual(package['airlines'], old['airlines'])
        self.assertEqual(len(report['preserved'][0]['sources']), 2)

    def test_ambiguous_or_closed_airport(self):
        self.rows.append(dict(self.rows[0], id='2', longitude_deg='9'))
        self.write_csv()
        _, report = r.assemble([snapshot(airports=['XIY'])], self.sources, 'new', self.root)
        self.assertEqual(report['status'], 'blocked')
        self.assertEqual(len(report['unresolved'][0]['source']['records']), 2)
        self.rows = [dict(self.rows[0], type='closed')]
        self.write_csv()
        _, report = r.assemble([snapshot(airports=['XIY'])], r.Sources(self.csv, self.html), 'new', self.root)
        self.assertIn('closed', report['unresolved'][0]['reason'])

    def test_missing_data_and_long_names_block(self):
        _, report = r.assemble([snapshot(airlines=['XX'])], self.sources, 'new', self.root)
        self.assertEqual(report['status'], 'blocked')
        self.rows[0]['name'] = 'X'*29
        self.write_csv()
        _, report = r.assemble([snapshot(airports=['XIY'])], self.sources, 'new', self.root)
        self.assertEqual(report['status'], 'blocked')

    def test_local_sources_and_limits(self):
        (self.root/'data/airline_names.de.json').write_text('{"ELG":"Local name"}')
        package, report = r.assemble([snapshot(airlines=['ELG'])], self.sources, 'new', self.root)
        self.assertEqual(package['airlines'][0]['name'], 'Local name')
        self.assertIn('files', report['additions'][0]['source'])
        invalid = empty()
        invalid['airlines'] = [dict(code=f'{i:04}', name='Name') for i in range(257)]
        with self.assertRaises(ValueError):
            r.validate_package(invalid)
        with self.assertRaises(ValueError):
            r.decode_json('{"version":1,"version":2}')
        with self.assertRaises(ValueError):
            r.controller_url('http://admin:password@localhost')

    def test_bad_coordinates_keep_source_evidence(self):
        self.rows[0]['latitude_deg'] = 'not-a-number'
        self.write_csv()
        _, report = r.assemble([snapshot(airports=['XIY'])], self.sources, 'new', self.root)
        self.assertEqual(report['status'], 'blocked')
        self.assertEqual(report['unresolved'][0]['source']['records'][0]['latitude_deg'], 'not-a-number')

    def test_source_failure_and_conflict_never_publish(self):
        with patch.object(self.sources, 'load', side_effect=OSError('do not log secrets')):
            _, report = r.assemble([snapshot(airports=['XIY'])], self.sources, 'new', self.root)
        self.assertEqual(report['status'], 'blocked')
        self.assertNotIn('do not log secrets', json.dumps(report))
        a, b = empty(), empty()
        a['airlines'] = [dict(code='DLH', name='First')]
        b['airlines'] = [dict(code='DLH', name='Second')]
        with self.controller(lookups=a) as first, self.controller(lookups=b) as second:
            with contextlib.redirect_stdout(io.StringIO()):
                result = r.main(self.args([first[0], second[0]]), password_provider=lambda _: first[2])
        self.assertEqual(result, 2)
        self.assertFalse((self.root/'result.json').exists())
        self.assertEqual(len(json.loads((self.root/'review.json').read_text())['conflicts']), 1)

    @contextlib.contextmanager
    def controller(self, unknown_airports=None, unknown_airlines=None, lookups=None, redirect=False):
        paths = []
        secret = 'TEST-SECRET-NEVER-REPORT'
        auth = 'Basic ' + base64.b64encode(('admin:' + secret).encode()).decode()
        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass
            def do_POST(self):
                raise AssertionError('research must never POST')
            def do_GET(self):
                paths.append(self.path)
                if self.path == '/api/status':
                    data = dict(version='test', unknown_airports=unknown_airports or [], unknown_airlines=unknown_airlines or [])
                elif self.path == '/lookups.json':
                    if redirect:
                        self.send_response(302)
                        self.send_header('Location', '/credentials-must-not-follow')
                        self.end_headers()
                        return
                    if self.headers.get('Authorization') != auth:
                        self.send_response(401)
                        self.end_headers()
                        return
                    data = lookups or empty()
                else:
                    raise AssertionError('unexpected endpoint: ' + self.path)
                raw = json.dumps(data).encode()
                self.send_response(200)
                self.send_header('Content-Length', str(len(raw)))
                self.end_headers()
                self.wfile.write(raw)
        server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            yield 'http://127.0.0.1:' + str(server.server_port), paths, secret
        finally:
            server.shutdown()
            server.server_close()
            thread.join()

    def args(self, controllers):
        return [item for host in controllers for item in ('--controller', host)] + [
            '--version', 'new', '--output', str(self.root/'result.json'),
            '--report', str(self.root/'review.json'), '--airports-csv', str(self.csv),
            '--airlines-html', str(self.html)]

    def test_two_controllers_get_only_and_no_secrets(self):
        old = empty()
        old['airlines'] = [dict(code='DLH', name='My Lufthansa')]
        with self.controller(['XIY'], lookups=old) as a, self.controller(unknown_airlines=['ELG']) as b:
            with patch.object(r, 'ROOT', self.root), contextlib.redirect_stdout(io.StringIO()):
                result = r.main(self.args([a[0], b[0]]), password_provider=lambda _: a[2])
            self.assertEqual(result, 0)
            self.assertEqual(a[1], ['/api/status', '/lookups.json'])
            self.assertEqual(b[1], ['/api/status', '/lookups.json'])
            package = json.loads((self.root/'result.json').read_text())
            self.assertIn(old['airlines'][0], package['airlines'])
            self.assertNotIn(a[2], (self.root/'review.json').read_text())
            self.assertNotIn(a[2], (self.root/'review.md').read_text())

    def test_missing_controller_does_not_publish_partial_or_lose_old_output(self):
        (self.root/'result.json').write_text('previous file')
        with self.controller(redirect=True) as a:
            with contextlib.redirect_stdout(io.StringIO()):
                result = r.main(self.args([a[0]]), password_provider=lambda _: a[2])
            self.assertEqual(result, 2)
            self.assertEqual(a[1], ['/api/status', '/lookups.json'])
            self.assertEqual((self.root/'result.json').read_text(), 'previous file')
            report = json.loads((self.root/'review.json').read_text())
            self.assertFalse(report['output_written'])
            self.assertNotIn(a[2], json.dumps(report))


if __name__ == '__main__':
    unittest.main()
