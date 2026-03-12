#!/usr/bin/env python3
"""Injecte les données A-GPS AssistNow sur un module u-blox via port série."""

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


def fetch_assistnow_payload(token: str, timeout_s: float = 10.0) -> bytes:
    params = {
        "token": token,
        "gnss": "gps,glo,gal,bds",
        "datatype": "eph",
        "format": "aid",
    }
    url = f"{ASSISTNOW_URL}?{urllib.parse.urlencode(params)}"

    try:
        with urllib.request.urlopen(url, timeout=timeout_s) as response:
            payload = response.read()
    except urllib.error.URLError as exc:
        raise RuntimeError(f"Echec reseau AssistNow: {exc}") from exc

    if not payload:
        raise ValueError("Reponse AssistNow vide")

    return payload


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


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Chargeur A-GPS AssistNow")
    parser.add_argument(
        "port",
        nargs="?",
        default="/dev/serial0",
        help="Port serie cible (defaut: /dev/serial0)",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    token = os.getenv("UBLOX_TOKEN")
    if not token:
        print("UBLOX_TOKEN absent", file=sys.stderr)
        return EXIT_MISSING_TOKEN

    try:
        payload = fetch_assistnow_payload(token)
    except RuntimeError as exc:
        print(str(exc), file=sys.stderr)
        return EXIT_NETWORK_ERROR
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return EXIT_EMPTY_PAYLOAD

    try:
        inject_payload(args.port, payload)
    except OSError as exc:
        print(f"Echec injection serie sur {args.port}: {exc}", file=sys.stderr)
        return EXIT_SERIAL_ERROR

    print(f"Injection AssistNow reussie: {len(payload)} octets envoyes sur {args.port}")
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
