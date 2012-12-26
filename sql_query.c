#include <math.h>
#include <string.h>

#include "mex.h"
#include "matrix.h"
#include "sqlite3.h"

/*
 * sql_query.c
 * Select data from sqlite database into matlab data structures
 *
 * This is a MEX-file for MATLAB.
*/

/* The gateway function */
void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
/* variable declarations here */
  mwSize buflen, subs[2], nblob;
  mxArray *mxppDb, *mxRet, *trowmajor, *rnum, *blob;
  char *filename, *zsql, *tp;
  const char *pzTail;
  int i, j, k, iret, istep, irow, ncol;
  int isnumeric=1, arg_has_select, coltyp;
  sqlite3 *ppDb;
  sqlite3_stmt *ppStmt;
  double *valp;

  char *defselect =
    "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;";

/* check for proper number of arguments */
  if( nrhs<2) {
    mexErrMsgIdAndTxt("Sqlite4m:sql_select:nrhs",
                      "at least two input arguments are required.");
  }
  /* Make sure the input arguments after the first are strings: */
  for( k=1; k<nrhs; k++ )
    if( !mxIsChar(prhs[k]) ) {
      mexErrMsgIdAndTxt("Sqlite4m:sql_select::notChar",
			"String input expected.");
    }

/* if the first argument is a string, then open a database connection  */
  if( mxIsChar(prhs[0]) ) {
    mexCallMATLAB(1, &mxppDb, 1, (mxArray **) &prhs[0], "sql_open");
    ppDb = *((sqlite3 **) mxGetData(mxppDb));
  } else
    ppDb = *((sqlite3 **) mxGetData(prhs[0]));

  if( strlen(mxArrayToString(prhs[1]))>0 ) {  
    /* Allow for input where the second argument
       contains the complete select statement,
       then it should include the string "SELECT" */
    arg_has_select = (strstr(mxArrayToString(prhs[1]), "SELECT")!=0);
    /* Determine how much buffer is needed for the SQL select statement: */
    if( arg_has_select )
      buflen = 0;
    else
      buflen = strlen("SELECT ")*sizeof(mxChar)+1;
    buflen += mxGetN(prhs[1])*sizeof(mxChar);
    if( nrhs>=3 ) {
      if( strlen(mxArrayToString(prhs[2]))>0 ) {
	buflen += strlen(" FROM ")*sizeof(mxChar);
	buflen += mxGetN(prhs[2])*sizeof(mxChar);
      }
      if( nrhs>=4 ) {
	if( strlen(mxArrayToString(prhs[3]))>0 ) {
	  buflen += strlen(" WHERE ")*sizeof(mxChar);
	  buflen += mxGetN(prhs[3])*sizeof(mxChar);
	  if( nrhs>=5 ) {
	    if( strlen(mxArrayToString(prhs[4]))>0 ) {
	      buflen += strlen(" ORDER BY ")*sizeof(mxChar);
	      buflen += mxGetN(prhs[4])*sizeof(mxChar);
	    }
	  }
	}
      }
    }

    buflen += strlen(";")*sizeof(mxChar);
    zsql = mxMalloc(buflen);

    /* Now set up the SQL select statement: */
    if( arg_has_select )
      strcpy(zsql, "");
    else
      strcpy(zsql, "SELECT ");
    iret = mxGetString(prhs[1], zsql+strlen(zsql), buflen-strlen(zsql));
    if( nrhs>=3 ) {
      if( strlen(mxArrayToString(prhs[2]))>0 ) {
	strcat(zsql, " FROM ");
	iret = mxGetString(prhs[2], zsql+strlen(zsql), buflen-strlen(zsql));
      }
      if( nrhs>=4 ) {
	if( strlen(mxArrayToString(prhs[3]))>0 ) {
	  strcat(zsql, " WHERE ");
	  iret = mxGetString(prhs[3], zsql+strlen(zsql), buflen-strlen(zsql));
	  if( nrhs>=5 ) {
	    if( strlen(mxArrayToString(prhs[4]))>0 ) {
	      strcat(zsql, " ORDER BY ");
	      iret = mxGetString(prhs[4], zsql+strlen(zsql), buflen-strlen(zsql));
	    }
	  }
	}
      }
    }
    strcat(zsql, ";");
  } else
    zsql = defselect;
  /* mexPrintf("%s\n", zsql); */

  /* Return the statement in the second optional output argument*/
  if( nlhs>=2 )
    if( zsql!=defselect )
      plhs[1] = mxCreateString(zsql);
    else { /* Need to copy zsql into dynamic memory */
      tp = mxMalloc(sizeof(char)*(strlen(zsql)+1));
      strcpy(tp, zsql);
      plhs[1] = mxCreateString(tp);
    }

  /* Prepare the SQL library for the select statement: */
  iret = sqlite3_prepare_v2(ppDb, zsql, strlen(zsql), &ppStmt, &pzTail);
  /* mexPrintf("sqlite3_prepare_v2 returned %d\n", iret); */
  if( iret!=SQLITE_OK )
    mexErrMsgIdAndTxt("Sqlite4m:sql_select:prepare", sqlite3_errmsg(ppDb));
  /* The select statement is no more needed? */
  if( zsql!=defselect )
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
  if( istep!=SQLITE_DONE )
    mexErrMsgIdAndTxt("Sqlite4m:sql_select:done", sqlite3_errmsg(ppDb));

  iret = sqlite3_finalize(ppStmt);
  /* mexPrintf("sqlite3_finalize returned %d\n", iret); */
  if( iret!=SQLITE_OK )
    mexErrMsgIdAndTxt("Sqlite4m:sql_select:finalize", sqlite3_errmsg(ppDb));

/* if the first argument was a string, then close the database connection  */
  if( mxIsChar(prhs[0]) )
    mexCallMATLAB(1, &mxRet, 1, &mxppDb, "sql_close");

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
