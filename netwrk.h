
#ifndef NETWRK_H
#define NETWRK_H

#include <arpa/inet.h>
//#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
//#include <netinet/in.h>
//#include <poll.h>
#include <stdio.h>
#include <string.h>
//#include <sys/select.h>
//#include <sys/socket.h>
#include <unistd.h>

#include "defs.h"

/** socket_tcp
    create a passive (listening) TCP socket */
int socket_tcp (int port, int queue) ;
/** get_actual_port
    return the port the server is using.
    useful when allowing the OS to select the port */
int get_actual_port (int fd) ;

/* send a complete msg to client (could block) */
int write_cli (int fd, char *s, int len) ;
/* read a mesg from client (could block) */
int read_cli (int fd, char *buf) ;

#endif /* NETWRK_H */
