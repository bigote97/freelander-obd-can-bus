#!/usr/bin/env python3
"""Registro serie del lector K-Line. Corre en la Raspberry Pi Zero W o en la PC."""

import datetime
import queue
import sys
import threading
from pathlib import Path

try:
    import serial
except ImportError:
    sys.stderr.write("Falta pyserial. En la Pi: sudo apt install python3-serial\n")
    sys.exit(1)


def main() -> None:
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
    log_dir = Path(__file__).resolve().parent / "logs"
    log_dir.mkdir(exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    log_path = log_dir / f"kline-{stamp}.log"

    comandos = queue.Queue()

    def leer_teclado() -> None:
        for linea in sys.stdin:
            comandos.put(linea)

    threading.Thread(target=leer_teclado, daemon=True).start()

    link = serial.Serial(port, 115200, timeout=0.2)
    sys.stderr.write(f"Puerto {port}\nRegistro {log_path}\n")
    sys.stderr.write("Comandos: LEVELS, CHECK, SCAN, INIT 1C. Salir con Ctrl-C.\n")

    with log_path.open("a", encoding="utf-8") as log:
        log.write(f"# {stamp} {port}\n")
        try:
            while True:
                data = link.readline()
                if data:
                    text = data.decode("utf-8", errors="replace").rstrip("\r\n")
                    line = f"{datetime.datetime.now().isoformat(timespec='seconds')} {text}"
                    print(line, flush=True)
                    log.write(line + "\n")
                    log.flush()
                try:
                    cmd = comandos.get_nowait()
                except queue.Empty:
                    cmd = None
                if cmd:
                    if not cmd.endswith("\n"):
                        cmd += "\n"
                    link.write(cmd.encode("utf-8"))
        except KeyboardInterrupt:
            sys.stderr.write("\nCorte.\n")
        finally:
            link.close()


if __name__ == "__main__":
    main()
