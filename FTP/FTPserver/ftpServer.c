#include <stdio.h>
#include "ftpServer.h"
#include <pthread.h>

SockInfo infos[512];

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
void listenToClient()
{
	// 创建server socket套接字
	// 参数分别表示IP协议版本、套接字类型、协议类型
	// 此处使用IPv4、流式套接字、TCP协议
	SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serfd == INVALID_SOCKET)
	{
		printf("socket failed:%d\n", WSAGetLastError());
		return;
	}

	// 给socket绑定IP地址和端口号
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET; // 协议版本，必须和创建socket时一致
	serAddr.sin_port = htons(SPORT); // 端口号，htons函数把本地字节序转换成网络字节序
	serAddr.sin_addr.S_un.S_addr = ADDR_ANY; // IP地址，监听本机所有网卡
	if (bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr)) != 0)
	{
		printf("bind failed:%d\n", WSAGetLastError());
		return;
	}

	// 监听客户端连接
	if (listen(serfd, 10) != 0)
	{
		printf("listen failed:%d\n", WSAGetLastError());
		return;
	}
	printf("listen to client...\n");

	// 初始化结构体数组
	int max = sizeof(infos) / sizeof(SockInfo);
	for (int i = 0; i < max; i++)
	{
		memset(&infos[i], 0, sizeof(SockInfo));
		infos[i].fd = INVALID_SOCKET;
	}

	// 有客户端连接时，接受连接
	int len = sizeof(struct sockaddr_in);
	while (1)
	{
		// 在结构体数组中寻找空闲位置
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

		// 创建子线程
		pthread_t tid;
		pthread_create(&tid, NULL, working, pinfo);
		pthread_detach(tid);
	}
}

// 子线程的工作函数
void* working(void* arg)
{
	SockInfo* pinfo = (SockInfo*)arg;

	char ip[33] = { 0 };
	inet_ntop(AF_INET, &pinfo->addr.sin_addr.S_un.S_addr, ip, sizeof(ip));
	printf("new client [%s] connect success...\n", ip);

	char filePath[256] = { 0 }; // 文件完整路径
	int fileSize = 0; // 文件大小

	// 开始处理消息
	while (processMsg(pinfo->fd, filePath, &fileSize))
	{
		//Sleep(100);
	}

	pinfo->fd = INVALID_SOCKET;
}

// 处理消息
bool processMsg(SOCKET clifd, char* filePath, int* fileSize)
{
	char recvBuf[1024] = { 0 }; // 用于接受客户端发送的消息
	int nRes = recv(clifd, recvBuf, 1024, 0);
	if (nRes <= 0)
	{
		printf("客户端断开连接。。。%d\n", WSAGetLastError());
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

// 读取文件，发送文件大小
bool readFile(SOCKET clifd, struct MsgHeader* pMsgHeader, char* filePath, int* fileSize)
{
	// 提取文件名
	char tfname[200] = { 0 }; // 文件名
	char text[100]; // 后缀
	_splitpath(pMsgHeader->fileInfo.fileName, NULL, NULL, tfname, text); // 提取文件名和后缀
	strcat(tfname, text); // 把文件名和后缀拼接在一起

	// 获得相对路径
	sprintf(filePath, "%s%s", "send/", tfname);

	// 打开文件
	FILE* pread = fopen(filePath, "rb");
	if (pread == NULL)
	{
		printf("找不到[%s]文件...\n", filePath);

		struct MsgHeader msg = { .msgID = MSG_OPENFILE_FAILED };
		if (send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
		{
			printf("send failed:%d\n", WSAGetLastError());
		}
		return false;
	}

	// 获取文件大小
	fseek(pread, 0, SEEK_END);
	*fileSize = ftell(pread);
	fseek(pread, 0, SEEK_SET);
	fclose(pread);

	// 把文件大小和文件名发送给客户端
	struct MsgHeader msg = { .msgID = MSG_FILESIZE, .fileInfo.fileSize = *fileSize };
	strcpy(msg.fileInfo.fileName, tfname);
	if (send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
	{
		printf("发送文件大小失败:%d\n", WSAGetLastError());
		return false;
	}

	return true;
}

// 发送文件
bool sendFile(SOCKET clifd, struct MsgHeader* pMsg, char* filePath, int fileSize)
{
	// 打开文件
	FILE* pread = fopen(filePath, "rb");
	if (pread == NULL)
	{
		printf("找不到[%s]文件...\n", filePath);

		struct MsgHeader msg = { .msgID = MSG_OPENFILE_FAILED };
		if (send(clifd, (char*)&msg, sizeof(struct MsgHeader), 0) == SOCKET_ERROR)
		{
			printf("send failed:%d\n", WSAGetLastError());
		}
		return false;
	}
	fseek(pread, 0, SEEK_SET);

	// 读取文件
	char* fileBuf = NULL; // 存储文件内容
	fileBuf = calloc(fileSize + 1, sizeof(char));
	if (fileBuf == NULL)
	{
		printf("内存申请失败\n");
		return false;
	}

	fseek(pread, 0, SEEK_SET);
	fread(fileBuf, sizeof(char), fileSize, pread);
	fileBuf[fileSize] = '\0';

	fclose(pread);

	for (int i = 0; i < fileSize; i += PACKET_SIZE)
	{
		struct MsgHeader msg = { .msgID = MSG_SENDFILE, .packet.nStart = i };
		
		if (i + PACKET_SIZE + 1 > fileSize) // 包的大小大于剩余文件的大小
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
			printf("文件发送失败:%d\n", WSAGetLastError());
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