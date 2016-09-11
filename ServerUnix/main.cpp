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

using namespace std;

#define MYPORT 4000
#define BACKLOG 10

int main() {
    int mySocket, newSocket;        // mySocket - здесь слушаем, newSocket - сокет для нового подключения
    struct sockaddr_in server_addr; // адресная информация сервера
    struct sockaddr_in client_addr; // адресная информация запрашивающей стороны (клиента)
    char buf[1024];

    int sin_size;

    // создание сокета
    if ( (mySocket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Socket error");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(MYPORT);

    bzero(&(server_addr.sin_zero), 8);  // обнуление остальной части структуры

    // свяывание соданного сокета с локальным адресом
    if ( bind(mySocket, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1){
        perror("Bind error");
        exit(1);
    }

    // Создание очереди прослушивания сети на порту MYPORT
    if ( listen(mySocket, BACKLOG) == -1 ){
        perror("Listen error");
        exit(1);
    }

    // цикл accept()
    while(1){
        sin_size = sizeof(struct sockaddr_in);
        if ( (newSocket = accept(mySocket, (struct sockaddr*)&client_addr, &sin_size)) == -1){
            perror("Accept error");
            continue;
        }

        // запрос на соединение принят, теперь нужно ответить
        cout << "Accept complete" << endl;

        if ( !fork() ){
            if ( send(newSocket, "Hello, World\n", 13, 0) == -1 ){
                perror("Send error");
            }

            close(newSocket);
            exit(0);
        }

        close(newSocket);
        while(waitpid(-1, NULL, WNOHANG) > 0);
    }
}