#include <WinSock2.h>
#include <iostream>
#include <map>

#pragma comment(lib, "ws2_32.lib")

DWORD WINAPI WORKTHREAD(LPVOID lpparam);

int main()
{
	WORD version = MAKEWORD(2, 2);
	WSADATA wsaData;

	WSAStartup(version, &wsaData);

	SOCKET server = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (SOCKET_ERROR == server)
	{
		std::cout << "Create socket error." << std::endl;
		WSACleanup();
		return 0;
	}
	
	SOCKADDR_IN serverAddress = {0};
	serverAddress.sin_port = htonl(999);
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.S_un.S_addr = htons(INADDR_ANY);

	int ret = bind(server, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
	if (SOCKET_ERROR == ret)
	{
		std::cout << "bind failure." << std::endl;
		closesocket(server);
		WSACleanup();
		return 0;
	}

	if (SOCKET_ERROR == listen(server, 128))
	{
		std::cout << "listen failure." << std::endl;
		closesocket(server);
		WSACleanup();
		return 0;
	}

	// Create completion port for server
	HANDLE hServerCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (!hServerCompletionPort)
	{
		std::cout << "Create completion port failure." << std::endl;
		closesocket(server);
		WSACleanup();
		return 0;
	}

	//Create work threads for any IO's response
	SYSTEM_INFO system = {0};
	DWORD dwThreadId = 0;
	GetSystemInfo(&system);
	for (int i = 0; i < system.dwNumberOfProcessors*2; ++i)
	{
		HANDLE workThread = CreateThread(NULL, 0, WORKTHREAD, hServerCompletionPort, 0, &dwThreadId);
		if (!workThread)
		{
			std::cout << "Create work thread failure." << std::endl;
			break;
		}
	}
	return 1;
}

DWORD WINAPI WORKTHREAD(LPVOID lpparam)
{
	return 1;
}