from pathlib import Path
import re


PROJECT_DIR = Path(__file__).resolve().parent
SERVERMEMORY_DIR = PROJECT_DIR / "Server"
UI_FILE = PROJECT_DIR / "MainWindow.ui"
CRUD_UI_FILE = PROJECT_DIR / "CRUDWindow.ui"
SERVER_EXECUTABLE = PROJECT_DIR / "server"
LOG_DATABASE = PROJECT_DIR.parent / "ServerVaultDB"
LOG_LINES_TO_SHOW = 20
SERVER_PORT = 2000
CLIENT_LINE_PATTERN = re.compile(
    r"ip=(?P<ip>\S+)\s+port=(?P<port>\d+)\s+device_type=(?P<device>.*)$"
)
