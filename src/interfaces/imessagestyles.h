#ifndef IMESSAGESTYLES_H
#define IMESSAGESTYLES_H

#include <QUrl>
#include <QUuid>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QStringList>
#include <QTextDocumentFragment>
#include <interfaces/ioptionsmanager.h>
#include <utils/jid.h>
#include <utils/options.h>

#define MESSAGESTYLES_UUID  "{6329de5c-ff9b-4814-a4c8-855c9127bf13}"

struct IMessageStyleOptions
{
	QString pluginId;
	QMap<QString, QVariant> extended;
};

struct IMessageContentOptions
{
	enum ContentKind {
		Message,
		Status,
		Topic
	};
	enum ContentType {
		Groupchat       = 0x01,
		History         = 0x02,
		Event           = 0x04,
		Mention         = 0x08,
		Notification    = 0x10,
	};
	enum ContentStatus {
		DateSeparator,
		HistoryShow
	};
	enum ContentDirection {
		DirectionIn,
		DirectionOut
	};
	enum ContentAction {
		InsertAfter,
		InsertBefore,
		Replace,
		Remove          
	};
	enum ContentExtension {
		Unread          = 0x01,
		Offline         = 0x02
	};
	IMessageContentOptions() { 
		kind            = Message;
		type            = 0;
		status          = 0;
		direction       = DirectionIn;
		action          = InsertAfter;
		extensions      = 0;
		noScroll        = false;
	}
	int kind;
	int type;
	int status;
	int direction;
	int action;
	int extensions;
	bool noScroll;
	QUuid contentId;
	QDateTime time;
	QString timeFormat;
	QString senderId;
	QString senderName;
	QString senderAvatar;
	QString senderColor;
	QString senderIcon;
	QString textBGColor;
	QString notice;
};

class IMessageStyle
{
public:
	virtual QObject *instance() =0;
	virtual bool isValid() const =0;
	virtual QString styleId() const =0;
	virtual QList<QWidget *> styleWidgets() const =0;
	virtual QWidget *createWidget(const IMessageStyleOptions &AOptions, QWidget *AParent) =0;
	virtual QString senderColor(const QString &ASenderId) const =0;
	virtual QTextDocumentFragment selection(QWidget *AWidget) const =0;
	virtual bool changeOptions(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClean = true) =0;
	virtual QUuid changeContent(QWidget *AWidget, const QString &AHtml, const IMessageContentOptions &AOptions) =0;
protected:
	virtual void widgetAdded(QWidget *AWidget) const =0;
	virtual void widgetRemoved(QWidget *AWidget) const =0;
	virtual void optionsChanged(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClean) const =0;
	virtual void contentChanged(QWidget *AWidget, const QUuid &AContentId, const QString &AHtml, const IMessageContentOptions &AOptions) const =0;
	virtual void urlClicked(QWidget *AWidget, const QUrl &AUrl) const =0;
};

class IMessageStylePlugin
{
public:
	virtual QObject *instance() = 0;
	virtual QString pluginId() const =0;
	virtual QString pluginName() const =0;
	virtual QList<QString> styles() const =0;
	virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions) =0;
	virtual IMessageStyleOptions styleOptions(const OptionsNode &ANode, int AMessageType) const =0;
	virtual IOptionsWidget *styleSettingsWidget(const OptionsNode &ANode, int AMessageType, QWidget *AParent) =0;
	virtual void saveStyleSettings(IOptionsWidget *AWidget, OptionsNode ANode = OptionsNode::null) =0;
	virtual void saveStyleSettings(IOptionsWidget *AWidget, IMessageStyleOptions &AOptions) = 0;
protected:
	virtual void styleCreated(IMessageStyle *AStyle) const =0;
	virtual void styleDestroyed(IMessageStyle *AStyle) const =0;
	virtual void styleWidgetAdded(IMessageStyle *AStyle, QWidget *AWidget) const =0;
	virtual void styleWidgetRemoved(IMessageStyle *AStyle, QWidget *AWidget) const =0;
};

class IMessageStyles
{
public:
	virtual QObject *instance() = 0;
	virtual QList<QString> pluginList() const =0;
	virtual IMessageStylePlugin *pluginById(const QString &APluginId) const =0;
	virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions) const =0;
	virtual IMessageStyleOptions styleOptions(const OptionsNode &ANode, int AMessageType) const =0;
	virtual IMessageStyleOptions styleOptions(int AMessageType, const QString &AContext = QString::null) const =0;
	virtual QString contactAvatar(const Jid &AContactJid) const =0;
	virtual QString contactName(const Jid &AStreamJid, const Jid &AContactJid = Jid()) const =0;
	virtual QString contactIcon(const Jid &AStreamJid, const Jid &AContactJid = Jid()) const =0;
	virtual QString contactIcon(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const =0;
	virtual QString timeFormat(const QDateTime &AMessageTime, const QDateTime &ACurTime = QDateTime::currentDateTime()) const =0;
protected:
	virtual void styleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext) const =0;
};

Q_DECLARE_INTERFACE(IMessageStyle,"Virtus.Plugin.IMessageStyle/1.0")
Q_DECLARE_INTERFACE(IMessageStylePlugin,"Virtus.Plugin.IMessageStylePlugin/1.0")
Q_DECLARE_INTERFACE(IMessageStyles,"Virtus.Plugin.IMessageStyles/1.0")

#endif
