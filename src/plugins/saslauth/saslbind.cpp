#include "saslbind.h"
#include <utils/log.h>

SASLBind::SASLBind(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
}

SASLBind::~SASLBind()
{
	FXmppStream->removeXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
	emit featureDestroyed();
}

bool SASLBind::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE && AStanza.id()=="bind")
	{
		FXmppStream->removeXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
		if (AStanza.type() == "result")
		{
			Jid streamJid = AStanza.firstElement().firstChild().toElement().text();
			if (streamJid.isValid())
			{
				deleteLater();
				FXmppStream->setStreamJid(streamJid);
				emit finished(false);
			}
			else
			{
				Log(QString("[SASLBind stanza error] %1").arg(tr("Invalid XMPP stream JID in SASL bind response")));
				emit error(tr("Invalid XMPP stream JID in SASL bind response"));
			}
		}
		else
		{
			ErrorHandler err(AStanza.element());
			Log(QString("[SASLBind stanza error] %1").arg(err.message()));
			emit error(err.message());
		}
		return true;
	}
	return false;
}

bool SASLBind::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

bool SASLBind::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "bind")
	{
		Stanza bind("iq");
		bind.setType("set").setId("bind");
		bind.addElement("bind",NS_FEATURE_BIND);
		if (!FXmppStream->streamJid().resource().isEmpty())
			bind.firstElement("bind",NS_FEATURE_BIND).appendChild(bind.createElement("resource")).appendChild(bind.createTextNode(FXmppStream->streamJid().resource()));
		FXmppStream->insertXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
		FXmppStream->sendStanza(bind);
		return true;
	}
	deleteLater();
	return false;
}
