// #define _POSIX_C_SOURCE
#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// #define __USE_POSIX

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#define FIFO1_NAME "myfifo1"
#define FIFO2_NAME "myfifo2"

volatile sig_atomic_t child_counter = 0;

// Signal handler function
void sigchld_handler(int signum)
{
    int status;
    pid_t pid;

    // Reap all children that have exited
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        // Print process ID of the terminated child
        char buffer[256];
        int len = snprintf(buffer, sizeof(buffer), "Child process %d exited.\n", pid);
        write(STDOUT_FILENO, buffer, len);
        child_counter++;
    }
}

int main(int argc, char *argv[])
{
    argv = malloc(sizeof(char *) * 2);
    argc = 2;
    argv[1] = "5";
    if (mkfifo(FIFO1_NAME, 0777) == -1)
    {
        if (errno != EEXIST)
        {
            printf("Could not create fifo file\n");
            return 1;
        }
    }

    //    //signal handler using sigaction
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;

    // set up signal handler
    sigaction(SIGCHLD, &sa, NULL);

    if (mkfifo(FIFO2_NAME, 0777) == -1)
    {
        if (errno != EEXIST)
        {
            printf("Could not create fifo file\n");
            return 1;
        }
    }

    int numberCount;

    // pid_t pid = create_child();

    pid_t pid = fork();
    printf("Created fork\n");

    if (pid > 0)
    {
        child_counter++;

        // parent

        // create another child
        pid = fork();

        if (pid > 0) // parent
        {
            printf("Starting parent\n");

            child_counter++;

            // get integer
            int status;

            if (argc < 2)
            {
                printf("Please provide an integer argument\n");
                return 1;
            }

            numberCount = atoi(argv[1]);

            struct sigaction sa;
            sa.sa_handler = sigchld_handler;
            sa.sa_flags = SA_RESTART;
            // set up signal handler
            sigaction(SIGCHLD, &sa, NULL);

            int array[] = {1, 2, 3, 4, 5};

            // write to fifo1 and fifo2
            int fd1 = open(FIFO1_NAME, O_WRONLY);
            int fd2 = open(FIFO2_NAME, O_WRONLY);

            write(fd1, array, sizeof(array));
            write(fd2, array, sizeof(array));

            // send "multiplication to fifo2"
            char *command = "multiply";
            write(fd2, command, strlen(command) + 1);

            while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
            {
                // Print process ID of the terminated child
                char buffer[256];
                int len = snprintf(buffer, sizeof(buffer), "Child process %d exited.\n", pid);
                write(STDOUT_FILENO, buffer, len);
                child_counter++;
            }
            
            close(fd1);
            close(fd2);

            return 0;
        }

        if (pid == 0)
        {
            printf("Started child1\n");
        sleep(10);

            // this one reads from fifo1, read numbers and perform summation
            // writes the result to second fifo

            int fd1 = open(FIFO1_NAME, O_RDONLY);
            int fd2 = open(FIFO2_NAME, O_WRONLY);

            int array[numberCount];
            read(fd1, array, sizeof(array));

            int sum = 0;
            for (int i = 0; i < numberCount; i++)
            {
                sum += array[i];
            }

            write(fd2, &sum, sizeof(sum));


            printf("Child1 exitting..\n");

            exit(-1);
        }

        if (pid < 0)
        {
            // error forking
        }
    }

    if (pid == 0)
    {
        printf("Started2 child\n");
        // opens second fifo, reads the command and performs multiplication
        // prints the sum of the results of all child processes
        sleep(10);
        int fd2 = open(FIFO2_NAME, O_RDONLY);

        int array[numberCount];
        read(fd2, array, sizeof(array));

        //print array
        printf("child 2 read array\n");
        for (int i = 0; i < numberCount; i++)
        {
            printf("%d ", array[i]);
        }

        char command[256];
        read(fd2, command, sizeof(command));

        if (strcmp(command, "multiply") == 0)
        {
            int product = 1;
            for (int i = 0; i < numberCount; i++)
            {
                product *= array[i];
            }

            printf("Product: %d\n", product);
        }


        printf("Child2 exitting..\n");
    }

    if (pid < 0)
    {
        // error forking
    }

    unlink(FIFO1_NAME);
    unlink(FIFO2_NAME);

    return 0;
}
