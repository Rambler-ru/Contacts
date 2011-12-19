#ifndef IMESSAGEPROCESSOR_H
#define IMESSAGEPROCESSOR_H

#include <QTextDocument>
#include <interfaces/irostersmodel.h>
#include <interfaces/inotifications.h>
#include <utils/jid.h>
#include <utils/message.h>

#define MESSAGEPROCESSOR_UUID "{eb4a8aff-ba36-4946-832a-08f7c0d8b770}"

class IMessageHandler
{
public:
	enum ShowMode {
		SM_ASSIGN,
		SM_SHOW,
		SM_MINIMIZED
	};
public:
	virtual bool messageCheck(int AOrder, const Message &AMessage, int ADirection) =0;
	virtual bool messageDisplay(const Message &AMessage, int ADirection) =0;
	virtual INotification messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection) =0;
	virtual bool messageShowWindow(int AMessageId) =0;
	virtual bool messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode) =0;
};

class IMessageWriter
{
public:
	virtual void writeMessageToText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang) =0;
	virtual void writeTextToMessage(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang) =0;
};

class IMessageEditor
{
public:
	virtual bool messageReadWrite(int AOrder, const Jid &AStreamJid, Message &AMessage, int ADirection) =0;
};

class IMessageProcessor
{
public:
	enum MessageDirection {
		MessageIn,
		MessageOut
	};
public:
	virtual QObject *instance() = 0;
	virtual bool sendMessage(const Jid &AStreamJid, Message &AMessage, int ADirection) =0;
	virtual bool processMessage(const Jid &AStreamJid, Message &AMessage, int ADirection) =0;
	virtual bool displayMessage(const Jid &AStreamJid, Message &AMessage, int ADirection) =0;
	virtual int notifyByMessage(int AMessageId) const =0;
	virtual int messageByNotify(int ANotifyId) const =0;
	virtual void showNotifiedMessage(int AMessageId) =0;
	virtual void removeMessageNotify(int AMessageId) =0;
	virtual void textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang = QString::null) const =0;
	virtual void messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang = QString::null) const =0;
	virtual bool createMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode) const =0;
	virtual void insertMessageHandler(int AOrder, IMessageHandler *AHandler) =0;
	virtual void removeMessageHandler(int AOrder, IMessageHandler *AHandler) =0;
	virtual void insertMessageWriter(int AOrder, IMessageWriter *AWriter) =0;
	virtual void removeMessageWriter(int AOrder, IMessageWriter *AWriter) =0;
	virtual void insertMessageEditor(int AOrder, IMessageEditor *AEditor) =0;
	virtual void removeMessageEditor(int AOrder, IMessageEditor *AEditor) =0;
protected:
	virtual void messageSent(const Message &AMessage) =0;
	virtual void messageReceived(const Message &AMessage) =0;
	virtual void messageNotifyInserted(int AMessageId) =0;
	virtual void messageNotifyRemoved(int AMessageid) =0;
	virtual void messageHandlerInserted(int AOrder, IMessageHandler *AHandler) =0;
	virtual void messageHandlerRemoved(int AOrder, IMessageHandler *AHandler) =0;
	virtual void messageWriterInserted(int AOrder, IMessageWriter *AWriter) =0;
	virtual void messageWriterRemoved(int AOrder, IMessageWriter *AWriter) =0;
	virtual void messageEditorInserted(int AOrder, IMessageEditor *AEditor) =0;
	virtual void messageEditorRemoved(int AOrder, IMessageEditor *AEditor) =0;
};

Q_DECLARE_INTERFACE(IMessageHandler,"Virtus.Plugin.IMessageHandler/1.0")
Q_DECLARE_INTERFACE(IMessageWriter,"Virtus.Plugin.IMessageWriter/1.0")
Q_DECLARE_INTERFACE(IMessageEditor,"Virtus.Plugin.IMessageEditor/1.0")
Q_DECLARE_INTERFACE(IMessageProcessor,"Virtus.Plugin.IMessageProcessor/1.0")

#endif // IMESSAGEPROCESSOR_H
