#ifndef RCAMERAFLOW_H
#define RCAMERAFLOW_H

#include <QObject>
#include <QImage>
#include <QTimerEvent>
//#include <opencv/cv.h>
//#include <opencv/highgui.h>

class videoInput;

class RCameraFlow : public QObject
{
  Q_OBJECT

public:
  RCameraFlow(QObject *parent);
  ~RCameraFlow();

public:
  QImage currentFrame() const;
  bool isOk() const;
	bool cameraPresent() const;
	QSize frameSize() const;

	// ������� ���������������� ������ � �������� �������� �����.
	// posibleSize - � ������ ���� �������� ������ ����� �� ��������������, ������������ ��������� ������ �����
	bool tryThisFrameSize(int width, int height, QSize& posibleSize);

	// ������������� �����������-��������� ������ �������� ��� ������
	QSize setMaxFrameSize();

public slots:
	bool stop();
	bool start();

protected:
  void timerEvent(QTimerEvent*);

private:
  //void toQImage(IplImage *cvimage);
  void toQImage(uchar* pixels, int width, int height);

private:
	int _numDevices;
  int _flowTimer;
  bool _cameraStart;
  //CvCapture *_camera;
  QImage _frame;
	QSize _frameSize;

	uchar *__pixels;

  videoInput* pVInput;
};

#endif // RCAMERAFLOW_H
