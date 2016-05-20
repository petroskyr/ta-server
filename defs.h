
#ifndef DEFS_H
#define DEFS_H

/* boundaries for the port */
/* OS will choose server port if 0 is used */
#define DEFAULT_P         0
#define MIN_P             0
#define MAX_P     USHRT_MAX


/* the queue param in syscall listen */
#define DEFAULT_Q        10
#define MIN_Q             0
#define MAX_Q            25


/* boundaries for server timeout in seconds */
#define DEFAULT_T      7200  /* 2 hours = 2h x 60m x 60s */
#define MIN_T             0
#define MAX_T       1209600  /* 2 weeks = 14d x 24h x 60m x 60s */


/* number of args the exec'd program will need */
#define DEFAULT_A         0
#define MIN_A             0
#define MAX_A            20
#define MAX_STRLEN_ARG   64 /* length of each arg should not exceed this */



#define MAX_CONNECTIONS  35  /* maximum number of clients */
#define DEFAULT_BG        0  /* make a background process? */

#endif /* DEFS_H */
