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
    char buf[BUF_SIZE];
    char ans[BUF_SIZE];
    memset(ans,0,BUF_SIZE);

    while (1){
        if (readn(socket, buf, BUF_SIZE) < 0){
            cout << "Error: Recive from " << socket << " fail" << endl;
            continue;
        }

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
    close(socket);

    return 0;
}

void* acceptFun(void* socket){

    int mySocket =*(int*) socket;

    int newSocket[MAX_CLIENT];
    int count = 0;
    struct sockaddr_in client_addr;
    socklen_t sin_size;

    pthread_t client_t[MAX_CLIENT];

    while(1){
        sin_size = sizeof(struct sockaddr_in);
        if (count < MAX_CLIENT) {
            if ((newSocket[count] = accept(mySocket, (struct sockaddr *) &client_addr, &sin_size)) == -1) {
                perror("Accept error");
                break;
            }

            // запрос на соединение принят, теперь нужно ответить
            cout << "OK : Accept by " << newSocket[count] << " success" << endl;

            pthread_create(&client_t[count], NULL, &clientFun, &newSocket[count]);
            count++;
        }
        else{
            continue;
        }
    }

    for (int i = 0; i < count; i++){
        close(newSocket[i]);
    }
    cout << "!! : All clients sockets close" << endl;

    for (int i = 0; i < count; i++){
        pthread_join(client_t[i], NULL);
    }
    cout << "!! : All clients threads join" << endl;

    return NULL;
}

int main() {
    int mySocket;        // mySocket - здесь слушаем, newSocket - сокет для нового подключения
    struct sockaddr_in server_addr; // адресная информация сервера

    // создание сокета
    if ( (mySocket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Socket error");
        exit(1);
    }

    cout << "OK : Create socket" << endl;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(MYPORT);

    bzero(&(server_addr.sin_zero), 8);  // обнуление остальной части структуры

    // свяывание соданного сокета с локальным адресом
    if ( bind(mySocket, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1){
        perror("Bind error");
        exit(1);
    }

    cout << "OK : Bind" << endl;

    // Создание очереди прослушивания сети на порту MYPORT
    if ( listen(mySocket, MAX_CLIENT) == -1 ){
        perror("Listen error");
        exit(1);
    }

    cout << "OK : Listen in process" << endl;

    pthread_t accept_t;
    pthread_create(&accept_t, NULL, acceptFun, &mySocket);

    while(1){
        char command[10];
        memset(command, 0, 10);
        cin.getline(command, 10);

        if (strncmp(command, "stop", 4) == 0) {
            break;
        }
        else
            cout << "Error: Unknown command" << endl;
    }


    close(mySocket);
    pthread_join(accept_t, (void**)server_addr);

}