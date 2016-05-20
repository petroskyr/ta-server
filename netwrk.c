/* -*- Mode: C -*- */
/** 
 * R. Petrosky
 * May 2016
 *
 * This file contains the networking set up methods.
 * Socket creation, and retrieving the server's port.
 *
 */

#include "netwrk.h"


/** make_socket ("Doing stuff while waiting for alarm....")
    create a socket and bind to address */
int make_socket (int type, int port) {
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof (addr);
  bzero (&addr, addrlen);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  int fd = socket (AF_INET, type, 0);
  if (-1 == fd) {
    dprintf (STDERR_FILENO, "socket failed\n");
    return -1;
  }

  /* socket should be non-blocking */
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
  
  if (-1 == bind (fd, (struct sockaddr *)&addr, addrlen)) {
    dprintf(STDERR_FILENO, "bind failed\n");
    return -1;
  }
  return fd;
}

/** socket_tcp
    create a passive (listening) TCP socket */
int socket_tcp (int port, int queue) {
  int fd = make_socket (SOCK_STREAM, port);
  if (-1 == fd) return -1;

  if (-1 == listen (fd, queue)) {
    dprintf(STDERR_FILENO, "listen failed\n");
    return -1;
  }
  return fd;
}

/** get_actual_port
    return the port the server is using.
    useful when allowing the OS to select the port */
int get_actual_port (int fd) {
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof (addr);
  if (-1 == getsockname (fd, (struct sockaddr *)&addr, &addrlen)) {
    dprintf(STDERR_FILENO, "getsockname failed\n");
    return -1;
  }
  return (int) ntohs(addr.sin_port);
}

/** write_cli
    send all mesg to client */
int write_cli (int fd, char *s, int len) {
  int bytes = 0;
  while (bytes < len) {
    bytes += write(fd, s+bytes, len - bytes);
  }
  return bytes-len;
}

/** read_cli
    read a mesg from a client */
int read_cli (int fd, char *buf) {
  int bytes = 0;
  do {
    bytes += read(fd, buf+bytes, MAX_STRLEN_ARG - bytes);
  } while ('\n' != *(buf+bytes-1));
  *(buf+bytes-1) = '\0';
  return bytes;
}
