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

void CloseAndExec(int *pfd_A, int *pfd_B, char **cmd);

int main(int argc, char **argv) {
	char *cmd1[SIZE];
	char *cmd2[SIZE];
	char **command;
	int pipefd_1[2];
	int pipefd_2[2];
	pid_t pid1;
	pid_t pid2;

	if (argc < 4) {
		printf("Too few arguments!\n");
		exit(EXIT_FAILURE);
	}

	command = cmd1;
	int flag = 0;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], DELIM) == 0) {
			if (flag || i == 1) {
				printf("Invalid syntax!\n");
				exit(EXIT_FAILURE);
			}
			*command = NULL;
			command = cmd2;
			flag = 1;
			continue;
		}
		*command = argv[i];
		command++;
	}

	if (!flag) {
		printf("Invalid syntax!\n");
		exit(EXIT_FAILURE);
	}
	*command = NULL;

	if (pipe(pipefd_1) == -1) {
		perror("Pipe1 error: ");
		exit(EXIT_FAILURE);
	}

	if (pipe(pipefd_2) == -1) {
		perror("Pipe2 error: ");
		exit(EXIT_FAILURE);
	}

	if ((pid1 = fork()) < 0) {
		perror("Fork1 error: ");
		exit(EXIT_FAILURE);
	}

	if (pid1 == 0)
		CloseAndExec(pipefd_1, pipefd_2, cmd1);
	
	pid2 = fork();
	if (pid2 < 0) {
		perror("Fork2 error: ");
		exit(EXIT_FAILURE);
	}
	if (pid2 == 0)
		CloseAndExec(pipefd_2, pipefd_1, cmd2);

	char *xor;
	char buffer1[SIZE_BUF];
	char buffer2[SIZE_BUF];
	int counter = 0;
	int length = SIZE_BUF;

	xor = malloc(sizeof (char) * 2 * SIZE_BUF;
	if (xor == NULL) {
		perror("Memory allocation error: ");
		exit(EXIT_FAILURE);
	}
	memset(xor, 0, sizeof (char) * SIZE_BUF);
	close(pipefd_1[1]);
	close(pipefd_2[1]);

	while (1) {
		int n1, n2, max, min;
		n1 = read(pipefd_1[0], buffer1, sizeof (char) * SIZE_BUF);
		n2 = read(pipefd_2[0], buffer2, sizeof (char) * SIZE_BUF);
		if (!n1 && !n2)
			break;

		write(0, buffer1, n1);
		write(0, buffer2, n2);

		max = n1 > n2 ? n1 : n2;
		min = max == n1 ? n2 : n1;
		counter += max;
		if (counter > length) {
			char *ptr;
			ptr = realloc(xor, 2 * counter);
			if (ptr == NULL) {
				perror("Memory reallocation error: ");
				free(xor);
				close(pipefd_1[0]);
				close(pipefd_2[0]);
				exit(EXIT_FAILURE);
			}
			xor = ptr;
			length = 2 * counter;
		}

		char *temp_ptr;
		if (n1 >= n2) {
			memcpy(xor + counter - max, buffer1,
			       sizeof (char) * n1);
			temp_ptr = buffer2;
		} else {
			memcpy(xor + counter - max, buffer2,
			       sizeof (char) * n2);
			temp_ptr = buffer1;
		}
		for (int i = 0; i < min; i++)
			xor[counter - max + i] ^= temp_ptr[i];
	}
	printf("%d\n", counter);
	close(pipefd_1[0]);
	close(pipefd_2[0]);

	if (counter) {
		int fd;
		fd = open(FILENAME, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
		if (fd == -1) {
			perror("File error: ");
			free(xor);
			exit(EXIT_FAILURE);
		}
		write(fd, xor, counter * sizeof (char));
		/*
		   printf("-------------reuslt-of-gamma----------------\n");
		   for (int i = 0;i<counter;i++)
		   printf("%X",xor[i]);
		   printf("\n"); */
		//write(0,xor,counter*sizeof(char));
		close(fd);
	}

	free(xor);

	exit(EXIT_SUCCESS);
}

void
CloseAndExec(int *pfd_A, int *pfd_B, char **cmd)
{
	char path[SIZE];
	char *ptr;
	close(pfd_B[0]);
	close(pfd_B[1]);
	close(pfd_A[0]);
	close(1);
	dup(pfd_A[1]);
	close(pfd_A[1]);

	strcpy(path, cmd[0]);
	ptr = strrchr(cmd[0], '/');
	if (ptr != NULL) {
		char temp[SIZE];
		strcpy(temp, ptr + 1);
		strcpy(cmd[0], temp);
	}
	if (execvp(path, cmd) == -1) {
		pid_t parent = getppid();
		perror(path);
		kill(parent, SIGTERM);
		exit(EXIT_FAILURE);
	}
}
