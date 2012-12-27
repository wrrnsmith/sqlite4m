#include <math.h>
#include <string.h>

#include "mex.h"
#include "matrix.h"
#include "sqlite3.h"

/*
 * sql_open.c
 * Open a new SQLITE3 database connection
 *
 * This is a MEX-file for MATLAB.
*/

sqlite3 *ppDb = NULL;

char *uigetfile( const mxArray *mxFilterSpec ) {
  mxArray *swap, *mxFullFilename[1], *mxFilename[2];
  const mxArray *mxArgs[2];

  mxArgs[0] = mxFilterSpec; 
  mxArgs[1] = mxCreateString("Select a database file");
  mexCallMATLAB(2, mxFilename, 2, (mxArray **) mxArgs, "uigetfile");
  if( *mxGetPr(mxFilename[0])==0 )
    mexErrMsgIdAndTxt("Sqlite4m:sql_open:uigetfile", "user has canceled");
  swap = mxFilename[0];
  mxFilename[0] = mxFilename[1];
  mxFilename[1] = swap;
  mexCallMATLAB(1, mxFullFilename, 2, mxFilename, "fullfile");
  return mxArrayToString(mxFullFilename[0]);
}

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  char *filename;
  char cflag[2];
  mxArray *mxFilterspec[1], *mxRet[1];
  int iret, flags;
  mwSize dims[] = {2, 2};

/* If no input argument, return the pointer to connected database. */
  if( nrhs<1 ) {
    plhs[0] = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
    *((sqlite3 **) mxGetData(plhs[0])) = ppDb;

    return;
  }

/* If the first input argument is numeric and 0, close and unlock: */
  if( mxIsNumeric(prhs[0]) && *mxGetPr(prhs[0])==0 ) {
    iret = sqlite3_close(ppDb);
    if( iret!=SQLITE_OK )
      mexErrMsgIdAndTxt("Sqlite4m:sql_open:close", sqlite3_errmsg(ppDb));
    
    ppDb = NULL;
    plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
    *mxGetPr(plhs[0]) = iret;
      
    mexUnlock();
    /* mexPrintf("closed and unlocked\n"); */
    return;
  }

  if( !mxIsChar(prhs[0]) )
    mexErrMsgIdAndTxt("sqlite4m:sql_open:input", 
		      "Expecting a string with the file name!");

  if( ppDb!=NULL )
    mexErrMsgIdAndTxt("sqlite4m:sql_open:alreadyConnected",
		      "Already connected to a database, use ATTACH in SQL.");

/* If an empty string or a directory is passed,
   start the matlab gui to get the file name: */
  if( strlen(mxArrayToString(prhs[0]))==0 ) {
    mxFilterspec[0] = mxCreateCellArray(2, dims);
    mxSetCell(mxFilterspec[0], 0, mxCreateString("*.sqlite;*.db;*.db?"));
    mxSetCell(mxFilterspec[0], 1, mxCreateString("*.*"));
    mxSetCell(mxFilterspec[0], 2,
	      mxCreateString("Database files (*.sqlite,*.db,*.db?)"));
    mxSetCell(mxFilterspec[0], 3, mxCreateString("All Files (*.*)"));

    filename = uigetfile(mxFilterspec[0]);
  } else {
    /* mexPrintf("prhs[0] = %s\n", mxArrayToString(prhs[0])); */
    mexCallMATLAB(1, mxRet, 1, (mxArray **) prhs, "isdir");
    filename = ( *mxGetPr(mxRet[0])!=0 )?
      uigetfile(prhs[0]) : mxArrayToString(prhs[0]);
  }

  /* Open database, default read only, 
     we enable shared cache (hopefully ok?): */
  flags = SQLITE_OPEN_READONLY|SQLITE_OPEN_SHAREDCACHE;
  if( nrhs>1 && mxIsChar(prhs[1]) ) {
    mxGetString(prhs[1], cflag, 2);
    switch (cflag[0]) {
    case 'w':
      flags = SQLITE_OPEN_READWRITE|SQLITE_OPEN_SHAREDCACHE;
      break;
    case 'c':
      flags = SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_SHAREDCACHE;
      break;
    }
  }

  plhs[0] = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
  *mxGetPr(plhs[0]) = -1.0;

  iret = sqlite3_open_v2(filename, &ppDb, flags, NULL);
  /* mexPrintf("sqlite3_open_v2 returned %d\n", iret); */
  if( iret!=SQLITE_OK )
    /* mexPrintf("%s\n", sqlite3_errmsg(ppDb)); */
    mexErrMsgIdAndTxt("Sqlite4m:sql_open", "%s", sqlite3_errmsg(ppDb));
  else {

    *((sqlite3 **) mxGetData(plhs[0])) = ppDb;

    /* Prevent clearance of the mex file and unloading of the sqlite3 library: */
    mexLock();
  }
}
