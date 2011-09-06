#ifndef VIDEOTRANSLATOR_H
#define VIDEOTRANSLATOR_H

#include <QObject>
#include <QHostAddress>
#include <QMutex>
#include "RCameraFlow.h"
#include "netdatatranslator.h"
//#include "rtpdatasender.h"

class QUdpSocket;
class H264Container;


class EncodedFrame
{
public:
  EncodedFrame(qint32 timestamp);
  ~EncodedFrame();

public:
  bool insertSubframe(const QByteArray& subFrame, qint16 seqNum, qint32 timeStamp);

  inline quint32 frameTS() const { return _frameTimeStamp;}

  QByteArray frame();


private:
  QByteArray _netBuffer2;
  quint32 _frameTimeStamp;
  QList<QByteArray*> _frameHash;
};





//////////////////////////////////////////////////////////////////////////
class VideoTranslator : public QObject
{
  Q_OBJECT

public:
  VideoTranslator(int width, int height, QObject *parent);
  VideoTranslator(int width, int height, const QHostAddress& remoteHost, int remoteVideoPort, int localVideoPort, QObject *parent);
  ~VideoTranslator();

	bool cameraPresent() const;

signals:
  // ���������� ���������� ��������
  void updatePicture(const QImage& img);
  void updatePicture(const QPixmap& pix);

  // ���������� �������� ���������� � ������
  void updateLocalPicture(const QImage& img);
  void updateLocalPicture(const QPixmap& pix);

  void updateSysInfo(const QString&);

protected:
  void timerEvent(QTimerEvent*);

  // �������� ����������� �� ����
  void sendImage(QImage img);
	void sendImage1(QImage img);


public slots:
	bool stopCamera();
	bool startCamera();
	void setFrameSize( int width, int height );

protected slots:
  //void hostFound();
  void dataArrived();
	void dataArrived1();
  void onSocketError(QAbstractSocket::SocketError);
  void sendImage();
  void updText();
  void procceedDataProcessing( const QByteArray& rtpPacket );
	void procceedDataProcessingDebug( const QByteArray& rtpPacket );
	void processPortionOfData();
	
private:
	void pingOther();
	


private:
  // ����� � ������. ��������� �������� � ������.
  RCameraFlow* _cameraFlow;
	// ������ �������������� ��������.
	// � ������ ����� �������� ��������������������� ������� � ����� ������������
	// ������������ �� �� ������� _frameSize
	QSize _frameSize;

	uchar* _rxRgbBuffer;

  // ������ ��� �������� �� ���� � ��� ������
  QUdpSocket *_udpSocketForSend;
  QHostAddress _remoteHost;
  int _remPort;

	NetDataTranslator* _netTransThread;

  //RtpDataSender* _dataSender;

  QUdpSocket *_udpSocketForRead;

  // �������� �����������
  QImage _incImage;

  // ����� ��� �������� ������ �� ����
  //uchar* _netBuffer;
  QByteArray _netBuffer2;
  QString _testStr;
  QString _testStrIn;

  int _ssrc; // �������� ������������� ��� �����
  quint32 _startTime;
  quint16 _senderSeq;
 
  // �������������� ��������: ����������� �������� / ������� �������� �� ����
  int _showTimer;
  int _sendTimer;
	int _pingTimer;

  // ������� �������������� �������� �� (�����/���.)
  int _showTimerFreq;
  int _sendTimerFreq;

  // ������ ������ ������������ ������������/�������������� ������� H.264
  H264Container *_pH264;

	QMutex mutex;
	bool __suspended;
	// ������� "������" ������ � ������

};

#endif // VIDEOTRANSLATOR_H
