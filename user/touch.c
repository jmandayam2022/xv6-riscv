#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  int fd;
  if (argc <= 1)
  {
    fprintf(2, "usage: touch file\n");
    exit(0);
  }

  fd = open(argv[1], O_RDWR|O_CREATE);

  if (fd == -1) {
    fprintf(2, "Unable to create %s\n", argv[1]);
    exit(1);
  }
  
  close(fd);
  exit(0);
}
