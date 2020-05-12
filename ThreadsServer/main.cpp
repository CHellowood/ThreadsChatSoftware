#include <WinSock2.h>
#include <process.h>
#include <iostream>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

#define ERROR_RET(cz, arg) \
	std::cout << cz << GetLastError() << std::endl; \
	arg

// 最大连接数
#define MAX_CONNECT_NUM 124

// 接收的数据的最大长度
#define MAX_DATABUF_LEN 1024

// 已连接上服务器(在线)的套接字
SOCKET connSocks[MAX_CONNECT_NUM];

// 当前连接数
int connCount = 0;

// 定义互斥对象
HANDLE hMutex = NULL;

// 群发消息
void GroupSend(char* buf, int& len, SOCKET& caller) {
	// 等待互斥对象hMutex的信号
	WaitForSingleObject(hMutex, INFINITE);

	for (int i = 0; i < connCount; i++) {

		if (connSocks[i] == caller) continue;

		if (send(connSocks[i], buf, len, 0) == SOCKET_ERROR) {
			std::cout << "send error: " << GetLastError() << std::endl;
			break;
		}
	}
	// 清除互斥对象hMutex的信号
	ReleaseMutex(hMutex);
}

// 清理connSocks数组里的指定套接字
void CloseConnSock(SOCKET& sock) {
	
	// 等待互斥对象hMutex的信号
	WaitForSingleObject(hMutex, INFINITE);

	for (int i = 0; i < connCount; i++) {
		if (connSocks[i] = sock) {
			connSocks[i] = connSocks[connCount--]; // 把最后的套接字移到与该客户端连接的套接字的位置
			break;
		}
	}
	// 清除互斥对象hMutex的信号
	ReleaseMutex(hMutex);
}

// 客户端消息处理
unsigned WINAPI CliMsg(void* arg) {
	// 把传进来的参数转成套接字
	SOCKET connSock = *(SOCKET*)arg;
	
	char czBuf[MAX_DATABUF_LEN] = {0};
	int count = 0; // 接收到的字节数

	// 循环接收消息, 然后把消息发送给所有已连接上服务器的客户端, 直到客户端断开连接
	while ((count = recv(connSock, czBuf, MAX_DATABUF_LEN, 0)) != 0) {
		// 接收出错
		if (count == SOCKET_ERROR) {
			//std::cout << "recv error: " << GetLastError() << std::endl;
			//break;
			ERROR_RET("recv error: ", break);
		}

		GroupSend(czBuf, count, connSock);
	}

	// 客户端已断开连接, 清除与该客户端连接的套接字
	CloseConnSock(connSock);

	return 0;
}

int main(void) {
	int retValue = 0; // main函数返回值
	WORD wVersion;
	WSADATA wsaData;
	SOCKET listenSock;          // 监听套接字
	SOCKADDR_IN svrAddr;        // 该服务器的地址数据
	SOCKADDR_IN cliAddr;        // 最后一个连上服务器的客户端的地址数据
	int len = sizeof(SOCKADDR);


	// 初始化套接字库
	wVersion = MAKEWORD(1, 1);
	int err = WSAStartup(wVersion, &wsaData);

	if (err) {
		//std::cout << "WSAStartup error: " << GetLastError() << std::endl;
		retValue = -1;
		//ret: // goto node
		ERROR_RET("WSAStartup error: ", goto ret);
	
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
		//std::cout << "error: wsaData.wVersion != " << wVersion << std::endl;
		retValue = -1;
		//wsa: // goto node
		ERROR_RET("wsaData.wVersion error: ", goto wsa);
	}

	// 创建互斥对象
	hMutex = CreateMutex(NULL, FALSE, NULL);

	// 互斥对象创建失败
	if (!hMutex) {
		retValue = -1;
		ERROR_RET("CreateMutex error: ", goto wsa);
		//wsa: // goto node
	}

	// 创建服务器监听套接字
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	
	// 创建监听套接字失败
	if (listenSock == INVALID_SOCKET) {
		//std::cout << "socket error: " << GetLastError() << std::endl;
		retValue = -1;
		//closeh: // goto node
		ERROR_RET("socket error: ", goto closeh);
	}

	// 初始化服务器地址数据
	memset(&svrAddr, 0, sizeof(svrAddr));
	svrAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	svrAddr.sin_family = AF_INET;
	svrAddr.sin_port = htons(2020);

	// 绑定地址
	if (bind(listenSock, (SOCKADDR*)&svrAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		// 绑定失败
		//std::cout << "bind error: " << GetLastError() << std::endl;
		retValue = -1;
		//closes: // goto node
		ERROR_RET("bind error: ", goto closes);
	}

	// 监听, 最多同时监听10个连接请求
	if (listen(listenSock, 10) == SOCKET_ERROR) {
		//std::cout << "listen error: " << GetLastError() << std::endl;
		retValue = -1;
		//closes: // goto node
		ERROR_RET("listen error: ", goto closes);
	}

	std::cout << "start listen...\n";

	

	while (true) {
		// 当接收到客户端的连接请求时, 与客户端对接连接
		SOCKET connSock = accept(listenSock, (SOCKADDR*)&cliAddr, &len);

		if (connSock == INVALID_SOCKET) {
			//std::cout << "accept error: " << GetLastError() << std::endl;
			//continue;
			ERROR_RET("accept error: ", continue);
		}

		// 等待互斥对象hMutex的信号
		WaitForSingleObject(hMutex, INFINITE);

		if (connCount + 1 < MAX_CONNECT_NUM) {
			if (!connSock) continue;

			// 把与客户端连接的套接字添加到数组里
			connSocks[connCount] = connSock;

			// 给每个连接创建一个线程处理消息
			HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, 
				CliMsg, (void*)&connSocks[connCount], 0, NULL);
			connCount++;

			std::cout << "connect of client ip: " << inet_ntoa(cliAddr.sin_addr) << std::endl;
		}
		else {
			closesocket(connSock);
		}

		// 清除互斥对象hMutex的信号
		ReleaseMutex(hMutex);
	}

	closes:

	// 关闭监听套接字
	closesocket(listenSock);

	closeh:

	// 清除互斥对象
	CloseHandle(hMutex);

	wsa:
	
	// 清理套接字库
	WSACleanup();

	ret:

	system("pause");
	return retValue;
}