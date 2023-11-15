#include "sha256.h"

#if defined(__clang__)
#define IS_DIR 10 //10 seems to be the id of a folder element that is a directory on (my) android. DT_DIR macro does not work
#else
#define IS_DIR DT_DIR
#endif


