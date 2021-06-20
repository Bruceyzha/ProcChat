#ifndef SERVER_H
#define SERVER_H

#define MSG_LENGTH 2048
#define GEVENT ("gevent")

typedef struct{
    char type[2];
    char identifier[256];
    char domain[1790];
}connect;

typedef struct{
    char type[2];
    char msg[2046];
}say;

typedef struct{
    char type[2];
    char identifier[256];
    char message[1790];
}recive;

void kill_process(int sigval);
int StringFind(const char *pSrc, const char *pDst);
int disconnect(char* path_name_WR,char* path_name_RD);
int ping_pong(char* path_name_WR,char* path_name_RD);
int main_CH(char* path_name_WR,char* path_name_RD,char* domain, char* identifier);
int client_handler(char* domain,char* identifier);
int global_process();
#endif
