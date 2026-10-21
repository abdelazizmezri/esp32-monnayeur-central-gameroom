#!/usr/bin/env python3
"""Run production services on a host with in-memory NVS, GPIO-free posts and LittleFS."""
import argparse
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--arduinojson', type=pathlib.Path, default=pathlib.Path.home() / 'Documents/Arduino/libraries/ArduinoJson/src')
args = parser.parse_args()
if not (args.arduinojson / 'ArduinoJson.h').is_file():
    parser.error('Supply --arduinojson /path/to/ArduinoJson/src (version 7).')
with tempfile.TemporaryDirectory(prefix='gameroom-pricing-') as temp:
    binary = pathlib.Path(temp) / 'pricing_test'
    subprocess.run(['c++', '-std=c++17', '-Wno-deprecated-declarations', '-I'+str(root/'tests/host/stubs'),
                    '-I'+str(root/'central-esp32'), '-I'+str(args.arduinojson),
                    str(root/'tests/host/pricing_test.cpp'),
                    *[str(root/'central-esp32'/name) for name in ['StorageService.cpp','PostService.cpp','RevenueService.cpp']],
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
