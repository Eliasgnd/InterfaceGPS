#!/usr/bin/env python3
"""Injecte les donnees A-GPS AssistNow sur un module u-blox via port serie."""

from __future__ import annotations

import argparse
import os
import sys
import termios
import urllib.error
import urllib.parse
import urllib.request

EXIT_OK = 0
EXIT_MISSING_TOKEN = 10
EXIT_NETWORK_ERROR = 11
EXIT_EMPTY_PAYLOAD = 12
EXIT_SERIAL_ERROR = 13

ASSISTNOW_URL = "https://online-live1.services.u-blox.com/GetOnlineData.ashx"

def fetch_assistnow_payload(token: str, timeout_s: float = 20.0) -> bytes:
    params = {
        "token": token,
        "gnss": "gps,glo,gal,bds",
        "datatype": "eph",
        "format": "aid",
    }
    url = f"{ASSISTNOW_URL}?{urllib.parse.urlencode(params)}"

    try:
        # Ajout d un User-Agent pour eviter les blocages reseaux
        headers = {"User-Agent": "Mozilla/5.0"}
        req = urllib.request.Request(url, headers=headers)
        
        with urllib.request.urlopen(req, timeout=timeout_s) as response:
            payload = response.read()
            return payload
    except urllib.error.HTTPError as exc:
        raise RuntimeError(f"Erreur HTTP AssistNow: {exc.code} {exc.reason}") from exc
    except urllib.error.URLError as exc:
        raise RuntimeError(f"Echec reseau AssistNow: {exc.reason}") from exc

def configure_serial_9600(fd: int) -> None:
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0  # iflag
    attrs[1] = 0  # oflag
    attrs[2] = attrs[2] | termios.CLOCAL | termios.CREAD
    attrs[2] &= ~termios.CSTOPB
    attrs[2] &= ~termios.PARENB
    attrs[2] &= ~termios.CSIZE
    attrs[2] |= termios.CS8
    attrs[3] = 0  # lflag
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 5
    termios.cfsetispeed(attrs, termios.B9600)
    termios.cfsetospeed(attrs, termios.B9600)
    termios.tcsetattr(fd, termios.TCSANOW, attrs)

def inject_payload(port: str, payload: bytes) -> None:
    fd = os.open(port, os.O_WRONLY | os.O_NOCTTY)
    try:
        configure_serial_9600(fd)
        total = 0
        while total < len(payload):
            written = os.write(fd, payload[total:])
            if written <= 0:
                raise OSError("Ecriture serie interrompue")
            total += written
        termios.tcdrain(fd)
    finally:
        os.close(fd)

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("port", nargs="?", default="/dev/serial0")
    args = parser.parse_args()

    token = os.getenv("UBLOX_TOKEN")
    
    if not token:
        print("Erreur: UBLOX_TOKEN absent des variables d environnement", file=sys.stderr)
        return EXIT_MISSING_TOKEN

    # Affichage partiel pour debug sans risque securite
    masked_token = f"{token[:4]}...{token[-4:]}" if len(token) > 8 else "***"
    print(f"Token detecte: {masked_token}")

    try:
        print(f"Telechargement des donnees A-GPS (timeout 20s)...")
        payload = fetch_assistnow_payload(token)
        if not payload:
            print("Erreur: Reponse AssistNow vide", file=sys.stderr)
            return EXIT_EMPTY_PAYLOAD
            
        print(f"Injection sur {args.port}...")
        inject_payload(args.port, payload)
        print(f"Succes: {len(payload)} octets envoyes.")
        return EXIT_OK

    except RuntimeError as exc:
        print(f"Erreur Reseau: {exc}", file=sys.stderr)
        return EXIT_NETWORK_ERROR
    except OSError as exc:
        print(f"Erreur Serie sur {args.port}: {exc}", file=sys.stderr)
        return EXIT_SERIAL_ERROR

if __name__ == "__main__":
    sys.exit(main())