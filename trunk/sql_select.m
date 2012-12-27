function [resultdata, sqlstmt] = sql_select(dbp, columns, from_str, where_exp, other)
% SQL_SELECT - executes a SELECT statement for an SQLITE data base, see also
%   http://www.sqlite.org/lang_select.html
%
%   Input:
%      dbp         pointer to a database obtained by sql_open; alternatively
%                  dbp can be a file name, then the database will be opened
%                  before and closed after execution of the
%                  statement.
%
%   optional:
%      columns     specifies which columns to return in one of the following manners:
%               1) string with comma separated column names like "col1, col2, col5",
%                  the string can start with "SELECT", for example "SELECT DISTINCT ..."
%                  (the default is "SELECT ALL ...");
%               2) a cell array of strings, each cell specifying one column;
%               3) a numeric array of indices into the cell array of column names returned
%                  by "sql_columnnames(dbp, from_str, columns)", where from_str is the
%                  table name;
%               4) for empty "columns" all columns are returned.
%
%      from_str    string specifiying the source, which often is the table name.
%
%      where_exp   string with any expression that can be in the WHERE clause.
%
%      other       string specifying possible "GROUP BY ...", "ORDER BY ...", and/or
%                  "LIMIT ..." clauses.
%
%   Output:
%      resultdata  selected data, SQLITE type conversion is not used:
%                  resultdata is a cell array, if mixed types (doubles, text etc) were
%                  selected, and it is a double matrix, if only numeric data were selected.
%                  Database TEXT is returned in a cell containing a row vector of characters
%                  (Matlab string), BLOB in a cell containing a row vector of bytes (unsigned
%                  int8), NULL as an empty cell.
%
%   optional
%      sqlstmt     contains the full SQL statement that was executed.

