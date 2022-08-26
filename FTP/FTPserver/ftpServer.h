#pragma once
#pragma comment(lib, "ws2_32.lib") // 链接库
#pragma comment(lib, "pthreadVC2.lib")

#include <WinSock2.h>
#include <stdbool.h>
#include <Ws2tcpip.h>

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
	MSG_OPENFILE_FAILED = 6, // 找不到文件
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

// 传递给子线程的参数结构体
typedef struct SockInfo
{
	struct sockaddr_in addr;
	SOCKET fd;
} SockInfo;

#pragma pack() // 恢复默认

// 初始化socket库
bool initSocket();

// 关闭socket库
bool closeSocket();

// 监听客户端连接
void listenToClient();

// 子线程的工作函数
void* working(void* arg);

// 处理消息
bool processMsg(SOCKET, char*, int*);

// 读取文件，发送文件大小
bool readFile(SOCKET, struct MsgHeader*, char*, int*);

// 发送文件
bool sendFile(SOCKET, struct MsgHeader*, char*, int);