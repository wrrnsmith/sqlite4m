#ifndef _SQLITE4M_H_
#define _SQLITE4M_H_

#include "mex.h"
#include "matrix.h"
#include "sqlite3.h"

char errmsgstr1[256]="Sqlite4m:sql_";

#define ERRABORT(cause, errmsgstr2)		\
  do {						\
    strcat(errmsgstr1, cause);			\
    mexErrMsgIdAndTxt(errmsgstr1, errmsgstr2);	\
  } while(0)

mxArray *mxppDb=NULL, *mxRet=NULL;
/* const mxArray *mxBindPar=NULL; */
sqlite3 *ppDb;
sqlite3_stmt *ppStmt;

char *getzsql(int, const mxArray**, int*);
const mxArray *bind_m2sql(sqlite3_stmt*, int, const mxArray**, int*, int*);

/* If the first argument was a string, then close the database connection,
   this must be added to each error handling:  
*/
#define CLOSE_DB					\
  do {if( mxIsChar(prhs[0]) ) {				\
    mexCallMATLAB(1, &mxRet, 1, &mxppDb, "sql_close");  \
    mxDestroyArray(mxppDb);				\
    mxDestroyArray(mxRet);				\
      } } while(0)
 
/* Call an sqlite function,
   if error, then clean up and throw a matlab error: */
#define TRY_SQLFUN(sqlfun, mexfun, action)	    \
  do {						    \
    if( sqlfun!=SQLITE_OK ) {			    \
      char errmsgbuf[strlen(sqlite3_errmsg(ppDb))+1];   \
      strcpy(errmsgbuf, sqlite3_errmsg(ppDb));		\
      CLOSE_DB;						\
      mexErrMsgIdAndTxt("Sqlite4m:sql_mexfun:action",	\
			errmsgbuf);			\
  } } while(0)
 
/* Call an sqlite function,
   if error, then finalize the prepared statement,
   clean up and throw a matlab error: */
#define TRY_SQLFUN_FIN(sqlfun, mexfun, action)		\
  do {								\
    if( sqlfun!=SQLITE_OK ) {					\
      char errmsgbuf[strlen(sqlite3_errmsg(ppDb))+1];		\
      strcpy(errmsgbuf, sqlite3_errmsg(ppDb));			\
      sqlite3_finalize(ppStmt);					\
      ppStmt = NULL;						\
      CLOSE_DB;							\
      mexErrMsgIdAndTxt("Sqlite4m:sql_mexfun:action",		\
			errmsgbuf);				\
  } } while(0)
 
/* Call an sqlite function,
   if error, then finalize the prepared statement,
   roll back and throw a matlab error: */
#define TRY_SQLFUN_FIN_RB(sqlfun, mexfun, action)		\
  do {								\
    if( sqlfun!=SQLITE_OK ) {					\
      char errmsgbuf[strlen(sqlite3_errmsg(ppDb))+1];		\
      strcpy(errmsgbuf, sqlite3_errmsg(ppDb));			\
      sqlite3_finalize(ppStmt);					\
      ppStmt = NULL;						\
      sqlite3_exec(ppDb, "ROLLBACK;", NULL, NULL, NULL);	\
      CLOSE_DB;							\
      mexErrMsgIdAndTxt("Sqlite4m:sql_mexfun:action",		\
			errmsgbuf);				\
	} } while(0)

/* Macro for binding Matlab matrix type "mtype" as database type "btype."
   "btype" can be double, int or int64.
   "kpar" is the index of sql parameter to set, starting from 1!
   "mxvar" is the mex variable containing the parameter value,
   "kval" is the index into the mex variable.

   The macro is used like:

   BIND_ONE(float, double, kk+1, mxx, kval)
   (btype is double because sqlite uses always double precision, 
   so sql_bind_float() is not available),

   BIND_ONE(double, double, kk+1, mxx, kval)

   BIND_ONE(mxLogical, int, kk+1, mxx, kval)
   BIND_ONE(int16_t, int, kk+1, mxx, kval)
   BIND_ONE(uint16_t, int, kk+1, mxx, kval)
   BIND_ONE(int32_t, int, kk+1, mxx, kval)

   BIND_ONE(uint32_t, int64, kk+1, mxx, kval)
   BIND_ONE(int64_t, int64, kk+1, mxx, kval)
   BIND_ONE(uint64_t, int64, kk+1, mxx, kval)

   Assumed declarations:

   sqlite3 *ppDb;
   sqlite3_stmt *ppStmt;
*/
#define BIND_ONE(mtype, btype, kpar, mxvar, kval)			\
  TRY_SQLFUN_FIN(sqlite3_bind_ ## btype(ppStmt, kpar,			\
				mxGetN(mxvar)*mxGetM(mxvar)>1?		\
				((mtype *) mxGetData(mxvar))[kval]	\
				:((mtype *) mxGetData(mxvar))[0]),      \
			MEXFUN, bind_btype);

/* Macro for binding Matlab variable mxvar of type "mtype"
   to a database blob.
   "kpos" is the index of sql parameter to set, starting from 1!
 */
#define BIND_BLOB(mtype, kpos, mxvar)					\
  TRY_SQLFUN_FIN(sqlite3_bind_blob(ppStmt, kpos,			\
				 (mtype *) mxGetData(mxvar),		\
				 mxGetM(mxvar)*mxGetN(mxvar),		\
				 SQLITE_STATIC),			\
	       MEXFUN, bind_blob);

/* Macro for binding Matlab variable mxx containing a string
   to a database text.
   "kpos" is the index of sql parameter to set, starting from 1!

   Define MEXFUN as the name of the mex function without "sql_",
   eg "select", "select_numeric", ...

   Assumed declaration:

   char *strbuf;
 */
#define BIND_TEXT(kpos, mxvar)						\
  strbuf = mxArrayToString(mxvar);				\
  TRY_SQLFUN_FIN(sqlite3_bind_text(ppStmt, kpos, strbuf,		\
					  strlen(strbuf), mxFree),	\
			MEXFUN, bind_text);

  /* mexPrintf("TEXT \"%s\" bound to %d\n", strbuf, k+1); */

#define BIND_MATPAR(MEXFUN)						\
  /* Loop for binding the matlab parameters.                      */	\
  /* If there are no parameters to bind, then n_par_tobind=0      */	\
  /* and this loop gets omitted:                                  */	\
  for( kk=0; kk<n_par_tobind; kk++ ) {					\
    mxArray *mxx = mxGetCell(mxBindPar, kk);				\
    /* Bind an empty parameter db null :                        */	\
    if( mxIsEmpty(mxx) ) {						\
      TRY_SQLFUN_FIN(sqlite3_bind_null(ppStmt, kval+1),			\
		   MEXFUN, bind_null);					\
    } else {								\
      classid = mxGetClassID(mxx);					\
      switch( classid ) {						\
      case mxSINGLE_CLASS:						\
	BIND_ONE(float, double, kk+1, mxx, kval)			\
	  break;							\
      case mxDOUBLE_CLASS:						\
	BIND_ONE(double, double, kk+1, mxx, kval)			\
	  break;							\
	  								\
      case mxLOGICAL_CLASS:						\
	BIND_ONE(mxLogical, int, kk+1, mxx, kval)			\
	  break;							\
      case mxINT8_CLASS:						\
	BIND_BLOB(int8_t, kk+1, mxx)					\
	  break;							\
      case mxUINT8_CLASS:						\
	BIND_BLOB(uint8_t, kk+1, mxx)					\
	  break;							\
      case mxINT16_CLASS:						\
	BIND_ONE(int16_t, int, kk+1, mxx, kval)				\
	  break;							\
      case mxUINT16_CLASS:						\
	BIND_ONE(uint16_t, int, kk+1, mxx, kval)			\
	  break;							\
      case mxINT32_CLASS:						\
	BIND_ONE(int32_t, int, kk+1, mxx, kval)				\
	  break;							\
      case mxUINT32_CLASS:						\
	BIND_ONE(uint32_t, int, kk+1, mxx, kval)			\
	  break;							\
									\
      case mxINT64_CLASS:						\
	BIND_ONE(int64_t, int64, kk+1, mxx, kval)			\
	  break;							\
      case mxUINT64_CLASS:						\
	BIND_ONE(uint64_t, int64, kk+1, mxx, kval)			\
	  break;							\
									\
      case mxCHAR_CLASS:						\
	BIND_TEXT(kk+1, mxx)						\
	  break;							\
									\
      default:								\
	sqlite3_finalize(ppStmt);					\
	CLOSE_DB;							\
	mexErrMsgIdAndTxt("Sqlite4m:sql_MEXFUN:category",		\
			  "cannot bind this parameter type");		\
      }									\
    }									\
  }

#endif
