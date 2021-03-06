#include "privatestorage.h"

#include <QCryptographicHash>
#include <utils/log.h>

#define PRIVATE_STORAGE_TIMEOUT       30000

PrivateStorage::PrivateStorage()
{
	FStanzaProcessor = NULL;
}

PrivateStorage::~PrivateStorage()
{

}

//IPlugin
void PrivateStorage::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Private Storage");
	APluginInfo->description = tr("Allows other modules to store arbitrary data on a server");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool PrivateStorage::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		connect(plugin->instance(), SIGNAL(opened(IXmppStream *)), SLOT(onStreamOpened(IXmppStream *)));
		connect(plugin->instance(), SIGNAL(aboutToClose(IXmppStream *)), SLOT(onStreamAboutToClose(IXmppStream *)));
		connect(plugin->instance(), SIGNAL(closed(IXmppStream *)), SLOT(onStreamClosed(IXmppStream *)));
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	return FStanzaProcessor!=NULL;
}

void PrivateStorage::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (AStanza.type() == "result")
	{
		if (FSaveRequests.contains(AStanza.id()))
		{
			QDomElement elem = FSaveRequests.take(AStanza.id());
			emit dataSaved(AStanza.id(),AStreamJid,elem);
		}
		else if (FLoadRequests.contains(AStanza.id()))
		{
			FLoadRequests.remove(AStanza.id());
			QDomElement dataElem = AStanza.firstElement("query",NS_JABBER_PRIVATE).firstChildElement();
			emit dataLoaded(AStanza.id(),AStreamJid,insertElement(AStreamJid,dataElem));
		}
		else if (FRemoveRequests.contains(AStanza.id()))
		{
			QDomElement dataElem = FRemoveRequests.take(AStanza.id());
			emit dataRemoved(AStanza.id(),AStreamJid,dataElem);
			removeElement(AStreamJid,dataElem.tagName(),dataElem.namespaceURI());
		}
	}
	else
	{
		ErrorHandler err(AStanza.element());
		if (FSaveRequests.contains(AStanza.id()))
		{
			QDomElement elem = FSaveRequests.take(AStanza.id());
			LogError(QString("[PrivateStorage] Failed to save private data '%1': %2").arg(elem.namespaceURI(), err.message()));
		}
		else if (FLoadRequests.contains(AStanza.id()))
		{
			QDomElement elem = FLoadRequests.take(AStanza.id());
			LogError(QString("[PrivateStorage] Failed to load private data '%1': %2").arg(elem.namespaceURI(), err.message()));
		}
		else if (FRemoveRequests.contains(AStanza.id()))
		{
			QDomElement elem = FRemoveRequests.take(AStanza.id());
			LogError(QString("[PrivateStorage] Failed to remove private data '%1': %2").arg(elem.namespaceURI(), err.message()));
		}
		emit dataError(AStanza.id(),err.message());
	}
}

void PrivateStorage::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	Q_UNUSED(AStreamJid);
	ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
	if (FSaveRequests.contains(AStanzaId))
	{
		QDomElement elem = FSaveRequests.take(AStanzaId);
		LogError(QString("[PrivateStorage] Failed to save private data '%1': %2").arg(elem.namespaceURI(), err.message()));
	}
	else if (FLoadRequests.contains(AStanzaId))
	{
		QDomElement elem = FLoadRequests.take(AStanzaId);
		LogError(QString("[PrivateStorage] Failed to load private data '%1': %2").arg(elem.namespaceURI(), err.message()));
	}
	else if (FRemoveRequests.contains(AStanzaId))
	{
		QDomElement elem = FRemoveRequests.take(AStanzaId);
		LogError(QString("[PrivateStorage] Failed to remove private data '%1': %2").arg(elem.namespaceURI(), err.message()));
	}
	emit dataError(AStanzaId,err.message());
}

bool PrivateStorage::hasData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const
{
	return getData(AStreamJid,ATagName,ANamespace).hasChildNodes();
}

QDomElement PrivateStorage::getData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const
{
	QDomElement elem = FStreamElements.value(AStreamJid).firstChildElement(ATagName);
	while (!elem.isNull() && elem.namespaceURI()!=ANamespace)
		elem= elem.nextSiblingElement(ATagName);
	return elem;
}

QString PrivateStorage::saveData(const Jid &AStreamJid, const QDomElement &AElement)
{
	if (AStreamJid.isValid() && !AElement.namespaceURI().isEmpty())
	{
		Stanza stanza("iq");
		stanza.setType("set").setId(FStanzaProcessor->newId());
		QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
		elem.appendChild(AElement.cloneNode(true));
		if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
		{
			LogDetaile(QString("[PrivateStorage] Private data '%1' save request sent").arg(AElement.namespaceURI()));
			FSaveRequests.insert(stanza.id(),insertElement(AStreamJid,AElement));
			return stanza.id();
		}
		else
		{
			LogError(QString("[PrivateStorage] Failed to send private data '%1' save request").arg(AElement.namespaceURI()));
		}
	}
	return QString::null;
}

QString PrivateStorage::loadData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (AStreamJid.isValid() && !ATagName.isEmpty() && !ANamespace.isEmpty())
	{
		Stanza stanza("iq");
		stanza.setType("get").setId(FStanzaProcessor->newId());
		QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
		QDomElement dataElem = elem.appendChild(stanza.createElement(ATagName,ANamespace)).toElement();
		if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
		{
			LogDetaile(QString("[PrivateStorage] Private data '%1' load request sent").arg(ANamespace));
			FLoadRequests.insert(stanza.id(),dataElem);
			return stanza.id();
		}
		else
		{
			LogError(QString("[PrivateStorage] Failed to send private data '%1' load request").arg(ANamespace));
		}
	}
	return QString::null;
}

QString PrivateStorage::removeData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (AStreamJid.isValid() && !ATagName.isEmpty() && !ANamespace.isEmpty())
	{
		Stanza stanza("iq");
		stanza.setType("set").setId(FStanzaProcessor->newId());
		QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
		elem = elem.appendChild(stanza.createElement(ATagName,ANamespace)).toElement();
		if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
		{
			LogDetaile(QString("[PrivateStorage] Private data '%1' remove request sent").arg(ANamespace));
			QDomElement dataElem = getData(AStreamJid,ATagName,ANamespace);
			if (dataElem.isNull())
				dataElem = insertElement(AStreamJid,elem);
			FRemoveRequests.insert(stanza.id(),dataElem);
			return stanza.id();
		}
		else
		{
			LogError(QString("[PrivateStorage] Failed to send private data '%1' remove request").arg(ANamespace));
		}
	}
	return QString::null;
}

QDomElement PrivateStorage::getStreamElement(const Jid &AStreamJid)
{
	if (!FStreamElements.contains(AStreamJid))
	{
		QDomElement elem = FStorage.appendChild(FStorage.createElement("stream")).toElement();
		FStreamElements.insert(AStreamJid,elem);
	}
	return FStreamElements.value(AStreamJid);
}

void PrivateStorage::removeStreamElement(const Jid &AStreamJid)
{
	FStreamElements.remove(AStreamJid);
}

QDomElement PrivateStorage::insertElement(const Jid &AStreamJid, const QDomElement &AElement)
{
	removeElement(AStreamJid,AElement.tagName(),AElement.namespaceURI());
	QDomElement streamElem = getStreamElement(AStreamJid);
	return streamElem.appendChild(AElement.cloneNode(true)).toElement();
}

void PrivateStorage::removeElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (FStreamElements.contains(AStreamJid))
		FStreamElements[AStreamJid].removeChild(getData(AStreamJid,ATagName,ANamespace));
}

void PrivateStorage::onStreamOpened(IXmppStream *AXmppStream)
{
	emit storageOpened(AXmppStream->streamJid());
}

void PrivateStorage::onStreamAboutToClose(IXmppStream *AXmppStream)
{
	emit storageAboutToClose(AXmppStream->streamJid());
}

void PrivateStorage::onStreamClosed(IXmppStream *AXmppStream)
{
	emit storageClosed(AXmppStream->streamJid());
	removeStreamElement(AXmppStream->streamJid());
}

Q_EXPORT_PLUGIN2(plg_privatestorage, PrivateStorage)
