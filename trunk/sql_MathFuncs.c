#include <math.h>
#include <sqlite3.h>

static void sinFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  assert( argc==1 );
  switch( sqlite3_value_type(argv[0]) ) {
    /* sin(X) returns NULL if X is NULL. */
  case SQLITE_NULL:
    sqlite3_result_null(context);
    break;
 
  default:
    double rVal = sqlite3_value_double(argv[0]);
      /* Because sqlite3_value_double() returns 0.0 if the argument is not
      ** something that can be converted into a number, we have:
      ** sin(X) returns 0.0 if X is a string or blob that
      ** cannot be converted to a numeric value. 
      */
    sqlite3_result_double(context, sin(rVal));
  }
}

static void cosFunc(sqlite3_context *context, int argc, sqlite3_value **argv){
  assert( argc==1 );
  switch( sqlite3_value_type(argv[0]) ) {
    /* cos(x) returns NULL if x is NULL. */
  case SQLITE_NULL:
    sqlite3_result_null(context);
    break;
 
  default:
    double rVal = sqlite3_value_double(argv[0]);
      /* Because sqlite3_value_double() returns 0.0 if the argument is not
      ** something that can be converted into a number, we have:
      ** cos(x) returns 1.0 if x is a string or blob that
      ** cannot be converted to a numeric value. 
      */
    sqlite3_result_double(context, cos(rVal));
    break;
  }
}

/*
** This function registers all of the above C functions as SQL
** functions.  This should be the only routine in this file with
** external linkage.
*/
void sqlite3_register_math_functions(sqlite3 *db){
  static const struct {
     char *zName;
     signed char nArg;
     u8 argType;           /* 0: none.  1: db  2: (-1) */
     u8 eTextRep;          /* 1: UTF-16.  0: UTF-8 */
     u8 needCollSeq;
     void (*xFunc)(sqlite3_context*,int,sqlite3_value **);
  } aFuncs[] = {
    { "sin",                1, 0, SQLITE_UTF8,    0, sinFunc    },
    { "cos",                1, 0, SQLITE_UTF8,    0, cosFunc    },
  };
  int k;

  for( k=0; k<sizeof(aFuncs)/sizeof(aFuncs[0]); k++ ) {
    void *pArg = 0;
    switch( aFuncs[k].argType ){
      case 1: pArg = db; break;
      case 2: pArg = (void *)(-1); break;
    }
    sqlite3_create_function(db, aFuncs[k].zName, aFuncs[k].nArg,
			    aFuncs[k].eTextRep, pArg, aFuncs[k].xFunc, 0, 0);
    }
  }
}

