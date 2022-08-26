#include <stdio.h>
#include "ftpServer.h"
#include <pthread.h>

SockInfo infos[512];

// ��ʼ��socket��
bool initSocket()
{
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		printf("WSAStartup failed:%d\n", WSAGetLastError());
		return false;
	}
	return true;
}

// �ر�socket��
bool closeSocket()
{
	if (WSACleanup() != 0)
	{
		printf("WSACleanup failed:%d\n", WSAGetLastError());
		return false;
	}
	return true;
}

// �����ͻ�������
void listenToClient()
{
	// ����server socket�׽���
	// �����ֱ��ʾIPЭ��汾���׽������͡�Э������
	// �˴�ʹ��IPv4����ʽ�׽��֡�TCPЭ��
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serfd == INVALID_SOCKET)
	{
		printf("socket failed:%d\n", WSAGetLastError());
		return;
	}

	// ��socket��IP��ַ�Ͷ˿ں�
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET; // Э��汾������ʹ���socketʱһ��
	serAddr.sin_port = htons(SPORT); // �˿ںţ�htons�����ѱ����ֽ���ת���������ֽ���
	serAddr.sin_addr.S_un.S_addr = ADDR_ANY; // IP��ַ������������������
	if (bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)) != 0)
	{
		printf("bind failed:%d\n", WSAGetLastError());
		return;
	}

	// �����ͻ�������
	if (listen(serfd, 10) != 0)
	{
		printf("listen failed:%d\n", WSAGetLastError());
		return;
	}
	printf("listen to client...\n");

	// ��ʼ���ṹ������
	int max = sizeof(infos) / sizeof(SockInfo);
	for (int i = 0; i < max; i++)
	{
		memset(&infos[i], 0, sizeof(SockInfo));
		infos[i].fd = INVALID_SOCKET;
	}

	// �пͻ�������ʱ����������
	int len = sizeof(struct sockaddr_in);
	while (1)
	{
		// �ڽṹ��������Ѱ�ҿ���λ��
		SockInfo* pinfo;
		for (int i = 0; i < max; i++)
		{
			if (infos[i].fd == INVALID_SOCKET)
			{
				pinfo = &infos[i];
				break;
			}
		}

		SOCKET clifd = accept(serfd, (struct sockaddr*)&pinfo->addr, &len);
		if (clifd == INVALID_SOCKET)
		{
			printf("accept failed:%d\n", WSAGetLastError());
			continue;
		}
		pinfo->fd = clifd;

		// �������߳�
		pthread_t tid;
		pthread_create(&tid, NULL, working, pinfo);
		pthread_detach(tid);
	}
}

// ���̵߳Ĺ�������
void* working(void* arg)
{
	SockInfo* pinfo = (SockInfo*)arg;

	char ip[33] = { 0 };
	inet_ntop(AF_INET, &pinfo->addr.sin_addr.S_un.S_addr, ip, sizeof(ip));
	printf("new client [%s] connect success...\n", ip);

	char filePath[256] = { 0 }; // �ļ�����·��
	int fileSize = 0; // �ļ���С

	// ��ʼ������Ϣ
	while (processMsg(pinfo->fd, filePath, &fileSize))
	{
		//Sleep(100);
	}

	pinfo->fd = INVALID_SOCKET;
}

// ������Ϣ
bool processMsg(SOCKET clifd, char* filePath, int* fileSize)
{
	char recvBuf[1024] = { 0 }; // ���ڽ��ܿͻ��˷��͵���Ϣ
	int nRes = recv(clifd, recvBuf, 1024, 0);
	if (nRes <= 0)
	{
		printf("�ͻ��˶Ͽ����ӡ�����%d\n", WSAGetLastError());
		return false;
	}

	struct MsgHeader *msg = (struct MsgHeader*) recvBuf;
	switch (msg->msgID)
	{
	case MSG_FILENAME:
		readFile(clifd, msg, filePath, fileSize);
		break;
	case MSG_READY_READ:
		sendFile(clifd, msg, filePath, *fileSize);
		break;
	default:
		break;
	}

	return true;
}

// ��ȡ�ļ��������ļ���С
bool readFile(SOCKET clifd, struct MsgHeader* pMsgHeader, char* filePath, int* fileSize)
{
	// ��ȡ�ļ���
	char tfname[200] = { 0 }; // �ļ���
	char text[100]; // ��׺
	_splitpath(pMsgHeader->fileInfo.fileName, NULL, NULL, tfname, text); // ��ȡ�ļ����ͺ�׺
	strcat(tfname, text); // ���ļ����ͺ�׺ƴ����һ��

	// ������·��
	sprintf(filePath, "%s%s", "send/", tfname);

	// ���ļ�
	FILE* pread = fopen(filePath, "rb");
	if (pread == NULL)
	{
		printf("�Ҳ���[%s]�ļ�...\n", filePath);

		struct MsgHeader msg = { .msgID = MSG_OPENFILE_FAILED };
		if (send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
		{
			printf("send failed:%d\n", WSAGetLastError());
		}
		return false;
	}

	// ��ȡ�ļ���С
	fseek(pread, 0, SEEK_END);
	*fileSize = ftell(pread);
	fseek(pread, 0, SEEK_SET);
	fclose(pread);

	// ���ļ���С���ļ������͸��ͻ���
	struct MsgHeader msg = { .msgID = MSG_FILESIZE, .fileInfo.fileSize = *fileSize };
	strcpy(msg.fileInfo.fileName, tfname);
	if (send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
	{
		printf("�����ļ���Сʧ��:%d\n", WSAGetLastError());
		return false;
	}

	return true;
}

// �����ļ�
bool sendFile(SOCKET clifd, struct MsgHeader* pMsg, char* filePath, int fileSize)
{
	// ���ļ�
	FILE* pread = fopen(filePath, "rb");
	if (pread == NULL)
	{
		printf("�Ҳ���[%s]�ļ�...\n", filePath);

		struct MsgHeader msg = { .msgID = MSG_OPENFILE_FAILED };
		if (send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
		{
			printf("send failed:%d\n", WSAGetLastError());
		}
		return false;
	}
	fseek(pread, 0, SEEK_SET);

	// ��ȡ�ļ�
	char* fileBuf = NULL; // �洢�ļ�����
	fileBuf = calloc(fileSize + 1, sizeof(char));
	if (fileBuf == NULL)
	{
		printf("�ڴ�����ʧ��\n");
		return false;
	}

	fseek(pread, 0, SEEK_SET);
	fread(fileBuf, sizeof(char), fileSize, pread);
	fileBuf[fileSize] = '\0';

	fclose(pread);

	for (int i = 0; i < fileSize; i += PACKET_SIZE)
	{
		struct MsgHeader msg = { .msgID = MSG_SENDFILE, .packet.nStart = i };
		
		if (i + PACKET_SIZE + 1 > fileSize) // ���Ĵ�С����ʣ���ļ��Ĵ�С
		{
			msg.packet.packetSize = fileSize - i;
		}
		else
		{
			msg.packet.packetSize = PACKET_SIZE;
		}

		memcpy(msg.packet.buf, fileBuf + msg.packet.nStart, msg.packet.packetSize);
		if (send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
		{
			printf("�ļ�����ʧ��:%d\n", WSAGetLastError());
			return false;
		}
	}
	
	return true;
}

int main()
{
	initSocket();

	listenToClient();

	closeSocket();
	return 0;
}