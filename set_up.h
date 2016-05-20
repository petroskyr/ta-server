
#ifndef SET_UP_H
#define SET_UP_H

#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "defs.h"

/** _set_var_to_int
    logic to correctly set the integer variable "var" */
void _set_var_to_int (void *var, int val, int min, int max, int dflt) ;

/** set_var_to_int
    void *var is a pointer to an int.
    char *val_str is the string that var should be set to.
    if argv[(*i)] is "-X" use  argv[(*i)+1] for val_str
    else argv[(*i)] is "-XValue, use &(argv[(*i)][2]) */
void set_var_to_int (char **argv, int *i, void *var, int min, int max, int dflt) ;
void set_port (char **argv, int *i, void *var) ;
void set_queue (char **argv, int *i, void *var) ;
void set_exec_args (char **argv, int *i, void *var) ;
void set_bg (char **argv, int *i, void *var) ;

/** _set_timeout
    a helper function to find the value for timeout
    matchDigits tells were the digits match in the string argv[i]
    matchChar tells where the modifying character is (s,m,d,h)
    return is either: DEFAULT_T if no match / bad match OR
    the value defined in the command line flag, if it was acceptable */
int _set_timeout (char **argv, int i,  regmatch_t matchDigits, regmatch_t matchChar) ;

/** set_timeout
    check for a regex match on the -t flag
    (could be on argv[i] or argv[i+1])
    get the timeout number
    convert to seconds if needed */
void set_timeout (char **argv, int *i, void *var) ;
void set_run (char **argv, int *i, void *var) ;
void set_mode (char **argv, int *i, void *var) ;


void execvp_java(void *, void *, void *);
/* void execlp_python (char *interpreter, char *name) ; */
/* void execl_python (char *abs_path, char *name) ; */
/* void execlp_java (char *abs_path, char *name) ; */
/* void execl_c (char *abs_path, char *name) ; */
/** set_exec_fn
    determine which exec to use based on the file name */
void *set_exec_fn (char *name, char *mode, int *m) ;


#endif /* SET_UP_H */
