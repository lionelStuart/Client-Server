#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock.h>
#include <iostream>
#include <string.h>

using namespace std;

#pragma comment(lib,"Ws2_32.lib")

#define BUF_SIZE 2048

int readn(SOCKET socket, char* bf, int len) {
	int load = 0;
	char tmpbuf[BUF_SIZE];
	memset(bf, 0, len);
	memset(tmpbuf, 0, len);
	while (load < len)
	{
		int numbyte = recv(socket, tmpbuf, len - load, 0);
		
		if (numbyte <= 0) return -1;
		
		load += numbyte;
		strcat(bf, tmpbuf);
		strcpy(tmpbuf, "");
	}
	return load;
}

void main() {

	char IPAddress[15];
	int myPort;

	char command[BUF_SIZE];
	char answer[BUF_SIZE];

	//Connect to host

	cout << "Input IP address: ";
	cin.getline(IPAddress, sizeof(IPAddress));
	cout << "Input port: ";
	cin >> myPort;

	WSADATA wsadata;
	int error = WSAStartup(0x0202, &wsadata);

	SOCKADDR_IN mySockAddr;

	SOCKET mySocket = socket(AF_INET, SOCK_STREAM, 0);

	mySockAddr.sin_family = AF_INET;
	mySockAddr.sin_port = htons(myPort);
	mySockAddr.sin_addr.s_addr = inet_addr(IPAddress);
	
	cout << "Ready to connect: " << IPAddress << ":" << myPort << endl;
	system("pause");

	if (mySocket == INVALID_SOCKET)
	{
		cout << "Create socket fail" << endl;
		system("pause");
		return;
	}

	if (connect(mySocket, (SOCKADDR *)&mySockAddr, sizeof(mySockAddr)) == SOCKET_ERROR)
	{
		cout << "Connect to " << IPAddress << ":" << myPort << " fail" << endl;
		system("pause");
		return;
	}
	else {
		cout << "Connect to " << IPAddress << ":" << myPort << " success!" << endl;
	}

	
	while (true)
	{
		memset(command, 0, BUF_SIZE);
		memset(answer, 0, BUF_SIZE);

		cout << " > ";
		cin.getline(command, BUF_SIZE);

		if (strcmp(command, "exit") == 0)
		{
			if (send(mySocket, command, BUF_SIZE, 0) < 0) {
				cout << "Error: Send fail" << endl;
				continue;
			}
			break;
		}
		else 
		{
			if (send(mySocket, command, BUF_SIZE, 0) < 0) {
				cout << "Error: Send fail" << endl;
				continue;
			}

			if (readn(mySocket, answer, BUF_SIZE) < 0) {
				cout << "Error: Resive fail" << endl;
				continue;
			}

			cout << endl << " " << answer << endl << endl;
		}
	}
	
	system("pause");
}