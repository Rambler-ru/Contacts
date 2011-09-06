#ifndef AVCONTROL_H
#define AVCONTROL_H

#include <QWidget>
#include "ui_avcontrol.h"

#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <utils/stylestorage.h>


class BtnSynchro : public QObject
{
	Q_OBJECT
	int _refCount;
	~BtnSynchro(){}

	QList<QAbstractButton*> _buttons;

public:
	BtnSynchro(QAbstractButton* btn)  : _refCount(1)
	{
		_buttons.append(btn);
	}

	int AddRef(QAbstractButton* btn)
	{
		if(!_buttons.contains(btn)) // �� ������� ��� ����� ����� ���������, �� ��� �� �����
			_buttons.append(btn);
		return _refCount++;
	}
	int Release(QAbstractButton* btn)
	{
		_refCount--;
		_buttons.removeOne(btn);
		if (_refCount == 0)
    {
			delete this;
      return 0;
    }
		return _refCount;
	}

signals:
	void stateChange(bool);

public slots:
	void onStateChange(bool);
};

class AVControl : public QWidget
{
	Q_OBJECT

	static BtnSynchro* __bSyncCamera;
	static BtnSynchro* __bSyncMic;
	static BtnSynchro* __bSyncHQ;

public:
	AVControl(QWidget *parent = 0);
	~AVControl();

	void setDark(bool isDark);

public slots:
	void SetCameraOn(bool);
	void SetResolutionHigh(bool isHigh);
	void setCameraEnabled(bool);
	void setMicEnabled( bool );
	void setVolumeEnabled( bool );

signals:
	void camStateChange(bool);
	void camPresentChanged(bool);
	void camResolutionChange(bool);
	void micStateChange(bool);
	void micVolumeChange(int);

	void micPresentChanged(bool);
	void volumePresentChanged(bool);

protected:
	void paintEvent(QPaintEvent *);
	
private slots:
	void onAudioSettings();

private:
	Ui::AVControl ui;
	bool _isDark;
};

#endif // AVCONTROL_H
