if exist('testdb.sqlite', 'file')==2
  delete('testdb.sqlite');
end

dbp=sql_open('testdb.sqlite', 'c');
fprintf('test database was opened, dbp = %f\n\n', dbp);
%
try 
  sql_exec(dbp, 'CREATE TABLE table1 (col1, col2);');
catch sqlite4m_err
  sql_close(dbp);
  rethrow(sqlite4m_err);
end
fprintf('CREATE TABLE table1 (col1, col2);\nhas been executed\n\n');
%
mstmt = 'sql_insert(dbp, ''table1'', ''col2'', NaN);';
fprintf(['evaluating ' mstmt '\n']);
try 
  zsql = eval(mstmt);
catch sqlite4m_err
  sql_close(dbp);
  rethrow(sqlite4m_err);
end
fprintf('%s\nwas returnd\n\n', zsql);
%
mstmt = 'sql_select(dbp, [], ''table1'');';
fprintf(['evaluating ' mstmt '\n']);
try 
  [r, zsql] =  eval(mstmt);
catch sqlite4m_err
  sql_close(dbp);
  rethrow(sqlite4m_err);
end
whos('r');
disp(r);
fprintf('and\n');
fprintf('%s\nwas returnd\n\n', zsql);
%
mstmt = 'sql_select_numeric(dbp, [], ''table1'');';
fprintf(['evaluating ' mstmt '\n']);
try 
  [r, zsql] = eval(mstmt);
catch sqlite4m_err
  sql_close(dbp);
  rethrow(sqlite4m_err);
end
whos('r');
disp(r);
fprintf('and\n');
fprintf('%s\nwas returnd\n\n', zsql);
%
mstmt = 'sql_insert(dbp, ''table1'', ''col2'', []);';
fprintf(['evaluating ' mstmt '\n']);
try 
  zsql = eval(mstmt);
catch sqlite4m_err
  sql_close(dbp);
  rethrow(sqlite4m_err);
end
fprintf('%s\nwas returnd\n\n', zsql);
%
mstmt = 'sql_select(dbp, [], ''table1'');';
fprintf(['evaluating ' mstmt '\n']);
try 
  [r, zsql] =  eval(mstmt);
catch sqlite4m_err
  sql_close(dbp);
  rethrow(sqlite4m_err);
end
whos('r');
disp(r);
fprintf('and\n');
fprintf('%s\nwas returnd\n\n', zsql);
%
mstmt = 'sql_select_numeric(dbp, [], ''table1'');';
fprintf(['evaluating ' mstmt '\n']);
try 
  [r, zsql] =  eval(mstmt);
catch sqlite4m_err
  sql_close(dbp);
  rethrow(sqlite4m_err);
end
whos('r');
disp(r);
fprintf('and\n');
fprintf('%s\nwas returnd\n\n', zsql);
%
mstmt = 'sql_insert(dbp, ''table1'', ''col2'', [''eine ''; ''lange''; ''Zeit '']);';
fprintf(['evaluating ' mstmt '\n']);
try 
  zsql = eval(mstmt);
catch sqlite4m_err
  sql_close(dbp);
  rethrow(sqlite4m_err);
end
fprintf('%s\nwas returnd\n\n', zsql);
%
mstmt = 'sql_select(dbp, [], ''table1'');';
fprintf(['evaluating ' mstmt '\n']);
try 
  [r, zsql] =  eval(mstmt);
catch sqlite4m_err
  sql_close(dbp);
  rethrow(sqlite4m_err);
end
whos('r');
disp(r);
fprintf('and\n');
fprintf('%s\nwas returnd\n\n', zsql);
%
sql_close(dbp);
fprintf('test database has been closed\n');
