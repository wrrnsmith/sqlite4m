#include <math.h>
#include <string.h>

#include "mex.h"
#include "matrix.h"
#include "sqlite3.h"

/*
 * sql_close.c
 * Closes a SQLITE3 database connection
 *
 * This is a MEX-file for MATLAB.
*/

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  mxArray *mxRet, *mxNumArg;
  sqlite3 *ppDb;

/* get pointer to the database: */
  mexCallMATLAB(1, &mxRet, 0, NULL, "sql_open");
  ppDb = *((sqlite3 **) mxGetData(mxRet));
/* check if is's the same database connection */
  if( nrhs>0) {
    if( ppDb!=*((sqlite3 **) mxGetData(prhs[0])) )
      mexErrMsgIdAndTxt("Sqlite4m:sql_close:close",
			"wrong database pointer"); 
  }
 /* Close the database via sql_open (which also unlocks the mex file): */
  mxNumArg = mxCreateDoubleMatrix(1, 1, mxREAL);
  mxGetPr(mxNumArg)[0] = 0;
  mexCallMATLAB(1, &mxRet, 1, &mxNumArg, "sql_open");

  plhs[0] = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
  /* Copy the data base pointer */
  mxGetPr(plhs[0])[0] = mxGetPr(mxRet)[0];
  mxDestroyArray(mxRet);
}
