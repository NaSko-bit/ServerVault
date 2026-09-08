import sqlite3

from config import CLIENT_LINE_PATTERN, LOG_DATABASE, LOG_LINES_TO_SHOW


LOG_COLUMNS = ["LogID", "TImeStamp", "Source", "LogInfo"]
CREATE_LOGS_TABLE = (
    "CREATE TABLE IF NOT EXISTS LOGS ("
    "LogID INTEGER PRIMARY KEY AUTOINCREMENT, "
    "TImeStamp TEXT NOT NULL, "
    "Source TEXT NOT NULL, "
    "LogInfo TEXT NOT NULL"
    ")"
)


def fetch_recent_logs():
    with sqlite3.connect(LOG_DATABASE) as database:
        database.execute(CREATE_LOGS_TABLE)
        return database.execute(
            "SELECT LogID, TImeStamp, Source, LogInfo "
            "FROM LOGS ORDER BY LogID DESC LIMIT ?",
            (LOG_LINES_TO_SHOW,),
        ).fetchall()


def format_log_rows(rows):
    return [
        f"[{timestamp}] [{source}] {info}"
        for _log_id, timestamp, source, info in rows
    ]


def find_connected_client(rows, server_running):
    if not server_running:
        return None

    chronological_lines = list(reversed(format_log_rows(rows)))
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

    return client
