function sql_close(dbp)
% SQL_CLOSE - closes an SQLITE database connection previously openend with sql_open.
%   See also http://www.sqlite.org/c3ref/close.html
%
%   Input:
%      dbp         Pointer (handle) to the database connection structure previously returned
%                  by sql_open.
