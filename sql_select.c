/*
 * sql_select.c
 * Execute an sql select statement, return the results
 *    as matlab variables.
 *
 * This is a MEX-file for MATLAB.
*/

#include <math.h>
#include <string.h>
#include <stdint.h>

#include "mex.h"
#include "matrix.h"
#include "sqlite3.h"
#include "sqlite4m.h"

#define MEXFUN select

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  mwSize subs[2];
  mxArray *trowmajor;
  char *zsql, *strbuf;
  int i, j, k, iret, istep, irow=0, nrow_alloc=0, ncol, kval, nval, kk, iargbind=4;
  int isnumeric=1,  ldestroy=1, coltyp, classid;

  int n_par_tobind=0, n_val_tobind=1;
  const mxArray *mxBindPar=NULL;

  strcat(errmsgstr1, "select:");

/* check for proper number of arguments */
  if( nrhs<2)
    ERRABORT("nrhs", "at least two input arguments are needed.");

  /* Make sure that input arguments 2...2nd last are strings: */
  for( k=2; k<nrhs-1; k++ )
    if( !mxIsChar(prhs[k]) )
      ERRABORT("notChar", "After the 2nd argument only string input is expected.");

/* if the first argument is a string, then open a database connection  */
  if( mxIsChar(prhs[0]) ) {
    mexCallMATLAB(1, &mxppDb, 1, (mxArray **) &prhs[0], "sql_open");
    ppDb = *((sqlite3 **) mxGetData(mxppDb));
  } else
    ppDb = *((sqlite3 **) mxGetData(prhs[0]));

  zsql = getzsql(nrhs, prhs, &iargbind);
  /* mexPrintf("zsql = %s\n", zsql); */

  /* Prepare the SQL library for the select statement: */
  iret = sqlite3_prepare_v2(ppDb, zsql, strlen(zsql), &ppStmt, NULL);
  /* mexPrintf("sqlite3_prepare_v2 returned %d\n", iret); */

  /* Return the statement in the second optional output argument*/
  if( nlhs>=2 )
    plhs[1] = mxCreateString(zsql);
  else
    mxFree(zsql);

  if( iret!=SQLITE_OK ) {
    char errmsgbuf[strlen(sqlite3_errmsg(ppDb))+1];
    strcpy(errmsgbuf, sqlite3_errmsg(ppDb));
    CLOSE_DB;
    ERRABORT("prepare", errmsgbuf);
  }

  /* If there are parameters to bind, check the matlab variables
     and find out how many parameters/values: */
  bind_m2sql(ppStmt, iargbind, prhs, &n_par_tobind, &n_val_tobind);

  /* How many columns? */
  ncol = sqlite3_column_count(ppStmt);
  /* mexPrintf("sqlite3_column_count returned %d\n", ncol); */

  /* The numbers of rows is not known at this point. Therefore
     the elements are first stored row-by-row in a temporary cell array,
     so that memory can be reallocated as database rows are added. */
  trowmajor = mxCreateCellMatrix(ncol, 0);

  /* Loop over the values that need to be bound to parameters, 
     if no parameters to bind n_val_tobind=1 and the loop is executed once: */
  for( kval=0; kval<n_val_tobind; kval++ ) {
    BIND_MATPAR(MEXFUN)

    /* Step along the selected rows: */
    while( (istep = sqlite3_step(ppStmt))==SQLITE_ROW ) {
      /* A row is available,
	 if needed then enlarge the output cell array: */
      if( irow>=nrow_alloc ) {
	/* mexPrintf("irow = %d, nrow_alloc = %d\n", irow, nrow_alloc);*/
	if( nrow_alloc==0 )  /* The first row is found */
	  nrow_alloc++;
	else
	  nrow_alloc *= 2;
	mxSetData(trowmajor,
		  mxRealloc(mxGetData(trowmajor),
			    sizeof(mxArray *)*nrow_alloc*ncol));
	/* mexPrintf("%d bytes reallocated\n", sizeof(mtype)*nrow_alloc*ncol); */
      }
      /* Increment the nr of columns of the output matrix by one: */ 
      mxSetN(trowmajor, mxGetN(trowmajor)+1);
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
	  mxArray *rnum;
	  int nblob;
	case SQLITE_NULL: /* NULL values are mapped in Matlab to empty cells: */
	  /* mexPrintf("NULL selected at ncol = %d and irow %d\n", subs[0], subs[1]); */
	  rnum = mxCreateDoubleMatrix(0, 1, mxREAL);
	  mxSetCell(trowmajor, 
		    mxCalcSingleSubscript(trowmajor, 2, subs), rnum);
	  isnumeric = 0;
	  break;
	case SQLITE_TEXT:
	  mxSetCell(trowmajor, 
		    mxCalcSingleSubscript(trowmajor, 2, subs),
		    mxCreateString(sqlite3_column_text(ppStmt, k)));
	  isnumeric = 0;
	  break;
	case SQLITE_BLOB:
	  nblob = sqlite3_column_bytes(ppStmt, k);
	  mxArray *blob = mxCreateNumericMatrix(1, nblob, mxUINT8_CLASS, mxREAL);
	  memcpy(mxGetPr(blob), sqlite3_column_blob(ppStmt, k), nblob);
	  mxSetCell(trowmajor, 
		    mxCalcSingleSubscript(trowmajor, 2, subs), blob);
	  isnumeric = 0;
	  break;
	case SQLITE_FLOAT:
	  rnum = mxCreateDoubleMatrix(1, 1, mxREAL);
	  *mxGetPr(rnum) = sqlite3_column_double(ppStmt, k);
	  mxSetCell(trowmajor, 
		    mxCalcSingleSubscript(trowmajor, 2, subs), rnum);
	  break;
	default:
	  sqlite3_finalize(ppStmt);
	  CLOSE_DB;
	  ERRABORT("column_type", "db should not have this column type");
	}
      }
    }
    /* mexPrintf("last return from sqlite3_step was %d\n", istep); */
    if( istep!=SQLITE_DONE ) {
      char errmsgbuf[strlen(sqlite3_errmsg(ppDb))+1];
      strcpy(errmsgbuf, sqlite3_errmsg(ppDb));
      sqlite3_finalize(ppStmt);
      CLOSE_DB;
      ERRABORT("done", sqlite3_errmsg(ppDb));
    }

    iret = sqlite3_reset(ppStmt);
    if( iret!=SQLITE_OK )
      mexWarnMsgIdAndTxt("Sqlite4m:sql_insert:reset", sqlite3_errmsg(ppDb));
  }
  TRY_SQLFUN(sqlite3_finalize(ppStmt), select, finalize);
  /* Release memory to the actual nr of db rows/matlab columns: */
  mxSetData(trowmajor,
	    mxRealloc(mxGetData(trowmajor),
		      sizeof(mxArray *)*mxGetN(trowmajor)*ncol));
  /* mexPrintf("final allocation is %d bytes\n", sizeof(mtype)*mxGetN(plhs[0])*ncol); */

  /* if the first argument was a string, then close the database connection  */
  CLOSE_DB;

  /* Transpose the row-by-row cell array into a double matrix
     if all elements are numeric: */
  /* if( isnumeric ) { */
  /*   plhs[0] = mxCreateDoubleMatrix(mxGetN(trowmajor), */
  /* 				   mxGetM(trowmajor), mxREAL); */
  /*   /\* Traverse the temporary row major array column by column, */
  /*      then the output array can be filled linearly: *\/ */
  /*   k = 0; */
  /*   for( i=0; i<mxGetM(trowmajor); i++ ) { */
  /*     subs[0] = i; */
  /*     for( j=0; j<mxGetN(trowmajor); j++ ) { */
  /* 	subs[1] = j; */
  /* 	mxGetPr(plhs[0])[k++] = */
  /* 	  *mxGetPr(mxGetCell(trowmajor, */
  /* 			     mxCalcSingleSubscript(trowmajor, 2, subs))); */
  /*     } */
  /*   } */
  /*   mxDestroyArray(trowmajor); */
  /* /\*  or else use Matlab transpose to copy into the output cell array, */
  /*       if at least one element is text or a blob: *\/ */
  /* } else */
    mexCallMATLAB(1, plhs, 1, &trowmajor, "transpose");
}
