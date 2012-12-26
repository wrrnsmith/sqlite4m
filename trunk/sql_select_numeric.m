function [resultdata, sqlstmt] = sql_select_numeric(dbp, columns, ...
                                                    from_str, where_exp, other, bindpar, xtype)
% SQL_SELECT_NUMERIC - executes a SELECT statement for an SQLITE data base. The result
%   is returned in a numeric matrix. Database NULLs are converted to NaNs in Matlab, otherwise 
%   the result data are type conversions are performed by SQLITE.
%   See also http://www.sqlite.org/c3ref/column_blob.html
%
%   The result data are returned in the way that Matlab arrays with a-priori unknown length can 
%   be filled fastest. That is, each database row becomes a Matlab array column.
%   To get the layout as in the database, the output data matrix should be transposed.
%
% Function statement:
% function [resultdata, sqlstmt] = sql_select_numeric(dbp, columns, from_str, where_exp, other, bindpar, xtype)
%
%   Input:
%      dbp         pointer to a database obtained by sql_open; alternatively
%                  dbp can be a file name, then the database will be opened
%                  before and closed after execution of the statement.
%
%      columns     specifies which columns to return in one of the following manners:
%               1) string with comma separated column names like "col1, col2, col5",
%                  the string can be prepended with "SELECT", for example "SELECT DISTINCT ..."
%                  (the default is "SELECT ALL ...");
%               2) a cell array of strings, each cell specifying one column name;
%               3) a numeric array of indices into the cell array of column names returned
%                  by "sql_columnnames(dbp, from_str, columns)", where from_str is the
%                  table name;
%               4) for empty "columns" all columns are returned.
%
%   optional:
%
%      from_str    string specifiying the source, often the table name.
%
%      where_exp   string with any expression that can be in the WHERE clause.
%
%      other       string specifying possible "GROUP BY ...", "ORDER BY ...", and/or
%                  "LIMIT ..." clauses, can be omitted.
%
%      bindpar     cell array of values to bind to parameters of the SELECT statement,
%                  see also http://sqlite.org/c3ref/bind_blob.html.
%                  For example, the matlab statement
%                  r = sql_select_numeric(dbp, 'x', 'tab1', 'y<?', {besselj(1, sqrt(2))});
%                  returns from table tab1 the column x for all rows where column y
%                  less than the Bessel function of first kind of order 1 and argument sqrt(2).
%                  "bindpar" must be a cell array. The number of cells in the array must be equal
%                  to the number of parameters in the prepared statement. Each cell must contain
%                  one value, or, if some cells contain several values, their length must be the same.
%                  A Matlab character array is bound to SQLITE text, Matlab 8-bit signed and unsigned
%                  integer (int8 and uint8) arrays are bound to SQLITE blobs.
%
%      xtype       if there is a last numeric argument, it specifies the Matlab type of the
%                  returned numerical matrix. For example, to return results in single precision,
%                  use r=sql_select_numeric(dbp, [], 'tablename', single(1));
%                  The default is to return a double precision matrix.
%
%   Output:
%      resultdata  selected data
%
%      sqlstmt     full SQL statement that was executed

