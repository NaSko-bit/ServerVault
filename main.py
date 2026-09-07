from pathlib import Path
import re
import socket

from PySide6.QtUiTools import QUiLoader
from PySide6.QtCore import QProcess, QTimer
from PySide6.QtWidgets import QApplication, QLabel, QVBoxLayout


PROJECT_DIR = Path(__file__).resolve().parent
UI_FILE = PROJECT_DIR / "MainWindow.ui"
SERVER_EXECUTABLE = PROJECT_DIR / "server"
LOG_FILE = PROJECT_DIR / "LOG.txt"
LOG_LINES_TO_SHOW = 20
SERVER_PORT = 2000
CLIENT_LINE_PATTERN = re.compile(
    r"ip=(?P<ip>\S+)\s+port=(?P<port>\d+)\s+device_type=(?P<device>.*)$"
)


def get_host_ip():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as address_socket:
            address_socket.connect(("8.8.8.8", 80))
            return address_socket.getsockname()[0]
    except OSError:
        return "localhost"


def set_server_status(online):
    status = "ONLINE" if online else "OFFLINE"
    host_ip = get_host_ip() if online else "localhost"
    window.label.setText(
        f"Server STATUS: {status}\nServer IP: {host_ip}\nPort: {SERVER_PORT}"
    )


def set_client_info(client=None):
    if client is None:
        window.label_3.setText("NO CONNECTED CLIENT")
        return

    window.label_3.setText(
        "Device_type: {device}\nIp: {ip}\nPort: {port}".format(**client)
    )


def refresh_logs():
    try:
        lines = LOG_FILE.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError as error:
        lines = [f"Unable to read LOG.txt: {error}"]

    log_label.setText("\n".join(lines[-LOG_LINES_TO_SHOW:]) or "No logs yet.")
    refresh_client_info(lines)


def refresh_client_info(lines):
    if server_process.state() != QProcess.ProcessState.Running:
        set_client_info()
        return

    server_start = -1
    for index, line in enumerate(lines):
        if "[SERVER] Server starting on port" in line:
            server_start = index

    client = None
    for line in lines[server_start + 1:]:
        if "[CLIENT] Client disconnected:" in line:
            client = None
            continue

        if "[CLIENT] Client connected:" not in line and \
                "[CLIENT] Client identity:" not in line:
            continue

        match = CLIENT_LINE_PATTERN.search(line)
        if match:
            client = match.groupdict()

    set_client_info(client)


def server_process_finished(_exit_code, _exit_status):
    window.pushButton_2.setText("Start Server")
    set_server_status(False)
    set_client_info()
    refresh_logs()


def server_button_clicked():
    if server_process.state() == QProcess.ProcessState.Running:
        server_process.write(b"exit\n")
        server_process.closeWriteChannel()
        window.pushButton_2.setEnabled(False)
        return

    if not SERVER_EXECUTABLE.is_file():
        window.label.setText("Server executable not found")
        return

    server_process.setWorkingDirectory(str(PROJECT_DIR))
    server_process.start(str(SERVER_EXECUTABLE), ["tcp_start"])

    if not server_process.waitForStarted(1000):
        window.label.setText("Could not start server")
        refresh_logs()
        return

    window.pushButton_2.setText("Stop Server")
    window.pushButton_2.setEnabled(True)
    set_server_status(True)
    set_client_info()
    refresh_logs()


app = QApplication([])
window = QUiLoader().load(str(UI_FILE))

server_process = QProcess(window)
server_process.finished.connect(server_process_finished)

log_label = QLabel()
log_label.setWordWrap(True)
log_label.setTextInteractionFlags(
    log_label.textInteractionFlags()
)
log_layout = QVBoxLayout(window.scrollAreaWidgetContents)
log_layout.setContentsMargins(6, 6, 6, 6)
log_layout.addWidget(log_label)
window.scrollArea.setWidgetResizable(True)

window.pushButton_2.clicked.connect(server_button_clicked)
window.scrollArea.verticalScrollBar().setValue(
    window.scrollArea.verticalScrollBar().maximum()
)

log_timer = QTimer(window)
log_timer.timeout.connect(refresh_logs)
log_timer.start(1000)
refresh_logs()
set_server_status(False)
set_client_info()
window.show()

app.aboutToQuit.connect(lambda: server_process.write(b"exit\n")
                         if server_process.state() == QProcess.ProcessState.Running
                         else None)

app.exec()