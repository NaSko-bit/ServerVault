from pathlib import Path
import socket
import subprocess


def get_host_ip():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as address_socket:
            address_socket.connect(("8.8.8.8", 80))
            return address_socket.getsockname()[0]
    except OSError:
        return "localhost"


def open_path(path: Path):
    subprocess.Popen(["xdg-open", str(path)])
