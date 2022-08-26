#pragma once
#pragma comment(lib, "ws2_32.lib") // 链接库

#include <WinSock2.h>
#include <stdbool.h>

#define SPORT 8888 // 服务器端口号
#define PACKET_SIZE (1024 - sizeof(int) * 3) // 包的大小

// 定义标记
enum MSGTAG
{
	MSG_FILENAME = 1, // 文件名
	MSG_FILESIZE = 2, // 文件大小
	MSG_READY_READ = 3, // 准备接收
	MSG_SENDFILE = 4, // 开始发送
	MSG_SUCCESSED = 5, // 传输完成
	MSG_OPENFILE_FAILED = 6, // 文件找不到
	MSG_MEMORY_ERROR = 7, // 内存分配失败
	MSG_FILE_ERROR = 8 // 文件操作错误
};

#pragma pack(1) // 结构体取消对齐

// 封装消息头
struct MsgHeader
{
	enum MSGTAG msgID; // 当前消息标记
	union
	{
		struct
		{
			char fileName[256]; // 文件名
			int fileSize; // 文件大小
		} fileInfo;

		struct
		{
			int nStart; // 包的起始字节
			int packetSize; // 数据部分的大小
			char buf[PACKET_SIZE];
		} packet;
	};
};

#pragma pack() // 恢复默认

// 初始化socket库
bool initSocket();

// 关闭socket库
bool closeSocket();

// 监听客户端连接
void connectToHost();

// 处理消息
bool processMsg(SOCKET);

bool sendFileName(SOCKET);

void readyRead(SOCKET, struct MsgHeader*);

// 接收文件
bool recvFile(SOCKET, struct MsgHeader*);