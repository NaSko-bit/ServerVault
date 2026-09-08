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


def execute_query(query):
    query = query.strip()
    if not query:
        raise ValueError("Enter a SQL query")

    with sqlite3.connect(LOG_DATABASE) as database:
        cursor = database.execute(query)

        if cursor.description is None:
            return ["Result"], [(f"{cursor.rowcount} row(s) affected",)]

        columns = [description[0] for description in cursor.description]
        return columns, cursor.fetchall()


def update_table(table_name, columns, original_rows, updated_rows):
    with sqlite3.connect(LOG_DATABASE) as database:
        table_info = database.execute(
            f"PRAGMA table_info({_quote_identifier(table_name)})"
        ).fetchall()
        primary_keys = [column for column in table_info if column[5] > 0]

        if len(primary_keys) != 1:
            raise ValueError(
                "The selected table must have exactly one primary key"
            )

        primary_key_name = primary_keys[0][1]
        primary_key_index = columns.index(primary_key_name)
        editable_columns = [
            column for index, column in enumerate(columns)
            if index != primary_key_index
        ]

        if not editable_columns:
            return 0

        quoted_table = _quote_identifier(table_name)
        assignments = ", ".join(
            f"{_quote_identifier(column)} = ?"
            for column in editable_columns
        )
        query = (
            f"UPDATE {quoted_table} SET {assignments} "
            f"WHERE {_quote_identifier(primary_key_name)} = ?"
        )

        updated_count = 0
        for original_row, updated_row in zip(original_rows, updated_rows):
            original_values = [
                original_row[columns.index(column)]
                for column in editable_columns
            ]
            updated_values = [
                updated_row[columns.index(column)]
                for column in editable_columns
            ]
            if ["" if value is None else str(value)
                    for value in original_values] == updated_values:
                continue

            database.execute(
                query,
                (*updated_values, original_row[primary_key_index]),
            )
            updated_count += 1

        return updated_count
