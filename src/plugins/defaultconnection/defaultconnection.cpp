#include "defaultconnection.h"

#include <QNetworkProxy>
#include <utils/log.h>

#define START_QUERY_ID        0
#define STOP_QUERY_ID         -1

#define DISCONNECT_TIMEOUT    5000

DefaultConnection::DefaultConnection(IConnectionPlugin *APlugin, QObject *AParent) : QObject(AParent)
{
	FPlugin = APlugin;

	FSrvQueryId = START_QUERY_ID;
	connect(&FDns, SIGNAL(resultsReady(int, const QJDns::Response &)),SLOT(onDnsResultsReady(int, const QJDns::Response &)));
	connect(&FDns, SIGNAL(error(int, QJDns::Error)),SLOT(onDnsError(int, QJDns::Error)));
	connect(&FDns, SIGNAL(shutdownFinished()),SLOT(onDnsShutdownFinished()));

	FSocket.setProtocol(QSsl::AnyProtocol);
	connect(&FSocket, SIGNAL(connected()), SLOT(onSocketConnected()));
	connect(&FSocket, SIGNAL(encrypted()), SLOT(onSocketEncrypted()));
	connect(&FSocket, SIGNAL(readyRead()), SLOT(onSocketReadyRead()));
	connect(&FSocket, SIGNAL(modeChanged(QSslSocket::SslMode)), SIGNAL(modeChanged(QSslSocket::SslMode)));
	connect(&FSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onSocketError(QAbstractSocket::SocketError)));
	connect(&FSocket, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(onSocketSSLErrors(const QList<QSslError> &)));
	connect(&FSocket, SIGNAL(disconnected()), SLOT(onSocketDisconnected()));
}

DefaultConnection::~DefaultConnection()
{
	disconnectFromHost();
	emit connectionDestroyed();
}

bool DefaultConnection::isOpen() const
{
	return FSocket.state() == QAbstractSocket::ConnectedState;
}

bool DefaultConnection::isEncrypted() const
{
	return FSocket.isEncrypted();
}

QString DefaultConnection::errorString() const
{
	return FErrorString;
}

bool DefaultConnection::connectToHost()
{
	if (FSrvQueryId==START_QUERY_ID && FSocket.state()==QAbstractSocket::UnconnectedState)
	{
		emit aboutToConnect();

		FRecords.clear();
		FSSLError = false;
		FErrorString = QString::null;

		QString host = option(IDefaultConnection::COR_HOST).toString();
		quint16 port = option(IDefaultConnection::COR_PORT).toInt();
		QString domain = option(IDefaultConnection::COR_DOMAINE).toString();
		FSSLConnection = option(IDefaultConnection::COR_USE_SSL).toBool();
		FIgnoreSSLErrors = option(IDefaultConnection::COR_IGNORE_SSL_ERRORS).toBool();
		FChangeProxyType = option(IDefaultConnection::COR_CHANGE_PROXY_TYPE).toBool();

		QJDns::Record record;
		record.name = !host.isEmpty() ? host.toLatin1() : domain.toLatin1();
		record.port = port;
		record.priority = 0;
		record.weight = 0;
		FRecords.append(record);

		if (host.isEmpty() && FDns.init(QJDns::Unicast, QHostAddress::Any))
		{
			FDns.setNameServers(QJDns::systemInfo().nameServers);
			FSrvQueryId = FDns.queryStart(QString("_xmpp-client._tcp.%1.").arg(domain).toLatin1(),QJDns::Srv);
		}
		else
			connectToNextHost();

		return true;
	}
	return false;
}

void DefaultConnection::disconnectFromHost()
{
	FRecords.clear();

	if (FSocket.state() != QSslSocket::UnconnectedState)
	{
		if (FSocket.state() == QSslSocket::ConnectedState)
		{
			emit aboutToDisconnect();
			FSocket.flush();
			FSocket.disconnectFromHost();
		}
		else
		{
			FSocket.abort();
			emit disconnected();
		}
	}
	else if (FSrvQueryId != START_QUERY_ID)
	{
		FSrvQueryId = STOP_QUERY_ID;
		FDns.shutdown();
	}

	if (FSocket.state()!=QSslSocket::UnconnectedState && !FSocket.waitForDisconnected(DISCONNECT_TIMEOUT))
	{
		setError(tr("Disconnection timed out"));
		emit disconnected();
	}
}

qint64 DefaultConnection::write(const QByteArray &AData)
{
	return FSocket.write(AData);
}

QByteArray DefaultConnection::read(qint64 ABytes)
{
	return FSocket.read(ABytes);
}

void DefaultConnection::startClientEncryption()
{
	FSocket.startClientEncryption();
}

QSsl::SslProtocol DefaultConnection::protocol() const
{
	return FSocket.protocol();
}

void DefaultConnection::setProtocol(QSsl::SslProtocol AProtocol)
{
	FSocket.setProtocol(AProtocol);
}

void DefaultConnection::addCaCertificate(const QSslCertificate &ACertificate)
{
	FSocket.addCaCertificate(ACertificate);
}

QList<QSslCertificate> DefaultConnection::caCertificates() const
{
	return FSocket.caCertificates();
}

QSslCertificate DefaultConnection::peerCertificate() const
{
	return FSocket.peerCertificate();
}

void DefaultConnection::ignoreSslErrors()
{
	FSSLError = false;
	FSocket.ignoreSslErrors();
}

QList<QSslError> DefaultConnection::sslErrors() const
{
	return FSocket.sslErrors();
}

QNetworkProxy DefaultConnection::proxy() const
{
	return FSocket.proxy();
}

void DefaultConnection::setProxy(const QNetworkProxy &AProxy)
{
	if (AProxy!= FSocket.proxy())
	{
		FSocket.setProxy(AProxy);
		emit proxyChanged(AProxy);
	}
}

QVariant DefaultConnection::option(int ARole) const
{
	return FOptions.value(ARole);
}

void DefaultConnection::setOption(int ARole, const QVariant &AValue)
{
	FOptions.insert(ARole, AValue);
}

QString DefaultConnection::localAddress()
{
	QHostAddress hostAddress = FSocket.localAddress();
	if(hostAddress != QHostAddress::Null)
	{
		return hostAddress.toString();
	}
	return "";
}

void DefaultConnection::connectToNextHost()
{
	if (!FRecords.isEmpty())
	{
		QJDns::Record record = FRecords.takeFirst();

		while (record.name.endsWith('.'))
			record.name.chop(1);

		if (FChangeProxyType && FSocket.proxy().type()!=QNetworkProxy::NoProxy)
		{
			QNetworkProxy httpProxy = FSocket.proxy();
			httpProxy.setType(QNetworkProxy::HttpProxy);
			FSocket.setProxy(httpProxy);
		}

		connectSocketToHost(record.name,record.port);
	}
}

void DefaultConnection::connectSocketToHost(const QString &AHost, quint16 APort)
{
	FHost = AHost;
	FPort = APort;

	if (FSSLConnection)
		FSocket.connectToHostEncrypted(FHost, FPort);
	else
		FSocket.connectToHost(FHost, FPort);
}

void DefaultConnection::setError(const QString &AError)
{
	FErrorString = AError;
	emit error(FErrorString);
	LogError(QString("[DefaultConnection error]: %1").arg(AError));
}

void DefaultConnection::onDnsResultsReady(int AId, const QJDns::Response &AResults)
{
	if (FSrvQueryId == AId)
	{
		if (!AResults.answerRecords.isEmpty())
		{
			FSSLConnection = false;
			FRecords = AResults.answerRecords;
		}
		FDns.shutdown();
	}
}

void DefaultConnection::onDnsError(int AId, QJDns::Error AError)
{
	if (FSrvQueryId == AId)
	{
		FDns.shutdown();
		LogError(QString("[DefaultConnection error]: %1 %2").arg("QJDns error").arg(AError));
	}
}

void DefaultConnection::onDnsShutdownFinished()
{
	if (FSrvQueryId != STOP_QUERY_ID)
	{
		FSrvQueryId = START_QUERY_ID;
		connectToNextHost();
	}
	else
	{
		FSrvQueryId = START_QUERY_ID;
		emit disconnected();
	}
}

void DefaultConnection::onSocketConnected()
{
	if (!FSSLConnection)
	{
		FRecords.clear();
		emit connected();
	}
}

void DefaultConnection::onSocketEncrypted()
{
	emit encrypted();
	if (FSSLConnection)
	{
		FRecords.clear();
		emit connected();
	}
}

void DefaultConnection::onSocketReadyRead()
{
	emit readyRead(FSocket.bytesAvailable());
}

void DefaultConnection::onSocketSSLErrors(const QList<QSslError> &AErrors)
{
	FSSLError = true;
	QStringList errors;
	foreach (QSslError err, AErrors)
		errors << err.errorString();
	LogError(QString("[DefaultConnection error]: SSL errors %1").arg(errors.join("; ")));
	if (!FIgnoreSSLErrors)
		emit sslErrors(AErrors);
	else
		ignoreSslErrors();
}

void DefaultConnection::onSocketError(QAbstractSocket::SocketError)
{
	if (FChangeProxyType && FSocket.proxy().type()==QNetworkProxy::HttpProxy)
	{
		QNetworkProxy socksProxy = FSocket.proxy();
		socksProxy.setType(QNetworkProxy::Socks5Proxy);
		FSocket.setProxy(socksProxy);
		connectSocketToHost(FHost,FPort);
	}
	else if (FRecords.isEmpty())
	{
		if (FSocket.state()!=QSslSocket::ConnectedState || FSSLError)
		{
			setError(FSocket.errorString());
			emit disconnected();
		}
		else
			setError(FSocket.errorString());
	}
	else
	{
		connectToNextHost();
	}
}

void DefaultConnection::onSocketDisconnected()
{
	emit disconnected();
}
