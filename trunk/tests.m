if exist('testdb.sqlite', 'file')==2
  delete('testdb.sqlite');
end

fprintf('dbp  = sql_open(''testdb.sqlite'', ''c'');\n'); 
dbp  = sql_open('testdb.sqlite', 'c');
fprintf('Database testdb.sqlite has been opened.\n\n'); 

fprintf('[r, zsql] = sql_exec(dbp, ''CREATE TABLE table1 (col1 FLOAT, col2 REAL);'');\n');
[r, zsql] = sql_exec(dbp, 'CREATE TABLE table1 (col1 FLOAT, col2 REAL);');
fprintf('Statement\n%s\n  has been executed.\n\n', zsql); 

fprintf('zsql = sql_insert(dbp, ''table1'', ''col1,col2'', [1, NaN]);\n');
zsql = sql_insert(dbp, 'table1', 'col1,col2', [1, NaN]);
fprintf('Statement\n%s\n  has been executed.\n\n', zsql); 

fprintf('zsql = sql_insert(dbp, ''table1'', ''col2'', []);\n');
zsql = sql_insert(dbp, 'table1', 'col2', []);
fprintf('Statement\n%s\n  has been executed.\n\n', zsql); 

fprintf('[r, zsql] = sql_select(dbp, [], ''table1'');\n');
[r, zsql] = sql_select(dbp, [], 'table1');
fprintf('Statement\n\n%s\n  has been executed.\n\n', zsql);
whos r
disp(r);
fprintf('was returned.\n\n'); 

fprintf('[r, zsql] = sql_select_numeric(dbp, [], ''table1'');\n');
[r, zsql] = sql_select_numeric(dbp, [], 'table1');
fprintf('Statement\n\n%s\n  has been executed.\n\n', zsql);
whos r
disp(r);
fprintf('was returned.\n\n');

fprintf('[r, zsql] = sql_select_numeric(dbp, [], ''table1'', int32(1));\n');
[r, zsql] = sql_select_numeric(dbp, [], 'table1', int32(1));
fprintf('Statement\n\n%s\n  has been executed.\n\n', zsql);
whos r
disp(r);
fprintf('was returned.\n\n');

fprintf('zsql = sql_insert(dbp, ''table1'', ''col1,col2'', [1.00000001, 1.00000001]);\n');
zsql = sql_insert(dbp, 'table1', 'col1,col2', [1.00000001, 1.00000001]);
fprintf('Statement\n%s\n  has been executed.\n\n', zsql); 

fprintf('[r, zsql] = sql_select_numeric(dbp, [], ''table1'', ''rowid=3'');\n');
[r, zsql] = sql_select_numeric(dbp, [], 'table1', 'rowid=3');
fprintf('Statement\n\n%s\n  has been executed.\n\n', zsql);
whos r
disp(r);
fprintf('was returned.\n\n');


fprintf('sql_close(dbp);\n');
sql_close(dbp);
fprintf('Database has been closed.\n\n'); 
