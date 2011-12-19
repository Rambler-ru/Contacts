#ifndef METAPROXYMODEL_H
#define METAPROXYMODEL_H

#include <QTimer>
#include <QSortFilterProxyModel>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/rosterfootertextorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterindextypeorders.h>
#include <definitions/rosterdataholderorders.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/ipresence.h>
#include <utils/iconstorage.h>
#include <utils/imagemanager.h>

class MetaProxyModel :
	public QSortFilterProxyModel,
	public IRosterDataHolder
{
	Q_OBJECT
	Q_INTERFACES(IRosterDataHolder)
public:
	MetaProxyModel(IMetaContacts *AMetaContacts, IRostersView *ARostersView);
	~MetaProxyModel();
	virtual QObject *instance() { return this; }
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
	//MetaProxyModel
	QList<IRosterIndex *> findMetaIndexes(IMetaRoster *AMetaRoster, const QString &AMetaId) const;
signals:
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
protected:
	virtual bool filterAcceptsRow(int ASourceRow, const QModelIndex &ASourceParent) const;
protected:
	IRosterIndex *findChildIndex(const QList<IRosterIndex *> &AIndexes, IRosterIndex *AParent) const;
protected slots:
	void onInvalidateTimerTimeout();
	void onRostersModelSet(IRostersModel *AModel);
	void onRostersNotifyInserted(int ANotifyId);
	void onRostersNotifyActivated(int ANotifyId);
	void onRostersNotifyRemoved(int ANotifyId);
	void onMetaRosterEnabled(IMetaRoster *AMetaRoster, bool AEnabled);
	void onMetaAvatarChanged(IMetaRoster *AMetaRoster, const QString &AMetaId);
	void onMetaPresenceChanged(IMetaRoster *AMetaRoster, const QString &AMetaId);
	void onMetaContactReceived(IMetaRoster *AMetaRoster, const IMetaContact &AContact, const IMetaContact &ABefore);
private:
	IRostersView *FRostersView;
	IRostersModel *FRostersModel;
	IMetaContacts *FMetaContacts;
private:
	int FRostersLabel;
	QTimer FInvalidateTimer;
	QMap<int, int> FIndexNotifies;
private:
	QMap<IMetaRoster *, QMultiHash<QString, IRosterIndex *> > FMetaIndexes;
};

#endif // METAPROXYMODEL_H
