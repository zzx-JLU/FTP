#pragma once
#pragma comment(lib, "ws2_32.lib") // ���ӿ�
#pragma comment(lib, "pthreadVC2.lib")

#include <WinSock2.h>
#include <stdbool.h>
#include <Ws2tcpip.h>

#define SPORT 8888 // �������˿ں�
#define PACKET_SIZE (1024 - sizeof(int) * 3) // ���Ĵ�С

// ������
enum MSGTAG
{
	MSG_FILENAME = 1, // �ļ���
	MSG_FILESIZE = 2, // �ļ���С
	MSG_READY_READ = 3, // ׼������
	MSG_SENDFILE = 4, // ��ʼ����
	MSG_SUCCESSED = 5, // �������
	MSG_OPENFILE_FAILED = 6, // �Ҳ����ļ�
	MSG_MEMORY_ERROR = 7, // �ڴ����ʧ��
	MSG_FILE_ERROR = 8 // �ļ���������
};

#pragma pack(1) // �ṹ��ȡ������

// ��װ��Ϣͷ
struct MsgHeader
{
	enum MSGTAG msgID; // ��ǰ��Ϣ���
	union
	{
		struct
		{
			char fileName[256]; // �ļ���
			int fileSize; // �ļ���С
		} fileInfo;

		struct
		{
			int nStart; // ������ʼ�ֽ�
			int packetSize; // ���ݲ��ֵĴ�С
			char buf[PACKET_SIZE];
		} packet;
	};
};

// ���ݸ����̵߳Ĳ����ṹ��
typedef struct SockInfo
{
	struct sockaddr_in addr;
	SOCKET fd;
} SockInfo;

#pragma pack() // �ָ�Ĭ��

// ��ʼ��socket��
bool initSocket();

// �ر�socket��
bool closeSocket();

// �����ͻ�������
void listenToClient();

// ���̵߳Ĺ�������
void* working(void* arg);

// ������Ϣ
bool processMsg(SOCKET, char*, int*);

// ��ȡ�ļ��������ļ���С
bool readFile(SOCKET, struct MsgHeader*, char*, int*);

// �����ļ�
bool sendFile(SOCKET, struct MsgHeader*, char*, int);