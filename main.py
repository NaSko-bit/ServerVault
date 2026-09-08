from PySide6.QtUiTools import QUiLoader
from PySide6.QtCore import QEvent, QObject, QProcess, QTimer, Qt
from PySide6.QtGui import QStandardItem, QStandardItemModel
from PySide6.QtWidgets import QApplication

from config import (
    LOG_DATABASE,
    CRUD_UI_FILE,
    PROJECT_DIR,
    SERVERMEMORY_DIR,
    SERVER_EXECUTABLE,
    SERVER_PORT,
    UI_FILE,
)
from log_service import (
    LOG_COLUMNS,
    fetch_recent_logs,
    find_connected_client,
)
from crud_service import execute_query, fetch_table, list_tables, update_table
from system_service import get_host_ip, open_path


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
        rows = fetch_recent_logs()
        log_model.clear()
        log_model.setHorizontalHeaderLabels(LOG_COLUMNS)
        for row in rows:
            log_model.appendRow([
                QStandardItem(str(value)) for value in row
            ])
    except Exception as error:
        log_model.clear()
        log_model.setHorizontalHeaderLabels(LOG_COLUMNS)
        log_model.appendRow([
            QStandardItem(""),
            QStandardItem(""),
            QStandardItem("ERROR"),
            QStandardItem(f"Unable to read ServerVaultDB: {error}"),
        ])

    table_view.resizeColumnsToContents()
    set_client_info(find_connected_client(
        rows, server_process.state() == QProcess.ProcessState.Running
    ))


def server_process_finished(_exit_code, _exit_status):
    window.pushButton_2.setText("Start Server")
    window.pushButton_2.setEnabled(True)
    set_server_status(False)
    set_client_info()
    refresh_logs()

def log_button_clicked():
    open_path(LOG_DATABASE)

def memory_button_clicked():
    open_path(SERVERMEMORY_DIR)

def load_crud_table(table_name):
    global crud_table_name, crud_columns, crud_original_rows

    crud_model.clear()

    try:
        columns, rows = fetch_table(table_name)
        crud_table_name = table_name
        crud_columns = columns
        crud_original_rows = rows
        display_crud_rows(columns, rows)
        crud_window.statusBar().showMessage(
            f"Loaded {len(rows)} rows from {table_name}"
        )
    except Exception as error:
        crud_model.setHorizontalHeaderLabels(["Error"])
        crud_model.appendRow([
            QStandardItem(f"Unable to load {table_name}: {error}")
        ])
        crud_window.statusBar().showMessage("Could not load table")


def display_crud_rows(columns, rows):
    crud_model.clear()
    crud_model.setHorizontalHeaderLabels(columns)
    for row in rows:
        crud_model.appendRow([
            QStandardItem("" if value is None else str(value))
            for value in row
        ])
    crud_table_view.resizeColumnsToContents()


def execute_crud_query():
    global crud_table_name

    try:
        columns, rows = execute_query(crud_window.textEdit.toPlainText())
        crud_table_name = None
        display_crud_rows(columns, rows)
        crud_window.statusBar().showMessage(
            f"Query returned {len(rows)} row(s)"
        )
    except Exception as error:
        crud_window.statusBar().showMessage(f"Query failed: {error}")


def update_crud_table():
    if crud_table_name is None:
        crud_window.statusBar().showMessage("No table selected")
        return

    updated_rows = []
    for row_index in range(crud_model.rowCount()):
        row = []
        for column_index in range(crud_model.columnCount()):
            item = crud_model.item(row_index, column_index)
            row.append(item.text())
        updated_rows.append(tuple(row))

    try:
        updated_count = update_table(
            crud_table_name,
            crud_columns,
            crud_original_rows,
            updated_rows,
        )
        crud_window.statusBar().showMessage(
            f"Updated {updated_count} row(s) in {crud_table_name}"
        )
        load_crud_table(crud_table_name)
    except Exception as error:
        crud_window.statusBar().showMessage(
            f"Unable to update {crud_table_name}: {error}"
        )


def initialize_crud_window():
    global crud_model, crud_table_view, query_input_filter

    crud_model = QStandardItemModel(crud_window)
    crud_table_view = crud_window.tableView
    crud_table_view.setModel(crud_model)
    crud_table_view.setAlternatingRowColors(True)
    crud_table_view.setSortingEnabled(False)
    crud_table_view.verticalHeader().setVisible(False)

    crud_window.comboBox.currentTextChanged.connect(load_crud_table)
    crud_window.pushButton.clicked.connect(update_crud_table)
    query_input_filter = CrudQueryInputFilter(crud_window)
    crud_window.textEdit.installEventFilter(query_input_filter)
    crud_window.comboBox.clear()

    try:
        tables = list_tables()
        crud_window.comboBox.addItems(tables)
        if tables:
            load_crud_table(tables[0])
        else:
            crud_window.statusBar().showMessage("No database tables found")
    except Exception as error:
        crud_window.statusBar().showMessage(
            f"Unable to list database tables: {error}"
        )


def crud_window_clicked():
    global crud_window

    if crud_window is None:
        crud_window = QUiLoader().load(str(CRUD_UI_FILE), window)
        initialize_crud_window()

    if crud_window is not None:
        crud_window.show()
        crud_window.raise_()
        crud_window.activateWindow()

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


class CrudQueryInputFilter(QObject):
    def eventFilter(self, watched, event):
        if watched is crud_window.textEdit and \
                event.type() == QEvent.Type.KeyPress and \
                event.key() in (Qt.Key.Key_Return, Qt.Key.Key_Enter):
            if event.modifiers() & Qt.KeyboardModifier.ShiftModifier:
                return super().eventFilter(watched, event)

            execute_crud_query()
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
crud_window = None
crud_model = None
crud_table_view = None
crud_table_name = None
crud_columns = []
crud_original_rows = []
query_input_filter = None

log_model = QStandardItemModel(window)
log_model.setHorizontalHeaderLabels(LOG_COLUMNS)
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
window.pushButton_5.clicked.connect(crud_window_clicked)
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