#include <WinSock2.h>
#include <process.h>
#include <iostream>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

#define ERROR_RET(cz, arg) \
	std::cout << cz << GetLastError() << std::endl; \
	arg

// ���������
#define MAX_CONNECT_NUM 124

// ���յ����ݵ���󳤶�
#define MAX_DATABUF_LEN 1024

// �������Ϸ�����(����)���׽���
SOCKET connSocks[MAX_CONNECT_NUM];

// ��ǰ������
int connCount = 0;

// ���廥�����
HANDLE hMutex = NULL;

// Ⱥ����Ϣ
void GroupSend(char* buf, int& len, SOCKET& caller) {
	// �ȴ��������hMutex���ź�
	WaitForSingleObject(hMutex, INFINITE);

	for (int i = 0; i < connCount; i++) {

		if (connSocks[i] == caller) continue;

		if (send(connSocks[i], buf, len, 0) == SOCKET_ERROR) {
			std::cout << "send error: " << GetLastError() << std::endl;
			break;
		}
	}
	// ����������hMutex���ź�
	ReleaseMutex(hMutex);
}

// ����connSocks�������ָ���׽���
void CloseConnSock(SOCKET& sock) {
	
	// �ȴ��������hMutex���ź�
	WaitForSingleObject(hMutex, INFINITE);

	for (int i = 0; i < connCount; i++) {
		if (connSocks[i] = sock) {
			connSocks[i] = connSocks[connCount--]; // �������׽����Ƶ���ÿͻ������ӵ��׽��ֵ�λ��
			break;
		}
	}
	// ����������hMutex���ź�
	ReleaseMutex(hMutex);
}

// �ͻ�����Ϣ����
unsigned WINAPI CliMsg(void* arg) {
	// �Ѵ������Ĳ���ת���׽���
	SOCKET connSock = *(SOCKET*)arg;
	
	char czBuf[MAX_DATABUF_LEN] = {0};
	int count = 0; // ���յ����ֽ���

	// ѭ��������Ϣ, Ȼ�����Ϣ���͸������������Ϸ������Ŀͻ���, ֱ���ͻ��˶Ͽ�����
	while ((count = recv(connSock, czBuf, MAX_DATABUF_LEN, 0)) != 0) {
		// ���ճ���
		if (count == SOCKET_ERROR) {
			//std::cout << "recv error: " << GetLastError() << std::endl;
			//break;
			ERROR_RET("recv error: ", break);
		}

		GroupSend(czBuf, count, connSock);
	}

	// �ͻ����ѶϿ�����, �����ÿͻ������ӵ��׽���
	CloseConnSock(connSock);

	return 0;
}

int main(void) {
	int retValue = 0; // main��������ֵ
	WORD wVersion;
	WSADATA wsaData;
	SOCKET listenSock;          // �����׽���
	SOCKADDR_IN svrAddr;        // �÷������ĵ�ַ����
	SOCKADDR_IN cliAddr;        // ���һ�����Ϸ������Ŀͻ��˵ĵ�ַ����
	int len = sizeof(SOCKADDR);


	// ��ʼ���׽��ֿ�
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

	// �����������
	hMutex = CreateMutex(NULL, FALSE, NULL);

	// ������󴴽�ʧ��
	if (!hMutex) {
		retValue = -1;
		ERROR_RET("CreateMutex error: ", goto wsa);
		//wsa: // goto node
	}

	// ���������������׽���
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	
	// ���������׽���ʧ��
	if (listenSock == INVALID_SOCKET) {
		//std::cout << "socket error: " << GetLastError() << std::endl;
		retValue = -1;
		//closeh: // goto node
		ERROR_RET("socket error: ", goto closeh);
	}

	// ��ʼ����������ַ����
	memset(&svrAddr, 0, sizeof(svrAddr));
	svrAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	svrAddr.sin_family = AF_INET;
	svrAddr.sin_port = htons(2020);

	// �󶨵�ַ
	if (bind(listenSock, (SOCKADDR*)&svrAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		// ��ʧ��
		//std::cout << "bind error: " << GetLastError() << std::endl;
		retValue = -1;
		//closes: // goto node
		ERROR_RET("bind error: ", goto closes);
	}

	// ����, ���ͬʱ����10����������
	if (listen(listenSock, 10) == SOCKET_ERROR) {
		//std::cout << "listen error: " << GetLastError() << std::endl;
		retValue = -1;
		//closes: // goto node
		ERROR_RET("listen error: ", goto closes);
	}

	std::cout << "start listen...\n";

	

	while (true) {
		// �����յ��ͻ��˵���������ʱ, ��ͻ��˶Խ�����
		SOCKET connSock = accept(listenSock, (SOCKADDR*)&cliAddr, &len);

		if (connSock == INVALID_SOCKET) {
			//std::cout << "accept error: " << GetLastError() << std::endl;
			//continue;
			ERROR_RET("accept error: ", continue);
		}

		// �ȴ��������hMutex���ź�
		WaitForSingleObject(hMutex, INFINITE);

		if (connCount + 1 < MAX_CONNECT_NUM) {
			if (!connSock) continue;

			// ����ͻ������ӵ��׽�����ӵ�������
			connSocks[connCount] = connSock;

			// ��ÿ�����Ӵ���һ���̴߳�����Ϣ
			HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, 
				CliMsg, (void*)&connSocks[connCount], 0, NULL);
			connCount++;

			std::cout << "connect of client ip: " << inet_ntoa(cliAddr.sin_addr) << std::endl;
		}
		else {
			closesocket(connSock);
		}

		// ����������hMutex���ź�
		ReleaseMutex(hMutex);
	}

	closes:

	// �رռ����׽���
	closesocket(listenSock);

	closeh:

	// ����������
	CloseHandle(hMutex);

	wsa:
	
	// �����׽��ֿ�
	WSACleanup();

	ret:

	system("pause");
	return retValue;
}