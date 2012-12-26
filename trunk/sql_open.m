function dbp = sql_open(filename, flag)
% SQL_OPEN - opens an SQLITE database file as specified by the filename argument.
%   A pointer (handle) to the database connection structure is returned. The mex
%   functions for SQLITE can handle only one open database connection. To access
%   more than one database execute an "ATTACH" statement with "sql_exec". See also
%   http://www.sqlite.org/c3ref/open.html
%
%   Input:
%      The pointer is also stored in locked memory. Calling sql_open without any
%      arguments returns the previously stored pointer or NULL.
%
%      filename    Database filename in UTF-8 encoding.
%
%      flag        The default is read only, database must exist. If flag
%                  'w' the database is opened for read and write, if flag
%                  'c' the database is also created, if it doesn't exist.
%                  The database is always opened with the SQLITE_OPEN_SHAREDCACHE flag.