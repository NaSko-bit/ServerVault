from pathlib import Path

from PySide6.QtUiTools import QUiLoader
from PySide6.QtCore import QProcess, QTimer
from PySide6.QtWidgets import QApplication, QLabel, QVBoxLayout


PROJECT_DIR = Path(__file__).resolve().parent
UI_FILE = PROJECT_DIR / "MainWindow.ui"
SERVER_EXECUTABLE = PROJECT_DIR / "server"
LOG_FILE = PROJECT_DIR / "LOG.txt"
LOG_LINES_TO_SHOW = 20


def set_server_status(online):
    status = "ONLINE" if online else "OFFLINE"
    window.label.setText(f"Server STATUS: {status}\nIP: 127.0.0.1\nPORT: 2000")


def refresh_logs():
    try:
        lines = LOG_FILE.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError as error:
        lines = [f"Unable to read LOG.txt: {error}"]

    log_label.setText("\n".join(lines[-LOG_LINES_TO_SHOW:]) or "No logs yet.")


def server_process_finished(_exit_code, _exit_status):
    window.pushButton_2.setText("Start Server")
    set_server_status(False)
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
window.show()

app.aboutToQuit.connect(lambda: server_process.write(b"exit\n")
                         if server_process.state() == QProcess.ProcessState.Running
                         else None)

app.exec()