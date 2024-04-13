#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#define FIFO_NAME "myfifo1"

volatile sig_atomic_t child_counter = 0;

// Signal handler function
void sigchld_handler(int signum)
{
    pid_t pid;
    int status;

    // Reap terminated child processes
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (WIFEXITED(status))
        {
            // Child process terminated normally
            printf("Child process %d terminated with exit status: %d\n", pid, WEXITSTATUS(status));
        }
        else
        {
            // Child process terminated abnormally
            printf("Child process %d terminated abnormally\n", pid);
        }
        child_counter--;
    }
}

pid_t create_child();

int main(int argc, char *argv[])
{

    if (mkfifo(FIFO_NAME, 0777) == -1)
    {
        if (errno != EEXIST)
        {
            printf("Could not create fifo file\n");
            return 1;
        }
    }

    if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
    {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    // pid_t pid = create_child();

    pid_t pid = fork();
    printf("Created fork\n");

    child_counter++;

    if (pid > 0)
    {
        printf("Starting parent\n");
        // parent
        char *arr;

        arr = calloc(26, sizeof(int));

        int status;

        printf("Trying to open fifo\n");
        int fd = open(FIFO_NAME, O_RDONLY);


        printf("Starting wait for child\n");
        waitpid(pid, &status, 0);
        printf("Child wait is over\n");

        printf("Starting read\n");

        if (read(fd, arr, sizeof(char) * 25) == -1)
        {
            return 2;
        }

        printf("Read is over\n");

        arr[25] = '\0';

            write(STDOUT_FILENO, arr, sizeof(char)*25);
        free(arr);
        close(fd);
        return 0;
    }

    if (pid == 0)
    {
        printf("Started child\n");
        char *arr;

        arr = calloc(26, sizeof(char));

        for (int i = 0; i < 26; ++i)
        {
            arr[i] = i + 'A';
        }

        int fd = open(FIFO_NAME, O_WRONLY);

        printf("Writing child\n");

        if (write(fd, arr, sizeof(char) * 26) == -1)
        {
            exit(2);
        }
        
        printf("Have written: ");
        write(STDOUT_FILENO, arr, 26);

        printf("\nWriting is over, child\n");

        free(arr);
        close(fd);

        printf("Child exitting..\n");
        exit(0);
    }

    if (pid < 0)
    {
        // error forking
    }
}

pid_t create_child()
{
    pid_t pid = fork();
    if (pid > 0)
    {
        // Parent process
        child_counter++;
    }
    return pid;
}