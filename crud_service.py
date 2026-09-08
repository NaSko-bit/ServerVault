import sqlite3

from config import LOG_DATABASE


SQLITE_INTERNAL_TABLE_PREFIX = "sqlite_"


def _quote_identifier(identifier):
    return '"' + identifier.replace('"', '""') + '"'


def list_tables():
    with sqlite3.connect(LOG_DATABASE) as database:
        rows = database.execute(
            "SELECT name FROM sqlite_master "
            "WHERE type = 'table' AND name NOT LIKE ? "
            "ORDER BY name",
            (f"{SQLITE_INTERNAL_TABLE_PREFIX}%",),
        ).fetchall()
    return [name for (name,) in rows]


def fetch_table(table_name):
    quoted_table = _quote_identifier(table_name)
    with sqlite3.connect(LOG_DATABASE) as database:
        cursor = database.execute(f"SELECT * FROM {quoted_table}")
        columns = [description[0] for description in cursor.description]
        rows = cursor.fetchall()
    return columns, rows
