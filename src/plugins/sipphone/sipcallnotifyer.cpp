#include "sipcallnotifyer.h"
#include "ui_sipcallnotifyer.h"

#include <QPropertyAnimation>
#include <QDesktopWidget>
#include <QPainter>
#include <QPaintEvent>

#include <utils/stylestorage.h>
#include <utils/iconstorage.h>
#include <utils/customborderstorage.h>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <definitions/menuicons.h>
#include <definitions/customborder.h>
#include <definitions/soundfiles.h>

SipCallNotifyer::SipCallNotifyer(const QString & caption, const QString & notice, const QIcon & icon, const QImage & avatar) :
	QWidget(NULL),
	ui(new Ui::SipCallNotifyer),
#ifdef QT_PHONON_LIB
	FMediaObject(NULL),
	FAudioOutput(NULL)
#else
	FSound(NULL)
#endif

{
	ui->setupUi(this);
	ui->caption->setText(caption);
	setWindowTitle(tr("%1 calling").arg(caption));
	ui->notice->setText(notice);
	ui->icon->setPixmap(icon.pixmap(icon.availableSizes().at(0)));
	ui->avatar->setPixmap(QPixmap::fromImage(avatar));

	ui->accept->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CALL_ANSWER));
	ui->reject->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_CALL_HANGUP));
	ui->mute->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SIPPHONE_MUTE));

	connect(ui->accept, SIGNAL(clicked()), SLOT(acceptClicked()));
	connect(ui->reject, SIGNAL(clicked()), SLOT(rejectClicked()));
	connect(ui->mute, SIGNAL(clicked()), SLOT(muteClicked()));

	_muted = false;

	// style
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this, STS_SIPPHONE_CALL_NOTIFYER);

	// border
	border = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_NOTIFICATION);
	if (border)
	{
		border->setResizable(false);
		border->setMovable(false);
		border->setMaximizeButtonVisible(false);
		border->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(border, SIGNAL(closeClicked()), SLOT(rejectClicked()));
	}
}

SipCallNotifyer::~SipCallNotifyer()
{
	muteSound();
#ifdef QT_PHONON_LIB
	delete FMediaObject;
	delete FAudioOutput;
#else
	delete FSound;
#endif
	delete ui;
}

bool SipCallNotifyer::isMuted() const
{
	return _muted;
}

double SipCallNotifyer::opacity() const
{
	return (border ? (QWidget *)border : (QWidget *)this)->windowOpacity();
}

void SipCallNotifyer::setOpacity(double op)
{
	(border ? (QWidget *)border : (QWidget *)this)->setWindowOpacity(op);
}

void SipCallNotifyer::appear()
{
	QPropertyAnimation * animation = new QPropertyAnimation(this, "opacity");
	animation->setDuration(1000); // 1 sec
	animation->setEasingCurve(QEasingCurve(QEasingCurve::InQuad));
	animation->setStartValue(0.0);
	animation->setEndValue(1.0);

	QWidget * w = (border ? (QWidget *)border : (QWidget *)this);
	if (!w->isVisible())
	{
		w->setWindowOpacity(0.0);
		w->setWindowFlags(w->windowFlags() | Qt::WindowStaysOnTopHint);
		w->adjustSize();
		QRect wrect = w->geometry();
		QDesktopWidget * dw = QApplication::desktop();
		QRect screen = dw->screenGeometry(dw->primaryScreen());
		QRect centered = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, wrect.size(), screen);
		w->setGeometry(centered);
		w->show();
	}

	animation->start(QAbstractAnimation::DeleteWhenStopped);
	startSound();
}

void SipCallNotifyer::disappear()
{
	(border ? (QWidget *)border : (QWidget *)this)->close();
}

void SipCallNotifyer::startSound()
{
	if (soundFile.isEmpty())
		soundFile = FileStorage::staticStorage(RSR_STORAGE_SOUNDS)->fileFullName(SDF_SIPPHONE_CALL_RINGING);
#ifdef QT_PHONON_LIB
	if (!FMediaObject)
	{
		FMediaObject = new Phonon::MediaObject(this);
		FMediaObject->setCurrentSource(soundFile);
		FAudioOutput = new Phonon::AudioOutput(Phonon::NoCategory, this);
		Phonon::createPath(FMediaObject, FAudioOutput);
		connect(FMediaObject, SIGNAL(aboutToFinish()), SLOT(loopPlay())); // looping
	}
	if (FMediaObject->state() != Phonon::PlayingState)
	{
		FMediaObject->play();
	}
#else
	if (QSound::isAvailable())
	{
		if (!FSound || (FSound && FSound->isFinished()))
		{
			delete FSound;
			FSound = new QSound(soundFile);
			FSound->setLoops(-1);
			FSound->play();
		}
	}
# ifdef Q_WS_X11
	else
	{
		QProcess::startDetached(Options::node(OPV_NOTIFICATIONS_SOUND_COMMAND).value().toString(),QStringList()<<soundFile);
	}
# endif
#endif
}

void SipCallNotifyer::muteSound()
{
#ifdef QT_PHONON_LIB
	if (FMediaObject)
	{
		FMediaObject->pause();
	}
#else
	if (FSound)
	{
		FSound->stop();
	}
#endif
}

void SipCallNotifyer::acceptClicked()
{
	emit accepted();
	disappear();
}

void SipCallNotifyer::rejectClicked()
{
	emit rejected();
	disappear();
}

void SipCallNotifyer::muteClicked()
{
	if (_muted)
		startSound();
	else
		muteSound();
	_muted = !_muted;
	ui->mute->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(_muted ? MNI_SIPPHONE_UNMUTE : MNI_SIPPHONE_MUTE));
}

#ifdef QT_PHONON_LIB
void SipCallNotifyer::loopPlay()
{
	if (FMediaObject)
	{
		FMediaObject->enqueue(Phonon::MediaSource(soundFile));
	}
}
#endif

void SipCallNotifyer::paintEvent(QPaintEvent * pe)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	p.setClipRect(pe->rect());
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
