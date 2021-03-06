// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
#define PORT 999
#define IP_ADDRESS "127.0.0.1"
int main()
{
	WSADATA  Ws;
	SOCKET CientSocket;
	struct sockaddr_in ServerAddr;
	int Ret = 0;
	int AddrLen = 0;
	char SendBuffer[MAX_PATH];


	//Init Windows Socket
	if ( WSAStartup(MAKEWORD(2,2), &Ws) != 0 )
	{
		cout<<"Init Windows Socket Failed::"<<GetLastError()<<endl;
		 return -1;
	}

	//Create Socket
	CientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( CientSocket == INVALID_SOCKET )
	{
		cout<<"Create Socket Failed::"<<GetLastError()<<endl;
		return -1;
	}

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	ServerAddr.sin_port = htons(PORT);
	memset(ServerAddr.sin_zero, 0x00, 8);

	Ret = connect(CientSocket,(struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
	if ( Ret == SOCKET_ERROR )
	{
		cout<<"Connect Error::"<<GetLastError()<<endl;
		getchar();
		return -1;
	}
	else
	{
		cout<<"Connected with server"<<endl;
	}
	int i = 0;
	char buffer[256] = {0};
	while (i<=10)
	{
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "lixijiang:%d", i);
		send(CientSocket, buffer, sizeof(buffer), 0);
		std::cout << "data sent" << std::endl;
		++i;
	}
	getchar();
	return 0;
}
