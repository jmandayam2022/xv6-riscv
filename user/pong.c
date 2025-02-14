#include "kernel/types.h"
#include "user/user.h"

#define READ_END (0)
#define WRITE_END (1)

void perror(const char *msg)
{
  write(2, msg, strlen(msg));
  exit(1);
}

int main(int argc, char *argv[])
{
  int parent_to_child[2];
  int child_to_parent[2];
  int pid;
  char byte = 'A';

  // Create pipes
  if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1)
  {
    perror("pipe not created\n");
  }

  // fork
  pid = fork();
  if (pid < 0)
  {
    perror("fork failed\n");
  }

  if (pid > 0)
  {
    // Parent
    close(parent_to_child[READ_END]);
    close(child_to_parent[WRITE_END]);

    int start_time = uptime();
    // Send initial byte
    if (write(parent_to_child[WRITE_END], &byte, 1) != 1)
    {
      perror("Parent: write error\n");
    }
    printf("Parent sent: %c\n", byte);

    while (byte < 'Z')
    {
      if (read(child_to_parent[READ_END], &byte, 1) != 1)
      {
        perror("Parent: read error\n");
      }
      sleep(1);
      printf("Parent received: %c\n", byte);

      if (byte != 'Z')
      {
        byte++;
        if (write(parent_to_child[WRITE_END], &byte, 1) != 1)
        {
          perror("Parent loop: write error\n");
        }

        printf("Parent sent: %c\n", byte);
        sleep(1);
      } else{
         int end_time = uptime();
         printf("total time %d\n", end_time - start_time);
      }
    }

    close(parent_to_child[WRITE_END]);
    close(child_to_parent[READ_END]);

    wait(0);
  }
  else
  {
    // Child
    close(parent_to_child[WRITE_END]);
    close(child_to_parent[READ_END]);

    while (byte < 'Z')
    {
      if (read(parent_to_child[READ_END], &byte, 1) != 1)
      {
        perror("Child: read error\n");
      }
      sleep(1);
      printf("Child received: %c\n", byte);

      byte++;
      if (write(child_to_parent[WRITE_END], &byte, 1) != 1)
      {
        perror("Child loop: write error\n");
      }
      printf("Child sent: %c\n", byte);
      sleep(1);
    }

    close(parent_to_child[READ_END]);
    close(child_to_parent[WRITE_END]);

  }
  exit(0);
}
