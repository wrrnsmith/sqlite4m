#include <math.h>
#include <string.h>

#include "mex.h"
#include "matrix.h"
#include "sqlite3.h"
#include "sqlite4m.h"

/*
 * sql_exec.c
 * Executes an SQL statement
 *
 * This is a MEX-file for MATLAB.
*/

static int callback(void *answer, int ncol, char **argv, char **ColName) {
  int i;
  mwSize subs[2];

  /*
  for( i=0; i<ncol; i++ )
    mexPrintf("%s = %s\n", ColName[i], argv[i] ? argv[i]: "NULL");
  mexPrintf("\n");
  */

  /* First row? Insert column names: */
  if( mxGetM(answer)==0 ) {
    mxDestroyArray(answer); /* Destroy the anyway empty array */
    answer = mxCreateCellMatrix(ncol, 1);
    for( i=0; i<ncol; i++ )
      mxSetCell(answer, i, mxCreateString(ColName[i]));
  }

  mxSetN(answer, mxGetN(answer)+1);
  mxSetData(answer, mxRealloc(mxGetData(answer),
			      sizeof(mxArray *)*mxGetN(answer)*ncol));
  subs[1] = mxGetN(answer)-1;
  for( i=0; i<ncol; i++ ) {
    subs[0] = i;
    mxSetCell(answer, 
	      mxCalcSingleSubscript(answer, 2, subs),
	      mxCreateString(argv[i] ? argv[i] : ""));
  }
  
  return 0;
}

static char *zsql=NULL;

static void cleanup(void) {
  if( mxppDb ) {
    mexCallMATLAB(1, &mxRet, 1, &mxppDb, "sql_close");
    mxppDb = NULL;
  }
  if( zsql ) {
    mxFree(zsql);
    zsql = NULL;
  }
}

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  mxArray *mxAnswer;
  char *zErrMsg=NULL;
  int iret;

  strcat(errmsgstr1, "exec:");

  /* Assert that there are two input arguments */
  if( nrhs!=2 )
    ERRABORT("nrhs", "two input arguments are needed.");

  /* Assert that the 2nd input argument is a string: */
  if( !mxIsChar(prhs[1]) )
    ERRABORT("notChar", "String in 3nd input argument is expected.");

  mexAtExit(cleanup);

/* if the first argument is a string, then open a database connection  */
  if( mxIsChar(prhs[0]) ) {
    mexCallMATLAB(1, &mxppDb, 1, (mxArray **) &prhs[0], "sql_open");
    ppDb = *((sqlite3 **) mxGetData(mxppDb));
  } else /* Other the first argument should be the database pointer: */
    ppDb = *((sqlite3 **) mxGetData(prhs[0]));

  zsql = mxArrayToString(prhs[1]);

  mxAnswer = mxCreateCellMatrix(0, 0); /* Empty cell matrix */
  iret = sqlite3_exec(ppDb, zsql, callback, mxAnswer, &zErrMsg);
  if( nlhs>=2 )
    plhs[1] = mxCreateString(zsql);
  else
    mxFree(zsql);
  if( iret!=SQLITE_OK ) {
    /* This will free zErrMsg if assigned */
    if( zErrMsg )
      sqlite3_free(zErrMsg);
    cleanup;
    mexErrMsgIdAndTxt("Sqlite4m:sql_exec:exec", sqlite3_errmsg(ppDb));
  }

/* if the first argument was a string, then close the database connection  */
  CLOSE_DB;

/* use Matlab transpose to copy the transposed answer
   into the output cell array: */
   mexCallMATLAB(1, plhs, 1, &mxAnswer, "transpose");
}
