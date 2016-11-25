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
//#include <gmpxx.h>
#include <stdlib.h>     /* atoi */

using namespace std;

#define MYPORT 4000
#define BUF_SIZE 2048
#define MAX_CLIENT 3
#define MAX_NEWS 10
#define MAX_THEMES 10
#define LEN_THEME 12
#define LEN_TITLE 12
#define LEN_BODY 1000

// *************************************************************************************

// Логика индивидуального задания - Публикация/чтение новостей (1.2.13)

struct Article{
    int id;
    char theme[LEN_THEME];
    char title[LEN_TITLE];
    char body[LEN_BODY];
};
struct Article news[MAX_NEWS];
int newsCount = 0;
int newsIDCount = 0;

char themes[MAX_THEMES][LEN_THEME];
int themesCount = 0;

void addTheme(char* theme){
    strcpy(themes[themesCount++], theme);
}
bool checkTheme(char* theme){
    for (int i = 0; i < themesCount; ++i) {
        if (strcmp(themes[i], theme)){
            return true;
        }
    }
    return false;
}

int addArticle(char* theme, char* title, char* body){

    if (newsCount >= MAX_NEWS)
        return -1;

    if (!checkTheme(theme))
        return -2;

    strcpy(news[newsCount].theme, theme);
    strcpy(news[newsCount].body, body);
    strcpy(news[newsCount].title, title);

    news[newsCount++].id = newsIDCount++;

    return news[newsCount - 1].id;
}

bool delArticle(int id){
    bool find = false;
    for (int i = 0; i < newsCount; ++i) {
        if (!find && news[i].id == id){
            find = true;
        }
        else if (find){
            news[i-1] = news[i];
        }
    }
    newsCount--;
    return find;
}


// ************************************************************************************

// ************************************************************************************

// Все, что связанно с работой сервера

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
        else if (strcmp(buf, "show themes") == 0) {                      // Показать список тем

            char tmp[BUF_SIZE];
            memset(tmp, 0, BUF_SIZE);
            for (int i = 0; i < themesCount; i++){
                char str[3];
                sprintf(str, "%d", i);
                strcat(tmp, str);
                strcat(tmp, " ");
                strcat(tmp, themes[i]);
                strcat(tmp, "\n");
            }
            strcpy(ans, tmp);
            if (send(socket, ans, BUF_SIZE, 0) < 0){
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }

        }
        else if (strcmp(buf, "show articles") == 0) {    // показать список всех статей

            char tmp[BUF_SIZE];
            memset(tmp, 0, BUF_SIZE);
            for (int i = 0; i < newsCount; i++){
                    char str[3];
                    sprintf(str, "%d", news[i].id);
                    strcat(tmp, str);
                    strcat(tmp, " ");
                    strcat(tmp, news[i].title);
                    strcat(tmp, "\n");
            }
            strcpy(ans, tmp);
            if (send(socket, ans, BUF_SIZE, 0) < 0){
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }

        }
        else if (strncmp(buf, "show theme", 10) == 0) {

            char tmp[BUF_SIZE];
            memset(tmp, 0, BUF_SIZE);
            strncpy(tmp, buf+11, BUF_SIZE-11);
            int themeID = atoi(tmp);
            memset(tmp, 0, BUF_SIZE);
            for (int i = 0; i < newsCount; i++){
                if (strcmp(news[i].theme, themes[themeID]) == 0) {
                    char str[3];
                    sprintf(str, "%d", news[i].id);
                    strcat(tmp, str);
                    strcat(tmp, " ");
                    strcat(tmp, news[i].title);
                    strcat(tmp, "\n");
                }
            }
            strcpy(ans, tmp);
            if (send(socket, ans, BUF_SIZE, 0) < 0){
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }
        }
        else if (strncmp(buf, "show article", 12) == 0) {    // показать статью

            char tmp[BUF_SIZE];
            memset(tmp, 0, BUF_SIZE);
            strncpy(tmp, buf+13, BUF_SIZE-13);
            int articleID = atoi(tmp);
            memset(tmp, 0, BUF_SIZE);
            for (int i = 0; i < newsCount; i++){
                if (news[i].id == articleID) {
                    strcat(tmp, "Title: ");
                    strcat(tmp, news[i].title);
                    strcat(tmp, "\nTheme: ");
                    strcat(tmp, news[i].theme);
                    strcat(tmp, "\nBody:\n");
                    strcat(tmp, news[i].body);
                }
            }
            strcpy(ans, tmp);
            if (send(socket, ans, BUF_SIZE, 0) < 0){
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }
        }
        else if (strncmp(buf, "add theme", 9) == 0) {

            char tmp[BUF_SIZE];
            memset(tmp, 0, BUF_SIZE);
            strncpy(tmp, buf+10, BUF_SIZE-10);

            pthread_mutex_lock(&lock);
            strcpy(themes[themesCount++], tmp);
            pthread_mutex_unlock(&lock);

            strcpy(ans, "Add theme success!");
            if (send(socket, ans, BUF_SIZE, 0) < 0){
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }
        }
        else if (strncmp(buf, "add article", 11) == 0) {

                sleep(20);
            char newTitle[LEN_TITLE];
            char newTheme[LEN_THEME];
            char newBody[LEN_BODY];
            memset(newTitle, 0, LEN_TITLE);
            memset(newTheme, 0, LEN_THEME);
            memset(newBody, 0, LEN_BODY);

            strcpy(ans, "Title: ");
            if (send(socket, ans, BUF_SIZE, 0) < 0){
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }

            if ( (numRead = readn(socket, buf, BUF_SIZE)) > 0 ) {
                strcpy(newTitle, buf);
            }

            strcpy(ans, "Theme: ");
            if (send(socket, ans, BUF_SIZE, 0) < 0){
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }

            if ( (numRead = readn(socket, buf, BUF_SIZE)) > 0 ) {
                strcpy(newTheme, buf);
            }

            strcpy(ans, "Body: ");
            if (send(socket, ans, BUF_SIZE, 0) < 0){
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }

            if ( (numRead = readn(socket, buf, BUF_SIZE)) > 0 ) {
                strcpy(newBody, buf);
            }

            pthread_mutex_lock(&lock);
            if (addArticle(newTheme, newTitle, newBody) < 0){
                pthread_mutex_unlock(&lock);
                strcpy(ans, "Add article error");
                if (send(socket, ans, BUF_SIZE, 0) < 0){
                    cout << "Error: Send to " << socket << " fail" << endl;
                    continue;
                }
            }
            pthread_mutex_unlock(&lock);

            strcpy(ans, "Add article success!");
            if (send(socket, ans, BUF_SIZE, 0) < 0){
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }
        }
        else if (strncmp(buf, "del theme", 9) == 0) {

            char tmp[BUF_SIZE];
            memset(tmp, 0, BUF_SIZE);
            strncpy(tmp, buf+10, BUF_SIZE-10);
            int themeID = atoi(tmp);

            pthread_mutex_lock(&lock);
            for (int i = themeID; i < themesCount-1; ++i) {
                strcpy(themes[i], themes[i+1]);
            }
            pthread_mutex_unlock(&lock);

            strcpy(ans, "Del theme success!");
            if (send(socket, ans, BUF_SIZE, 0) < 0){
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }
        }
        else if (strncmp(buf, "del article", 11) == 0) {

            char tmp[BUF_SIZE];
            memset(tmp, 0, BUF_SIZE);
            strncpy(tmp, buf+12, BUF_SIZE-12);
            int articleID = atoi(tmp);

            pthread_mutex_lock(&lock);
            if (delArticle(articleID)) {
                pthread_mutex_unlock(&lock);
                strcpy(ans, "Del article success!");
                if (send(socket, ans, BUF_SIZE, 0) < 0) {
                    cout << "Error: Send to " << socket << " fail" << endl;
                    continue;
                }
            }
            else{
                pthread_mutex_unlock(&lock);
                strcpy(ans, "Del article error");
                if (send(socket, ans, BUF_SIZE, 0) < 0) {
                    cout << "Error: Send to " << socket << " fail" << endl;
                    continue;
                }
            }
        }
        else if (strcmp(buf, "exit") == 0){
            break;
        }
        else {
            strcpy(ans, "Invalid command");
            if (send(socket, ans, BUF_SIZE, 0) < 0) {
                cout << "Error: Send to " << socket << " fail" << endl;
                continue;
            }
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

// **************************************************************************************

int main() {

    // Подготовка структуры

    for (int i=0; i<MAX_NEWS; i++){
        news[i].id = -1;
        strcpy(news[i].body, "");
        strcpy(news[i].theme, "");
        strcpy(news[i].title, "");
    }

    for (int i=0; i<MAX_THEMES; i++){
        strcpy(themes[i], "");
    }

    // Запуск сервера

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

            char tmp[10];
            memset(tmp, 0, 10);
            strncpy(tmp, command+5, 10-5);
            int threadID = atoi(tmp);

            shut(clientStruct[threadID-1].id);

            pthread_join(clientStruct[threadID-1].thread, NULL);

            cout << "Kill success" << endl;
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