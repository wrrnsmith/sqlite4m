#include <math.h>
#include <string.h>
#include <stdint.h>

#include "mex.h"
#include "matrix.h"
#include "sqlite3.h"
#include "sqlite4m.h"

/*
 * sql_insert.c
 * Inserts Matlab matrices into an sqlite database table
 *
 * This is a MEX-file for MATLAB.
 */

/* Macros for inserting Matlab matrix type "mtype" as database type "btype."
   "btype" can be double, int or int64.
   "xvar" holds temporarily one matrix/table element. It must be of suitable
   type to get the right type conversions. The macro is used like:

   LOOP_ROW_COL(float, double, x)
   (btype is double because sqlite uses always double precision),

   LOOP_ROW_COL(double, double, x)

   LOOP_ROW_COL(mxLogical, int, ix)
   LOOP_ROW_COL(int16_t, int, ix)
   LOOP_ROW_COL(uint16_t, int, ix)
   LOOP_ROW_COL(int32_t, int, ix)

   LOOP_ROW_COL(uint32_t, int64, i64x)
   LOOP_ROW_COL(int64_t, int64, i64x)
   LOOP_ROW_COL(uint64_t, int64, i64x)
*/
#define LOOP_ROW_COL(mtype, btype, xvar)				\
  do { for( krow=0; krow<nrow; krow++ ) {				\
      for( k=0; k<ncol; k++ ) {						\
	xvar = ((mtype *) mxGetData(prhs[ilast]))[k*nrow+krow];		\
	TRY_SQLFUN_FIN_RB(sqlite3_bind_ ## btype(ppStmt, k+1, xvar),	\
			      insert, bind_btype);			\
	  }								\
      iret = step_reset(ppStmt, ppDb);					\
    }} while(0)

/* Similar as the first macro, but bind only the krow'th element
   from a row vector of cells:  */
#define BIND_ONE_I(mtype, btype, xvar)					\
  do { xvar = ((mtype *) mxGetData(mxx))[krow];				\
  TRY_SQLFUN_FIN_RB(sqlite3_bind_ ## btype(ppStmt, k+1, xvar),	\
			insert, bind_btype); } while(0)

/* Bind a column of matlab bytes (int8/uint8) to a db blob: */
#define BIND_COL_BLOB(mtype, xvar, nrow_blob)				\
  do { xvar = &(((mtype *) mxGetData(mxx))[krow*nrow_blob]);		\
    TRY_SQLFUN_FIN_RB(sqlite3_bind_blob(ppStmt, k+1, xvar, nrow_blob, SQLITE_STATIC), \
			insert, bind_blob); } while(0)

/* Like previous macro, but bind exactly the first and only element
   in a matrix of cells:  */
#define BIND_ONE_0(mtype, btype, xvar)					\
  do { xvar = ((mtype *) mxGetData(mxx))[0];				\
  TRY_SQLFUN_FIN_RB(sqlite3_bind_ ## btype(ppStmt, k+1, xvar),	\
			insert, bind_btype); } while(0)

/* Similar as the first macro, but loops only over one row
   (for cells with column vectors, each vector has same length): 
*/
#define LOOP_COL(mtype, btype, xvar)					\
  do { for( k=0; k<ncol2; k++ ) {					\
    xvar = ((mtype *) mxGetData(mxx))[k];				\
    TRY_SQLFUN_FIN_RB(sqlite3_bind_ ## btype(ppStmt, k+1, xvar),	\
			  insert, bind_btype);				\
      } } while(0)

/* Loop over db columns, bind in each column a column of matlab bytes (int8/uint8) to a db blob: */
#define LOOP_COL_BLOB(mtype, xvar, nrow_blob)				\
  do { for( k=0; k<ncol2; k++ ) {					\
      xvar = &(((mtype *) mxGetData(mxx))[k*nrow_blob]);		\
      TRY_SQLFUN_FIN_RB(sqlite3_bind_blob(ppStmt, k+1, xvar, nrow_blob, SQLITE_STATIC), \
			  insert, bind_blob);				\
      } } while(0)

#define BIND_BLOBS(mtype)						\
  do { for( krow=0; krow<nrow; krow++ ) {				\
    TRY_SQLFUN_FIN_RB(sqlite3_bind_blob(ppStmt, 1,			\
					    (mtype *) mxGetData(prhs[ilast]) + krow*ncol, \
					    ncol, SQLITE_STATIC),	\
			  insert, bind_blobs);				\
      iret = step_reset(ppStmt, ppDb);					\
    } } while(0)

#define BIND_ONE_BLOB(mtype)						\
  do { TRY_SQLFUN_FIN_RB(sqlite3_bind_blob(ppStmt, 1,		\
				 (mtype *) mxGetData(mxx),		\
				 mxGetM(mxx)*mxGetN(mxx),		\
					       SQLITE_STATIC),		\
			     insert, bind_blob); } while(0)

#define BIND_TEXT_I(kpos)						\
  do { strbuf = mxArrayToString(mxx);				\
  nchar = strlen(strbuf);						\
  TRY_SQLFUN_FIN_RB(sqlite3_bind_text(ppStmt, kpos, strbuf, nchar,	\
					  mxFree),			\
			insert, bind_text); } while(0)		
  /* mexPrintf("TEXT \"%s\" bound to %d\n", strbuf, k+1); */

int step_reset(sqlite3_stmt *ps, sqlite3 *pdb) {
  int iret;

  iret = sqlite3_step(ps);
  if( iret!=SQLITE_DONE && iret!=SQLITE_OK )
    return iret;
  /* After version 3.6.23.1 sqlite3_step calls also sqlite3_reset,
     but sqlite3_reset seems to be still needed?*/
  iret = sqlite3_reset(ps);
  if( iret!=SQLITE_OK )
    return iret;

  return sqlite3_clear_bindings(ps);
}

/* Get length of a matlab array,
 empty, character and byte (int8, uint8) arrays have length 1,
 because in the data base they will become NULL or blob: */
size_t GetLength( const mxArray *mxx ) {
  mxClassID cid;

  cid = mxGetClassID(mxx);
  if( mxIsEmpty(mxx) || cid==mxCHAR_CLASS || cid==mxINT8_CLASS || cid==mxUINT8_CLASS )
    return 1;
  else
    return mxGetM(mxx)*mxGetN(mxx);
}

/* Get length of a matlab array:
   - an empty array has length 1, because in the data base it will become NULL,
   - int8 and uint8 have a length, which is the nr of columns,
     because each column is inserted as a blob,
   - other types have a length which is nr of rows times nr of columns 
*/
size_t GetLength2( const mxArray *mxx ) {
  mxClassID cid = mxGetClassID(mxx);
  if( mxIsEmpty(mxx) )
    return 1;
  else {
    mxClassID cid = mxGetClassID(mxx);
    if( cid==mxINT8_CLASS || cid==mxUINT8_CLASS )
      return mxGetN(mxx);
    else
      return mxGetM(mxx)*mxGetN(mxx);
  }
}

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  int ilast, iret, krow, nrow, k, ncol, ncol2, nchar;
  double x;
  int ix;
  int8_t *bx;
  uint8_t *ux;
  sqlite3_int64 i64x;
  mxArray *mxTable=0, *mxColList, *mxStr;
  const mxArray *mxTableName, *mxArgs[3], *mxx;
  char *zsql, *strbuf;
  int arg_has_insert, ldestroy=1, nrow2;

  strcat(errmsgstr1, "insert:");

  /* Assert that there are at least two input arguments */
  if( nrhs<2 )
    ERRABORT("nrhs", "at least two input arguments are needed.");
  
  if( nlhs>1 )
    ERRABORT("nlhs", "Not more than one output argument can be assigned.");

  ilast = nrhs-1;
  if( mxGetNumberOfDimensions(prhs[ilast])>2 )
    mexErrMsgIdAndTxt("Sqlite4m:sql_insert:",
		      "cannot insert variables with more than two dimensions");

/* if the first argument is a string, then open a database connection  */
  if( mxIsChar(prhs[0]) ) {
    iret = mexCallMATLAB(1, &mxppDb, 1, (mxArray **) &prhs[0], "sql_open");
    if( iret!=0 )
      mexErrMsgIdAndTxt("Sqlite4m:sql_insert:open",
			"open of database failed");
    ppDb = *((sqlite3 **) mxGetData(mxppDb));
  } else { /* otherwise the first argument should be the pointer to an already opened database: */
    ppDb = *((sqlite3 **) mxGetData(prhs[0]));
    mxppDb = mxCreateDoubleScalar(NAN);
    ((sqlite3 **) mxGetData(mxppDb))[0] = ppDb;
  }

/* 2nd argument is the table name or can be an index into the list of table names.
   If there are only 2 arguments, the 2nd argument is the variable to insert.
   Then insert into the first table returned by "sql_tables": */
  if( nrhs==2 || (nrhs>2 && (mxIsEmpty(prhs[1]) || mxIsNumeric(prhs[1]))) ) {
     mexCallMATLAB(1, &mxTable, 1, &mxppDb, "sql_tables");
     mxTableName = mxGetCell(mxTable,
			     mxIsEmpty(prhs[1])? 0 : mxGetPr(prhs[1])[0]);
     arg_has_insert = 0;
  } else {
    mxTableName = prhs[1];
    strbuf = mxArrayToString(prhs[1]);
    arg_has_insert = strstr(strbuf, "INSERT")!=0 || strstr(strbuf, "REPLACE")!=0;
    mxFree(strbuf);
  }

/* 3rd argument, if present, is the table column names or numbers, one of either
   1) if empty insert into all columns;
   2) a string of comma separated column names like "col1, col2, col3";
   3) a cell array each cell containing one column name;
   4) a numeric array with column numbers.*/
  if( nrhs==2 || nrhs==3 || mxIsEmpty(prhs[2]) )
    mxColList = mxCreateString("");
  else if( mxIsChar(prhs[2]) || mxIsCell(prhs[2]) ) {
    mxColList = (mxArray *) prhs[2];
    ldestroy = 0;
  } else if( mxIsNumeric(prhs[2]) ) {
    mxArgs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
    *((sqlite3 **) mxGetData(mxArgs[0])) = ppDb;
    mxArgs[1] = mxTableName;
    mxArgs[2] = prhs[2];
    mexCallMATLAB(1, &mxColList, 3, (mxArray **) mxArgs, "sql_columnnames");
  } else {
    CLOSE_DB;
    mexErrMsgIdAndTxt("Sqlite4m:sql_select_numeric::colnames",
			"Which columns?");
  }

  ncol = mxGetN(prhs[ilast]);
  nrow = mxGetM(prhs[ilast]);
  ncol2 = 0;

  /* Column vector of cells can be inserted if all cells have the same length.
     Then determine the length of the cells, ncol2: */
  if( (mxGetClassID(prhs[ilast])==mxCELL_CLASS || mxGetClassID(prhs[ilast])==mxSTRUCT_CLASS) && ncol==1 )
    /* Assert that each cell has the same length: */
    for( krow=0; krow<nrow; krow++ ) {
      mxx = ((mxArray **) mxGetData(prhs[ilast]))[krow];
      if( ncol2==0 )
	ncol2 = GetLength(mxx);
      else
	if( GetLength(mxx)!=ncol2 )
	  mexErrMsgIdAndTxt("Sqlite4m:sql_insert:length",
			    "cells must contain arrays with same length");
    }
  /* mexPrintf("ncol = %d, nrow = %d, ncol2 = %d\n", ncol, nrow, ncol2); */

  /* Start the string of the insert statement: */
  if( arg_has_insert ) {
    zsql = mxMalloc(strlen("")+3);
    strcpy(zsql, "");
  } else {
    zsql = mxMalloc(strlen("INSERT INTO ")+3);
    strcpy(zsql, "INSERT INTO ");
  }
  
  /* Append the table name: */
  strbuf = mxArrayToString(mxTableName);
  zsql = mxRealloc(zsql, strlen(zsql) + strlen(strbuf) + 3);
  strcat(zsql, strbuf);
  mxFree(strbuf);

  /* Destroy mxTable, in case it has been used */
  mxDestroyArray(mxTable);

  /* Append column names, if there are any: */
  if( mxGetM(mxColList)*mxGetN(mxColList)>0 ) {
    zsql=mxRealloc(zsql, strlen(zsql) + strlen(" (") + 3);
    strcat(zsql, " (");
    if( mxIsCell(mxColList) ) {
      for( k=0; k<mxGetM(mxColList)*mxGetN(mxColList); k++ ) {
	strbuf = mxArrayToString(mxGetCell(mxColList, k));
	zsql = mxRealloc(zsql, strlen(zsql) + strlen(strbuf) + 2 + 3);
	strcat(zsql, strbuf);
	strcat(zsql, ", ");
	mxFree(strbuf);
      }
      /* Overwrite the last ", " with ") "*/
      zsql[strlen(zsql)-2] = ')';
    } else {
      strbuf = mxArrayToString(mxColList);
      zsql = mxRealloc(zsql, strlen(zsql) + strlen(strbuf) + 3);
      strcat(zsql, strbuf);
      strcat(zsql, ")");
      mxFree(strbuf);
    }
  }
  if( ldestroy )
    mxDestroyArray(mxColList);

  zsql=mxRealloc(zsql, strlen(zsql) + strlen(" VALUES (?001") + 3);
  strcat(zsql, " VALUES (");
  /* Floats, integers, structures and cells are inserted as matrices,
     row by row, so the prepare statement needs to contain
     "?001,?002,..." one place holder for each the column: */
  strcat(zsql, "?001");
  /* Character and byte arrays are inserted as text or blob, respectively, "?001" is enough then */ 
  if( mxGetClassID(prhs[ilast])!=mxCHAR_CLASS
      && mxGetClassID(prhs[ilast])!=mxINT8_CLASS && mxGetClassID(prhs[ilast])!=mxUINT8_CLASS )
    if( ncol2==0 ) {
      zsql=mxRealloc(zsql, strlen(zsql) + 5*ncol + 3);
      for( k=2; k<=ncol; k++ )
	sprintf(zsql+strlen(zsql), ",?%.3d", k);
    } else {
      zsql=mxRealloc(zsql, strlen(zsql) + 5*ncol2 + 3);
      for( k=2; k<=ncol2; k++ )
	sprintf(zsql+strlen(zsql), ",?%.3d", k);
    }

  strcat(zsql, ");");

  /* mexPrintf("zsql = %s\n", zsql); */

  /* If the insert statement is bracketed by BEGIN; ... COMMIT;
     this is supposedly faster: */
  TRY_SQLFUN(sqlite3_exec(ppDb, "BEGIN;", NULL, NULL, NULL),
	       insert, begin_transaction);
  /* mexPrintf("BEGIN TRANSACTION;\n"); */

  /* Prepare the insert statement. It is independent of the type (ClassId)
     and can then be reused for each row: */
  TRY_SQLFUN(sqlite3_prepare_v2(ppDb, zsql, strlen(zsql), &ppStmt, NULL),
	       insert, prepare);
    /*  mexPrintf("statement prepared\n"); */

  if( nlhs>=0 )
    plhs[0] = mxCreateString(zsql);
  else
    mxFree(zsql);

  /* First handle the special case of an empty numeric array, insert SQLITE NULL: */
  if( mxIsEmpty(prhs[ilast]) && mxIsNumeric(prhs[ilast]) ) {
    TRY_SQLFUN_FIN_RB(sqlite3_bind_null(ppStmt, 1), insert, bind);
    iret = sqlite3_step(ppStmt);
    if( iret!=SQLITE_DONE ) {
      sqlite3_finalize(ppStmt);
      sqlite3_exec(ppDb, "ROLLBACK;", NULL, NULL, NULL);
      CLOSE_DB;
      mexErrMsgIdAndTxt("Sqlite4m:sql_insert:step",
			sqlite3_errmsg(ppDb)); 
    }
  } else 
    switch( mxGetClassID(prhs[ilast]) ) {
      /* Insert single matrices:
	 Sqlite handles single precision floats internally with double
	 precision, therefore each single precision matrix element gets
	 converted to double.
      */
    case mxSINGLE_CLASS:
      LOOP_ROW_COL(float, double, x);
      break;
    case mxDOUBLE_CLASS:
      for( krow=0; krow<nrow; krow++ ) {
	for( k=0; k<ncol; k++ ) {
	  x = mxGetPr(prhs[ilast])[k*nrow+krow];
	  /* if( mxIsNaN(x) )
	     iret = sqlite3_bind_null(ppStmt, k+1);	      
	     else */
	  TRY_SQLFUN_FIN_RB(sqlite3_bind_double(ppStmt, k+1, x), insert, bind);
	}
	TRY_SQLFUN_FIN_RB(step_reset(ppStmt, ppDb), insert, step/reset);
      }
      break;
    case mxLOGICAL_CLASS:
      LOOP_ROW_COL(mxLogical, int, ix);
      break;
    case mxINT8_CLASS:
      BIND_BLOBS(int8_t);
      break;
    case mxUINT8_CLASS:
      BIND_BLOBS(uint8_t);
      break;
    case mxINT16_CLASS:
      LOOP_ROW_COL(int16_t, int, ix);
      break;
    case mxUINT16_CLASS:
      LOOP_ROW_COL(uint16_t, int, ix);
      break;
    case mxINT32_CLASS:
      LOOP_ROW_COL(int32_t, int, ix);
      break;
    case mxUINT32_CLASS:
      LOOP_ROW_COL(uint32_t, int, ix);
      break;
    case mxINT64_CLASS:
      LOOP_ROW_COL(int64_t, int64, i64x);
      break;
    case mxUINT64_CLASS:
      LOOP_ROW_COL(uint64_t, int64, i64x);
      break;

    case mxCHAR_CLASS:
      mexCallMATLAB(1, &mxStr, 1, &(((mxArray **) prhs)[ilast]), "transpose");
      strbuf = mxArrayToString(mxStr);
      nchar = strlen(strbuf)/nrow;
      for( krow=0; krow<nrow; krow++ ) {
	TRY_SQLFUN_FIN_RB(sqlite3_bind_text(ppStmt, 1, strbuf+krow*nchar, nchar, SQLITE_STATIC),
			  insert, bind);
	TRY_SQLFUN_FIN_RB(step_reset(ppStmt, ppDb), insert, step/reset);
      }
      mxFree(strbuf);
      mxDestroyArray(mxStr);
      break;

    case mxCELL_CLASS:
    case mxSTRUCT_CLASS:
      if( nrow==1 ) { /* Handle a row vector of cells each cell having same length: */
	/* Assert that each cell has the same length: */
	nrow2 = 0;
	for( k=0; k<ncol; k++ ) {
	  mxx =  ((mxArray **) mxGetData(prhs[ilast]))[k];
	  if( nrow2==0 ) {
	    nrow2 = GetLength2(mxx);
	  } else
	    if( GetLength2(mxx)!=nrow2 ) {
	      sqlite3_finalize(ppStmt);
	      sqlite3_exec(ppDb, "ROLLBACK;", NULL, NULL, NULL);
	      CLOSE_DB;
	      mexErrMsgIdAndTxt("Sqlite4m:sql_insert:length",
				"row of cells must contain arrays with the same length, at column %d", k);
	    }
	}
	/* mexPrintf("nrow2 = %d\n", nrow2); */
	for( krow=0; krow<nrow2; krow++ ) {
	  for( k=0; k<ncol; k++ ) {
	    mxx  = ((mxArray **) mxGetData(prhs[ilast]))[k];
	    if( mxIsEmpty(mxx) ) {
	      TRY_SQLFUN_FIN_RB(sqlite3_bind_null(ppStmt, k+1), insert, bind);
	    } else
	      switch( mxGetClassID(mxx) ) {
	      case mxSINGLE_CLASS:
		BIND_ONE_I(float, double, x);
		break;
		
	      case mxDOUBLE_CLASS:
		x = mxGetPr(mxx)[krow];
		/* if( mxIsNaN(x) )
		   iret = sqlite3_bind_null(ppStmt, k+1);	      
		   else */
		TRY_SQLFUN_FIN_RB(sqlite3_bind_double(ppStmt, k+1, x), insert, bind);
		break;
		
	      case mxLOGICAL_CLASS:
		BIND_ONE_I(mxLogical, int, ix);
		break;
	      case mxINT8_CLASS:
		BIND_COL_BLOB(int8_t, bx, mxGetM(mxx));
		break;
	      case mxUINT8_CLASS:
		BIND_COL_BLOB(uint8_t, ux, mxGetM(mxx));
		break;
	      case mxINT16_CLASS:
		BIND_ONE_I(int16_t, int, ix);
		break;
	      case mxUINT16_CLASS:
		BIND_ONE_I(uint16_t, int, ix);
		break;
	      case mxINT32_CLASS:
		BIND_ONE_I(int32_t, int, ix);
		break;
	      case mxUINT32_CLASS:
		BIND_ONE_I(uint32_t, int, ix);
		break;
	      
	      case mxINT64_CLASS:
		BIND_ONE_I(int64_t, int64, i64x);
		break;
	      case mxUINT64_CLASS:
		BIND_ONE_I(uint64_t, int64, i64x);
		break;

	      case mxCHAR_CLASS:
		BIND_TEXT_I(k+1);
		break;
		
	      default:
		sqlite3_finalize(ppStmt);
		sqlite3_exec(ppDb, "ROLLBACK;", NULL, NULL, NULL);
      		CLOSE_DB;
		mexErrMsgIdAndTxt("Sqlite4m:sql_insert:category",
				  "cannot insert this category nested into table");
	      }
	  }
	  TRY_SQLFUN_FIN_RB(step_reset(ppStmt, ppDb), insert, step/reset);
	}
      } else if( ncol==1 ) { /* Handle a column vector of cells: */
	/* Assert that arrays in each cell have equal length (nr of db columns): */
	ncol2 = 0;
	for( krow=0; krow<nrow; krow++ ) {
	  mxx =  ((mxArray **) mxGetData(prhs[ilast]))[krow];
	  if( ncol2==0 )
	    ncol2 = GetLength2(mxx);
	}
	for( krow=0; krow<nrow; krow++ ) {
	  mxx  = ((mxArray **) mxGetData(prhs[ilast]))[krow];
	  if( mxIsEmpty(mxx) ) {
	    TRY_SQLFUN_FIN_RB(sqlite3_bind_null(ppStmt, krow+1), insert, bind_null);
	  } else
	    switch( mxGetClassID(mxx) ) {
	    case mxSINGLE_CLASS:
	      LOOP_COL(float, double, x);
	      break;
	    case mxDOUBLE_CLASS:
	      for( k=0; k<ncol2; k++ ) {
		x = mxGetPr(mxx)[k];
		TRY_SQLFUN_FIN_RB(sqlite3_bind_double(ppStmt, k+1, x), insert, bind_double);
	      }
	      break;
	    case mxLOGICAL_CLASS:
	      LOOP_COL(mxLogical, int, ix);
	      break;
	    case mxINT8_CLASS:
	      LOOP_COL_BLOB(int8_t, bx, mxGetM(mxx));
	      break;
	    case mxUINT8_CLASS:
	      LOOP_COL_BLOB(uint8_t, ux, mxGetM(mxx));
	      break;
	    case mxINT16_CLASS:
	      LOOP_COL(int16_t, int, ix);
	      break;
	    case mxUINT16_CLASS:
	      LOOP_COL(uint16_t, int, ix);
	      break;
	    case mxINT32_CLASS:
	      LOOP_COL(int32_t, int, ix);
	      break;
	    case mxUINT32_CLASS:
	      LOOP_COL(uint32_t, int, ix);
	      break;
	      
	    case mxINT64_CLASS:
	      LOOP_COL(int64_t, int64, i64x);
	      break;
	    case mxUINT64_CLASS:
	      LOOP_COL(uint64_t, int64, i64x);
	      break;
	    
	    case mxCHAR_CLASS:
	      BIND_TEXT_I(1);
	      break;
	      
	    default:
	      sqlite3_finalize(ppStmt);
	      sqlite3_exec(ppDb, "ROLLBACK;", NULL, NULL, NULL);
	      CLOSE_DB;
	      mexErrMsgIdAndTxt("Sqlite4m:sql_insert:category",
				"cannot insert this category nested into table");
	    }
	}
	TRY_SQLFUN_FIN_RB(step_reset(ppStmt, ppDb), insert, step/reset);
      } else /* Cell matrix, store cell by cell, only scalars, strings or byte arrays are allowed: */
	for( krow=0; krow<nrow; krow++ ) {
	  for( k=0; k<ncol; k++ ) {
	    mxx  = ((mxArray **) mxGetData(prhs[ilast]))[k*nrow+krow];
	  
	    /* Cannot insert arrays inside a cell array or structure, except strings and byte arrays: */
	    if( mxGetClassID(mxx)!=mxCHAR_CLASS && mxGetClassID(mxx)!=mxINT8_CLASS && mxGetClassID(mxx)!=mxUINT8_CLASS
		&& (mxGetN(mxx)>1 || mxGetM(mxx)>1) ) {
	      sqlite3_finalize(ppStmt);
	      sqlite3_exec(ppDb, "ROLLBACK;", NULL, NULL, NULL);
	      CLOSE_DB;
	      mexErrMsgIdAndTxt("Sqlite4m:sql_insert:nested",
				"cannot insert arrays inside a cell array or structure into table");
	    }

	    /* Insert NULL for empty cells/structure fields: */
	    if( mxIsEmpty(mxx) ) {
	      TRY_SQLFUN_FIN_RB(sqlite3_bind_null(ppStmt, k+1), insert, bind_null);
	    } else 
	      switch( mxGetClassID(mxx) ) {
	      case mxSINGLE_CLASS:
		BIND_ONE_0(float, double, x);
		break;
	      case mxDOUBLE_CLASS:
		x = mxGetPr(mxx)[0];
		/* if( mxIsNaN(x) )
		   iret = sqlite3_bind_null(ppStmt, k+1);	      
		   else */
		BIND_ONE_0(double, double, x);
		break;

	      case mxLOGICAL_CLASS:
		BIND_ONE_0(mxLogical, int, ix);
		break;
	      case mxINT8_CLASS:
		BIND_ONE_BLOB(int8_t);
		break;
	      case mxUINT8_CLASS:
		BIND_ONE_BLOB(uint8_t);
		break;
	      case mxINT16_CLASS:
		BIND_ONE_0(int16_t, int, ix);
		break;
	      case mxUINT16_CLASS:
		BIND_ONE_0(uint16_t, int, ix);
		break;
	      case mxINT32_CLASS:
		BIND_ONE_0(int32_t, int, ix);
		/* mexPrintf("krow = %d, kcol= %d, ix=%d\n", krow, k, ix); */
		break;
	      case mxUINT32_CLASS:
		BIND_ONE_0(uint32_t, int, ix);
		break;
		
	      case mxINT64_CLASS:
		BIND_ONE_0(int64_t, int64, i64x);
		break;
	      case mxUINT64_CLASS:
		BIND_ONE_0(uint64_t, int64, i64x);
		break;
		
	      case mxCHAR_CLASS:
		BIND_TEXT_I(k+1);
		break;
	      
	      default:
		sqlite3_finalize(ppStmt);
		sqlite3_exec(ppDb, "ROLLBACK;", NULL, NULL, NULL);
		CLOSE_DB;
		mexErrMsgIdAndTxt("Sqlite4m:sql_insert:category",
				  "cannot insert this category nested into table");
	      }
	  }
	  TRY_SQLFUN_FIN_RB(step_reset(ppStmt, ppDb), insert, step/reset);
	}
      break;
      
    default:
      sqlite3_finalize(ppStmt);
      sqlite3_exec(ppDb, "ROLLBACK;", NULL, NULL, NULL);
      CLOSE_DB;
      mexErrMsgIdAndTxt("Sqlite4m:sql_insert:category",
			"cannot insert this category into table");
    }
  TRY_SQLFUN(sqlite3_finalize(ppStmt), insert, finalize);
  ppStmt = NULL;
  /* mexPrintf("insert finalized\n"); */

  TRY_SQLFUN(sqlite3_exec(ppDb, "COMMIT;", NULL, NULL, NULL),
	       insert, commit);
    /* mexPrintf("insert committed\n"); */

  /* if the first argument was a string, then close the database connection  */
  CLOSE_DB;

  /* mexPrintf("returning from sql_insert\n"); */
}
