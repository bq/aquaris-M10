#ifndef YL_VERSION_INCLUDED
#define YL_VERSION_INCLUDED

#define YL_VERSION             "Release_0"

#ifndef WIN32
  #define YL_VERSION_TOUCH(v)    do { v[7] = '_';  } while (0)
  #define __init_version         __attribute__((section (".yoli_version")))
  extern char yoli_version[];

#else /* WIN32 */
  #define YL_VERSION_TOUCH(v) 

#endif /* WIN32 */


#endif /* YL_VERSION_INCLUDED */

