#include <math.h>
#include <string.h>

#include "mex.h"
#include "matrix.h"
#include "sqlite3.h"

/*
 * sql_tables.c
 * Returns the tables of an SQLITE3 database in a cell array
 *
 * This is a MEX-file for MATLAB.
*/

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  const mxArray *mxOpenArg[2];

  mxOpenArg[0] = prhs[0];
  mxOpenArg[1] = mxCreateString("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;");
  mexCallMATLAB(1, plhs, 2, (mxArray **) mxOpenArg, "sql_stmt");
}
