#include <stdio.h>
#include <string.h>
#include <fcntl.h>  // Include file control options for open() function
#include <unistd.h> // Include Unix standard functions for close() and read() functions
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <time.h>


#define DEVICE_NAME "/dev/myQueue" // Define the device file path
#define SIZE 100                   // Define the buffer size for reading data
#define SHM_SIZE 10

int main()
{
    // #########################socket###########################

    int sockfd[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) < 0) {
        perror("socketpair failed");
        exit(EXIT_FAILURE);
    }

    // #########################################################
    int fd;
    ssize_t bytes_written;

    char data_to_send[SIZE];
    char received_data[SIZE];
    char ch;
    int status;

    int shmid;
    int child_state_shmid;
    int zombie_time_shmid;

    key_t key = 555;
    key_t child_state_key = 444;
    key_t zombie_time_key = 333;

    char *shm;
    int *child_state;
    clock_t *zombie_time;

    pid_t pid1;
    pid_t pid2;
    pid_t pid3;

    shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
    child_state_shmid = shmget(child_state_key, 3 * sizeof(int), 0666 | IPC_CREAT);
    zombie_time_shmid = shmget(zombie_time_key, 3 * sizeof(clock_t), 0666 | IPC_CREAT);

    if (shmid < 0)
    {
        perror("faild to create shared memory1\n");
        exit(1);
    }
    if (child_state_shmid < 0)
    {
        perror("faild to create shared memory2\n");
        exit(1);
    }

    shm = (char *)shmat(shmid, NULL, 0);
    zombie_time = (clock_t *)shmat(zombie_time_shmid, NULL, 0);
    child_state = (int *)shmat(child_state_shmid, NULL, 0);

    child_state[0] = 0;
    child_state[1] = 0;
    child_state[2] = 0;

    shmdt((int *)child_state);

    pid1 = fork();

    if (pid1 < 0)
    { // failed
        fprintf(stderr, "Fork Failed");
    }
    else if (pid1 == 0)
    { // child process 1

        close(sockfd[0]);

        //printf("start child 1 : %d\n", getpid());
        printf("Input Data to Send: ");
        fgets(data_to_send, SIZE, stdin);

        send(sockfd[1], data_to_send, strlen(data_to_send) + 1, 0);
        close(sockfd[1]);

        child_state = (int *)shmat(child_state_shmid, NULL, 0);
        child_state[1] = 1;
        shmdt((int *)child_state);

        //printf("end child 1\n");

        zombie_time = (clock_t *)shmat(zombie_time_shmid, NULL, 0);
        zombie_time[0] = clock();
        shmdt((clock_t *)zombie_time);

        exit(0);
    }
    else
    { // parent process
        pid2 = fork();

        if (pid2 < 0)
        { // failed
            printf("Fork Failed");
        }
        else if (pid2 == 0)
        { // child process 2
            close(sockfd[1]);

            child_state = (int *)shmat(child_state_shmid, NULL, 0);
            while (true)
            {
                if (child_state[1])
                {
                    //printf("start child 2 : %d\n", getpid());

                    ssize_t bytes_received = recv(sockfd[0], received_data, SIZE, 0);

                    if (bytes_received > 0)
                    {
                        printf("Received data from socket: #%s#\n", received_data);

                        fd = open(DEVICE_NAME, O_WRONLY);
                        if (fd < 0)
                        {
                            perror("Failed to open the device file");
                            exit(EXIT_FAILURE);
                        }

                        printf("spare charecters: %f\n", (double)bytes_received - 10);

                        for (int i = 0; i < bytes_received; i++){
                            ch = received_data[i];

                            bytes_written = write(fd, &ch, sizeof(ch));

                            if (bytes_written < 0)
                            {
                                perror("Write failed");
                                close(sockfd[0]);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }

                    child_state[2] = 1;
                    //printf("end child 2\n");
                    break;
                }
            }
            shmdt((int *)child_state);

            zombie_time = (clock_t *)shmat(zombie_time_shmid, NULL, 0);
            zombie_time[1] = clock();
            shmdt((clock_t *)zombie_time);

            exit(0);
        }
        else
        { // parent process
            pid3 = fork();

            if (pid3 < 0)
            { // failed
                printf("Fork Failed");
            }
            else if (pid3 == 0)
            { // child process 3
            
                child_state = (int *)shmat(child_state_shmid, NULL, 0);
                shm = (char *)shmat(shmid, NULL, 0);
                while (true)
                {
                    if (child_state[2])
                    {
                        //printf("start child 3 : %d\n", getpid());

                        fd = open(DEVICE_NAME, O_RDONLY);

                        if (fd < 0)
                        {
                            perror("Failed to open the device file");
                            exit(EXIT_FAILURE);
                        }

                        if (read(fd, received_data, SIZE) < 0)
                        {
                            perror("Read failed");
                            close(fd);
                            exit(EXIT_FAILURE);
                        }

                        printf("data read from module: %s\n", received_data);

                        strncpy(shm, received_data, SIZE);

                        child_state[0] = 1;

                        //printf("end child 3\n");

                        break;
                    }
                }

                shmdt((int *)child_state);
                shmdt((char *)shm);

                zombie_time = (clock_t *)shmat(zombie_time_shmid, NULL, 0);
                zombie_time[2] = clock();
                shmdt((clock_t *)zombie_time);

                exit(0);
            }
            else{ // parent process
                child_state = (int *)shmat(child_state_shmid, NULL, 0);
                shm = (char *)shmat(shmid, NULL, 0);

                if (child_state[0])
                {
                    printf("Parent: shared memory content = %s\n", shm);
                }

                shmdt((int *)child_state);
                shmdt((char *)shm);


                waitpid(pid1, &status, 0);
                clock_t begin1 = clock();

                waitpid(pid2, &status, 0);
                clock_t begin2 = clock();

                waitpid(pid3, &status, 0);
                clock_t begin3 = clock();


                //printf("prant finished %d\n", getpid());

                zombie_time = (clock_t *)shmat(zombie_time_shmid, NULL, 0);

                zombie_time[0] = (double)(zombie_time[0] - begin1) / CLOCKS_PER_SEC;
                zombie_time[1] = (double)(zombie_time[1] - begin2) / CLOCKS_PER_SEC;
                zombie_time[2] = (double)(zombie_time[2] - begin3) / CLOCKS_PER_SEC;

                printf("\nchild 1 zombie time : %lds\n", zombie_time[0]);
                printf("child 2 zombie time : %lds\n", zombie_time[1]);
                printf("child 3 zombie time : %lds\n", zombie_time[2]);

                shmdt((clock_t *)zombie_time);
            }

        }
        
    }

    close(fd);
    close(sockfd[0]); 
    close(sockfd[1]);  

    return 0;
}