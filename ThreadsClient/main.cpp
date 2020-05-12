#include <WinSock2.h>
#include <process.h>
#include <iostream>
#include <Windows.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

// ��Ϣ����
#define MSGBUF_LEN 124

// �ǳƳ���
#define NICKBUF_LEN 10

// ���ͺͽ��յ����ݵĳ���
#define DATABUF_LEN NICKBUF_LEN + MSGBUF_LEN + 2

// �߳���
#define THREAD_COUNT 2

char nick[NICKBUF_LEN + 1] = {0};

// ������Ϣ
unsigned WINAPI SendMsg(void* arg) {
	
	// �Ѵ������Ĳ���ת���׽���
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

// ������Ϣ
unsigned WINAPI RecvMsg(void* arg) {

	// �Ѵ������Ĳ���ת���׽���
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
	// �����ǳ�
	std::cout << "input nickname: ";
	std::cin >> nick;

	// �ǳƳ��ȹ���
	if (strlen(nick) > NICKBUF_LEN) {
		std::cout << "nickname length greater than 10, input 'Y' wipe off redundance or input 'N' again input nickname: ";
	}
	else { // �Ƿ�ʹ��������ǳ�
		std::cout << "use " << nick << " regard as nickname ? (Y/N): ";
	}

	char ch = 0;
	std::cin >> ch;

	if (ch == 'Y' || ch == 'y') {      // �������ಿ��/ʹ��
		nick[NICKBUF_LEN] = 0;
	}
	else if (ch == 'N' || ch == 'n') { // ���������ǳ�
		nick[0] = 0;  // ����ǳ�
		SetNickname();
	}
}

int main(void) {
	WORD wVersion;
	WSADATA wsaData;

	// ��ʼ���׽��ֿ�
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
	
	// �����ǳ�
	SetNickname();

	// �����ͻ����׽���
	SOCKET cliSock = socket(AF_INET, SOCK_STREAM, 0);

	// ���ö˿ں�IP
	SOCKADDR_IN svrAddr;
	memset(&svrAddr, 0, sizeof(svrAddr)); // ��svrAddr��Ϊ0
	svrAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	svrAddr.sin_family = AF_INET;
	svrAddr.sin_port = htons(2020);

	// ���ӷ�����
	if (connect(cliSock, (SOCKADDR*)&svrAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		std::cout << "connect error: " << GetLastError() << std::endl;
		
		// �رռ����׽���
		closesocket(cliSock);
		// �����׽��ֿ�
		WSACleanup();

		system("pause");
		return -1;
	}	

	std::cout << "connect server success.\n";

	HANDLE hThread[THREAD_COUNT];

	// ������Ϣ�߳�
	hThread[0] = (HANDLE)_beginthreadex(NULL, 0,
		SendMsg, (void*)&cliSock, 0, NULL);
	
	// ������Ϣ�߳�
	hThread[1] = (HANDLE)_beginthreadex(NULL, 0,
		RecvMsg, (void*)&cliSock, 0, NULL);

	// �ȴ������߳̽���
	WaitForMultipleObjects(THREAD_COUNT, hThread, TRUE, INFINITE);

	// �رռ����׽���
	closesocket(cliSock);
	// �����׽��ֿ�
	WSACleanup();

	system("pause");
	return 0;
}