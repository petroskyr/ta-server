/* -*- Mode: C -*- */
/** 
 * R. Petrosky
 * May 2016
 *
 * This file contains the methods invoked during the 
 * initial stages of the program. These are the
 * functions that establish the important program
 * variables. No networking happens here. A large
 * number of side effects within these functions.
 *
 */

#include "set_up.h"

/** _set_var_to_int
    logic to correctly set the integer variable "var" */
void _set_var_to_int (void *var, int val, int min, int max, int dflt) {
  if(val < min || val > max) {
    val = dflt;
  }
  *((int *)var) = val;
}

/** set_var_to_int
    void *var is a pointer to an int.
    char *val_str is the string that var should be set to.
    if argv[(*i)] is "-X" use  argv[(*i)+1] for val_str
    else argv[(*i)] is "-Xvalue, use &(argv[(*i)][2])
    This allows user to type args as:
    -a 5 or -a5
    to set the number of args to 5, for example.
*/
void set_var_to_int (char **argv, int *i, void *var, int min, int max, int dflt) {
  char *var_str;
  int val;
  if ('\0' != argv[(*i)][2]) {
    /* value is smooshed onto the flag */
    var_str = &(argv[(*i)][2]);
  } else {
    /* check next arg for value */
    var_str = argv[(*i)+1];
  }
  val = (int)strtol(var_str, NULL, 10);
  if ((EINVAL | ERANGE) & errno) {
    /* error converting to int, use default */
    val = dflt;
  }
  /* set the value */
  _set_var_to_int(var, val, min, max, dflt);
}
/** the -p flag */
void set_port (char **argv, int *i, void *var) {
  set_var_to_int (argv, i, var, MIN_P, MAX_P, DEFAULT_P);
}
/** the -q flag */
void set_queue (char **argv, int *i, void *var) {
  set_var_to_int (argv, i, var, MIN_Q, MAX_Q, DEFAULT_Q);
}
/** the -a flag */
void set_exec_args (char **argv, int *i, void *var) {
  set_var_to_int (argv, i, var, MIN_A, MAX_A, DEFAULT_A);
}
/** the -bg flag */
void set_bg (char **argv, int *i, void *var) {
  /* set var to 1 to make it true */
  _set_var_to_int (var, 1, 0, 1, 1);
}

/** _set_timeout
    a helper function to find the value for timeout
    matchDigits tells were the digits match in the string argv[i]
    matchChar tells where the modifying character is (s,m,d,h)
    return is either: DEFAULT_T if no match / bad match OR
    the value defined in the command line flag, if it was acceptable */
int _set_timeout (char **argv, int i,  regmatch_t matchDigits, regmatch_t matchChar) {
  int val = DEFAULT_T;
  int conv = 1;  /* conversion factor */
  char c;        /* actual character (s,m,d,h) */
  char dc = 'h'; /* default character, if none was used */
  
  val = (int)strtol(&(argv[(i)][matchDigits.rm_so]), NULL, 10); /* 10 = base */
  if (EINVAL == errno || ERANGE == errno || MIN_T > val || MAX_T < val) {
    /* overflow or problem */
    return DEFAULT_T;
  }
  
  /* convert to seconds? */
  c = (-1 != matchChar.rm_so) ? argv[(i)][matchChar.rm_so] : dc;
  switch (c) {
  case 'd' :
    conv *= 24;
  case 'h' :
    conv *= 60;
  case 'm' :
    conv *= 60;
  default : /* s, no conversion needed */
    conv *= 1;
  }
  if (conv > (conv*val) || val > (conv*val) ||
      (conv*val) > MAX_T || (conv*val) < MIN_T) {
    /* conversion results in overflow */
    val = DEFAULT_T;
  } else {
    /* convert to seconds */
    val *= conv;
  }
  return val;
}

/** set_timeout
    check for a regex match on the -t flag
    (could be on argv[i] or argv[i+1])
    get the timeout number
    convert to seconds if needed
    ex: to shut the server down in 4 hours:
    -t4h OR -t4 OR -t 4 OR -t 4h
*/
void set_timeout (char **argv, int *i, void *var) {
  int val = 0;
  int m = 4; /* number of parenthasized expressions being match +1 */
  regex_t preg;
  regmatch_t pmatch[m];
  if (0 != regcomp(&preg, "^(-t)?([0-9]+)([d,h,m,s])?$", REG_EXTENDED)) {
    dprintf (STDERR_FILENO,
	     "Error in matcher, using default value %d for timeout\n", DEFAULT_T);
    *((int*)var) = DEFAULT_T;
    return;
  }
  
  if (0 == regexec(&preg, argv[(*i)], (size_t)m, pmatch, 0)) {
    /* flags were -tTIMEc */
    val = _set_timeout (argv, *i, pmatch[2], pmatch[3]);
  } else if (argv[(*i)+1] && 0 == regexec(&preg, argv[(*i)+1], (size_t)m, pmatch, 0)) {
    /* flags were -t TIMEc */
    val = _set_timeout (argv, (*i)+1, pmatch[2], pmatch[3]);
  } else  { /* regex did not match */
    val = DEFAULT_T;
  }
  
  regfree(&preg);
  _set_var_to_int(var, val, MIN_T, MAX_T, DEFAULT_T);
}
/** the -r flag */
void set_run (char **argv, int *i, void *var) {
  *((char **)var) = argv[(*i)+1];
}
/** the -m flag */
void set_mode (char **argv, int *i, void *var) {
  *((char **)var) = argv[(*i)+1];
}


/* void test_execv_all (char *abs_path, char **argv) { */
/*   e */
/* } */

/** interpreter could be any python interpreter on PATH
    (i.e., python, python2, python3) */
void execvp_python_i (void *interpreter, void *arg2, void *argv) {
  execvp((char *)interpreter, (char **)argv);
}
/** python file should be interpreter file
    (i.e., first line looks like: #!/usr/bin/python3) */
void execvp_python (void *abs_path, void *arg2, void *argv) {
  execvp((char *)abs_path, (char **)argv);
}
void execvp_c (void *abs_path, void *arg2, void *argv) {
  execvp((char *)abs_path, (char **)argv);
}
void execvp_java (void *arg1, void *arg2, void *argv) {
  execvp("java", (char **)argv);
}

/** set_exec_fn
    determine which exec to use based on the file name
    the regex accepts any file name that ends with either:
    .py OR .java OR .c
    In the java and c case, the name is truncated to 
    remove the extension by inserting a '\0'
*/
void *set_exec_fn (char *name, char *mode, int *m) {
  regex_t preg;
  regmatch_t pmatch[1];
  void (*exec_fn)(void *, void *, void *) = NULL;
  if (0 != regcomp( &preg, "[.](py|java|c)$", REG_EXTENDED)) {
    dprintf(STDERR_FILENO,
	    "Error in matcher, cannot determine exec function");
    return exec_fn;
  }
  
  if (0 == regexec(&preg, name, (size_t)1, pmatch, 0)) {
    int ext = pmatch[0].rm_eo - pmatch[0].rm_so;
    switch (ext) { /* checks how many characters the extension is */
    case 5 : /* .java */
      name[pmatch[0].rm_so] = '\0';
      *m = 1;
      exec_fn = execvp_java;
      break;
    case 3 : /* .py */
      if (NULL == mode) {
	exec_fn = execvp_python;
	*m = 0;
      } else {
	exec_fn = execvp_python_i;
	*m = 1;
      }
      break;
    case 2 : /* .c */
      exec_fn = execvp_c;
      *m = 0;
      name[pmatch[0].rm_so] = '\0';
      break;
    default :
      dprintf(STDERR_FILENO, "Error matching regex pattern with file name '%s'\n", name);
      exec_fn = NULL;
      *m = -1;
    }
  }
  regfree(&preg);
  return exec_fn;
}
