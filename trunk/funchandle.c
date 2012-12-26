#include <math.h>

#include "mex.h"
#include "matrix.h"

static char *MatlabFunctionNames[100];

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  int k;

  if( !mxIsClass(prhs[0] , "function_handle") ) 
    mexErrMsgIdAndTxt("Sqlite4m:sql_create_function::notFunctionHandle",
		      "First input argument is not a function handle.");

  mexPrintf("function handle is a %s\n", mxGetClassName(prhs[0]));
  mexPrintf("function handle has %d fields\n", mxGetNumberOfFields(prhs[0]));

  mexPrintf("size of function handle is %d %d\n", mxGetM(prhs[0]), mxGetN(prhs[0]));
  mexPrintf("pointer to function handle data is %x\n", mxGetData(prhs[0]));

  mexCallMATLAB(nlhs, plhs, nrhs, prhs, "feval");

  for( k=0; k<256; k++ )
    mexPrintf("%c", ((char *) mxGetData(prhs[0]))[k]);
  mexPrintf("\n");

  mexPrintf("double value of function handle is");
  for( k=0; k<8; k++ )
    mexPrintf(" %f", ((double *) mxGetData(prhs[0]))[k]);
  mexPrintf("\n");

}
