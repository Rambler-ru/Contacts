#ifndef ADIUMMESSAGESTYLEPLUGIN_H
#define ADIUMMESSAGESTYLEPLUGIN_H

#include <definitions/resources.h>
#include <definitions/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagestyles.h>
#include <utils/filestorage.h>
#include <utils/message.h>
#include <utils/options.h>
#include "adiummessagestyle.h"
#include "adiumoptionswidget.h"

#define ADIUMMESSAGESTYLE_UUID    "{91da3fcb-5c20-4b52-a4a5-80722eef6ea2}"

class AdiumMessageStylePlugin :
			public QObject,
			public IPlugin,
			public IMessageStylePlugin
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IMessageStylePlugin)
public:
	AdiumMessageStylePlugin();
	~AdiumMessageStylePlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return ADIUMMESSAGESTYLE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IMessageStylePlugin
	virtual QString pluginId() const;
	virtual QString pluginName() const;
	virtual QList<QString> styles() const;
	virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions);
	virtual IMessageStyleOptions styleOptions(const OptionsNode &ANode, int AMessageType) const;
	virtual IOptionsWidget *styleSettingsWidget(const OptionsNode &ANode, int AMessageType, QWidget *AParent);
	virtual void saveStyleSettings(IOptionsWidget *AWidget, OptionsNode ANode = OptionsNode::null);
	virtual void saveStyleSettings(IOptionsWidget *AWidget, IMessageStyleOptions &AOptions);
	//AdiumMessageStylePlugin
	QList<QString> styleVariants(const QString &AStyleId) const;
	QMap<QString,QVariant> styleInfo(const QString &AStyleId) const;
signals:
	void styleCreated(IMessageStyle *AStyle) const;
	void styleDestroyed(IMessageStyle *AStyle) const;
	void styleWidgetAdded(IMessageStyle *AStyle, QWidget *AWidget) const;
	void styleWidgetRemoved(IMessageStyle *AStyle, QWidget *AWidget) const;
protected:
	void updateAvailStyles();
protected slots:
	void onStyleWidgetAdded(QWidget *AWidget);
	void onStyleWidgetRemoved(QWidget *AWidget);
	void onClearEmptyStyles();
private:
	QMap<QString, QString> FStylePaths;
	QMap<QString, AdiumMessageStyle *> FStyles;
};

#endif // ADIUMMESSAGESTYLEPLUGIN_H
