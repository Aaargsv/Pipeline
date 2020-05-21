#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#define DELIM "delim"
#define FILENAME "result.txt"
#define SIZE 256
#define SIZE_BUF 256

void CloseAndExec(int* pfd_A,int* pfd_B,char** cmd);

int main(int argc,char** argv)
{
  char* cmd1[SIZE];
  char* cmd2[SIZE];
  char **command;
  int pipefd_1[2];
  int pipefd_2[2];
  pid_t pid1;
  pid_t pid2;

  if(argc<4)
  {
    printf("Too few arguments!\n");
    exit(EXIT_FAILURE);
  }

  command=cmd1;
  int flag=0;
  for (int i=1;i<argc;i++)
  {
    if (strcmp(argv[i],DELIM)==0)
    {
      if (flag || i==1)
      {
        printf("Invalid syntax!\n");
        exit(EXIT_FAILURE);
      }
      *command=NULL;
      command=cmd2;
      flag=1;
      continue;
    }
      *command=argv[i];
      command++;
  }

  if (!flag)
  {
    printf("Invalid syntax!\n");
    exit(EXIT_FAILURE);
  }
  *command=NULL;

  if (pipe(pipefd_1)==-1)
  {
    perror("Pipe1 error: ");
    exit(EXIT_FAILURE);
  }

  if (pipe(pipefd_2)==-1)
  {
    perror("Pipe2 error: ");
    exit(EXIT_FAILURE);
  }

  if((pid1=fork())<0)
  {
    perror("Fork1 error: ");
    exit(EXIT_FAILURE);
  }

  if (pid1==0)
    CloseAndExec(pipefd_1,pipefd_2,cmd1);

    if((pid2=fork())<0)
    {
      perror("Fork2 error: ");
      exit(EXIT_FAILURE);
    }
    if (pid2==0)
      CloseAndExec(pipefd_2,pipefd_1,cmd2);



  exit(EXIT_SUCCESS);
}


void CloseAndExec(int* pfd_A,int* pfd_B,char** cmd)
{

  char path[SIZE];
  char* ptr;
  close(pfd_B[0]);
  close(pfd_B[1]);
  close(pfd_A[0]);
  close(1);
  dup(pfd_A[1]);
  close(pfd_A[1]);

  strcpy(path,cmd[0]);
  if((ptr=strrchr(cmd[0],'/'))!=NULL)
  {
    char temp[SIZE];
    strcpy(temp,ptr+1);
    strcpy(cmd[0],temp);
  }
  if(execvp(path,cmd)==-1)
  {
    pid_t parent=getppid();
    perror(path);
    kill(parent, SIGTERM);
    exit(EXIT_FAILURE);
  }
}
