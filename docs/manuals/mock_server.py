#!/usr/bin/env python3
"""Serveur local reproductible pour les captures des manuels.

Il sert directement les pages HTML embarquees dans le firmware, puis repond aux
API avec des donnees de demonstration. Aucune NVS, aucun ESP32 et aucune donnee
de l'installation reelle ne sont lus ou modifies.
"""

from __future__ import annotations

import json
import mimetypes
import re
from datetime import date, timedelta
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse


ROOT = Path(__file__).resolve().parents[2]
CENTRAL = ROOT / "central-esp32"
POSTE = ROOT / "poste-esp32"
HOST = "127.0.0.1"
PORT = 8765


def extract_page(function_name: str) -> str:
    source = (CENTRAL / "WebPage.cpp").read_text(encoding="utf-8")
    match = re.search(
        rf"const char\*\s+{re.escape(function_name)}\(\)\s*\{{\s*"
        rf"return R\"rawliteral\((.*?)\)rawliteral\";",
        source,
        re.DOTALL,
    )
    if not match:
        raise RuntimeError(f"Page {function_name} introuvable")
    return match.group(1)


def extract_wifi_page(source_path: Path) -> str:
    source = source_path.read_text(encoding="utf-8")
    start = source.index("static String buildWifiPage")
    end = source.index("return html;", start)
    chunks = re.findall(
        r"R\"rawliteral\((.*?)\)rawliteral\"", source[start:end], re.DOTALL
    )
    if len(chunks) < 4:
        raise RuntimeError(f"Portail Wi-Fi incomplet dans {source_path}")
    networks = "".join(
        (
            "<button class='network' type='button' "
            f"data-ssid=\"{name}\" onclick='pickNetwork(this)'>"
            f"<span class='radio'></span><b>{name}</b><small>{rssi} dBm</small></button>"
        )
        for name, rssi in (
            ("RESEAU-DEMO-PRINCIPAL", -41),
            ("RESEAU-DEMO-INVITES", -58),
            ("RESEAU-MASQUE", -72),
        )
    )
    return chunks[0] + chunks[1] + networks + chunks[2] + chunks[-1]


LOGIN_HTML = extract_page("loginPage")
DASHBOARD_HTML = extract_page("dashboardPage")
CENTRAL_WIFI_HTML = extract_wifi_page(CENTRAL / "WifiProvisioning.cpp")
POSTE_WIFI_HTML = extract_wifi_page(POSTE / "WiFiConfigService.cpp")


DEMO_POSTS = [
    {
        "chipId": "DEMO-A001",
        "id": "poste-demo-1",
        "name": "Poste Démo 1",
        "ip": "192.168.50.31",
        "status": "active",
        "relay": True,
        "remaining": 872,
        "recoveryPending": False,
        "recoveryRemaining": 0,
    },
    {
        "chipId": "DEMO-A002",
        "id": "poste-demo-2",
        "name": "Poste Démo 2",
        "ip": "192.168.50.32",
        "status": "idle",
        "relay": False,
        "remaining": 0,
        "recoveryPending": False,
        "recoveryRemaining": 0,
    },
    {
        "chipId": "DEMO-A003",
        "id": "poste-demo-3",
        "name": "Poste Démo 3",
        "ip": "192.168.50.33",
        "status": "offline",
        "relay": False,
        "remaining": 0,
        "recoveryPending": False,
        "recoveryRemaining": 0,
    },
    {
        "chipId": "DEMO-A004",
        "id": "poste-demo-4",
        "name": "Poste Démo 4",
        "ip": "192.168.50.34",
        "status": "recovery_pending",
        "relay": False,
        "remaining": 0,
        "recoveryPending": True,
        "recoveryRemaining": 1268,
    },
]


def demo_posts(role: str) -> dict:
    is_admin = role == "admin"
    identity = (
        {
            "username": "admin-demo",
            "firstName": "Admin",
            "lastName": "Démo",
            "role": "admin",
        }
        if is_admin
        else {
            "username": "user-demo",
            "firstName": "Utilisateur",
            "lastName": "Démo",
            "role": "user",
        }
    )
    payload = {
        "posts": DEMO_POSTS,
        "pendingPosts": (
            [{"chipId": "DEMO-NOUVEAU-05", "ip": "192.168.50.35", "lastSeen": 12345}]
            if is_admin
            else []
        ),
        "availableCoins": 18,
        "coinDurationSeconds": 1800,
        "pulsesPerCoin": 1,
        "pulseValueMillimes": 1000,
        "pendingPulses": 0,
        "accessRole": role,
        "currentUser": identity,
    }
    if is_admin:
        payload.update(
            {
                "apiTokenMasked": "demo••••••••••••••••••••••••",
                "revenueStorageReady": True,
                "revenueTimeReady": True,
                "revenueStorageError": "",
            }
        )
    return payload


def demo_revenue(period: str = "day") -> dict:
    today = date(2026, 8, 23)
    if period == "month":
        points = [(today - timedelta(days=day), f"{23-day:02d}/08") for day in range(22, -1, -1)]
        label = "01/08/2026 - 23/08/2026"
    elif period == "week":
        points = [(today - timedelta(days=day), (today - timedelta(days=day)).strftime("%d/%m")) for day in range(6, -1, -1)]
        label = "17/08/2026 - 23/08/2026"
    else:
        points = [(today, f"{hour:02d} h") for hour in range(10, 23)]
        label = "23/08/2026"
    items = []
    for index, (day_value, item_label) in enumerate(points):
        pulses = (index * 3 + 2) % 8 + 1
        items.append(
            {
                "key": str(day_value),
                "label": item_label,
                "amountMillimes": pulses * 1000,
                "pulses": pulses,
                "credits": pulses,
                "events": pulses,
            }
        )
    return {
        "period": period,
        "rangeLabel": label,
        "startDate": str(points[0][0]),
        "endDate": str(points[-1][0]),
        "generatedAt": "2026-08-23 22:15:00",
        "storageReady": True,
        "timeReady": True,
        "storageError": "",
        "fileBytes": 6240,
        "storageUsedBytes": 58368,
        "storageTotalBytes": 655360,
        "totalMillimes": sum(item["amountMillimes"] for item in items),
        "undatedMillimes": 0,
        "pulses": sum(item["pulses"] for item in items),
        "credits": sum(item["credits"] for item in items),
        "events": sum(item["events"] for item in items),
        "undatedEvents": 0,
        "corruptRecords": 0,
        "items": items,
    }


DEMO_USERS = {
    "currentUsername": "admin-demo",
    "users": [
        {"username": "admin-demo", "firstName": "Admin", "lastName": "Démo", "role": "admin"},
        {"username": "user-demo", "firstName": "Utilisateur", "lastName": "Démo", "role": "user"},
        {"username": "responsable-demo", "firstName": "Responsable", "lastName": "Démo", "role": "admin"},
    ],
}


DEMO_LOGS = {
    "logs": [
        {"ts": 172000, "level": "INFO", "message": "Centrale démarrée - données de démonstration."},
        {"ts": 181000, "level": "INFO", "message": "Poste Démo 1 annoncé sur le réseau local."},
        {"ts": 196000, "level": "INFO", "message": "1 coin affecté à Poste Démo 1."},
        {"ts": 224000, "level": "WARN", "message": "Poste Démo 3 ne répond plus - état hors ligne."},
        {"ts": 241000, "level": "WARN", "message": "Reprise en attente détectée pour Poste Démo 4."},
        {"ts": 285000, "level": "INFO", "message": "Recette LittleFS mise à jour : 1,000 TND."},
        {"ts": 321000, "level": "INFO", "message": "Sauvegarde de démonstration prête à l'export."},
    ]
}


class DemoHandler(BaseHTTPRequestHandler):
    server_version = "PSTM-Manual-Demo/1.0"

    def log_message(self, fmt: str, *args: object) -> None:
        print(f"[demo] {self.address_string()} - {fmt % args}")

    def role(self) -> str:
        query_role = parse_qs(urlparse(self.path).query).get("role", [""])[0]
        if query_role in {"admin", "user"}:
            return query_role
        cookies = self.headers.get("Cookie", "")
        match = re.search(r"(?:^|;\s*)demo_role=(admin|user)", cookies)
        return match.group(1) if match else "admin"

    def send_bytes(self, body: bytes, content_type: str, status: int = 200, role: str | None = None) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        if role:
            self.send_header("Set-Cookie", f"demo_role={role}; Path=/; SameSite=Strict")
        self.end_headers()
        self.wfile.write(body)

    def send_text(self, text: str, content_type: str = "text/html; charset=utf-8", status: int = 200, role: str | None = None) -> None:
        self.send_bytes(text.encode("utf-8"), content_type, status, role)

    def send_json(self, payload: object, status: int = 200) -> None:
        self.send_text(json.dumps(payload, ensure_ascii=False), "application/json; charset=utf-8", status)

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        path = parsed.path
        query = parse_qs(parsed.query)
        role = self.role()

        if path == "/login":
            self.send_text(LOGIN_HTML)
            return
        if path == "/wifi-central":
            self.send_text(CENTRAL_WIFI_HTML)
            return
        if path == "/wifi-poste":
            self.send_text(POSTE_WIFI_HTML)
            return
        if path in {"/schema-centrale", "/schema-poste"}:
            schema_path = (
                "/schema-centrale.svg" if path == "/schema-centrale" else "/schema-poste.svg"
            )
            background = "#eef2f6" if path == "/schema-centrale" else "#07111f"
            self.send_text(
                "<!doctype html><html><head><meta charset='utf-8'><style>"
                "html,body{margin:0;width:100%;min-height:100%;background:"
                + background
                + ";}img{display:block;width:100vw;height:auto}</style></head><body>"
                f"<img src='{schema_path}' alt='Schéma de câblage'></body></html>"
            )
            return
        if path in {"/schema-centrale.svg", "/schema-poste.svg"}:
            schema = (
                CENTRAL / "docs" / "schema-cablage-central-final.svg"
                if path == "/schema-centrale.svg"
                else POSTE / "docs" / "schema-cablage-poste-tm1637.svg"
            )
            self.send_bytes(schema.read_bytes(), "image/svg+xml; charset=utf-8")
            return
        if path.startswith("/assets/"):
            asset_map = {
                "/assets/login-station-v4.webp": "login-station-embedded.webp",
                "/assets/game-room-logo-v5.webp": "game-room-logo-embedded.webp",
                "/assets/playstation-mark-v2.webp": "playstation-mark.webp",
            }
            filename = asset_map.get(path)
            if not filename:
                self.send_error(HTTPStatus.NOT_FOUND)
                return
            asset = CENTRAL / "ui-assets" / filename
            self.send_bytes(asset.read_bytes(), mimetypes.guess_type(asset.name)[0] or "application/octet-stream")
            return
        if path == "/posts":
            self.send_json(demo_posts(role))
            return
        if path == "/users/data":
            self.send_json(DEMO_USERS)
            return
        if path == "/logs/data":
            self.send_json(DEMO_LOGS)
            return
        if path == "/revenue/data":
            self.send_json(demo_revenue(query.get("period", ["day"])[0]))
            return
        if path == "/wifi/scan":
            self.send_json(
                {
                    "networks": [
                        {"ssid": "RESEAU-DEMO-PRINCIPAL", "rssi": -41},
                        {"ssid": "RESEAU-DEMO-INVITES", "rssi": -58},
                        {"ssid": "RESEAU-MASQUE", "rssi": -72},
                    ]
                }
            )
            return
        if path in {"/", "/recette", "/config", "/logs", "/discover", "/postes", "/users", "/security"}:
            self.send_text(DASHBOARD_HTML, role=role)
            return
        self.send_error(HTTPStatus.NOT_FOUND)

    def do_POST(self) -> None:
        length = int(self.headers.get("Content-Length", "0"))
        if length:
            self.rfile.read(length)
        if urlparse(self.path).path == "/login":
            self.send_json({"ok": True})
        else:
            self.send_json({"ok": True})


def main() -> None:
    server = ThreadingHTTPServer((HOST, PORT), DemoHandler)
    print(f"PS TIME MANAGER demo disponible sur http://{HOST}:{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
