#include <stdio.h>
#include "ftpClient.h"

char g_recvBuf[1024] = {0}; // ������Ϣ�Ļ�����
char g_fileName[256]; // �ļ���
char *g_fileBuf; // �洢�ļ�����
int g_fileSize; // �ļ���С

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
void connectToHost()
{
	// �����׽���
	// �����ֱ��ʾIPЭ��汾���׽������͡�Э������
	// �˴�ʹ��IPv4����ʽ�׽��֡�TCPЭ��
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serfd == INVALID_SOCKET)
	{
		printf("socket failed:%d\n", WSAGetLastError());
		return;
	}
	
	// ���ӵ�������
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET; // Э��汾������ʹ���socketʱһ��
	serAddr.sin_port = htons(SPORT); // �˿ںţ�htons�����ѱ����ֽ���ת���������ֽ���
	serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // ��������IP��ַ
	if (connect(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)) != 0)
	{
		printf("connect failed:%d\n", WSAGetLastError());
		return;
	}

	sendFileName(serfd);

	// ��ʼ������Ϣ
	while (processMsg(serfd))
	{
		//Sleep(100);
	}
}

// ������Ϣ
bool processMsg(SOCKET serfd)
{
	int size = recv(serfd, g_recvBuf, 1024, 0);
	if (size <= 0)
	{
		printf("��������Ͽ����ӣ������������ͻ���\n");
		if (g_fileBuf != NULL)
		{
			free(g_fileBuf);
			g_fileBuf = NULL;
		}
		return false;
	}

	struct MsgHeader *msg = (struct MsgHeader*)g_recvBuf;
	switch (msg->msgID)
	{
	case MSG_OPENFILE_FAILED:
		printf("�Ҳ������ļ�������\n");
		while (!sendFileName(serfd)); // ֻҪ���Ͳ��ɹ���һֱ���ͣ�ֱ�����ͳɹ�Ϊֹ
		break;
	case MSG_FILESIZE:
		readyRead(serfd, msg);
		break;
	case MSG_SENDFILE:
		recvFile(serfd, msg);
		break;
	default:
		break;
	}
	
	return true;
}

bool sendFileName(SOCKET serfd)
{
	printf("������Ҫ���ص��ļ�����");
	char fileName[1024] = { 0 };
	gets_s(fileName, 1023); // ��ȡ�ļ���

	struct MsgHeader file = { .msgID = MSG_FILENAME };
	strcpy(file.fileInfo.fileName, fileName);
	if (send(serfd, (char*)&file, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
	{
		printf("�����ļ���ʧ��:%d\n", WSAGetLastError());
		return false;
	}

	return true;
}

void readyRead(SOCKET serfd, struct MsgHeader* pMsg)
{
	printf("size:%d   filename:%s\n", pMsg->fileInfo.fileSize, pMsg->fileInfo.fileName);
	// strcpy(g_fileName, pMsg->fileInfo.fileName); // �����ļ���
	sprintf(g_fileName, "%s%s", "recv/", pMsg->fileInfo.fileName); // �����ļ���

	// ׼���ڴ�
	g_fileSize = pMsg->fileInfo.fileSize;
	g_fileBuf = calloc(g_fileSize + 1, sizeof(char));
	if (g_fileBuf == NULL)
	{
		// �ڴ�����ʧ��
		printf("�ڴ�����ʧ��\n");
	}
	else
	{
		// ����������� MSG_READY_READ
		struct MsgHeader msg = {.msgID = MSG_READY_READ};
		if (send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
		{
			printf("send error:%d\n", WSAGetLastError());
		}
	}
}

// �����ļ�
bool recvFile(SOCKET serfd, struct MsgHeader* pMsg)
{
	if (g_fileBuf == NULL)
	{
		return false;
	}

	memcpy(g_fileBuf + pMsg->packet.nStart, pMsg->packet.buf, pMsg->packet.packetSize);

	if (pMsg->packet.nStart + pMsg->packet.packetSize >= g_fileSize) // �յ����һ����֮�󣬴����ļ���д������
	{
		FILE* pfile = fopen(g_fileName, "wb");
		if (pfile == NULL)
		{
			printf("�����ļ�ʧ�ܡ�����\n\n");
			free(g_fileBuf);
			g_fileBuf = NULL;
			return false;
		}

		fwrite(g_fileBuf, sizeof(char), g_fileSize, pfile);
		fclose(pfile);

		free(g_fileBuf);
		g_fileBuf = NULL;

		struct MsgHeader msg = { .msgID = MSG_SUCCESSED };
		send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0);
		printf("�ļ����ճɹ���\n\n");

		while (!sendFileName(serfd)); // ֻҪ���Ͳ��ɹ���һֱ���ͣ�ֱ�����ͳɹ�Ϊֹ
	}

	return true;
}

int main()
{
	initSocket();

	connectToHost();

	closeSocket();
	return 0;
}