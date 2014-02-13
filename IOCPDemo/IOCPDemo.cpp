#include <WinSock2.h>
#include <iostream>
#include <map>

#pragma comment(lib, "ws2_32.lib")



#define DefaultPort 999
#define DataBuffSize 8 * 1024

typedef struct
{
	OVERLAPPED overlapped;
	WSABUF databuff;
	CHAR buffer[ DataBuffSize ];
	DWORD bytesSend;
	DWORD bytesRecv;
}PER_IO_OPERATEION_DATA, *LPPER_IO_OPERATION_DATA;

typedef struct
{
	SOCKET socket;
}PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

DWORD WINAPI ServerWorkThread( LPVOID CompletionPortID );

void main()
{

	SOCKET acceptSocket;
	HANDLE completionPort;
	LPPER_HANDLE_DATA pHandleData;
	LPPER_IO_OPERATION_DATA pIoData;
	DWORD recvBytes;
	DWORD flags;

	WSADATA wsaData;
	DWORD ret;
	if ( ret = WSAStartup( 0x0202, &wsaData ) != 0 )
	{
		std::cout << "WSAStartup failed. Error:" << ret << std::endl;
		return;
	}

	completionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
	if ( completionPort == NULL )
	{
		std::cout << "CreateIoCompletionPort failed. Error:" << GetLastError() << std::endl;
		return;
	}

	SYSTEM_INFO mySysInfo;
	GetSystemInfo( &mySysInfo );

	// 创建 2 * CPU核数 + 1 个线程
	DWORD threadID;
	for ( DWORD i = 0; i < ( mySysInfo.dwNumberOfProcessors * 2 + 1 ); ++i )
	{
		HANDLE threadHandle;
		threadHandle = CreateThread( NULL, 0, ServerWorkThread, completionPort, 0, &threadID );
		if ( threadHandle == NULL )
		{
			std::cout << "CreateThread failed. Error:" << GetLastError() << std::endl;
			return;
		}

		CloseHandle( threadHandle );
	}

	// 启动一个监听socket
	SOCKET listenSocket = WSASocket( AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );
	if ( listenSocket == INVALID_SOCKET )
	{
		std::cout << " WSASocket( listenSocket ) failed. Error:" << GetLastError() << std::endl;
		return;
	}

	SOCKADDR_IN internetAddr;
	internetAddr.sin_family = AF_INET;
	internetAddr.sin_addr.s_addr = htonl( INADDR_ANY );
	internetAddr.sin_port = htons( DefaultPort );

	// 绑定监听端口
	if ( bind( listenSocket, (PSOCKADDR)&internetAddr, sizeof( internetAddr ) ) == SOCKET_ERROR )
	{
		std::cout << "Bind failed. Error:" << GetLastError() << std::endl;
		return;
	}

	if ( listen( listenSocket, 5 ) ==  SOCKET_ERROR )
	{
		std::cout << "listen failed. Error:" << GetLastError() << std::endl;
		return;
	}

	// 开始死循环，处理数据
	while ( 1 )
	{
		std::cout << "Listening..." << std::endl;
		acceptSocket = WSAAccept( listenSocket, NULL, NULL, NULL, 0 );
		if ( acceptSocket == SOCKET_ERROR )
		{
			std::cout << "WSAAccept failed. Error:" << GetLastError() << std::endl;
			return;
		}
		std::cout << "Accept a socket." << std::endl;

		pHandleData = (LPPER_HANDLE_DATA)GlobalAlloc( GPTR, sizeof( PER_HANDLE_DATA ) );
		if ( pHandleData = NULL )
		{
			std::cout << "GlobalAlloc( HandleData ) failed. Error:" << GetLastError() << std::endl;
			return;
		}

		pHandleData->socket = acceptSocket;
		if ( CreateIoCompletionPort( (HANDLE)acceptSocket, completionPort, (ULONG_PTR)pHandleData, 0 ) == NULL )
		{
			std::cout << "CreateIoCompletionPort failed. Error:" << GetLastError() << std::endl;
			return;
		}

		pIoData = ( LPPER_IO_OPERATION_DATA )GlobalAlloc( GPTR, sizeof( PER_IO_OPERATEION_DATA ) );
		if ( pIoData == NULL )
		{
			std::cout << "GlobalAlloc( IoData ) failed. Error:" << GetLastError() << std::endl;
			return;
		}

		ZeroMemory( &( pIoData->overlapped ), sizeof( pIoData->overlapped ) );
		pIoData->bytesSend = 0;
		pIoData->bytesRecv = 0;
		pIoData->databuff.len = DataBuffSize;
		pIoData->databuff.buf = pIoData->buffer;

		flags = 0;
		if ( WSARecv( acceptSocket, &(pIoData->databuff), 1, &recvBytes, &flags, &(pIoData->overlapped), NULL ) == SOCKET_ERROR )
		{
			if ( WSAGetLastError() != ERROR_IO_PENDING )
			{
				std::cout << "WSARecv() failed. Error:" << GetLastError() << std::endl;
				return;
			}
			else
			{
				std::cout << "WSARecv() io pending" << std::endl;
				return;
			}
		}
	}
}

DWORD WINAPI ServerWorkThread( LPVOID CompletionPortID )
{
	HANDLE complationPort = (HANDLE)CompletionPortID;
	DWORD bytesTransferred;
	LPPER_HANDLE_DATA pHandleData = NULL;
	LPPER_IO_OPERATION_DATA pIoData = NULL;
	DWORD sendBytes = 0;
	DWORD recvBytes = 0;
	DWORD flags;

	while ( 1 )
	{
		if ( GetQueuedCompletionStatus( complationPort, &bytesTransferred, (PULONG_PTR)&pHandleData, (LPOVERLAPPED *)&pIoData, INFINITE ) == 0 )
		{
			std::cout << "GetQueuedCompletionStatus failed. Error:" << GetLastError() << std::endl;
			return 0;
		}

		// 检查数据是否已经传输完了
		if ( bytesTransferred == 0 )
		{
			std::cout << " Start closing socket..." << std::endl;
			if ( CloseHandle( (HANDLE)pHandleData->socket ) == SOCKET_ERROR )
			{
				std::cout << "Close socket failed. Error:" << GetLastError() << std::endl;
				return 0;
			}

			GlobalFree( pHandleData );
			GlobalFree( pIoData );
			continue;
		}

		// 检查管道里是否有数据
		if ( pIoData->bytesRecv == 0 )
		{
			pIoData->bytesRecv = bytesTransferred;
			pIoData->bytesSend = 0;
		}
		else
		{
			pIoData->bytesSend += bytesTransferred;
		}

		// 数据没有发完，继续发送
		if ( pIoData->bytesRecv > pIoData->bytesSend )
		{
			ZeroMemory( &(pIoData->overlapped), sizeof( OVERLAPPED ) );
			pIoData->databuff.buf = pIoData->buffer + pIoData->bytesSend;
			pIoData->databuff.len = pIoData->bytesRecv - pIoData->bytesSend;

			// 发送数据出去
			if ( WSASend( pHandleData->socket, &(pIoData->databuff), 1, &sendBytes, 0, &(pIoData->overlapped), NULL ) == SOCKET_ERROR )
			{
				if ( WSAGetLastError() != ERROR_IO_PENDING )
				{
					std::cout << "WSASend() failed. Error:" << GetLastError() << std::endl;
					return 0;
				}
				else
				{
					std::cout << "WSASend() failed. io pending. Error:" << GetLastError() << std::endl;
					return 0;
				}
			}

			std::cout << "Send " << pIoData->buffer << std::endl;
		}
		else
		{
			pIoData->bytesRecv = 0;
			flags = 0;

			ZeroMemory( &(pIoData->overlapped), sizeof( OVERLAPPED ) );
			pIoData->databuff.len = DataBuffSize;
			pIoData->databuff.buf = pIoData->buffer;

			if ( WSARecv( pHandleData->socket, &(pIoData->databuff), 1, &recvBytes, &flags, &(pIoData->overlapped), NULL ) == SOCKET_ERROR )
			{
				if ( WSAGetLastError() != ERROR_IO_PENDING )
				{
					std::cout << "WSARecv() failed. Error:" << GetLastError() << std::endl;
					return 0;
				}
				else
				{
					std::cout << "WSARecv() io pending" << std::endl;
					return 0;
				}
			}
		}
	}
}



/*
const int MAX_BUFFER = 1024;
DWORD WINAPI WORKTHREAD(LPVOID lpparam);

enum IO_TYPE
{
	IO_READ,
	IO_WRITE
};
typedef struct _IO_OPERATION_DATA
{
	WSAOVERLAPPED overlapped;
	IO_TYPE opType;
	unsigned long len;
	unsigned long dwFlags;
	SOCKET socket;
	WSABUF databuf;
	char buffer[MAX_BUFFER];
}IO_OPERATION_DATA;


int main()
{
	WORD version = MAKEWORD(2, 2);
	WSADATA wsaData;

	if (WSAStartup(version, &wsaData) !=0)
	{
		std::cout << "WSAStartup failed." << std::endl;
		return 0;
	}

	SOCKET server = WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
	if (SOCKET_ERROR == server)
	{
		std::cout << "Create socket error." << std::endl;
		WSACleanup();
		return 0;
	}
	
	SOCKADDR_IN serverAddress = {0};
	serverAddress.sin_port = htons(999);
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

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


	//Start acceptance clients
	while (true)
	{
		std::cout << "Listening..." << std::endl;
		SOCKADDR_IN clientAddress = {0};
		int length = sizeof(clientAddress);
		SOCKET client = WSAAccept(server, NULL, NULL, NULL, 0);//(server, (SOCKADDR*)&clientAddress, &length, 0, NULL);
		if (INVALID_SOCKET == client)
		{
			std::cout << "Accept one invalid socket." << std::endl;
			continue;
		}
		std::cout << "Received a request." << std::endl;
		if (NULL == CreateIoCompletionPort((HANDLE)client, hServerCompletionPort, (DWORD)server, 0))
		{
			std::cout << "Failed to create completion port for client." << std::endl;
			continue;
		}

		IO_OPERATION_DATA *pIO = new IO_OPERATION_DATA;
		ZeroMemory(pIO, sizeof(IO_OPERATION_DATA));
		pIO->databuf.buf = pIO->buffer;
		pIO->databuf.len = MAX_BUFFER;
		pIO->opType = IO_READ;

		ret = WSARecv(client, &pIO->databuf, 1, &pIO->len, &(pIO->dwFlags), &(pIO->overlapped), NULL);
		if (SOCKET_ERROR == ret)
		{
			std::cout << "Failed to post recv operation." << std::endl;
		}

		std::cout << "New socket received." << std::endl;
	}

	return 1;
}

DWORD WINAPI WORKTHREAD(LPVOID lpparam)
{
	return 1;
}
*/