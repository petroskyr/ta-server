
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#define BUF_MAX 256

int main (int argc, char **argv) {
  char buf[BUF_MAX];
  int bytes = 0;
  buf[BUF_MAX-1] = '\0';
  
  dprintf (0,"Welcome to a C example\n");

  if (1 >= argc) {
    dprintf(1, "(no args provided)\n");
  } else {
    for (bytes = 1; bytes < argc; bytes++) {
      dprintf(1,"argv[%d]: '%s'\n",bytes, argv[bytes]);
    }
  }

  dprintf (0, "Enter your name: ");

  bytes = read (0, buf, BUF_MAX);
  buf[bytes-1] = '\0';

  dprintf (0,"Hello '%s'\n", buf);


  
  return 0;
}
