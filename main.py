from pathlib import Path
import re
import sqlite3
import socket
import subprocess
from PySide6.QtUiTools import QUiLoader
from PySide6.QtCore import QEvent, QObject, QProcess, QTimer, Qt
from PySide6.QtGui import QStandardItem, QStandardItemModel
from PySide6.QtWidgets import QApplication


SERVERMEMORY_DIR = "./Server"
PROJECT_DIR = Path(__file__).resolve().parent
UI_FILE = PROJECT_DIR / "MainWindow.ui"
SERVER_EXECUTABLE = PROJECT_DIR / "server"
LOG_DATABASE = PROJECT_DIR.parent / "ServerVaultDB"
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
    rows = []
    try:
        with sqlite3.connect(LOG_DATABASE) as database:
            database.execute(
                "CREATE TABLE IF NOT EXISTS LOGS ("
                "LogID INTEGER PRIMARY KEY AUTOINCREMENT, "
                "TImeStamp TEXT NOT NULL, "
                "Source TEXT NOT NULL, "
                "LogInfo TEXT NOT NULL"
                ")"
            )
            rows = database.execute(
                "SELECT LogID, TImeStamp, Source, LogInfo "
                "FROM LOGS ORDER BY LogID DESC LIMIT ?",
                (LOG_LINES_TO_SHOW,),
            ).fetchall()
        log_model.clear()
        log_model.setHorizontalHeaderLabels(
            ["LogID", "TImeStamp", "Source", "LogInfo"]
        )
        for row in rows:
            log_model.appendRow([
                QStandardItem(str(value)) for value in row
            ])
    except (OSError, sqlite3.Error) as error:
        log_model.clear()
        log_model.setHorizontalHeaderLabels(
            ["LogID", "TImeStamp", "Source", "LogInfo"]
        )
        log_model.appendRow([
            QStandardItem(""),
            QStandardItem(""),
            QStandardItem("ERROR"),
            QStandardItem(f"Unable to read ServerVaultDB: {error}"),
        ])

    table_view.resizeColumnsToContents()
    lines = [f"[{timestamp}] [{source}] {info}"
             for _log_id, timestamp, source, info in rows]
    refresh_client_info(lines)


def refresh_client_info(lines):
    if server_process.state() != QProcess.ProcessState.Running:
        set_client_info()
        return

    chronological_lines = list(reversed(lines))
    server_start = -1
    for index, line in enumerate(chronological_lines):
        if "[SERVER] Server starting on port" in line:
            server_start = index

    client = None
    for line in chronological_lines[server_start + 1:]:
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
    window.pushButton_2.setEnabled(True)
    set_server_status(False)
    set_client_info()
    refresh_logs()

def log_button_clicked():
    subprocess.Popen(["xdg-open", str(LOG_DATABASE)])

def memory_button_clicked():
    subprocess.Popen(["xdg-open", str(SERVERMEMORY_DIR)])

def disconnect_client_clicked():
    if server_process.state() == QProcess.ProcessState.Running:
        server_process.write(b"kick\n")


class CommandInputFilter(QObject):
    def eventFilter(self, watched, event):
        if watched is window.textEdit and event.type() == QEvent.Type.KeyPress:
            if event.key() in (Qt.Key.Key_Return, Qt.Key.Key_Enter) and \
                    not event.modifiers() & Qt.KeyboardModifier.ShiftModifier:
                command = window.textEdit.toPlainText().strip()
                if command and server_process.state() == \
                        QProcess.ProcessState.Running:
                    server_process.write((command + "\n").encode("utf-8"))
                    window.textEdit.clear()
                return True

        return super().eventFilter(watched, event)


def server_button_clicked():
    if server_process.state() == QProcess.ProcessState.Running:
        server_process.write(b"exit\n")
        server_process.closeWriteChannel()
        window.pushButton_2.setEnabled(False)
        return

    if not SERVER_EXECUTABLE.is_file():
        window.label.setText("Server executable not found")
        window.pushButton_2.setEnabled(True)
        return

    server_process.setWorkingDirectory(str(PROJECT_DIR))
    server_process.start(str(SERVER_EXECUTABLE), ["tcp_start"])

    if not server_process.waitForStarted(1000):
        window.label.setText("Could not start server")
        window.pushButton_2.setEnabled(True)
        refresh_logs()
        return

    window.pushButton_2.setText("Stop Server")
    window.pushButton_2.setEnabled(True)
    set_server_status(True)
    set_client_info()
    refresh_logs()


app = QApplication([])
window = QUiLoader().load(str(UI_FILE))

log_model = QStandardItemModel(window)
log_model.setHorizontalHeaderLabels(
    ["LogID", "TImeStamp", "Source", "LogInfo"]
)
table_view = window.tableView
table_view.setModel(log_model)
table_view.setAlternatingRowColors(True)
table_view.setSortingEnabled(False)
table_view.verticalHeader().setVisible(False)

server_process = QProcess(window)
server_process.finished.connect(server_process_finished)

window.pushButton.clicked.connect(log_button_clicked)
window.pushButton_2.clicked.connect(server_button_clicked)
window.pushButton_3.clicked.connect(memory_button_clicked)
window.pushButton_4.clicked.connect(disconnect_client_clicked)
command_input_filter = CommandInputFilter(window)
window.textEdit.installEventFilter(command_input_filter)


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