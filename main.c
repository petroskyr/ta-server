/* -*- Mode: C -*- */
/**
 * R. Petrosky
 * May 2016
 *
 * A server program to allow students the ability to 
 * interact with a sample program. The sample program
 * should be in this same directory.
 *
 * For python:
 *   Either make the python program an interpreter file
 *   or specify the version of python with the parameter -m
 *    $ ./main -r program.py
 *    $ ./main -r program.py -m python3
 *
 * For java:
 *   The necessary .class and .java files should be in the
 *   directory.
 *    $ ./main -r program.java
 *    $ ./main -r program.java
 *
 * For C:
 *   The executable should be in this directory, named after
 *   the .c file itself. (i.e. compile program.c into program)
 *    $ ./main -r program.c
 *
 * Parameters may be passed in any order. 
 *   Required parameters
 *    -r ProgName      instructs the server to run ProgName when a client connects
 *
 *   Optional parameters (!! = not yet implemented!)
 *    [-p PORT]  (int)  instructs the server to use PORT
 *                        (the OS will choose any avaiable port by default)
 *    [-t TIME]  (int) instructs the server to shutdown after TIME hours
 *                        (the default is the duration of a lab, 2 hours)
 *                        (if no measurement character, the defualt interpretation is)
 *                        (hours, valid characters are: (no char), s, m, h, d)
 *                        ( -t5 OR -t 5 OR -t5h  OR -t 5h  : shut down in 5 hours)
 *                        ( -t90m OR -t 90m                : shut down in 90 minutes)
 *                        ( -t7d  OR -t 7d                 : shut down in 7 days)
 *                        ( -t1200s OR -t 1200s            : shut down in 1200 seconds)
 *    [-q QUEUE] (int)  instructs the socket to use QUEUE, in the syscall 'listen'
 *                        (the default size is 10, which should be fine)
 *    [-a NUM_ARGS] (int)instructs the server to request up to NUM_ARGS from client, and passing them
 *                        in the call to exec. The arg gathering ends when NUM_ARGS have been collected,
 *                        or if the client enters an empty string for an arg.
 *    [-m MODE]  (str)  intstructs the server to use MODE as the interpreter
 *                        (this flag is useful for specifying python vs python3, for example)
 *    !![-v]            instructs the server to record its session
 *                        (if the server is run with -bg, this is recoreded
 *                         in file 'ta_server_log_PID')
 *    [-bg]             put the server in the background
 *
 * There is no special protocol used by this server.
 * Thus, have the students run netcat to utilize the
 * server. For example, once the server has  started,
 * a student can issue this command:
 *
 *   $ nc ADDRESS PORT
 *   ex: $ nc 140.160.137.100 43122
 */

#include <stdio.h>
#include <sys/stat.h>
#include <strings.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <regex.h>
#include <limits.h>

#include <ifaddrs.h>

#include "defs.h"
#include "netwrk.h"
#include "set_up.h"

volatile sig_atomic_t g_time_is_up = 0;    /* Flag that ends server loop */
volatile sig_atomic_t g_connections = 0;   /* always <= MAX_CONNECTIONS */

/** handle_sigchld
    avoids zombies by calling wait once the SIGCHLD
    signal is sent. The parent process should register
    this handler for every successful fork */
void handle_sigchld (int signo) {
  g_connections--;
  int status = 0;
  wait (&status);
}

/** handle_sigalrm
    sets the global flag to 1 */
void handle_sigalrm (int signo) {
  g_time_is_up = 1;
  signal(signo, handle_sigalrm); 
}

/** handle_sigpipe
    in case of broken connection in the short period of time
    before the child can exec the program */
void handle_sigpipe (int signo) {
  _exit(2);
}

/** get_args_for_exec
    build argv that will be passed to the exec function */
char **get_args_for_exec (char *name, int num_args, int interpreter) {
  /* make room for prog name, null pointer, (and maybe "-m") */
  num_args+= interpreter ? 3 : 2;
  char **argv = (char**)malloc(num_args*sizeof(char*));
  int i = 0;
  int user_arg = 1;
  if (interpreter) {
    argv[i] = "-m";
    i++;
  }
  argv[i] = name;
  i++;
  int bytes;
  char buf_to[MAX_STRLEN_ARG];
  char buf_from[MAX_STRLEN_ARG];
  for ( ; i < num_args-1; i++, user_arg++) {
    /* malloc pointer, get str */
    snprintf(buf_to, MAX_STRLEN_ARG, "arg %d: ", user_arg);
    write_cli(STDOUT_FILENO, buf_to, 1+strlen(buf_to));
    if (1 == (bytes = read_cli(STDIN_FILENO, buf_from))) {
      break; /* no more args */
    }
    argv[i] = (char *)malloc(1+strlen(buf_from));
    strncpy(argv[i], buf_from, 1+strlen(buf_from));
  }
  argv[i] = NULL;
  return argv;
}

/** prep_for_exec
    redirect stdin/out/err to client (fd)
    gather command line args
    exec program */
void prep_for_exec (int fd, char *abs_path, char *name, int num_args,
		    int interpreter, void(*exec_fn)(void *, void *, void *)) {
  dup2(fd, STDIN_FILENO);
  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);
  close(fd);
  char **argv = get_args_for_exec(name, num_args, interpreter);
  exec_fn(abs_path, name, argv);
  _exit (12);
}

/** term_client
    need to ensure non-blocking, notify client, terminate connection */
void term_client (int fd) {
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
  write(fd, "Server Busy\n", 12);
  shutdown(fd, SHUT_RDWR);
  close(fd);
}

/** new_connection
    The parent forks and registers the SIGCHLD handler,
    then goes back to the server loop
    The child cleans its resources,
    redirects stdin, stdout, stderr, and exec's the program */
void new_connection (int entryfd, int timeout, char *abs_path, char *name,
		     int num_args, int interpreter, void(*exec_fn)(void *, void *, void *)) {
  struct sockaddr addr;
  socklen_t addrlen = sizeof (addr);
  
  int clientfd = accept (entryfd, &addr, &addrlen);
  if (((EWOULDBLOCK | EAGAIN) & errno) || clientfd < 0) {
    /* no client, or problem accepting */
    errno = 0; /* reset errno, important so the main server loop doesn't get stuck */
    return;
  }

  if (MAX_CONNECTIONS <= g_connections) {
    /* ensure non-blocking, notify client, kill connection */
    term_client(clientfd);
    return;
  }

  pid_t chid = fork ();
  if (0 > chid) {
    dprintf(STDERR_FILENO, "Unable to fork\n");
    term_client(clientfd);
    return;
  }
  if (chid) {
    /* parent */
    g_connections++;
    signal(SIGCHLD, handle_sigchld);
    close (clientfd);
  } else {
    close(entryfd);
    signal(SIGPIPE, handle_sigpipe);
    prep_for_exec(clientfd, abs_path, name, num_args, interpreter, exec_fn);
  }
}

/* void _srvr_loop_poll (int timeout, char *run) { */
/* } */

/** _srvr_lop_select
    The main server loop, implemented with syscall select.
    This is a simple loop that calls new_connection() when
    a client connects. */
void _srvr_loop_select (int entryfd, int timeout, char *abs_path, char *name,
			int num_args, int interpreter, void(*exec_fn)(void *, void *, void *)) {
  fd_set r, w, e;
  struct timeval *to = NULL;
  
  FD_ZERO (&r);
  FD_ZERO (&w);
  FD_ZERO (&e);
  FD_SET (entryfd, &r);
  FD_SET (entryfd, &e);

  while (select (entryfd+1, &r, &w, &e, to)) {
    if (g_time_is_up || (EINTR != errno && FD_ISSET(entryfd, &e))) {
      break;
    }
    if (FD_ISSET(entryfd, &r)) {
      new_connection(entryfd, timeout, abs_path, name, num_args, interpreter, exec_fn);
    }
    FD_SET(entryfd, &r);
    FD_SET(entryfd, &e);
  }
}

/** doesnt work yet !! */
/* this was going to be an alternate method of getting the IP addr */
/* void print_addr (int fd) { */
/*   struct sockaddr_in addr; */
/*   socklen_t addrlen = sizeof(addr); */
/*   getsockname(fd, (struct sockaddr*)&addr, &addrlen); */
/*   char buf[100]; */
/*   bzero(buf, sizeof(buf)); */
/*   inet_ntop (AF_INET,&(addr.sin_addr), buf, addrlen); */
/*   dprintf(STDOUT_FILENO, "IP addr: %s\n", buf); */
/*   //  dprintf (STDOUT_FILENO, "", addr. */
/* } */

/** srvr_loop
    Planning on implementing a poll and select version */
void srvr_loop (int entryfd, int timeout, char * abs_path, char *name,
		int num_args, int interpreter,  void(*exec_fn)(void *, void *, void *)) {
  _srvr_loop_select (entryfd, timeout, abs_path, name, num_args, interpreter, exec_fn);
  /* _srvr_loop_poll (entryfd, timeout, run); */
}

/** log_put_msg
    write something to the log file */
void log_put_msg (char *s) {
  struct tm *tm;
  time_t t;
  char date[100];

  t = time(NULL);
  tm = localtime(&t);
  strftime(date, sizeof(date), "%a %d %b %Y %H:%M:%S", tm);
  dprintf(STDOUT_FILENO, "%s - %s\n", date, s);
}

/** background_process
    turns the program into a background process
    if bg != 0, child silently continues in bg
    this must happen before the pid is printed,
    otherwise server pid wont be correct */
void background_process (int bg, int *pstop) {
  if (bg) {
    pid_t chid = fork();
    if (0 > chid) {
      dprintf(STDERR_FILENO, "Unable to make server a background process\n");
      return;
    }
    if (chid) {
      /* parent */
      char buf[1];
      /* wait until child writes to stdout (terminal) */
      read(pstop[0], buf, 1); 
      _exit(0);
    }
  }
}

/** background_process_logfile
    if this is a background process,
    reroute stdout/stderr to file */
void background_process_logfile (int bg, char *name) {
  int fd;
  char fname[128];
  if (bg) {
    bzero(fname, sizeof(fname));
    sprintf(fname, "ta_server_log_%d", (int)getpid());
    fd = open((NULL==name) ? fname : name,
	      O_CLOEXEC | O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);
    log_put_msg("server initialized");
  }
}

/** close_stdin
    this "closes" stdin but prevents the 
    calls to accept from assigning fd 0.
    This is good becuase the child needs
    to redirect stdin to the clientfd, which
    we now know can't be 0 */
void close_stdin () {
  int fd = open("/dev/null", O_RDONLY);
  dup2(fd, STDIN_FILENO);
  close(fd);
}

/** synch_parent
    wakes parent from call to read */
void synch_parent (int bg, int *pstop) {
  if (bg) {
    write(pstop[1], "c", 1);
  }
  close(pstop[0]);
  close(pstop[1]);
}

/** end_log
    this is registered to occur at exit time */
void end_log () {
  log_put_msg("server terminated");
}

/** main
 *   flow of execution:
 *    Set default values to essential program variables
 *    Build associator array that groups together:
 *      a setter function, a command arg, and a program variable
 *    Check command args, setting variables appropriately
 *    Create server entry point
 *    Display server information
 *    Decide if background/foreground process
 *    Set alarm, start server
 */
int main (int argc, char **argv) {
  char abs_path[PATH_MAX];
  bzero (abs_path, sizeof (abs_path));
  getcwd (abs_path, sizeof (abs_path));
  
  /* default values for program variables */
  int p = DEFAULT_P;        /* port (optional) */
  int q = DEFAULT_Q;        /* listen queue (optional) */
  int t = DEFAULT_T;        /* client timeout (optional) */
  int bg = DEFAULT_BG;      /* make a background process? (optional) */
  int a = DEFAULT_A;        /* number of command args to gather for exec */
  char *name = NULL;        /* program to run (REQUIRED) */
  char *mode = NULL;        /* mode (REQUIRED) */
  
  void (*exec_fn)(void *, void *, void *) = NULL; /* how to exec the program */

  int m = 0; /* mode flag: 1 = include -m flag */
  
  /* associate a command line parameter to a program variable */
  typedef struct assoc {
    char *descr;                                     /* command line flag */
    void *var;                                       /* program variable */
    void (*fn)(char **argv, int *i, void *var);      /* handler function */
  } assoc_t;

  assoc_t args[] = {
    {"-p", (void *)&p, set_port},
    {"-q", (void *)&q, set_queue},
    {"-t", (void *)&t, set_timeout},
    {"-r", (void *)&name, set_run},
    {"-m", (void *)&mode, set_mode},
    {"-bg", (void *)&bg, set_bg},
    {"-a", (void *)&a, set_exec_args}
  };

  /* set any variables defined in command line */
  int i, j;
  for (i = 1; i < argc; i++) {
    /* for each command arg */
    for (j = 0; j < sizeof(args) / sizeof(assoc_t); j++) {
      /* check the args[] for match */
      if (strncmp (argv[i], args[j].descr, strlen(args[j].descr)) == 0) {
	/* perform the setter function */
  	args[j].fn (argv, &i, args[j].var);
      }
    }
  }

  /* chech for required params */
  if (NULL == name) {
    dprintf(STDERR_FILENO,"Usage: ./main -r PROG [-m MODE] [-p PORT] [-t TIMEOUT] [-a NUMARGS] [-q QUEUE]\n");
    dprintf(STDERR_FILENO,"$ ./main -r prog1.py               # execs prog1.py (as interpreter file)\n");
    dprintf(STDERR_FILENO,"$ ./main -r prog2.py -m python2    # execs python2 prog2.py\n");
    dprintf(STDERR_FILENO,"$ ./main -r prog3.c                # execs ./prog3\n");
    dprintf(STDERR_FILENO,"$ ./main -r prog4.java             # execs java prog4\n");
    dprintf(STDERR_FILENO,"$ ./main -r prog4.java -bg         # srvr runs in background, creates file ta_server_log_PID\n");
    dprintf(STDERR_FILENO,"$ ./main -r prog5.c -a 5 -t15m     # gathers up to 5 args, passing them to exec ./prog5 and terminates the server after 15mins\n");
    return 0;
  }

  /* set exec function */
  if (NULL == (exec_fn = set_exec_fn (name, mode, &m))) {
    dprintf(STDERR_FILENO, "terminating because no exec function\n");
    return 21;
  }

  /* create full path to executable */
  sprintf (&(abs_path[strlen(abs_path)]), "/%s", name);

  /* sync parent if bg process */
  int pstop[2];
  pipe(pstop);
  background_process (bg, pstop);
  
  /* create server entry point */
  int entryfd = socket_tcp (p, q);
  dprintf(STDOUT_FILENO," Server (PID: %d)\n", (int)getpid());
 
  /* print out network addresses, look for eth01, or en0 or something like that */
  struct ifaddrs *addrs;
  struct ifaddrs *tmp;
  getifaddrs(&addrs);
  tmp = addrs;
  while (tmp) {
    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET &&
	tmp->ifa_name[0] == 'e'){
      struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
      //dprintf (STDOUT_FILENO,"%s: %s\n", tmp->ifa_name, inet_ntoa(pAddr->sin_addr)); /* uncomment to see all network connections */
      dprintf (STDOUT_FILENO," $ nc %s ", inet_ntoa(pAddr->sin_addr));
    }
    tmp = tmp->ifa_next;
  }
  freeifaddrs(addrs);

  dprintf(STDOUT_FILENO, "%d # connect to server with netcat\n", get_actual_port(entryfd));
  //print_addr (entryfd);
  synch_parent(bg, pstop); /* if running in background, this lets the info print to terminal before parent terminates */
  background_process_logfile(bg, NULL);
  close_stdin();
  atexit(end_log);
  
  /* set timer */
  signal(SIGALRM, handle_sigalrm);
  alarm(t);

  /* be a server */
  srvr_loop (entryfd, t, abs_path, name, a, m, exec_fn);

  /* clean up */
  shutdown(entryfd, SHUT_RDWR);
  close(entryfd);
  /* pthread_mutex_destroy(&g_child_wait); */
  return 0;
}  
