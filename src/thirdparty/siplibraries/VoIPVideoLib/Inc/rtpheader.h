#ifndef RTPDATAHEADER_H
#define RTPDATAHEADER_H

// For 32bit intel machines

typedef short int16;
typedef int   int32;
typedef unsigned char u_int8;
typedef unsigned short u_int16;
typedef unsigned int u_int32;


/*
���� ������� ��������� ��������� ��������� RTP. ����� 96 bit.
!!! ������ �������� �������� �� ��� ���� ��� � ������ �������� (Big endian/Little endian) �� ������� ������� ���� ���������
���� �������� ���� ������, �� ����������� ����� ����������� �����! ��������� ��� ��������� RTP ������.

������ ���� CSRC-��������������, ���� ��� ����. ���-�� ��������������� ���������� � ��������� cc.
������ CSRC [start + 96].

����� ������� �������������� �������������� ��������� ������������ ����� ����� ������ (AHL). �������� �
��������� ����������� ������� ���������.
��� ������ [start + 96 + cc*32].

� ��������� ����� ���� ������ � ������� [start + 96 + ��*32 + �*(AHL+16)]
??? �� ���� ��������� ����� AHL - 16 ��� ???
*/
typedef struct RtpHeader
{
#ifdef RTP_BIG_ENDIAN
	uint8_t version:2;
	uint8_t padding:1;
	uint8_t extension:1;
	uint8_t csrccount:4;

	uint8_t marker:1;
	uint8_t payloadtype:7;
#else // Little endian
	u_int8 cc:4;              // ���������� CSRC-���������������, ��������� �� ���������� ����������. 4 ����
	u_int8 x:1;               // ���� ������������� ���������� ���������. 1 ���
	u_int8 p:1;               // ���� ���������� ������� ������� �� �����. 1 ���
	u_int8 version:2;         // ������ ��������� (curr=2). 2 ����

	u_int8 pt:7;              // ������ �������� ��������
	u_int8 m:1;               // ������������ �� ������ ���������� � ������������ ��������. ���� ��� ���� �����������, �� ������ ������ ����� �����-�� ������ �������� ��� ����������. 1 ���
#endif

	u_int16 seq;              // ���������� ����� 16 ���
	u_int32 ts;                     // ����� ������� 32bits
	u_int32 ssrc;                   // �������� �������������

} rtp_hdr_t;

struct RtpPacket
{
	RtpHeader pHeader;
	int       iPayload;
	unsigned __int8  *pPayload;
	RtpPacket() : iPayload(0), pPayload(NULL) {}
	~RtpPacket() { if(pPayload) delete pPayload; }
};

#endif