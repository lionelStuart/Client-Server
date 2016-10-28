#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <iostream>
#include <pthread.h>

using namespace std;

#define MYPORT 4000
#define BACKLOG 10
#define BUF_SIZE 10
#define MAX_CLIENT 3

struct SocketSettings{
    int socket;
    struct sockaddr_in addr;
};

struct ClientStruct{
    int id;
    int socket;
    pthread_t thread;
    struct sockaddr_in addr;
};
struct ClientStruct clientStruct[100];

pthread_mutex_t lock;

void *clientFun(void *);
void *acceptFun(void *);

int add_desc(int socket){
    int id = 0;
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENT; ++i) {
        if (clientStruct[i].socket == 0){
            clientStruct[i].socket = socket;
            id = clientStruct[i].id;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
    return id;
}

int add_tdesc(int id, pthread_t snif_thread){
    if (id == 0)
        return 1;

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENT; ++i) {
        if (clientStruct[i].socket == id){
            clientStruct[i].thread = snif_thread;
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    return 0;
}

int add_sdesc(int id, struct sockaddr_in client){
    if (id == 0)
        return 1;

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENT; ++i) {
        if (clientStruct[i].socket == id){
            clientStruct[i].addr = client;
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    return 0;
}

int shut(int id){
    int ind = id -1;
    pthread_mutex_lock(&lock);
    if(clientStruct[ind].socket != 0){
        if (shutdown(clientStruct[ind].socket, SHUT_RDWR) == 0){
            close(clientStruct[ind].socket);
        }
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

int readn(int socket, char* bf, int len){
    int load = 0;
    char tmpbuf[BUF_SIZE];
    memset(bf, 0, len);
    memset(tmpbuf, 0, len);
    while (load < len) {
        int numbyte = recv(socket, tmpbuf, len-load, 0);

        if (numbyte < 0) return -1;

        load += numbyte;
        strcat(bf, tmpbuf);
        strcpy(tmpbuf, "");
    }
    return load;
}

void* clientFun(void* newSocket){
    int socket =  * (int *)newSocket;
    int numRead;
    char buf[BUF_SIZE];
    char ans[BUF_SIZE];
    memset(ans,0,BUF_SIZE);

    while ( (numRead = readn(socket, buf, BUF_SIZE)) > 0 ){

        if (strcmp(buf, "hello") == 0){
            strcpy(ans, "Welcome!");
            if (send(socket, ans, BUF_SIZE, 0) < 0){
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }
        }
        else if (strcmp(buf, "exit") == 0){
            break;
        }
    }

    if (numRead == 0) {
        cout << "!! : Client disconnect" << endl;
        if (shutdown(socket, SHUT_RDWR) == 0) {
            cout << "OK : Shotdown client" << endl;
            if (close(socket) == 0) {
                cout << "OK : Client socket close" << endl;
            } else {
                cout << "!! : Close client socket fail" << endl;
                pthread_mutex_lock(&lock);
                for (int i = 0; i < MAX_CLIENT; ++i) {
                    if (clientStruct[i].socket == socket) {
                        clientStruct[i].socket = 0;
                        break;
                    }
                }
                pthread_mutex_unlock(&lock);
            }
        } else {
            cout << "!! : Fail down client" << endl;
            pthread_mutex_lock(&lock);
            for (int i = 0; i < MAX_CLIENT; ++i) {
                if (clientStruct[i].socket == socket) {
                    clientStruct[i].socket = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&lock);
        }
    }
    else if (numRead == -1){
        cout << "!! : Recv fail" << endl;
    }
}

void* acceptFun(void* socktmp){

    //SocketSettings mySocket = *(SocketSettings*) myStruct;

    int sock = *(int*) socktmp;
    int client_sock, size, *newSock, id;
    struct sockaddr_in client;
    size = sizeof(struct sockaddr_in);

    while( (client_sock = accept(sock, (struct sockaddr*)&client, (socklen_t*)&size))) {

        if (client_sock < 0) {
            cout << "!! : Accept fail" << endl;
            for (int i = 0; i < MAX_CLIENT; ++i) {
                if (clientStruct[i].socket != 0)
                    shut(clientStruct[i].id);
            }

            for (int j = 0; j < MAX_CLIENT; ++j) {
                if (clientStruct[j].socket != 0)
                    pthread_join(clientStruct[j].thread, NULL);
            }

            break;
        }

        cout << "OK : Accept by " << client_sock << endl;

        id = add_desc(client_sock);
        if (id == 0)
            cout << "!! : Max client limit" << endl;
        else {
            pthread_t snif_thread;
            newSock = (int *) malloc(1);
            *newSock = client_sock;

            add_sdesc(id, client);
            if (pthread_create(&snif_thread, NULL, clientFun, (void *) newSock) < 0) {
                cout << "!! : Create thread fail" << endl;
                for (int i = 0; i < MAX_CLIENT; ++i) {
                    if (clientStruct[i].socket != 0)
                        shut(clientStruct[i].id);
                }

                for (int j = 0; j < MAX_CLIENT; ++j) {
                    if (clientStruct[j].socket != 0)
                        pthread_join(clientStruct[j].thread, NULL);
                }

            }

            add_tdesc(id, snif_thread);

            cout << "OK : Create new thread success" << endl;
        }
    }
}

int main() {

    SocketSettings mySocket;

    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENT; ++i) {
        clientStruct[i].socket = 0;
        clientStruct[i].id = i+1;
    }
    pthread_mutex_unlock(&lock);

    // создание сокета
    if ( (mySocket.socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Socket error");
        exit(1);
    }

    cout << "OK : Create socket" << endl;

    mySocket.addr.sin_family = AF_INET;
    mySocket.addr.sin_addr.s_addr = htonl(INADDR_ANY);
    mySocket.addr.sin_port = htons(MYPORT);

    bzero(&(mySocket.addr.sin_zero), 8);  // обнуление остальной части структуры

    // свяывание соданного сокета с локальным адресом
    if ( bind(mySocket.socket, (struct sockaddr*)&mySocket.addr, sizeof(struct sockaddr)) == -1){
        perror("Bind error");
        exit(1);
    }

    cout << "OK : Bind" << endl;

    // Создание очереди прослушивания сети на порту MYPORT
    if ( listen(mySocket.socket, MAX_CLIENT) == -1 ){
        perror("Listen error");
        exit(1);
    }

    cout << "OK : Listen in process" << endl;

    pthread_t accept_t;
    pthread_create(&accept_t, NULL, acceptFun, &mySocket.socket);

    while(1){
        char command[10];
        memset(command, 0, 10);
        cin.getline(command, 10);

        if (strncmp(command, "stop", 4) == 0) {
            break;
        }
        else if (strncmp(command, "kill", 4) == 0) {
            cout << "oops" << endl;
        }
        else
            cout << "Error: Unknown command" << endl;
    }

    if (shutdown(mySocket.socket, SHUT_RDWR) == 0){
        cout << "OK : Down mySocket" << endl;
        if (close(mySocket.socket) == 0)
            cout << "OK : Close mySocket" << endl;
    }

    if (pthread_join(accept_t, (void**)&mySocket.addr) != 0){
        cout << "!! : Error accept join" << endl;
    }
    cout << "OK : Join accept thread" << endl;
}