/* Auto-configuration for 16-bit Windows */
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <io.h>	/* for mktemp */

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

#define HAVE_STRICMP 1
#define HAVE_WINDOWS_H 1
#undef HAVE_WINIOCTL_H 
