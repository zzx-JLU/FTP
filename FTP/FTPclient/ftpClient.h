#pragma once
#pragma comment(lib, "ws2_32.lib") // ���ӿ�

#include <WinSock2.h>
#include <stdbool.h>

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
	MSG_OPENFILE_FAILED = 6, // �ļ��Ҳ���
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

#pragma pack() // �ָ�Ĭ��

// ��ʼ��socket��
bool initSocket();

// �ر�socket��
bool closeSocket();

// �����ͻ�������
void connectToHost();

// ������Ϣ
bool processMsg(SOCKET);

bool sendFileName(SOCKET);

void readyRead(SOCKET, struct MsgHeader*);

// �����ļ�
bool recvFile(SOCKET, struct MsgHeader*);