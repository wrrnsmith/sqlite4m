#include <string.h>

#include "mex.h"
#include "sqlite3.h"
#include "sqlite4m.h"

char *getzsql(int nrhs, const mxArray *prhs[], int *iargbind) {
  mxArray *mxColList;
  const mxArray *mxArgs[3];
  int ldestroy;
  
  char *zsql, *strbuf;

/* Argument is the table column names or numbers, one of either
   1) if empty select all columns, "*";
   2) a string of comma separated column names like "col1, col2, col3";
   3) a cell array each cell containing one column name;
   4) a numeric array with column numbers.*/
  if( mxIsEmpty(prhs[1]) )
    mxColList = mxCreateString("*");
  else if( mxIsChar(prhs[1]) || mxIsCell(prhs[1]) ) {
    mxColList = (mxArray *) prhs[1];
    ldestroy = 0;
  } else if( mxIsNumeric(prhs[1]) && nrhs>=3 && !mxIsEmpty(prhs[2]) ) {
    mxArgs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
    *((sqlite3 **) mxGetData(mxArgs[0])) = ppDb;
    mxArgs[1] = prhs[2];
    mxArgs[2] = prhs[1];
    mexCallMATLAB(1, &mxColList, 3, (mxArray **) mxArgs, "sql_columnnames");
  } else {
    CLOSE_DB;
    ERRABORT("colnames", "Column names are expected in the 2nd argument");
  }
 
  /* Now set up the SQL select statement: */
  zsql = mxMalloc(strlen("SELECT ")+2);
  strcpy(zsql, "SELECT ");
  /* Add result-columns: */
  if( mxIsChar(mxColList) ) {
    strbuf = mxArrayToString(mxColList);
    zsql = mxRealloc(zsql, strlen(zsql) + strlen(strbuf) + 2);
    strcat(zsql, strbuf);
    mxFree(strbuf);
  } else if( mxIsCell(mxColList) ) {
    int k;
    for( k=0; k<mxGetM(mxColList)*mxGetN(mxColList); k++ ) {
      strbuf = mxArrayToString(mxGetCell(mxColList, k));
      zsql = mxRealloc(zsql, strlen(zsql) + strlen(strbuf) + 2 + 2);
      strcat(zsql, strbuf);
      strcat(zsql, ", ");
      mxFree(strbuf);
    }
    zsql[strlen(zsql)-2] = '\0';
  }    
  if( ldestroy )
    mxDestroyArray(mxColList);
  /* mexPrintf("zsql = %s\n", zsql); */

  /* Add FROM source: */
  if( nrhs>=3 && !mxIsEmpty(prhs[2]) && mxIsChar(prhs[2]) ) {
    strbuf = mxArrayToString(prhs[2]);
    zsql = mxRealloc(zsql, strlen(zsql) + strlen(" FROM ") + strlen(strbuf) + 2);
    strcat(zsql, " FROM ");
    strcat(zsql, strbuf);
    mxFree(strbuf);
  }
  /* mexPrintf("zsql = %s\n", zsql); */
  /* Add WHERE expression: */
  if( nrhs>=4 && !mxIsEmpty(prhs[3]) && mxIsChar(prhs[3]) ) {
    strbuf = mxArrayToString(prhs[3]);
    zsql = mxRealloc(zsql, strlen(zsql) + strlen(" WHERE ") + strlen(strbuf) + 2);
    strcat(zsql, " WHERE ");
    strcat(zsql, strbuf);
    mxFree(strbuf);
  }
  /* mexPrintf("zsql = %s\n", zsql); */
  /* Add more instructions, can be "GROUP BY ...", "ORDER BY ...", "LIMIT ..." */ 
  if( nrhs>=5 && !mxIsEmpty(prhs[4]) && mxIsChar(prhs[4]) ) {
    strbuf = mxArrayToString(prhs[4]);
    zsql = mxRealloc(zsql, strlen(zsql) + strlen(strbuf) + 2);
    strcat(zsql, " ");
    strcat(zsql, strbuf);
    mxFree(strbuf);
    *iargbind = 5;
  }
  /* mexPrintf("zsql = %s\n", zsql); */

  return strcat(zsql, ";");
}

const mxArray *bind_m2sql(sqlite3_stmt *ppStmt, int iargbind, const mxArray *prhs[],
		int *pn_par_tobind, int *pn_val_tobind) {
  int kk, nval;
  const mxArray *mxBindPar=NULL;

  /* If there are parameters to bind, check for consistency and find out how many values: */
  if( sqlite3_bind_parameter_count(ppStmt)>0 ) {
    mxBindPar = prhs[iargbind];
    if( !mxIsCell(mxBindPar) ) {
      sqlite3_finalize(ppStmt);
      CLOSE_DB;
      ERRABORT("bind", "The parameters to bind must be in a cell array");
    }
    /* Nr of parameters to bind is the length of the cell array:*/    
    *pn_par_tobind = mxGetN(mxBindPar)*mxGetM(mxBindPar);
    if( sqlite3_bind_parameter_count(ppStmt)!=*pn_par_tobind ) {
      sqlite3_finalize(ppStmt);
      CLOSE_DB;
      ERRABORT("bind", "The nr of parameters to bind is not equal to nr of cells!");
    }
    /* Find out how many values to bind;
       if any cell has length>1, then all other cells must have the same length or length one: */
    *pn_val_tobind = 0;
    for( kk=0; kk<*pn_par_tobind; kk++ ) {
      mxArray *mxx = mxGetCell(mxBindPar, kk);
      if( *pn_val_tobind>1 && mxGetN(mxx)*mxGetM(mxx)!=*pn_val_tobind && !mxIsEmpty(mxx)
	  && mxGetClassID(mxx)!=mxCHAR_CLASS && mxGetClassID(mxx)!=mxINT8_CLASS && mxGetClassID(mxx)!=mxUINT8_CLASS ) {
	sqlite3_finalize(ppStmt);
	CLOSE_DB;
	ERRABORT("bind", "The nr of values to bind must be the same for each parameter or 1");
      }
      if( mxIsEmpty(mxx) || mxGetClassID(mxx)==mxCHAR_CLASS
	  || mxGetClassID(mxx)==mxINT8_CLASS || mxGetClassID(mxx)==mxUINT8_CLASS  )
	nval = 1;
      else
	nval = mxGetN(mxx)*mxGetM(mxx);
      if( nval>*pn_val_tobind )
	*pn_val_tobind = nval;
    }
  }
  return mxBindPar;
}
