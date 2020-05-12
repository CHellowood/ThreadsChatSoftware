#include <WinSock2.h>
#include <process.h>
#include <iostream>
#include <Windows.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

// 消息长度
#define MSGBUF_LEN 124

// 昵称长度
#define NICKBUF_LEN 10

// 发送和接收的数据的长度
#define DATABUF_LEN NICKBUF_LEN + MSGBUF_LEN + 2

// 线程数
#define THREAD_COUNT 2

char nick[NICKBUF_LEN + 1] = {0};

// 发送消息
unsigned WINAPI SendMsg(void* arg) {
	
	// 把传进来的参数转成套接字
	SOCKET cliSock = *(SOCKET*)arg;

	char czBuf[MSGBUF_LEN+1] = {0};

	while (true) {
		gets_s(czBuf, sizeof(czBuf));

		if (strlen(czBuf) < 1) {
			continue;
		}

		czBuf[MSGBUF_LEN] = 0;

		char czMsg[DATABUF_LEN+1] = {0};
		sprintf_s(czMsg,DATABUF_LEN+1, "%s: %s", nick, czBuf);

		if (send(cliSock, czMsg, strlen(czMsg)+1, 0) == SOCKET_ERROR) {
			std::cout << "send error: " << GetLastError() << std::endl;
			break;
		}
	}
	
	return 0;
}

// 接收消息
unsigned WINAPI RecvMsg(void* arg) {

	// 把传进来的参数转成套接字
	SOCKET cliSock = *(SOCKET*)arg;

	char czMsg[DATABUF_LEN+1] = {0};

	while (true) {
		if (recv(cliSock, czMsg, DATABUF_LEN+1, 0) == SOCKET_ERROR) {
			std::cout << "recv error: " << GetLastError() << std::endl;
			break;
		}

		std::cout << czMsg << std::endl;
	}

	return 0;
}

void SetNickname() {
	// 输入昵称
	std::cout << "input nickname: ";
	std::cin >> nick;

	// 昵称长度过长
	if (strlen(nick) > NICKBUF_LEN) {
		std::cout << "nickname length greater than 10, input 'Y' wipe off redundance or input 'N' again input nickname: ";
	}
	else { // 是否使用输入的昵称
		std::cout << "use " << nick << " regard as nickname ? (Y/N): ";
	}

	char ch = 0;
	std::cin >> ch;

	if (ch == 'Y' || ch == 'y') {      // 擦除多余部分/使用
		nick[NICKBUF_LEN] = 0;
	}
	else if (ch == 'N' || ch == 'n') { // 重新输入昵称
		nick[0] = 0;  // 清空昵称
		SetNickname();
	}
}

int main(void) {
	WORD wVersion;
	WSADATA wsaData;

	// 初始化套接字库
	wVersion = MAKEWORD(1, 1);
	int err = WSAStartup(wVersion, &wsaData);

	if (err) {
		std::cout << "WSAStartup error: " << GetLastError() << std::endl;
		return -1;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
		std::cout << "error: wsaData.wVersion != " << wVersion << std::endl;
		WSACleanup();
		return -1;
	}
	
	// 设置昵称
	SetNickname();

	// 创建客户端套接字
	SOCKET cliSock = socket(AF_INET, SOCK_STREAM, 0);

	// 配置端口和IP
	SOCKADDR_IN svrAddr;
	memset(&svrAddr, 0, sizeof(svrAddr)); // 把svrAddr设为0
	svrAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	svrAddr.sin_family = AF_INET;
	svrAddr.sin_port = htons(2020);

	// 连接服务器
	if (connect(cliSock, (SOCKADDR*)&svrAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		std::cout << "connect error: " << GetLastError() << std::endl;
		
		// 关闭监听套接字
		closesocket(cliSock);
		// 清理套接字库
		WSACleanup();

		system("pause");
		return -1;
	}	

	std::cout << "connect server success.\n";

	HANDLE hThread[THREAD_COUNT];

	// 发送消息线程
	hThread[0] = (HANDLE)_beginthreadex(NULL, 0,
		SendMsg, (void*)&cliSock, 0, NULL);
	
	// 接收消息线程
	hThread[1] = (HANDLE)_beginthreadex(NULL, 0,
		RecvMsg, (void*)&cliSock, 0, NULL);

	// 等待所有线程结束
	WaitForMultipleObjects(THREAD_COUNT, hThread, TRUE, INFINITE);

	// 关闭监听套接字
	closesocket(cliSock);
	// 清理套接字库
	WSACleanup();

	system("pause");
	return 0;
}