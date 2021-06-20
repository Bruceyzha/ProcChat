#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

void kill_process(int sigval)
{ //signal function:Recover the zombie processes
    wait(NULL);
}

int StringFind(const char *pSrc, const char *pDst) //find the string fragment
{
    int i, j;
    for (i = 0; pSrc[i] != '\0'; i++)
    {
        if (pSrc[i] != pDst[0])
            continue;
        j = 0;
        while (pDst[j] != '\0' && pSrc[i + j] != '\0')
        {
            j++;
            if (pDst[j] != pSrc[i + j])
                break;
        }
        if (pDst[j] == '\0')
            return i;
    }
    return -1;
}

int disconnect(char *path_name_WR, char *path_name_RD)
{                         //disconnect function
    unlink(path_name_WR); //close the named pipes
    unlink(path_name_RD);
    if (kill(getppid(), SIGUSR1) == -1) //send the SIGUSR1 to the father process
    {
        printf("fail to send signal\n");
        exit(1);
    }
    exit(0); //close the process
}

int ping_pong(char *path_name_WR, char *path_name_RD)
{
    while (1)
    { //Cycle to send the ping message, until no reply.
        int ping_fd = open(path_name_RD, O_WRONLY);
        char ping[MSG_LENGTH];
        ping[0] = 5;                      //write the type
        write(ping_fd, ping, MSG_LENGTH); //send the message
        close(ping_fd);
        sleep(15); //wait 15 seconds
        int pong_fd = open(path_name_WR, O_RDONLY);
        struct pollfd poll_fd;
        poll_fd.fd = pong_fd;
        poll_fd.events = POLLIN;
        poll(&poll_fd, 1, 0); //check the reply
        if (poll_fd.revents == POLLIN)
        {
            char pong[MSG_LENGTH];
            read(pong_fd, pong, MSG_LENGTH);
            close(pong_fd);
            if (pong[0] != 6)
            { //if the reply is not pong message,close the child process
                break;
            }
        }
        else
        { //no reply is not pong message,close the child process
            close(pong_fd);
            break;
        }
    }
    exit(0);
}

//main client handler
int main_CH(char *path_name_WR, char *path_name_RD, char *domain, char *identifier)
{
    while (1)
    {
        int fd = open(path_name_WR, O_RDONLY);
        say *message;
        recive sent_message;
        struct pollfd poll_fd;
        poll_fd.fd = fd;
        poll_fd.events = POLLIN;
        poll(&poll_fd, 1, -1); //use poll the check the fd
        if (poll_fd.revents == POLLIN)
        {
            message = malloc(sizeof(say));
            read(poll_fd.fd, message, sizeof(say));
            close(fd);
            if (message->type[0] == 1 && message->type[1] == 0)
            { //handle the "say" message
                sent_message.type[0] = 3;
                sent_message.type[1] = 0;
                strcpy(sent_message.identifier, identifier);
                strncpy(sent_message.message, message->msg, 1790);
                struct dirent *ptr;
                DIR *dir = opendir(domain); //find other processes in the same domain
                while ((ptr = readdir(dir)) != NULL)
                {
                    if (StringFind(ptr->d_name, "_RD") != -1 && StringFind(ptr->d_name, identifier) == -1)
                    {
                        char file_name[MSG_LENGTH];
                        strcpy(file_name, domain);
                        strcat(file_name, "/");
                        strcat(file_name, ptr->d_name);
                        int recv_fd = open(file_name, O_WRONLY);
                        write(recv_fd, &sent_message, sizeof(recive)); //send the recive message
                        close(recv_fd);
                    }
                }
                closedir(dir);
                free(message);
            }
            else if (message->type[0] == 2 && message->type[1] == 0)
            { //handle the "saycont" message
                sent_message.type[0] = 4;
                sent_message.type[1] = 0;
                strcpy(sent_message.identifier, identifier);
                strncpy(sent_message.message, message->msg, 1790);
                sent_message.message[1789] = message->msg[2045]; //copy the terminate byte
                struct dirent *ptr;
                DIR *dir = opendir(domain);
                while ((ptr = readdir(dir)) != NULL)
                {   // check the _RD named pipe and except itself
                    if (StringFind(ptr->d_name, "_RD") != -1 && StringFind(ptr->d_name, identifier) == -1)
                    {
                        char file_name[MSG_LENGTH];
                        strcpy(file_name, domain);
                        strcat(file_name, "/");
                        strcat(file_name, ptr->d_name);
                        int recv_fd = open(file_name, O_WRONLY);
                        write(recv_fd, &sent_message, sizeof(recive)); //send the recivecont message
                        close(recv_fd);
                    }
                }
                closedir(dir);
                free(message);
            }
            else if (message->type[0] == 7 && message->type[1] == 0)
            { //disconnect with the client
                free(message);
                disconnect(path_name_WR, path_name_RD);
            }
        }
    }
    return 0;
}

int client_handler(char *domain, char *identifier)
{
    char path_name_WR[MSG_LENGTH];
    char path_name_RD[MSG_LENGTH];
    mkdir(domain, S_IRWXU | S_IRWXG); //create the domain folder
    strcpy(path_name_WR, domain);
    strcat(path_name_WR, "/");
    strcat(path_name_WR, identifier);
    strcpy(path_name_RD, path_name_WR);
    strcat(path_name_WR, "_WR");
    strcat(path_name_RD, "_RD");             //create the names of named pipes
    mkfifo(path_name_WR, S_IRWXU | S_IRWXG); //create the named pipes
    mkfifo(path_name_RD, S_IRWXU | S_IRWXG);
    pid_t cpid;
    cpid = fork(); //create the child process for ping-pong
    if (cpid < 0)
    {
        printf("error in fork!");
    }
    else if (cpid == 0)
    {
        sleep(15); //until the father process finished
        ping_pong(path_name_WR, path_name_RD);
    }
    else
    {
        main_CH(path_name_WR, path_name_RD, domain, identifier);
    }

    return 0;
}

int global_process()
{
    if ((mkfifo(GEVENT, S_IRWXU | S_IRWXG)) < 0)
    {
        fprintf(stderr, "Unable to open pipe");
    }
    while (1)
    { //create the multiple client handler
        int fd = open(GEVENT, O_RDONLY);
        connect connection;
        if (read(fd, &connection, sizeof(connect)) < 0)
        {
            continue;
        }
        close(fd);
        if (connection.type[0] == 0 && connection.type[1] == 0)
        {
            pid_t cpid;
            cpid = fork();
            signal(SIGUSR1, kill_process);
            if (cpid < 0)
                printf("error in fork!");
            else if (cpid == 0)
            {
                client_handler(connection.domain, connection.identifier);
            } 
            else
            {
            }
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    global_process();
    return 0;
}
