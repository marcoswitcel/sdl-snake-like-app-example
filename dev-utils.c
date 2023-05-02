#include "./dev-utils.h"



#ifdef NO_TRACE
  #define trace(message) ((void)0);
#else
  #define trace(message) printf("[%s:%d] %s\n", __FILE__, __LINE__, message);
#endif
