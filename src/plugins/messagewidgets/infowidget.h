#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <interfaces/imessagewidgets.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/iavatars.h>
#include "ui_infowidget.h"

class InfoWidget :
	public QWidget,
	public IInfoWidget
{
	Q_OBJECT;
	Q_INTERFACES(IInfoWidget);
public:
	InfoWidget(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid);
	~InfoWidget();
	virtual QWidget *instance() { return this; }
	virtual const Jid &streamJid() const;
	virtual void setStreamJid(const Jid &AStreamJid);
	virtual const Jid &contactJid() const;
	virtual void setContactJid(const Jid &AContactJid);
	virtual void autoUpdateFields();
	virtual void autoUpdateField(InfoField AField);
	virtual QVariant field(InfoField AField) const;
	virtual void setField(InfoField AField, const QVariant &AValue);
	virtual int autoUpdatedFields() const;
	virtual bool isFiledAutoUpdated(IInfoWidget::InfoField AField) const;
	virtual void setFieldAutoUpdated(IInfoWidget::InfoField AField, bool AAuto);
	virtual int visibleFields() const;
	virtual bool isFieldVisible(IInfoWidget::InfoField AField) const;
	virtual void setFieldVisible(IInfoWidget::InfoField AField, bool AVisible);
signals:
	void streamJidChanged(const Jid &ABefour);
	void contactJidChanged(const Jid &ABefour);
	void fieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue);
protected:
	void initialize();
	void updateFieldLabel(IInfoWidget::InfoField AField);
protected slots:
	void onAccountChanged(const OptionsNode &ANode);
	void onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore);
	void onPresenceItemReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onAvatarChanged(const Jid &AContactJid);
private:
	Ui::InfoWidgetClass ui;
private:
	IAccount *FAccount;
	IRoster *FRoster;
	IPresence *FPresence;
	IAvatars *FAvatars;
	IMessageWidgets *FMessageWidgets;
private:
	int FAutoFields;
	int FVisibleFields;
	Jid FStreamJid;
	Jid FContactJid;
	QMap<int, QVariant> FFieldValues;
};

#endif // INFOWIDGET_H
