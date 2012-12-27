dbfnm = 'test_sqlite.db';
if exist(dbfnm, 'file')==2
  delete(dbfnm);
end

dbp=sql_open(dbfnm, 'c');
fprintf('test database was created and opened, dbp = %f\n\n', dbp);
if sql_open~=dbp
  sql_close(dbp);
  error('failed, sql_open didn''t return the same db connection');
else
  fprintf('success, sql_open returned this db connection\n\n');
end

try
  [zpath, fnm, ext] = fileparts(sql_db_filename);
catch sqlite4m_err
  rethrow(sqlite4m_err);
end
ffnm = [fnm ext];
if ~strcmp(ffnm, dbfnm)
  sql_close(dbp);
  error('failed, sql_db_filename didn''t return the expected file name, \nbut %s', ffnm);
else
  fprintf('sql_db_filename succeeded\n');
end
try
  [zpath, fnm, ext] = fileparts(sql_db_filename(dbp));
catch sqlite4m_err
  rethrow(sqlite4m_err);
end
ffnm = [fnm ext];
if ~strcmp(ffnm, dbfnm)
  sql_close(dbp);
  error('failed, sql_db_filename(dbp) didn''t return the expected file name, \nbut %s', ffnm);
else
  fprintf('sql_db_filename(dbp) succeeded\n');
end

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
if sql_close(dbp)==0
  fprintf('success, test database has been closed\n');
else
  fprintf('fail, sql_close(dbp) didn''t return 0\n');
end
delete(dbfnm);
fprintf('%s has (presumably) been deleted\n', dbfnm);


