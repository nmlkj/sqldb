/* System Catalog: all_tables (TableID 0) */
CREATE TABLE all_tables (
    table_id INT PRIMARY KEY,
    table_name TEXT UNIQUE
);

/* System Catalog: all_columns (TableID 1) */
CREATE TABLE all_columns (
    table_id INT,
    col_idx INT,
    col_name TEXT,
    data_type TEXT
);
