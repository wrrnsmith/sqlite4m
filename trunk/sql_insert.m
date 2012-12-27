function zsql = sql_insert(dbp, tablename, columns, data)
% SQL_INSERT - executes an INSERT statement for an SQLITE data base.
%
%   Input:
%      dbp         pointer to a database obtained by sql_open; alternatively
%                  dbp can be a file name, then the database will be opened
%                  before and closed after execution of the statement.
%
%      tablename   Matlab character array containing the name of the table into
%                  which to insert. The table name can be prepended
%                  by "REPLACE INTO ", which is then equivalent to
%                  an SQL statement "INSERT OR REPLACE INTO ...".
%                  Alternatively a numeric index into the list
%                  of table names as returned by "sql_tables(dbp)". If empty or
%                  omitted, the first table name returned by "sql_tables(dbp)" is used.
%
%      columns     Columns to insert into are specified in one of the following ways:
%                  1) if empty insert into all columns;
%                  2) a string of comma separated column names like "col1, col2, col3";
%                  3) a cell array each cell containing one column name;
%                  4) numeric indices into the list returned by "sql_columnnames(dbp)"
%
%      data        Matlab variable containing the VALUES to insert.
%                  - an empty Matlab variable is inserted as a single SQLITE NULL;
%                  - each row of a Matlab character array is inserted as SQLITE text;
%                  - Matlab int8 and uint8 arrays are inserted as one SQLITE blob;
%                  - Matlab int16/uint16 and int32/uint32 arrays are inserted as SQLITE ints;
%                  - Matlab int64/uint64 arrays are inserted as SQLITE int64s;
%                  - Matlab single arrays are inserted as SQLITE real (with double precision);
%                  - Matlab double arrays are inserted as SQLITE real (with double precision);
%                  - Matlab cell or structure arrays are inserted as follows:
%                    * row or column vectors of Matlab cells or structures can be
%                      inserted, if each cell or structure field has the same length.
%                      However, character, int8, and uint8 as well as empty arrays
%                      are considered to have length 1, and these are inserted 
%                      as SQLITE text, blob, and NULL, respectively. So if one or more cell
%                      or structure field is a character, int8, or uint8 array, then all other
%                      cells or structure fields must have length 1.
%                    * full Matlab cell or structure arrays can be inserted, if each cell
%                      or structure field has length 1. Again character, int8,
%                      and uint8 as well as empty arrays are considered to have length 1.
%   Output:
%      zsql        String with the SQL INSERT statement, that was executed
%
%  Examples:
%  1) > sql_insert(dbp, 'table1', 'col2', []);
%     executes
%     sqlite> INSERT INTO table1 (col2) VALUES (NULL);' 
%     which inserts NULL into column 'col2' of table 'table1', the other columns of the new row
%     are filled with their default value (as specified in CREATE COLUMN ... or NULL);
%
%  2) > sql_insert(dbp, 'table1', 'col2', ['eine '; 'lange'; 'Zeit ']);
%     executes
%     sqlite> INSERT INTO table1 (col2) VALUES ('eine ');
%     sqlite> INSERT INTO table1 (col2) VALUES ('lange');
%     sqlite> INSERT INTO table1 (col2) VALUES ('Zeit ');
%     This inserts three new rows, where the
%     text values 'eine ', 'lange', and 'Zeit ' are inserted into 'col2,
%     respectively, and the other columns of the new rows are filled with their default value
%     (as specified in CREATE COLUMN ... or NULL);
%
%  3) > sql_insert(dbp, 'table2', [], [1 3.14 pi; sqrt(2) 2^10 99]);
%     is equivalent to
%     sqlite> INSERT INTO table2 VALUES (1, 3.14, 3.141592653589793);
%     sqlite> INSERT INTO table2 VALUES (1.4142135623730951 1024 99);
%     This inserts two new rows into table 'table2', which must have 3 columns.
%
%  4) > fid = fopen('portrait.jpg');
%     > sql_insert(dbp, 'table2', [], {1, ['die  '; 'wilde'; ' 13  '],  [];...
%                                      2, fread(fid, '*uint8'), NaN});
%     > fclose(fid);
%     is equivalent to
%     sqlite> INSERT INTO table2 VALUES (1, 'die  wilde 13  ', NULL);
%     sqlite> INSERT INTO table2 VALUES (2, x'...', NULL);
%     where x'...' is a hexadecimal representation of the contents of "portrait.jgp".
%     NaNs are always converted to NULLs in the database. When using "sql_select_numeric" 
%     (see > help sql_select_numeric), NULLs are returned back as NaNs. "sql_select" returns NULLs as
%     empty cell array element.
