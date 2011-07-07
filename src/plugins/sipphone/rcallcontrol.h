#ifndef RCALLCONTROL_H
#define RCALLCONTROL_H

#include <QWidget>
#ifdef QT_PHONON_LIB
#include <Phonon/Phonon>
#include <Phonon/MediaSource>
#else
#	include <QSound>
#endif
#include "ui_rcallcontrol.h"

#include <definitions/menuicons.h>
#include <definitions/toolbargroups.h>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/soundfiles.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/jid.h>

class RCallControl : public QWidget
{
	Q_OBJECT

public:
	enum CallSide
	{
		Receiver = 0,
		Caller = 1
	};
	enum CallStatus
	{
		Undefined,
		Register,		// �����������
		Accepted,		// ������ ������
		Hangup,			// �������� ������
		Ringing,		// ������
		RingTimeout,// ����� ����� ������� ������ (��� ������)
		CallError		// ���� ������
		//Trying,			// ������� ����������� �����
	};

public:
	RCallControl(CallSide, QWidget *parent = 0);
	RCallControl(QString sid, CallSide, QWidget *parent = 0);
	~RCallControl();

	CallSide side() const {return _callSide;}
	QString getSessionID() const {return _sid;}
	CallStatus status() const {return _callStatus;}

	Jid getStreamJid() const { return _streamId; }
	QString getMetaId() const { return _metaId; }

	void setSessionId(const QString& sid);
	void setStreamJid(const Jid& AStreamJid);
	void setMetaId(const QString& AMetaId);

signals:
	void camStateChange(bool);
	void camResolutionChange(bool);
	void startCamera();
	void stopCamera();
	void micStateChange(bool);
	void micVolumeChange(int);

signals: // ������� ����������� �������
	void acceptCall();		// ������� ������
	void hangupCall();		// ����� ������
	void abortCall();			// ������ ������
	void redialCall();		// ������ �������� ���������� ������� � ������ ���������� ������
	void callbackCall();	// ������ �������� ���������� ������� � ������ ��������� ������ �� ����������� �����
	void closeAndDelete(bool);

public slots:
	void statusTextChange(const QString&);
	void callStatusChange(CallStatus);
	void callSideChange(CallSide);
	void setCameraEnabled(bool);

	void setMicEnabled(bool);
	void setVolumeEnabled(bool);

signals:
	void statusTextChanged(const QString&);
	void callStatusChanged(CallStatus);
	void callSideChanged(CallSide);

protected slots:
	void onAccept();
	void onHangup();
	void onCamStateChange(bool);


protected:
	void closeEvent(QCloseEvent *);
	void paintEvent(QPaintEvent *);
	void playSignal(CallStatus status, int loops);
	void stopSignal();
	void playSignalWait(int loops);
	void playSignalBusy(int loops);
	void playSignalRinging(int loops);
	
private:
	IconStorage* iconStorage;
	QIcon acceptIcon;
	QIcon hangupIcon;

#ifdef QT_PHONON_LIB
	Phonon::MediaObject *FMediaObject;
	Phonon::AudioOutput *FAudioOutput;
#else
	QSound *_pSoundWait;
	QSound *_pSoundBusy;
	QSound *_pSoundRinging;
#endif

	Jid _streamId;
	QString _metaId;
	QString _sid; // ������������� ������ ������� ��������� � ������ ������
	CallSide _callSide;			// �������������� ������� ������ (��������/�����������)
	CallStatus _callStatus;	// ������ ������
	Ui::RCallControl ui;
};

#endif // RCALLCONTROL_H
