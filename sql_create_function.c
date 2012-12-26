#include <math.h>
#include <string.h>
#include <sqlite3.h>

#include "mex.h"
#include "matrix.h"

static int nMFunc = 0;
static char *MFuncNm[100];

static void cleanup(void)
{
  mxArray *mxRet;
  sqlite3 *dbp;
  int iret, k;

/* get pointer to the database: */
  mexCallMATLAB(1, &mxRet, 0, NULL, "sql_open");
  dbp = *((sqlite3 **) mxGetData(mxRet));
  for( k=0; k<nMFunc; k++ ) {
    mxFree( MFuncNm[k] );    /* Free memory for matlab function names */
    /* Unregister SQL functions: */
    if( dbp )
      iret = sqlite3_create_function(dbp, NULL, 0, SQLITE_UTF8, NULL, NULL, NULL, NULL);
  }
  mxDestroyArray(mxRet);
}

static void callMatlabFunc(sqlite3_context *context, int argc, sqlite3_value **argv)
{
  int iret, karg, nblob, ndat;
  mxArray *lhs, *prhs[argc];

  for( karg=0; karg<argc; karg++ ) {
    switch( sqlite3_value_type(argv[karg]) ) {
    case SQLITE_NULL:
      prhs[karg] = mxCreateDoubleScalar(NAN);
      break;
    case SQLITE_TEXT:
      prhs[karg] = mxCreateString(sqlite3_value_text(argv[karg]));
      break;
    case SQLITE_BLOB:
      nblob = sqlite3_value_bytes(argv[karg]);
      prhs[karg] = mxCreateNumericMatrix(1, nblob, mxUINT8_CLASS, mxREAL);
      break;

    default:
      prhs[karg] = mxCreateDoubleScalar(sqlite3_value_double(argv[karg]));
    }
  }
  iret = mexCallMATLAB(1, &lhs, argc, prhs, (char *) sqlite3_user_data(context));
  if( iret!=0 )
    mexErrMsgIdAndTxt("Sqlite4m:sql_create_function::failCallback",
		      "Callback to matlab failed");

  if( mxIsEmpty(lhs) )
    sqlite3_result_null(context);
  else
    switch( mxGetClassID(lhs) ) {
    case mxCHAR_CLASS:
      ndat = mxGetM(lhs)*mxGetN(lhs);
      sqlite3_result_text(context, (char *) mxGetData(lhs), mxGetM(lhs)*mxGetN(lhs), NULL);
      break;

    default:
      sqlite3_result_double(context, *mxGetPr(lhs));
      break;
    }

  for( karg=0; karg<argc; karg++ )
    mxDestroyArray(prhs[karg]);
  mxDestroyArray(lhs);
}

static void callMatlabStep(sqlite3_context *context, int argc, sqlite3_value **argv)
{
  int iret, karg, nblob;
  char *funcnm;
  mxArray *lhs=NULL, *prhs[argc];

  for( karg=0; karg<argc; karg++ ) {
    switch( sqlite3_value_type(argv[karg]) ) {
    case SQLITE_NULL:
      prhs[karg] = mxCreateDoubleScalar(NAN);
      break;
    case SQLITE_TEXT:
      prhs[karg] = mxCreateString(sqlite3_value_text(argv[karg]));
      break;
    case SQLITE_BLOB:
      nblob = sqlite3_value_bytes(argv[karg]);
      prhs[karg] = mxCreateNumericMatrix(1, nblob, mxUINT8_CLASS, mxREAL);
      break;

    default:
      prhs[karg] = mxCreateDoubleScalar(sqlite3_value_double(argv[karg]));
    }
  }

  funcnm = mxMalloc(strlen((char *) sqlite3_user_data(context) + strlen("Step") + 1));
  strcpy(funcnm, (char *) sqlite3_user_data(context));
  strcat(funcnm, "Step");
  iret = mexCallMATLAB(1, &lhs, argc, prhs, funcnm);
  if( iret!=0 )
    mexErrMsgIdAndTxt("Sqlite4m:sql_create_function::failCallback",
		      "Callback to matlab failed");

  for( karg=0; karg<argc; karg++ )
    mxDestroyArray(prhs[karg]);
  mxDestroyArray(lhs);

  mxFree(funcnm);
}

static void callMatlabFinalize(sqlite3_context *context)
{
  int iret, ndat;
  char *funcnm;
  mxArray *lhs=NULL, *prhs=NULL;

  funcnm = mxMalloc(strlen((char *) sqlite3_user_data(context) + strlen("Finalize") + 1));
  strcpy(funcnm, (char *) sqlite3_user_data(context));
  strcat(funcnm, "Finalize");
  iret = mexCallMATLAB(1, &lhs, 0, &prhs, funcnm);
  if( iret!=0 )
    mexErrMsgIdAndTxt("Sqlite4m:sql_create_function::failCallback",
		      "Callback to matlab failed");

  if( mxIsEmpty(lhs) )
    sqlite3_result_null(context);
  else
    switch( mxGetClassID(lhs) ) {
    case mxCHAR_CLASS:
      ndat = mxGetM(lhs)*mxGetN(lhs);
      sqlite3_result_text(context, (char *) mxGetData(lhs), mxGetM(lhs)*mxGetN(lhs), NULL);
      break;

    default:
      sqlite3_result_double(context, *mxGetPr(lhs));
      break;
    }

  mxDestroyArray(lhs);

  mxFree(funcnm);
}

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  int iret, nArg;
  char *zFunctionName;
  char *strbuf;
  sqlite3 *ppDb;
  int aggregate=0;

/* The first argument must be a pointer to a database connection: */
  if( !mxIsDouble(prhs[0]) ) 
    mexErrMsgIdAndTxt("Sqlite4m:sql_create_function::notDouble",
		      "Double (data base pointer) expected in first argument");

  if( !mxIsChar(prhs[1]) )
    mexErrMsgIdAndTxt("Sqlite4m:sql_create_function::notChar",
		      "String (SQL function name) expected in 2nd argument");
  
  if( !mxIsChar(prhs[2]) ) 
    mexErrMsgIdAndTxt("Sqlite4m:sql_create_function::notChar",
		      "String (Matlab function name) expected in 3rd argument");

  if( !mxIsNumeric(prhs[3]) )
    mexErrMsgIdAndTxt("Sqlite4m:sql_create_function::notNumeric",
		      "Numeric (nr of function arguments) 4th argument expected");

  ppDb = *((sqlite3 **) mxGetData(prhs[0]));
  zFunctionName = mxArrayToString(prhs[1]);
  
  strbuf = mxArrayToString(prhs[2]);

  MFuncNm[nMFunc] = mxMalloc(strlen(strbuf)+1);
  mexMakeMemoryPersistent(MFuncNm[nMFunc]);

  if( nMFunc==0 )
    mexAtExit(cleanup);
  
  strcpy(MFuncNm[nMFunc], strbuf);

  nArg = *mxGetPr(prhs[3]);

  if( nrhs<=4 )
    iret = sqlite3_create_function(ppDb, zFunctionName, nArg, SQLITE_UTF8,
				   MFuncNm[nMFunc], callMatlabFunc, NULL, NULL);
  else
    iret = sqlite3_create_function(ppDb, zFunctionName, nArg, SQLITE_UTF8,
				   MFuncNm[nMFunc], NULL, callMatlabStep, callMatlabFinalize);

  if( iret!=0 )
    mexErrMsgIdAndTxt("Sqlite4m:sql_create_function::failed",
		      "sqlite3_create_function failed");

  mxFree(strbuf);
  mxFree(zFunctionName);
}
