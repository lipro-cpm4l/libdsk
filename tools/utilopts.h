
#undef strcmpi

#if HAVE_STRCMPI
	/* */
#else
 #if HAVE_STRICMP
  #define strcmpi stricmp
 #else
  #if HAVE_STRCASECMP
   #define strcmpi strcasecmp
  #endif
 #endif
#endif

char *check_type(char *arg, int argc, char **argv);
int check_forcehead(char *arg, int argc, char **argv);
int find_arg(char *arg, int argc, char **argv);
int version(void);
