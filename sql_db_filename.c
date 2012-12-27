#include <math.h>
#include <string.h>

#include "mex.h"
#include "matrix.h"
#include "sqlite3.h"

/*
 * sql_db_filename.c
 * Returns the filename to an opened or attached db.
 * The main database file has the name "main".
 *
 * This is a MEX-file for MATLAB.
*/

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  sqlite3 *db;
  mxArray *mxdb;
  const char *zmain = "main";
  const char *zDbName;

  /* If there is no input argument, then get the database connection with sql_open: */
  if( nrhs<1 ) {
    mexCallMATLAB(1, &mxdb, 0, NULL, "sql_open");
    db = *((sqlite3 **) mxGetData(mxdb));
  } else {
    if( !mxIsNumeric(prhs[0]) )
      mexErrMsgIdAndTxt("sqlite4m:sql_db_filename", "numeric first input argument is expected");
    db = *((sqlite3 **) mxGetData(prhs[0]));
  }

  if( nrhs>=2 ) {
    if( !mxIsChar(prhs[1]) )
      mexErrMsgIdAndTxt("sqlite4m:sql_db_filename", "character second input argument is expected");
    zDbName = mxArrayToString(prhs[1]);
  } else
    zDbName = zmain;

  plhs[0] = mxCreateString(sqlite3_db_filename(db, zDbName));

  /* if( zDbName!=zmain ) mxFree(zDbName); */
}
