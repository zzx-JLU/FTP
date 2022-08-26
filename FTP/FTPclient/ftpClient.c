#include <stdio.h>
#include "ftpClient.h"

char g_recvBuf[1024] = {0}; // 接收信息的缓冲区
char g_fileName[256]; // 文件名
char *g_fileBuf; // 存储文件内容
int g_fileSize; // 文件大小

// 初始化socket库
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

// 关闭socket库
bool closeSocket()
{
	if (WSACleanup() != 0)
	{
		printf("WSACleanup failed:%d\n", WSAGetLastError());
		return false;
	}
	return true;
}

// 监听客户端连接
void connectToHost()
{
	// 创建套接字
	// 参数分别表示IP协议版本、套接字类型、协议类型
	// 此处使用IPv4、流式套接字、TCP协议
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serfd == INVALID_SOCKET)
	{
		printf("socket failed:%d\n", WSAGetLastError());
		return;
	}
	
	// 连接到服务器
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET; // 协议版本，必须和创建socket时一致
	serAddr.sin_port = htons(SPORT); // 端口号，htons函数把本地字节序转换成网络字节序
	serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // 服务器的IP地址
	if (connect(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)) != 0)
	{
		printf("connect failed:%d\n", WSAGetLastError());
		return;
	}

	sendFileName(serfd);

	// 开始处理消息
	while (processMsg(serfd))
	{
		//Sleep(100);
	}
}

// 处理消息
bool processMsg(SOCKET serfd)
{
	int size = recv(serfd, g_recvBuf, 1024, 0);
	if (size <= 0)
	{
		printf("与服务器断开连接，请重新启动客户端\n");
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
		printf("找不到该文件。。。\n");
		while (!sendFileName(serfd)); // 只要发送不成功就一直发送，直到发送成功为止
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
	printf("请输入要下载的文件名：");
	char fileName[1024] = { 0 };
	gets_s(fileName, 1023); // 获取文件名

	struct MsgHeader file = { .msgID = MSG_FILENAME };
	strcpy(file.fileInfo.fileName, fileName);
	if (send(serfd, (char*)&file, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
	{
		printf("发送文件名失败:%d\n", WSAGetLastError());
		return false;
	}

	return true;
}

void readyRead(SOCKET serfd, struct MsgHeader* pMsg)
{
	printf("size:%d   filename:%s\n", pMsg->fileInfo.fileSize, pMsg->fileInfo.fileName);
	// strcpy(g_fileName, pMsg->fileInfo.fileName); // 保存文件名
	sprintf(g_fileName, "%s%s", "recv/", pMsg->fileInfo.fileName); // 保存文件名

	// 准备内存
	g_fileSize = pMsg->fileInfo.fileSize;
	g_fileBuf = calloc(g_fileSize + 1, sizeof(char));
	if (g_fileBuf == NULL)
	{
		// 内存申请失败
		printf("内存申请失败\n");
	}
	else
	{
		// 向服务器发送 MSG_READY_READ
		struct MsgHeader msg = {.msgID = MSG_READY_READ};
		if (send(serfd, (char*)&msg, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
		{
			printf("send error:%d\n", WSAGetLastError());
		}
	}
}

// 接收文件
bool recvFile(SOCKET serfd, struct MsgHeader* pMsg)
{
	if (g_fileBuf == NULL)
	{
		return false;
	}

	memcpy(g_fileBuf + pMsg->packet.nStart, pMsg->packet.buf, pMsg->packet.packetSize);

	if (pMsg->packet.nStart + pMsg->packet.packetSize >= g_fileSize) // 收到最后一个包之后，创建文件并写入数据
	{
		FILE* pfile = fopen(g_fileName, "wb");
		if (pfile == NULL)
		{
			printf("接收文件失败。。。\n\n");
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
		printf("文件接收成功！\n\n");

		while (!sendFileName(serfd)); // 只要发送不成功就一直发送，直到发送成功为止
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