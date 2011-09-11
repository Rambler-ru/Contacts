#ifndef EMOTICONS_H
#define EMOTICONS_H

#include <QHash>
#include <QStringList>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/messagewriterorders.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iemoticons.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/iconstorage.h>
#include <utils/options.h>
#include <utils/menu.h>
#include "emoticonscontainer.h"
#include "selecticonmenu.h"

struct EmoticonTreeItem
{
	QUrl url;
	QMap<QChar, EmoticonTreeItem *> childs;
};

class Emoticons :
	public QObject,
	public IPlugin,
	public IEmoticons,
	public IMessageWriter,
	public IOptionsHolder
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IEmoticons IMessageWriter IOptionsHolder)
public:
	Emoticons();
	~Emoticons();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return EMOTICONS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IMessageWriter
	virtual void writeTextToMessage(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang);
	virtual void writeMessageToText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang);
	//IEmoticons
	virtual QList<QString> activeIconsets() const;
	virtual QUrl urlByKey(const QString &AKey) const;
	virtual QString keyByUrl(const QUrl &AUrl) const;
	virtual QMap<int, QString> findTextEmoticons(const QTextDocument *ADocument, int AStartPos=0, int ALength=-1) const;
	virtual QMap<int, QString> findImageEmoticons(const QTextDocument *ADocument, int AStartPos=0, int ALength=-1) const;
protected:
	void createIconsetUrls();
	void createTreeItem(const QString &AKey, const QUrl &AUrl);
	void clearTreeItem(EmoticonTreeItem *AItem) const;
	bool isWordBoundary(const QString &AText) const;
	void replaceTextToImage(QTextDocument *ADocument, int AStartPos=0, int ALength=-1) const;
	void replaceImageToText(QTextDocument *ADocument, int AStartPos=0, int ALength=-1) const;
	SelectIconMenu *createSelectIconMenu(const QString &ASubStorage, QWidget *AParent);
	void insertSelectIconMenu(const QString &ASubStorage);
	void removeSelectIconMenu(const QString &ASubStorage);
protected slots:
	void onEditWidgetCreated(IEditWidget *AEditWidget);
	void onEditWidgetContentsChanged(int APosition, int ARemoved, int AAdded);
	void onEmoticonsContainerDestroyed(QObject *AObject);
	void onSelectIconMenuDestroyed(QObject *AObject);
	void onIconSelected(const QString &ASubStorage, const QString &AIconKey);
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IOptionsManager *FOptionsManager;
private:
	EmoticonTreeItem FRootTreeItem;
	QHash<QString, QUrl> FUrlByKey;
	QHash<QString, QString> FKeyByUrl;
	QMap<QString, IconStorage *> FStorages;
	QList<EmoticonsContainer *> FContainers;
	QMap<SelectIconMenu *, EmoticonsContainer *> FContainerByMenu;
};

#endif // EMOTICONS_H
