#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <definitions/messagedataroles.h>
#include <definitions/messagewriterorders.h>
#include <definitions/notificationdataroles.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/inotifications.h>
#include <interfaces/istanzaprocessor.h>

class MessageProcessor :
	public QObject,
	public IPlugin,
	public IMessageProcessor,
	public IMessageWriter,
	public IStanzaHandler
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IMessageProcessor IMessageWriter IStanzaHandler)
public:
	MessageProcessor();
	~MessageProcessor();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return MESSAGEPROCESSOR_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IMessageWriter
	virtual void writeTextToMessage(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang);
	virtual void writeMessageToText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang);
	//IMessageProcessor
	virtual bool sendMessage(const Jid &AStreamJid, Message &AMessage, int ADirection);
	virtual bool processMessage(const Jid &AStreamJid, Message &AMessage, int ADirection);
	virtual bool displayMessage(const Jid &AStreamJid, Message &AMessage, int ADirection);
	virtual int notifyByMessage(int AMessageId) const;
	virtual int messageByNotify(int ANotifyId) const;
	virtual void showNotifiedMessage(int AMessageId);
	virtual void removeMessageNotify(int AMessageId);
	virtual void textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang = QString::null) const;
	virtual void messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang = QString::null) const;
	virtual bool createMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode) const;
	virtual void insertMessageHandler(int AOrder, IMessageHandler *AHandler);
	virtual void removeMessageHandler(int AOrder, IMessageHandler *AHandler);
	virtual void insertMessageWriter(int AOrder, IMessageWriter *AWriter);
	virtual void removeMessageWriter(int AOrder, IMessageWriter *AWriter);
	virtual void insertMessageEditor(int AOrder, IMessageEditor *AEditor);
	virtual void removeMessageEditor(int AOrder, IMessageEditor *AEditor);
signals:
	void messageSent(const Message &AMessage);
	void messageReceived(const Message &AMessage);
	void messageNotifyInserted(int AMessageId);
	void messageNotifyRemoved(int AMessageid);
	void messageHandlerInserted(int AOrder, IMessageHandler *AHandler);
	void messageHandlerRemoved(int AOrder, IMessageHandler *AHandler);
	void messageWriterInserted(int AOrder, IMessageWriter *AWriter);
	void messageWriterRemoved(int AOrder, IMessageWriter *AWriter);
	void messageEditorInserted(int AOrder, IMessageEditor *AEditor);
	void messageEditorRemoved(int AOrder, IMessageEditor *AEditor);
protected:
	int newMessageId();
	IMessageHandler *findMessageHandler(const Message &AMessage, int ADirection);
	void notifyMessage(IMessageHandler *AHandler, const Message &AMessage, int ADirection);
	QString prepareBodyForSend(const QString &AString) const;
	QString prepareBodyForReceive(const QString &AString) const;
protected slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onStreamRemoved(IXmppStream *AXmppStream);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
private:
	IXmppStreams *FXmppStreams;
	INotifications *FNotifications;
	IStanzaProcessor *FStanzaProcessor;
private:
	QMap<Jid, int> FSHIMessages;
	QMap<int, int> FNotifyId2MessageId;
	QMap<int, IMessageHandler *> FHandlerForMessage;
	QMultiMap<int, IMessageHandler *> FMessageHandlers;
	QMultiMap<int, IMessageWriter *> FMessageWriters;
	QMultiMap<int, IMessageEditor *> FMessageEditors;
};

#endif // MESSAGEPROCESSOR_H
