function cn = sql_columnnames(dbid, table, idx)
% SQL_COLUMNNAMES - returns the column names of table "table"
%   in an SQLite database
%   

cn = sql_exec(dbid, ['PRAGMA table_info(' table ')']);
if ~isempty(cn)
  cn = cn(2:end, 2);
  if nargin==3
    cn = cn(idx);
  end
end
