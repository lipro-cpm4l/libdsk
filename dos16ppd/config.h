/* Auto-configuration for 16-bit DOS */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys.h>

#define PATH_MAX 67	/* DOS path maximum */

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define HAVE_STRICMP 1
#undef HAVE_WINDOWS_H 
#undef HAVE_WINIOCTL_H 
