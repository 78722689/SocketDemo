// SocketDemo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <iostream>
#include <map>

#pragma comment(lib, "ws2_32.lib")

int _tmain(int argc, _TCHAR* argv[])
{
	WORD version = MAKEWORD(2, 2);
	WSADATA ws;
	WSAStartup(version, &ws);

	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (SOCKET_ERROR == server)
	{
		std::cout << "Invalid socket." << std::endl;
		WSACleanup();
		return 0;
	}

	SOCKADDR_IN address = {0};
	address.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	address.sin_port = htons(999);
	address.sin_family = AF_INET;
	if (SOCKET_ERROR == bind(server, (SOCKADDR*)&address, sizeof(address)))
	{
		std::cout << "Bind failure." << std::endl;
		closesocket(server);
		WSACleanup();
		return 0;
	}

	if (SOCKET_ERROR == listen(server, 128))
	{
		std::cout << "Listen failure." << std::endl;
		closesocket(server);
		WSACleanup();
		return 0;
	}
	std::cout << "Listening" << std::endl;

	fd_set fdRead;
	fd_set fdWrite;
	FD_ZERO(&fdRead);
	FD_ZERO(&fdWrite);
	FD_SET(server, &fdRead);

	std::map<SOCKET, SOCKADDR_IN> mapClientSockets;
	mapClientSockets.insert(std::make_pair(server, address));
	while (true)
	{
		fd_set fdReadTemp = fdRead;
		int ret = select(mapClientSockets.rbegin()->first+1, &fdReadTemp, &fdWrite, NULL, NULL);
		if (ret <= 0)
		{
			continue;
		}

		//Accept&Receive data
		std::map<SOCKET, SOCKADDR_IN>::iterator it = mapClientSockets.begin();
		while (it != mapClientSockets.end())
		{
			if (FD_ISSET(it->first, &fdReadTemp))
			{
				//Accept
				if (it->first == server)
				{
					SOCKADDR_IN newSocketAddress = {0};
					int len = sizeof(newSocketAddress);
					SOCKET newSocket = accept(server, (SOCKADDR*)&newSocketAddress, &len);
					if (INVALID_SOCKET == newSocket)
					{
						std::cout << "Accept one invalid socket." << std::endl;
					}
					else
					{
						std::cout << "Accept one valid socket." << std::endl;
						mapClientSockets.insert(std::make_pair(newSocket, newSocketAddress));
						FD_SET(newSocket, &fdRead);
					}
				}
				else
				{
					//Receive data;
					int bufferLength = 256;
					char buffer[256] = {0};
					int ret = recv(it->first, buffer, bufferLength, 0);
					if (ret > 0)
					{
						std::cout << "Received data " << buffer << std::endl;
					}
				}

			}

			//Write data
			if (FD_ISSET(it->first, &fdWrite))
			{
				//TODO...
				std::cout << "Write data." << std::endl;
			}

			++it;
		}
	}

	closesocket(server);
	WSACleanup();
	return 0;
}

