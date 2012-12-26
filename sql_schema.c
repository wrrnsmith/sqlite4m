#include <math.h>
#include <string.h>

#include "mex.h"
#include "matrix.h"
#include "sqlite3.h"

/*
 * sql_schema.c
 * Returns the schema from an SQLITE3 database in a cell array
 *
 * This is a MEX-file for MATLAB.
*/

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  const mxArray *mxOpenArg[2];

  mxOpenArg[0] = prhs[0];
  mxOpenArg[1] = mxCreateString("SELECT sql FROM sqlite_master ORDER BY name;");
  mexCallMATLAB(1, plhs, 2, (mxArray **) mxOpenArg, "sql_stmt");
}
