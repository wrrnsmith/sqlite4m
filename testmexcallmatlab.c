#include "mex.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{ 
  mexCallMATLAB(0, NULL, 1, &prhs[0], "disp"); 
}
