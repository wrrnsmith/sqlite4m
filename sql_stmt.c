#include <math.h>
#include <string.h>

#include "mex.h"
#include "matrix.h"
#include "sqlite3.h"
#include "sqlite4m.h"

/*
 * sql_stmt.c
 * Execute an sql statement on a sqlite database, return any results
 *    in matlab data structures
 *
 * This is a MEX-file for MATLAB.
*/

/* The gateway function */
void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
/* variable declarations here */
  mwSize subs[2], nblob;
  mxArray *trowmajor, *rnum, *blob;
  char *zsql;
  int i, j, k, iret, istep, nchar, ncol, irow;
  int coltyp;
  int isnumeric=1;
  double *valp;

  strcat(errmsgstr1, "stmt:");

  /* Assert that there are two input arguments */
  if( nrhs!=2 )
    ERRABORT("nrhs", "two input arguments are needed.");

  /* Assert that the 2nd input argument is a string: */
  if( !mxIsChar(prhs[1]) )
    ERRABORT("notChar", "String in 3nd input argument is expected.");
  
/* if the first argument is a string, then open a database connection  */
  if( mxIsChar(prhs[0]) ) {
    mexCallMATLAB(1, &mxppDb, 1, (mxArray **) &prhs[0], "sql_open");
    ppDb = *((sqlite3 **) mxGetData(mxppDb));
  } else
    ppDb = *((sqlite3 **) mxGetData(prhs[0]));

  zsql = mxArrayToString(prhs[1]);
  /* Add terminating semicolon: */
  nchar = strlen(zsql);
  if( zsql[nchar-1]!=';' ) {
    zsql = mxRealloc(zsql, nchar+2);
    zsql[nchar+1] = 0;
    zsql[nchar] = ';';
  }

  /* Return the statement in the second optional output argument*/
  if( nlhs>=2 )
    plhs[1] = mxCreateString(zsql);

  /* Prepare the SQL library for the select statement: */
  iret = sqlite3_prepare_v2(ppDb, zsql, strlen(zsql), &ppStmt, NULL);
  /* mexPrintf("sqlite3_prepare_v2 returned %d\n", iret); */
  if( iret!=SQLITE_OK ) {
    if( mxIsChar(prhs[0]) ) {
      mexCallMATLAB(1, &mxRet, 1, &mxppDb, "sql_close");
      mxDestroyArray(mxppDb);
      mxDestroyArray(mxRet);
    }
    mexErrMsgIdAndTxt("Sqlite4m:sql_select:prepare", sqlite3_errmsg(ppDb));
  }

  mxFree(zsql);

  /* How many columns? */
  ncol = sqlite3_column_count(ppStmt);
  /*  mexPrintf("sqlite3_column_count returned %d\n", ncol); */

  /* The numbers of rows is not known in advance. Therefore
     the elements are first stored row-by-row in a temporary cell array,
     so that memory can be reallocated as database rows are added. */
  trowmajor = mxCreateCellMatrix(ncol, 0);

  irow = 0;
  /* Loop over selected rows: */
  while( (istep = sqlite3_step(ppStmt))==SQLITE_ROW ) {
    /* A row is available, so enlarge the output cell array */
    mxSetN(trowmajor, mxGetN(trowmajor)+1);
    mxSetData(trowmajor, mxRealloc(mxGetData(trowmajor),
				 sizeof(mxArray *)*mxGetN(trowmajor)*ncol));
    /* mexPrintf("plhs enlarged to %d %d\n",
       mxGetM(trowmajor), mxGetN(trowmajor)); */

    subs[1] = irow++;
    for( k=0; k<ncol; k++ ) {
      subs[0] = k;
      coltyp = sqlite3_column_type(ppStmt, k);
      /* mexPrintf("subs = %d %d, coltyp = %d, mxCalcSingleSubscript = %d\n",
	 subs[0], subs[1], coltyp,
		mxCalcSingleSubscript(trowmajor, 2, subs)); */
      
      switch( coltyp) {
      case SQLITE_FLOAT:
	rnum = mxCreateDoubleMatrix(1, 1, mxREAL);
	*mxGetPr(rnum) = sqlite3_column_double(ppStmt, k);
	mxSetCell(trowmajor, 
		  mxCalcSingleSubscript(trowmajor, 2, subs),
		  rnum);
	break;
      case SQLITE_INTEGER:
	rnum = mxCreateDoubleMatrix(1, 1, mxREAL);
	*mxGetPr(rnum) = (double) sqlite3_column_int(ppStmt, k);
	mxSetCell(trowmajor, 
		  mxCalcSingleSubscript(trowmajor, 2, subs),
		  rnum);
	break;
      case SQLITE_NULL: 
	rnum = mxCreateDoubleMatrix(1, 1, mxREAL);
	valp = mxGetPr(rnum);
	*mxGetPr(rnum) = NAN;
	mxSetCell(trowmajor, 
		  mxCalcSingleSubscript(trowmajor, 2, subs),
		  rnum);
	break;
      case SQLITE_TEXT:
	mxSetCell(trowmajor, 
		  mxCalcSingleSubscript(trowmajor, 2, subs),
		  mxCreateString(sqlite3_column_text(ppStmt, k)));
	isnumeric = 0;
	break;
      case SQLITE_BLOB:
	nblob = sqlite3_column_bytes(ppStmt, k);
	blob = mxCreateCharArray(1, &nblob);
	memcpy(mxGetPr(blob), sqlite3_column_blob(ppStmt, k), nblob);
	mxSetCell(trowmajor, 
		  mxCalcSingleSubscript(trowmajor, 2, subs),
		  blob);
	isnumeric = 0;
	break;
      }
    }
  }
  /* mexPrintf("last return from sqlite3_step was %d\n", istep); */
  if( istep!=SQLITE_DONE ) {
    if( mxIsChar(prhs[0]) ) {
      mexCallMATLAB(1, &mxRet, 1, &mxppDb, "sql_close");
      mxDestroyArray(mxppDb);
      mxDestroyArray(mxRet);
    }
    mexErrMsgIdAndTxt("Sqlite4m:sql_select:done", sqlite3_errmsg(ppDb));
  }

  iret = sqlite3_finalize(ppStmt);
  /* mexPrintf("sqlite3_finalize returned %d\n", iret); */
  if( iret!=SQLITE_OK ) {
    if( mxIsChar(prhs[0]) ) {
      mexCallMATLAB(1, &mxRet, 1, &mxppDb, "sql_close");
      mxDestroyArray(mxppDb);
      mxDestroyArray(mxRet);
    }
    mexErrMsgIdAndTxt("Sqlite4m:sql_select:finalize", sqlite3_errmsg(ppDb));
  }

/* if the first argument was a string, then close the database connection  */
  if( mxIsChar(prhs[0]) ) {
    mexCallMATLAB(1, &mxRet, 1, &mxppDb, "sql_close");
    mxDestroyArray(mxppDb);
    mxDestroyArray(mxRet);
  }
  
  /* Transpose the row-by-row cell array into a double matrix
     if all elements are numeric: */
  if( isnumeric ) {
    plhs[0] = mxCreateDoubleMatrix(mxGetN(trowmajor),
				   mxGetM(trowmajor), mxREAL);
    /* Traverse the temporary row major array column by column,
       then the output array can be filled linearly: */
    k = 0;
    for( i=0; i<mxGetM(trowmajor); i++ ) {
      subs[0] = i;
      for( j=0; j<mxGetN(trowmajor); j++ ) {
	subs[1] = j;
	mxGetPr(plhs[0])[k++] =
	  *mxGetPr(mxGetCell(trowmajor,
			     mxCalcSingleSubscript(trowmajor, 2, subs)));
      }
    }
    mxDestroyArray(trowmajor);
  /*  or else use Matlab transpose to copy into the output cell array,
        if at least one element is text or a blob: */
  } else
    mexCallMATLAB(1, plhs, 1, &trowmajor, "transpose");
}
