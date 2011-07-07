#ifndef ICOMMANDS_H
#define ICOMMANDS_H

#include <QDomElement>
#include <interfaces/idataforms.h>
#include <utils/jid.h>

#define COMMANDS_UUID "{9d8d6e36-8fa3-45dd-9eaa-dad69b4586c1}"

#define COMMAND_STATUS_EXECUTING        "executing"
#define COMMAND_STATUS_COMPLETED        "completed"
#define COMMAND_STATUS_CANCELED         "canceled"

#define COMMAND_ACTION_EXECUTE          "execute"
#define COMMAND_ACTION_CANCEL           "cancel"
#define COMMAND_ACTION_PREVIOUS         "prev"
#define COMMAND_ACTION_NEXT             "next"
#define COMMAND_ACTION_COMPLETE         "complete"

#define COMMAND_NOTE_INFO               "info"
#define COMMAND_NOTE_WARNING            "warn"
#define COMMAND_NOTE_ERROR              "error"

struct ICommand
{
	QString node;
	QString name;
	Jid itemJid;
};

struct ICommandNote
{
	QString type;
	QString message;
};

struct ICommandRequest
{
	Jid streamJid;
	Jid commandJid;
	QString node;
	QString stanzaId;
	QString sessionId;
	QString action;
	IDataForm form;
};

struct ICommandResult
{
	Jid streamJid;
	Jid commandJid;
	QString node;
	QString stanzaId;
	QString sessionId;
	QString status;
	QString execute;
	QList<QString> actions;
	QList<ICommandNote> notes;
	IDataForm form;
};

struct ICommandError
{
	int code;
	QString stanzaId;
	QString condition;
	QString message;
};

class ICommandServer
{
public:
	virtual QString commandName(const QString &ANode) const = 0;
	virtual bool receiveCommandRequest(const ICommandRequest &ARequest) =0;
	virtual bool receiveCommandError(const ICommandError &AError) =0;
};

class ICommandClient
{
public:
	virtual Jid streamJid() const =0;
	virtual Jid commandJid() const =0;
	virtual QString node() const =0;
	virtual QString sessionId() const =0;
	virtual bool receiveCommandResult(const ICommandResult &AResult) =0;
	virtual bool receiveCommandError(const ICommandError &AError) =0;
};

class ICommands
{
public:
	virtual QObject *instance() =0;
	virtual QList<QString> commandNodes() const =0;
	virtual ICommandServer *commandServer(const QString &ANode) const =0;
	virtual void insertServer(const QString &ANode, ICommandServer *AServer) =0;
	virtual void removeServer(const QString &ANode) =0;
	virtual void insertClient(ICommandClient *AClient) =0;
	virtual void removeClient(ICommandClient *AClient) =0;
	virtual QString sendCommandRequest(const ICommandRequest &ARequest) =0;
	virtual bool sendCommandResult(const ICommandResult &AResult) =0;
	virtual QList<ICommand> contactCommands(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual bool executeCommnad(const Jid &AStreamJid, const Jid &ACommandJid, const QString &ANode) =0;
protected:
	virtual void serverInserted(const QString &ANode, ICommandServer *AServer) =0;
	virtual void serverRemoved(const QString &ANode) =0;
	virtual void clientInserted(ICommandClient *AClient) =0;
	virtual void clientRemoved(ICommandClient *AClient) =0;
	virtual void commandsUpdated(const Jid &AstreamJid, const Jid &AContactJid, const QList<ICommand> &ACommands) =0;
};

Q_DECLARE_INTERFACE(ICommandServer,"Virtus.Plugin.ICommandServer/1.0")
Q_DECLARE_INTERFACE(ICommandClient,"Virtus.Plugin.ICommandClient/1.0")
Q_DECLARE_INTERFACE(ICommands,"Virtus.Plugin.ICommands/1.0")

#endif
