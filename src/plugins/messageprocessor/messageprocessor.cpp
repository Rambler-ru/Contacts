#include "messageprocessor.h"

#include <QVariant>
#include <QTextCursor>
#include <utils/log.h>

#define SHC_MESSAGE         "/message"
#define MAIL_NODE_PATTERN   "[a-zA-Z0-9_\\-\\.]+"

MessageProcessor::MessageProcessor()
{
	FXmppStreams = NULL;
	FStanzaProcessor = NULL;
	FNotifications = NULL;
}

MessageProcessor::~MessageProcessor()
{

}

void MessageProcessor::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Message Manager");
	APluginInfo->description = tr("Allows other modules to send and receive messages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MessageProcessor::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(jidAboutToBeChanged(IXmppStream *, const Jid &)),
				SLOT(onStreamJidAboutToBeChanged(IXmppStream *, const Jid &)));
			connect(FXmppStreams->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),
				SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	return FStanzaProcessor!=NULL && FXmppStreams!=NULL;
}

bool MessageProcessor::initObjects()
{
	insertMessageWriter(this,MWO_MESSAGEPROCESSOR);
	insertMessageWriter(this,MWO_MESSAGEPROCESSOR_ANCHORS);
	return true;
}

bool MessageProcessor::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIMessages.value(AStreamJid) == AHandlerId)
	{
		Message message(AStanza);
		AAccept = receiveMessage(message)>0 || AAccept;
	}
	return false;
}

void MessageProcessor::writeMessage(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang)
{
	if (AOrder == MWO_MESSAGEPROCESSOR)
	{
		AMessage.setBody(prepareBodyForSend(ADocument->toPlainText()),ALang);
	}
}

void MessageProcessor::writeText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang)
{
	if (AOrder == MWO_MESSAGEPROCESSOR)
	{
		QTextCursor cursor(ADocument);
		cursor.insertHtml(prepareBodyForReceive(AMessage.body(ALang)));
	}
	else if (AOrder == MWO_MESSAGEPROCESSOR_ANCHORS)
	{
		QRegExp href("\\b((https?|ftp)://|mailto:|www.|xmpp:)\\S+");
		href.setCaseSensitivity(Qt::CaseInsensitive);
		for (QTextCursor cursor = ADocument->find(href); !cursor.isNull();  cursor = ADocument->find(href,cursor))
		{
			if (!cursor.charFormat().isAnchor())
			{
				QTextCharFormat linkFormat = cursor.charFormat();
				linkFormat.setAnchor(true);
				QUrl url = cursor.selectedText();
				if (url.scheme().isEmpty())
					linkFormat.setAnchorHref("http://"+url.toString());
				else
					linkFormat.setAnchorHref(url.toString());
				cursor.setCharFormat(linkFormat);
			}
		}

		QRegExp mail("\\b"MAIL_NODE_PATTERN"@"JID_DOMAIN_PATTERN);
		mail.setCaseSensitivity(Qt::CaseInsensitive);
		for (QTextCursor cursor = ADocument->find(mail); !cursor.isNull();  cursor = ADocument->find(mail,cursor))
		{
			if (!cursor.charFormat().isAnchor())
			{
				QTextCharFormat linkFormat = cursor.charFormat();
				linkFormat.setAnchor(true);
				linkFormat.setAnchorHref("mailto:"+cursor.selectedText());
				cursor.setCharFormat(linkFormat);
			}
		}
	}
}

int MessageProcessor::receiveMessage(const Message &AMessage)
{
	int messageId = -1;
	IMessageHandler *handler = getMessageHandler(AMessage);
	if (handler)
	{
		Message message = AMessage;
		messageId = newMessageId();
		message.setData(MDR_MESSAGE_ID,messageId);
		FMessages.insert(messageId,message);
		FHandlerForMessage.insert(messageId,handler);

		emit messageReceive(message);
		if (handler->receiveMessage(messageId))
		{
			notifyMessage(messageId);
			emit messageReceived(message);
		}
		else
		{
			emit messageReceived(message);
			removeMessage(messageId);
		}
	}
	return messageId;
}

bool MessageProcessor::sendMessage(const Jid &AStreamJid, const Message &AMessage)
{
	Message message = AMessage;
	message.setFrom(AStreamJid.eFull());

	emit messageSend(message);
	if (FStanzaProcessor && FStanzaProcessor->sendStanzaOut(AStreamJid,message.stanza()))
	{
		emit messageSent(message);
		return true;
	}
	LogError(QString("[MessageProcessor send message error] Failed to send message with stanza:\n%1").arg(message.stanza().toString()));
	return false;
}

void MessageProcessor::showMessage(int AMessageId)
{
	IMessageHandler *handler = FHandlerForMessage.value(AMessageId,NULL);
	if (handler)
		handler->showMessage(AMessageId);
}

void MessageProcessor::removeMessage(int AMessageId)
{
	if (FMessages.contains(AMessageId))
	{
		unNotifyMessage(AMessageId);
		FHandlerForMessage.remove(AMessageId);
		Message message = FMessages.take(AMessageId);
		emit messageRemoved(message);
	}
}

int MessageProcessor::notifyByMessage(int AMessageId) const
{
	return FNotifyId2MessageId.key(AMessageId,-1);
}

int MessageProcessor::messageByNotify(int ANotifyId) const
{
	return FNotifyId2MessageId.value(ANotifyId,-1);
}

Message MessageProcessor::messageById(int AMessageId) const
{
	return FMessages.value(AMessageId);
}

QList<int> MessageProcessor::messages(const Jid &AStreamJid, const Jid &AFromJid, int AMesTypes)
{
	QList<int> mIds;
	for (QMap<int,Message>::const_iterator it = FMessages.constBegin(); it != FMessages.constEnd(); it++)
	{
		if (AStreamJid == it.value().to() &&
		    (!AFromJid.isValid() || AFromJid==it.value().from()) &&
		    (AMesTypes==Message::AnyType || (AMesTypes & it.value().type())>0))
		{
			mIds.append(it.key());
		}
	}
	return mIds;
}

void MessageProcessor::textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang) const
{
	QTextDocument *documentCopy = ADocument->clone();
	QMapIterator<int,IMessageWriter *> it(FMessageWriters);
	it.toBack();
	while (it.hasPrevious())
	{
		it.previous();
		it.value()->writeMessage(it.key(),AMessage,documentCopy,ALang);
	}
	delete documentCopy;
}

void MessageProcessor::messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang) const
{
	Message messageCopy = AMessage;
	QMapIterator<int,IMessageWriter *> it(FMessageWriters);
	it.toFront();
	while (it.hasNext())
	{
		it.next();
		it.value()->writeText(it.key(), messageCopy,ADocument,ALang);
	}
}

bool MessageProcessor::createMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode) const
{
	for (QMultiMap<int, IMessageHandler *>::const_iterator it = FMessageHandlers.constBegin(); it!=FMessageHandlers.constEnd(); it++)
		if (it.value()->createMessageWindow(it.key(),AStreamJid,AContactJid,AType,AShowMode))
			return true;
	return false;
}

void MessageProcessor::insertMessageHandler(IMessageHandler *AHandler, int AOrder)
{
	if (!FMessageHandlers.values(AOrder).contains(AHandler))
	{
		FMessageHandlers.insert(AOrder,AHandler);
		emit messageHandlerInserted(AHandler,AOrder);
	}
}

void MessageProcessor::removeMessageHandler(IMessageHandler *AHandler, int AOrder)
{
	if (FMessageHandlers.values(AOrder).contains(AHandler))
	{
		FMessageHandlers.remove(AOrder,AHandler);
		emit messageHandlerRemoved(AHandler,AOrder);
	}
}

void MessageProcessor::insertMessageWriter(IMessageWriter *AWriter, int AOrder)
{
	if (!FMessageWriters.values(AOrder).contains(AWriter))
	{
		FMessageWriters.insert(AOrder,AWriter);
		emit messageWriterInserted(AWriter,AOrder);
	}
}

void MessageProcessor::removeMessageWriter(IMessageWriter *AWriter, int AOrder)
{
	if (FMessageWriters.values(AOrder).contains(AWriter))
	{
		FMessageWriters.remove(AOrder,AWriter);
		emit messageWriterRemoved(AWriter,AOrder);
	}
}

int MessageProcessor::newMessageId()
{
	static int messageId = 1;
	return messageId++;
}

IMessageHandler *MessageProcessor::getMessageHandler(const Message &AMessage)
{
	for (QMultiMap<int, IMessageHandler *>::const_iterator it = FMessageHandlers.constBegin(); it!=FMessageHandlers.constEnd(); it++)
		if (it.value()->checkMessage(it.key(),AMessage))
			return it.value();
	return NULL;
}

void MessageProcessor::notifyMessage(int AMessageId)
{
	if (FMessages.contains(AMessageId))
	{
		int notifyId = -1;
		if (FNotifications)
		{
			const Message &message = FMessages.value(AMessageId);
			IMessageHandler *handler = FHandlerForMessage.value(AMessageId);
			INotification notify = handler->notifyMessage(FNotifications, message);
			if (notify.kinds > 0)
			{
				notifyId = FNotifications->appendNotification(notify);
				FNotifyId2MessageId.insert(notifyId,AMessageId);
			}
		}
		emit messageNotified(AMessageId, notifyId);
	}
}

void MessageProcessor::unNotifyMessage(int AMessageId)
{
	if (FMessages.contains(AMessageId))
	{
		int notifyId = -1;
		if (FNotifications)
		{
			notifyId = FNotifyId2MessageId.key(AMessageId);
			FNotifications->removeNotification(notifyId);
			FNotifyId2MessageId.remove(notifyId);
		}
		emit messageUnNotified(AMessageId, notifyId);
	}
}

void MessageProcessor::removeStreamMessages(const Jid &AStreamJid)
{
	foreach (int messageId, messages(AStreamJid))
		removeMessage(messageId);
}

QString MessageProcessor::prepareBodyForSend(const QString &AString) const
{
	QString result = AString;
	result.remove(QChar::ObjectReplacementCharacter);
	return result;
}

QString MessageProcessor::prepareBodyForReceive(const QString &AString) const
{
	QString result = Qt::escape(AString);
	result.replace('\n',"<br>");
	result.replace("  " ,"&nbsp; ");
	result.replace('\t',"&nbsp; &nbsp; ");
	return result;
}

void MessageProcessor::onStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = AXmppStream->streamJid();
		shandle.conditions.append(SHC_MESSAGE);
		FSHIMessages.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}
}

void MessageProcessor::onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter)
{
	if (!(AAfter && AXmppStream->streamJid()))
		removeStreamMessages(AXmppStream->streamJid());
}

void MessageProcessor::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
	QMap<int,Message>::iterator it = FMessages.begin();
	while (it != FMessages.end())
	{
		if (ABefour == it.value().to())
			it.value().setTo(AXmppStream->streamJid().eFull());
		it++;
	}
}

void MessageProcessor::onStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIMessages.take(AXmppStream->streamJid()));
	}
}

void MessageProcessor::onStreamRemoved(IXmppStream *AXmppStream)
{
	removeStreamMessages(AXmppStream->streamJid());
}

void MessageProcessor::onNotificationActivated(int ANotifyId)
{
	if (FNotifyId2MessageId.contains(ANotifyId))
		showMessage(FNotifyId2MessageId.value(ANotifyId));
}

void MessageProcessor::onNotificationRemoved(int ANotifyId)
{
	if (FNotifyId2MessageId.contains(ANotifyId))
		removeMessage(FNotifyId2MessageId.value(ANotifyId));
}

Q_EXPORT_PLUGIN2(plg_messageprocessor, MessageProcessor)
