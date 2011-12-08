#ifndef NOTICEWIDGET_H
#define NOTICEWIDGET_H

#include <QTimer>
#include <QWidget>
#include <QObjectCleanupHandler>
#include <definitions/textflags.h>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/imessagewidgets.h>
#include <utils/actionbutton.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include "ui_noticewidget.h"

class ChatNoticeWidget : 
	public QWidget,
	public IChatNoticeWidget
{
	Q_OBJECT
	Q_INTERFACES(IChatNoticeWidget)
public:
	ChatNoticeWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid);
	~ChatNoticeWidget();
	virtual QWidget *instance() { return this; }
	virtual const Jid &streamJid() const;
	virtual void setStreamJid(const Jid &AStreamJid);
	virtual const Jid &contactJid() const;
	virtual void setContactJid(const Jid &AContactJid);
	virtual int activeNotice() const;
	virtual QList<int> noticeQueue() const;
	virtual IChatNotice noticeById(int ANoticeId) const;
	virtual int insertNotice(const IChatNotice &ANotice);
	virtual void removeNotice(int ANoticeId);
signals:
	void streamJidChanged(const Jid &ABefour);
	void contactJidChanged(const Jid &ABefour);
	void noticeInserted(int ANoticeId);
	void noticeActivated(int ANoticeId);
	void noticeRemoved(int ANoticeId);
protected:
	void updateNotice();
	void updateWidgets(int ANoticeId);
protected:
	void paintEvent(QPaintEvent *AEvent);
protected slots:
	void onUpdateTimerTimeout();
	void onCloseTimerTimeout();
	void onCloseButtonClicked(bool);
	void onMessageLinkActivated(const QString &ALink);
private:
	Ui::NoticeWidgetClass ui;
private:
	IMessageWidgets *FMessageWidgets;
private:
	Jid FStreamJid;
	Jid FContactJid;
	int FActiveNotice;
	QTimer FUpdateTimer;
	QTimer FCloseTimer;
	QMap<int, IChatNotice> FNotices;
	QMultiMap<int, int> FNoticeQueue;
	QObjectCleanupHandler FButtonsCleanup;
};

#endif // NOTICEWIDGET_H
