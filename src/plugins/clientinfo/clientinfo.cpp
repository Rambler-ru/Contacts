#include "clientinfo.h"

#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QTextStream>

#if defined(Q_OS_UNIX)
# include <sys/utsname.h>
#endif

#if defined(Q_WS_HAIKU)
#include <Path.h>
#include <AppFileInfo.h>
#include <FindDirectory.h>
#endif

#define SHC_SOFTWARE_VERSION            "/iq[@type='get']/query[@xmlns='" NS_JABBER_VERSION "']"
#define SHC_LAST_ACTIVITY               "/iq[@type='get']/query[@xmlns='" NS_JABBER_LAST "']"
#define SHC_ENTITY_TIME                 "/iq[@type='get']/time[@xmlns='" NS_XMPP_TIME "']"
#define SHC_XMPP_PING                   "/iq[@type='get']/ping[@xmlns='" NS_XMPP_PING "']"

#define SOFTWARE_INFO_TIMEOUT           10000
#define LAST_ACTIVITY_TIMEOUT           10000
#define ENTITY_TIME_TIMEOUT             10000

#define ADR_STREAM_JID                  Action::DR_StreamJid
#define ADR_CONTACT_JID                 Action::DR_Parametr1
#define ADR_INFO_TYPES                  Action::DR_Parametr2

#define FORM_FIELD_SOFTWARE             "software"
#define FORM_FIELD_SOFTWARE_VERSION     "software_version"
#define FORM_FIELD_OS                   "os"
#define FORM_FIELD_OS_VERSION           "os_version"

ClientInfo::ClientInfo()
{
	FPluginManager = NULL;
	FStanzaProcessor = NULL;
	FPresencePlugin = NULL;
	FDiscovery = NULL;
	FDataForms = NULL;

	FPingHandle = 0;
	FTimeHandle = 0;
	FVersionHandle = 0;
	FActivityHandler = 0;
}

ClientInfo::~ClientInfo()
{

}

void ClientInfo::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Client Information");
	APluginInfo->description = tr("Allows to send and receive information about the version of the application, the local time and the last activity of contact");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool ClientInfo::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)),SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
		}
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
		}
	}

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FStanzaProcessor != NULL;
}

bool ClientInfo::initObjects()
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;

		shandle.conditions.append(SHC_SOFTWARE_VERSION);
		FVersionHandle = FStanzaProcessor->insertStanzaHandle(shandle);

		shandle.conditions.clear();
		shandle.conditions.append(SHC_LAST_ACTIVITY);
		FActivityHandler = FStanzaProcessor->insertStanzaHandle(shandle);

		shandle.conditions.clear();
		shandle.conditions.append(SHC_ENTITY_TIME);
		FTimeHandle = FStanzaProcessor->insertStanzaHandle(shandle);

		shandle.conditions.clear();
		shandle.conditions.append(SHC_XMPP_PING);
		FPingHandle = FStanzaProcessor->insertStanzaHandle(shandle);
	}

	if (FDiscovery)
	{
		registerDiscoFeatures();
	}

	if (FDataForms)
	{
		FDataForms->insertLocalizer(this,DATA_FORM_SOFTWAREINFO);
	}

	return true;
}

bool ClientInfo::initSettings()
{
	Options::setDefaultValue(OPV_MISC_SHAREOSVERSION,true);
	return true;
}

bool ClientInfo::startPlugin()
{
	SystemManager::instance()->startSystemIdle();
	return true;
}

bool ClientInfo::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AHandlerId == FVersionHandle)
	{
		AAccept = true;
		Stanza iq("iq");
		iq.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
		QDomElement elem = iq.addElement("query",NS_JABBER_VERSION);
		elem.appendChild(iq.createElement("name")).appendChild(iq.createTextNode(CLIENT_NAME));
		elem.appendChild(iq.createElement("version")).appendChild(iq.createTextNode(QString("%1.%2 %3").arg(FPluginManager->version()).arg(FPluginManager->revision()).arg(CLIENT_VERSION_SUFIX).trimmed()));
		if (Options::node(OPV_MISC_SHAREOSVERSION).value().toBool())
			elem.appendChild(iq.createElement("os")).appendChild(iq.createTextNode(osVersion()));
		FStanzaProcessor->sendStanzaOut(AStreamJid,iq);
	}
	else if (AHandlerId == FActivityHandler)
	{
		AAccept = true;
		Stanza iq("iq");
		iq.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
		QDomElement elem = iq.addElement("query",NS_JABBER_LAST);
		elem.setAttribute("seconds", SystemManager::systemIdle());
		FStanzaProcessor->sendStanzaOut(AStreamJid,iq);
	}
	else if (AHandlerId == FTimeHandle)
	{
		AAccept = true;
		Stanza iq("iq");
		iq.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
		QDomElement elem = iq.addElement("time",NS_XMPP_TIME);
		DateTime dateTime(QDateTime::currentDateTime());
		elem.appendChild(iq.createElement("tzo")).appendChild(iq.createTextNode(dateTime.toX85TZD()));
		elem.appendChild(iq.createElement("utc")).appendChild(iq.createTextNode(dateTime.toX85UTC()));
		FStanzaProcessor->sendStanzaOut(AStreamJid,iq);
	}
	else if (AHandlerId == FPingHandle)
	{
		AAccept = true;
		Stanza iq("iq");
		iq.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
		FStanzaProcessor->sendStanzaOut(AStreamJid,iq);
	}
	return false;
}

void ClientInfo::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FSoftwareId.contains(AStanza.id()))
	{
		Jid contactJid = FSoftwareId.take(AStanza.id());
		SoftwareItem &software = FSoftwareItems[contactJid];
		if (AStanza.type() == "result")
		{
			QDomElement query = AStanza.firstElement("query");
			software.name = query.firstChildElement("name").text();
			software.version = query.firstChildElement("version").text();
			software.os = query.firstChildElement("os").text();
			software.status = SoftwareLoaded;
			LogDetaile(QString("[ClientInfo] Received software version from %1").arg(contactJid.full()));
		}
		else if (AStanza.type() == "error")
		{
			ErrorHandler err(AStanza.element());
			software.name = err.message();
			software.version.clear();
			software.os.clear();
			software.status = SoftwareError;
			LogError(QString("[ClientInfo] Failed to request software version from %1: %2").arg(contactJid.full(),err.message()));
		}
		emit softwareInfoChanged(contactJid);
	}
	else if (FActivityId.contains(AStanza.id()))
	{
		Jid contactJid = FActivityId.take(AStanza.id());
		ActivityItem &activity = FActivityItems[contactJid];
		if (AStanza.type() == "result")
		{
			QDomElement query = AStanza.firstElement("query");
			activity.dateTime = QDateTime::currentDateTime().addSecs(0-query.attribute("seconds","0").toInt());
			activity.text = query.text();
			LogDetaile(QString("[ClientInfo] Received activity time from %1").arg(contactJid.full()));
		}
		else if (AStanza.type() == "error")
		{
			ErrorHandler err(AStanza.element());
			activity.dateTime = QDateTime();
			activity.text = err.message();
			LogError(QString("[ClientInfo] Failed to request activity time from %1: %2").arg(contactJid.full(),err.message()));
		}
		emit lastActivityChanged(contactJid);
	}
	else if (FTimeId.contains(AStanza.id()))
	{
		Jid contactJid = FTimeId.take(AStanza.id());
		QDomElement time = AStanza.firstElement("time");
		QString tzo = time.firstChildElement("tzo").text();
		QString utc = time.firstChildElement("utc").text();
		if (AStanza.type() == "result" && !tzo.isEmpty() && !utc.isEmpty())
		{
			TimeItem &tItem = FTimeItems[contactJid];
			tItem.zone = DateTime::tzdFromX85(tzo);
			tItem.delta = QDateTime::currentDateTime().secsTo(DateTime(utc).toLocal());
			tItem.ping = tItem.ping - QTime::currentTime().msecsTo(QTime(0,0,0,0));
			LogDetaile(QString("[ClientInfo] Received entity time from %1").arg(contactJid.full()));
		}
		else
		{
			FTimeItems.remove(contactJid);
			ErrorHandler err(AStanza.element());
			LogError(QString("[ClientInfo] Failed to request entity time from %1: %2").arg(contactJid.full(),err.message()));
		}
		emit entityTimeChanged(contactJid);
	}
}

void ClientInfo::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	Q_UNUSED(AStreamJid);
	if (FSoftwareId.contains(AStanzaId))
	{
		Jid contactJid = FSoftwareId.take(AStanzaId);
		SoftwareItem &software = FSoftwareItems[contactJid];
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		software.name = err.message();
		software.version.clear();
		software.os.clear();
		software.status = SoftwareError;
		LogError(QString("[ClientInfo] Failed to request software version from %1: Request Timeout").arg(contactJid.full()));
		emit softwareInfoChanged(contactJid);
	}
	else if (FActivityId.contains(AStanzaId))
	{
		Jid contactJid = FActivityId.take(AStanzaId);
		LogError(QString("[ClientInfo] Failed to request activity time from %1: Request Timeout").arg(contactJid.full()));
		emit lastActivityChanged(contactJid);
	}
	else if (FTimeId.contains(AStanzaId))
	{
		Jid contactJid = FTimeId.take(AStanzaId);
		FTimeItems.remove(contactJid);
		LogError(QString("[ClientInfo] Failed to request entity time from %1: Request Timeout").arg(contactJid.full()));
		emit entityTimeChanged(contactJid);
	}
}

IDataFormLocale ClientInfo::dataFormLocale(const QString &AFormType)
{
	IDataFormLocale locale;
	if (AFormType == DATA_FORM_SOFTWAREINFO)
	{
		locale.title = tr("Software information");
		locale.fields[FORM_FIELD_SOFTWARE].label = tr("Software");
		locale.fields[FORM_FIELD_SOFTWARE_VERSION].label = tr("Software Version");
		locale.fields[FORM_FIELD_OS].label = tr("OS");
		locale.fields[FORM_FIELD_OS_VERSION].label = tr("OS Version");
	}
	return locale;
}

QString ClientInfo::osVersion() const
{
	static QString osver;
	if (osver.isEmpty())
	{
#if defined(Q_WS_MAC)
		switch (QSysInfo::MacintoshVersion)
		{
		case QSysInfo::MV_SNOWLEOPARD:
			osver = "MacOS 10.6(SnowLeopard)";
			break;
		case QSysInfo::MV_LEOPARD:
			osver = "MacOS 10.5(Leopard)";
			break;
		case QSysInfo::MV_TIGER:
			osver = "MacOS 10.4(Tiger)";
			break;
		case QSysInfo::MV_PANTHER:
			osver = "MacOS 10.3(Panther)";
			break;
		case QSysInfo::MV_JAGUAR:
			osver = "MacOS 10.2(Jaguar)";
			break;
		case QSysInfo::MV_PUMA:
			osver = "MacOS 10.1(Puma)";
			break;
		case QSysInfo::MV_CHEETAH:
			osver = "MacOS 10.0(Cheetah)";
			break;
		case QSysInfo::MV_9:
			osver = "MacOS 9";
			break;
		case QSysInfo::MV_Unknown:
		default:
			osver = "MacOS (unknown)";
			break;
		}
#elif defined(Q_WS_X11)
		QStringList path;
		foreach(QString env, QProcess::systemEnvironment())
			if (env.startsWith("PATH="))
				path = env.split('=').value(1).split(':');

		QString found;
		foreach(QString dirname, path)
		{
			QDir dir(dirname);
			QFileInfo cand(dir.filePath("lsb_release"));
			if (cand.isExecutable())
			{
				found = cand.absoluteFilePath();
				break;
			}
		}

		if (!found.isEmpty())
		{
			QProcess process;
			process.start(found, QStringList()<<"--description"<<"--short", QIODevice::ReadOnly);
			if (process.waitForStarted())
			{
				QTextStream stream(&process);
				while (process.waitForReadyRead())
					osver += stream.readAll();
				process.close();
				osver = osver.trimmed();
			}
		}

		if (osver.isEmpty())
		{
			utsname buf;
			if (uname(&buf) != -1)
			{
				osver.append(buf.release).append(QLatin1Char(' '));
				osver.append(buf.sysname).append(QLatin1Char(' '));
				osver.append(buf.machine).append(QLatin1Char(' '));
				osver.append(QLatin1String(" (")).append(buf.machine).append(QLatin1Char(')'));
			}
			else
			{
				osver = QLatin1String("Linux/Unix (unknown)");
			}
		}
#elif defined(Q_WS_WIN) || defined(Q_OS_CYGWIN)
		switch (QSysInfo::WindowsVersion)
		{
		case QSysInfo::WV_CE_6:
			osver = "Windows CE 6.x";
			break;
		case QSysInfo::WV_CE_5:
			osver = "Windows CE 5.x";
			break;
		case QSysInfo::WV_CENET:
			osver = "Windows CE .NET";
			break;
		case QSysInfo::WV_CE:
			osver = "Windows CE";
			break;
		case QSysInfo::WV_WINDOWS7:
			osver = "Windows 7";
			break;
		case QSysInfo::WV_VISTA:
			osver = "Windows Vista";
			break;
		case QSysInfo::WV_2003:
			osver = "Windows Server 2003";
			break;
		case QSysInfo::WV_XP:
			osver = "Windows XP";
			break;
		case QSysInfo::WV_2000:
			osver = "Windows 2000";
			break;
		case QSysInfo::WV_NT:
			osver = "Windows NT";
			break;
		case QSysInfo::WV_Me:
			osver = "Windows Me";
			break;
		case QSysInfo::WV_98:
			osver = "Windows 98";
			break;
		case QSysInfo::WV_95:
			osver = "Windows 95";
			break;
		case QSysInfo::WV_32s:
			osver = "Windows 3.1 with Win32s";
			break;
		default:
			osver = "Windows (unknown)";
			break;
		}
#elif defined(Q_WS_HAIKU)
		QString strVersion("Haiku");
		BPath path;
		if (find_directory(B_BEOS_LIB_DIRECTORY, &path) == B_OK) {
		path.Append("libbe.so");

		BAppFileInfo appFileInfo;
		version_info versionInfo;
		BFile file;
		if (file.SetTo(path.Path(), B_READ_ONLY) == B_OK
			&& appFileInfo.SetTo(&file) == B_OK
			&& appFileInfo.GetVersionInfo(&versionInfo, 
				B_APP_VERSION_KIND) == B_OK
			&& versionInfo.short_info[0] != '\0')
				strVersion = versionInfo.short_info;
		}

		utsname uname_info;
		if (uname(&uname_info) == 0) {
		osver = uname_info.sysname;
		long revision = 0;
		if (sscanf(uname_info.version, "r%ld", &revision) == 1) {
			char version[16];
			snprintf(version, sizeof(version), "%ld", revision);
			osver += " ( " + strVersion + " Rev. ";
			osver += version;
			osver += ")";
		}
	}
#else
		osver = "Unknown";
#endif
	}
	return osver;
}

bool ClientInfo::hasSoftwareInfo(const Jid &AContactJid) const
{
	return FSoftwareItems.value(AContactJid).status == SoftwareLoaded;
}

bool ClientInfo::requestSoftwareInfo(const Jid &AStreamJid, const Jid &AContactJid)
{
	bool sent = FSoftwareId.values().contains(AContactJid);
	if (!sent && AStreamJid.isValid() && AContactJid.isValid())
	{
		Stanza iq("iq");
		iq.addElement("query",NS_JABBER_VERSION);
		iq.setTo(AContactJid.eFull()).setId(FStanzaProcessor->newId()).setType("get");
		sent = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,SOFTWARE_INFO_TIMEOUT);
		if (sent)
		{
			FSoftwareId.insert(iq.id(),AContactJid);
			FSoftwareItems[AContactJid].status = SoftwareLoading;
			LogDetaile(QString("[ClientInfo] Requesting software version from %1").arg(AContactJid.full()));
		}
	}
	return sent;
}

int ClientInfo::softwareStatus(const Jid &AContactJid) const
{
	return FSoftwareItems.value(AContactJid).status;
}

QString ClientInfo::softwareName(const Jid &AContactJid) const
{
	return FSoftwareItems.value(AContactJid).name;
}

QString ClientInfo::softwareVersion(const Jid &AContactJid) const
{
	return FSoftwareItems.value(AContactJid).version;
}

QString ClientInfo::softwareOs(const Jid &AContactJid) const
{
	return FSoftwareItems.value(AContactJid).os;
}

bool ClientInfo::hasLastActivity(const Jid &AContactJid) const
{
	return FActivityItems.value(AContactJid).dateTime.isValid();
}

bool ClientInfo::requestLastActivity(const Jid &AStreamJid, const Jid &AContactJid)
{
	bool sent = FActivityId.values().contains(AContactJid);
	if (!sent && AStreamJid.isValid() && AContactJid.isValid())
	{
		Stanza iq("iq");
		iq.addElement("query",NS_JABBER_LAST);
		iq.setTo(AContactJid.eFull()).setId(FStanzaProcessor->newId()).setType("get");
		sent = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,LAST_ACTIVITY_TIMEOUT);
		if (sent)
		{
			FActivityId.insert(iq.id(),AContactJid);
			LogDetaile(QString("[ClientInfo] Requesting last activitty from %1").arg(AContactJid.full()));
		}
	}
	return sent;
}

QDateTime ClientInfo::lastActivityTime(const Jid &AContactJid) const
{
	return FActivityItems.value(AContactJid).dateTime;
}

QString ClientInfo::lastActivityText(const Jid &AContactJid) const
{
	return FActivityItems.value(AContactJid).text;
}

bool ClientInfo::hasEntityTime(const Jid &AContactJid) const
{
	return FTimeItems.value(AContactJid).ping >= 0;
}

bool ClientInfo::requestEntityTime(const Jid &AStreamJid, const Jid &AContactJid)
{
	bool sent = FTimeId.values().contains(AContactJid);
	if (!sent && AStreamJid.isValid() && AContactJid.isValid())
	{
		Stanza iq("iq");
		iq.addElement("time",NS_XMPP_TIME);
		iq.setTo(AContactJid.eFull()).setType("get").setId(FStanzaProcessor->newId());
		sent = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,ENTITY_TIME_TIMEOUT);
		if (sent)
		{
			TimeItem &tItem = FTimeItems[AContactJid];
			tItem.ping = QTime::currentTime().msecsTo(QTime(0,0,0,0));
			FTimeId.insert(iq.id(),AContactJid);
			LogDetaile(QString("[ClientInfo] Requesting entity time from %1").arg(AContactJid.full()));
			emit entityTimeChanged(AContactJid);
		}
	}
	return sent;
}

QDateTime ClientInfo::entityTime(const Jid &AContactJid) const
{
	if (hasEntityTime(AContactJid))
	{
		TimeItem tItem = FTimeItems.value(AContactJid);
		QDateTime dateTime = QDateTime::currentDateTime().toUTC();
		dateTime.setTimeSpec(Qt::LocalTime);
		return dateTime.addSecs(tItem.zone).addSecs(tItem.delta);
	}
	return QDateTime();
}

int ClientInfo::entityTimeDelta(const Jid &AContactJid) const
{
	if (hasEntityTime(AContactJid))
		return FTimeItems.value(AContactJid).delta;
	return 0;
}

int ClientInfo::entityTimePing(const Jid &AContactJid) const
{
	return FTimeItems.value(AContactJid).ping;
}

void ClientInfo::registerDiscoFeatures()
{
	IDiscoFeature dfeature;

	dfeature.active = true;
	dfeature.var = NS_JABBER_VERSION;
	dfeature.name = tr("Software Version");
	dfeature.description = tr("Supports the exchanging of the information about the application version");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.active = true;
	dfeature.var = NS_JABBER_LAST;
	dfeature.name = tr("Last Activity");
	dfeature.description = tr("Supports the exchanging of the information about the user last activity");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.active = true;
	dfeature.var = NS_XMPP_TIME;
	dfeature.name = tr("Entity Time");
	dfeature.description = tr("Supports the exchanging of the information about the user local time");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.active = true;
	dfeature.icon = QIcon();
	dfeature.var = NS_XMPP_PING;
	dfeature.name = tr("XMPP Ping");
	dfeature.description = tr("Supports the exchanging of the application-level pings over XML streams");
	FDiscovery->insertDiscoFeature(dfeature);
}

void ClientInfo::onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline)
{
	Q_UNUSED(AStreamJid);
	if (AStateOnline)
	{
		if (FActivityItems.contains(AContactJid))
		{
			FActivityItems.remove(AContactJid);
			emit lastActivityChanged(AContactJid);
		}
	}
	else
	{
		if (FSoftwareItems.contains(AContactJid))
		{
			SoftwareItem &software = FSoftwareItems[AContactJid];
			if (software.status == SoftwareLoading)
				FSoftwareId.remove(FSoftwareId.key(AContactJid));
			FSoftwareItems.remove(AContactJid);
			emit softwareInfoChanged(AContactJid);
		}
		if (FActivityItems.contains(AContactJid))
		{
			FActivityItems.remove(AContactJid);
			emit lastActivityChanged(AContactJid);
		}
		if (FTimeItems.contains(AContactJid))
		{
			FTimeItems.remove(AContactJid);
			emit entityTimeChanged(AContactJid);
		}
	}
}

void ClientInfo::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	if (FDataForms && AInfo.node.isEmpty() && !hasSoftwareInfo(AInfo.contactJid))
	{
		foreach(IDataForm form, AInfo.extensions)
		{
			if (FDataForms->fieldValue("FORM_TYPE",form.fields).toString() == DATA_FORM_SOFTWAREINFO)
			{
				SoftwareItem &software = FSoftwareItems[AInfo.contactJid];
				software.name = FDataForms->fieldValue(FORM_FIELD_SOFTWARE,form.fields).toString();
				software.version = FDataForms->fieldValue(FORM_FIELD_SOFTWARE_VERSION,form.fields).toString();
				software.os = FDataForms->fieldValue(FORM_FIELD_OS,form.fields).toString() + " ";
				software.os += FDataForms->fieldValue(FORM_FIELD_OS_VERSION,form.fields).toString();
				software.status = SoftwareLoaded;
				emit softwareInfoChanged(AInfo.contactJid);
				break;
			}
		}
	}
}

void ClientInfo::onOptionsChanged(const OptionsNode &ANode)
{
	if (FDiscovery && ANode.path()==OPV_MISC_SHAREOSVERSION)
	{
		FDiscovery->updateSelfEntityCapabilities();
	}
}

Q_EXPORT_PLUGIN2(plg_clientinfo, ClientInfo)
