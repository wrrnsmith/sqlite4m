/*
 * sql_select_numeric.c
 * Select numeric data from an sqlite database, and
 *    return results in a matlab matrix.
 *
 * This is a MEX-file for MATLAB.
 */

#include <math.h>
#include <string.h>
#include <stdint.h>

#include "sqlite4m.h"

#define MEXFUN select_numeric

#define DB2MAT(mtype, btype)						\
  /* Get selected db rows to floating point (single or double),   */	\
  /*   db NULLs are converted to IEEE NANs.                       */	\
  /* A db row is returned n_val_tobind times, where n_val_tobind  */	\
  /* is the (largest) size of the bind parameter vectors.         */	\
  /* If there are no parameters to bind,                          */	\
  /* then n_val_tobind must be 1, and the selected rows           */	\
  /* are retrieved once:                                          */	\
  do {									\
  for( kval=0; kval<n_val_tobind; kval++ ) {				\
    BIND_MATPAR(MEXFUN);						\
						                        \
    /* Loop over all selected rows: */					\
    while( (istep = sqlite3_step(ppStmt))==SQLITE_ROW ) {		\
    /* A row is available,						\
       if needed then enlarge the output cell array: */			\
      if( irow>=nrow_alloc ) {						\
	/* mexPrintf("irow = %d, nrow_alloc = %d\n", irow, nrow_alloc);	*/ \
	if( nrow_alloc==0 )  /* The first row is found */		\
	  nrow_alloc++;							\
	else								\
	  nrow_alloc *= 2;						\
	mxSetData(plhs[0],						\
		  mxRealloc(mxGetData(plhs[0]),				\
			    sizeof(mtype)*nrow_alloc*ncol));		\
	/* mexPrintf("%d bytes reallocated\n", sizeof(mtype)*nrow_alloc*ncol); */ \
      }									\
      /* Increment the nr of columns of the output matrix by one: */	\
      mxSetN(plhs[0], mxGetN(plhs[0])+1);				\
      /* if( irow % 1000==0 )*/						\
      /*	mexPrintf("plhs enlarged to %d %d\n",			\
		  mxGetM(plhs[0]), mxGetN(plhs[0]));			\
      */								\
      for( k=0; k<ncol; k++ )						\
	/*Return IEEE NAN, if the database contains NULL*/		\
	((mtype*) mxGetData(plhs[0]))[irow*ncol + k] =			\
	  sqlite3_column_type(ppStmt, k)==SQLITE_NULL?			\
	  NAN : sqlite3_column_ ## btype(ppStmt, k);			\
	  irow++;							\
    }									\
    /* Check whether sqlite3_step did ok: */				\
    if( istep!=SQLITE_DONE ) {						\
      char errmsgbuf[strlen(sqlite3_errmsg(ppDb))+1];			\
      strcpy(errmsgbuf, sqlite3_errmsg(ppDb));				\
      sqlite3_finalize(ppStmt);						\
      CLOSE_DB;								\
      mexErrMsgIdAndTxt("Sqlite4m:sql_select_numeric:step",		\
			errmsgbuf);					\
    }									\
    TRY_SQLFUN_FIN(sqlite3_reset(ppStmt), MEXFUN, reset);		\
  }									\
  /* Release memory to the actual nr of db rows/matlab columns: */	\
  mxSetData(plhs[0],							\
	    mxRealloc(mxGetData(plhs[0]),				\
		      sizeof(mtype)*mxGetN(plhs[0])*ncol));		\
  /* mexPrintf("final allocation is %d bytes\n", sizeof(mtype)*mxGetN(plhs[0])*ncol); */ \
  } while(0)

#define DB2MAT_INT(mtype, btype)					\
  /* Get selected db rows to integer,                             */	\
  /*   no conversion of db NULLs is done (sqlite converts to 0).  */	\
  /* A db row is returned n_val_tobind times, where              */	\
  /* n_val_tobind is the (largest) size of the parameter vector.  */	\
  /* If there are no parameters to bind,                          */	\
  /* then n_val_tobind is set above to one, and the selected rows */	\
  /* are retrieved once:                                          */	\
  do {									\
  for( kval=0; kval<n_val_tobind; kval++ ) {				\
    BIND_MATPAR(MEXFUN);						\
						                        \
    /* Loop over all selected rows: */					\
    while( (istep = sqlite3_step(ppStmt))==SQLITE_ROW ) {		\
    /* A row is available,						\
       if needed then enlarge the output cell array: */			\
      if( irow>=nrow_alloc ) {						\
	if( nrow_alloc==0 )  /* The first row is found */		\
	  nrow_alloc++;							\
	else								\
	  nrow_alloc *= 2;						\
	mxSetData(plhs[0],						\
		  mxRealloc(mxGetData(plhs[0]),				\
			    sizeof(mtype)*nrow_alloc*ncol));		\
      }									\
      /* Increment the nr of columns of the output matrix by one: */	\
      mxSetN(plhs[0], mxGetN(plhs[0])+1);				\
      for( k=0; k<ncol; k++ )						\
	((mtype*) mxGetData(plhs[0]))[irow*ncol + k] =			\
	  sqlite3_column_ ## btype(ppStmt, k);				\
	  irow++;							\
   }									\
    TRY_SQLFUN_FIN(sqlite3_reset(ppStmt), MEXFUN, reset);		\
  }									\
  /* Release memory to the actual nr of db rows/matlab columns: */	\
  mxSetData(plhs[0],							\
	    mxRealloc(mxGetData(plhs[0]),				\
		      sizeof(mtype)*mxGetN(plhs[0])*ncol));		\
  } while(0)

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
/* variable declarations here */
  char *zsql, *strbuf;
  int k, iret, istep, irow=0, nrow_alloc=0, kk, kval, nval, ncol, iargbind=4;
  int classid, ldestroy=1;
  int n_par_tobind=0, n_val_tobind=1;
  const mxArray *mxBindPar=NULL;

  strcat(errmsgstr1, "select_numeric:");

/* check for proper number of arguments */
  if( nrhs<2)
    ERRABORT("nrhs", "at least two input arguments are needed.");

  /* Make sure that input arguments 2...3rd last are strings: */
  /* for( k=2; k<nrhs-2; k++ )
     if( !mxIsChar(prhs[k]) )
     ERRABORT("notChar", "After the 2nd argument string input is expected.");
  */

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
  mxBindPar = bind_m2sql(ppStmt, iargbind, prhs, &n_par_tobind, &n_val_tobind);
  /* mexPrintf("n_par_tobind = %d, n_val_tobind = %d\n", n_par_tobind, n_val_tobind); */

  /* How many columns? */
  ncol = sqlite3_column_count(ppStmt);
  /* mexPrintf("ncol = %d\n", ncol); */

  /* If there is a numeric last argument, it specifies which type to return.
     The default is double, so return a double matrix if there is no numeric
     last argument or a last numeric argument explicitely specifies double: */
  if( !mxIsNumeric(prhs[nrhs-1]) || mxGetClassID(prhs[nrhs-1])==mxDOUBLE_CLASS ) {
    plhs[0] = mxCreateNumericMatrix(ncol, 0, mxDOUBLE_CLASS, mxREAL);

    DB2MAT(double, double);
  } else if( nrhs>=3 && mxIsNumeric(prhs[nrhs-1]) ) {
    /* A numeric last argument specifies something else than double. */
    /* mexPrintf("numeric last argument %d found\n", *((int *) mxGetData(prhs[nrhs-1]))); */
    
    classid = mxGetClassID(prhs[nrhs-1]);
    switch( classid ) {
    case mxSINGLE_CLASS:
      plhs[0] = mxCreateNumericMatrix(ncol, 0, classid, mxREAL);
      DB2MAT(float, double);
      break;
    case mxUINT8_CLASS:
      plhs[0] = mxCreateNumericMatrix(ncol, 0, mxUINT8_CLASS, mxREAL);
      DB2MAT_INT(uint8_t, int);
      break;
    case mxINT8_CLASS:
      plhs[0] = mxCreateNumericMatrix(ncol, 0, mxINT8_CLASS, mxREAL);
      DB2MAT_INT(int8_t, int);
      break;
    case mxUINT16_CLASS:
      plhs[0] = mxCreateNumericMatrix(ncol, 0, mxUINT16_CLASS, mxREAL);
      DB2MAT_INT(uint16_t, int);
      break;
    case mxINT16_CLASS:
      plhs[0] = mxCreateNumericMatrix(ncol, 0, mxINT16_CLASS, mxREAL);
      DB2MAT_INT(int16_t, int);
      break;
    case mxUINT32_CLASS:
      plhs[0] = mxCreateNumericMatrix(ncol, 0, mxUINT32_CLASS, mxREAL);
      DB2MAT_INT(uint32_t, int);
      break;
    case mxINT32_CLASS:
      plhs[0] = mxCreateNumericMatrix(ncol, 0, mxINT32_CLASS, mxREAL);
      DB2MAT_INT(int32_t, int);
      break;
    case mxUINT64_CLASS:
      plhs[0] = mxCreateNumericMatrix(ncol, 0, mxUINT64_CLASS, mxREAL);
      DB2MAT_INT(uint64_t, int64);
      break;
    case mxINT64_CLASS:
      plhs[0] = mxCreateNumericMatrix(ncol, 0, mxINT64_CLASS, mxREAL);
      DB2MAT_INT(int64_t, int64);
      break;

    default:
      CLOSE_DB;
      ERRABORT("category", "this function cannot return db elements of this type");
    }
  }
  if( istep!=SQLITE_DONE ) {
    CLOSE_DB;
    ERRABORT("done", sqlite3_errmsg(ppDb));
  }

  TRY_SQLFUN(sqlite3_finalize(ppStmt), select_numeric, finalize);

/* if the first argument was a string, then close the database connection  */
  CLOSE_DB;
}
