#include "sortfilterproxymodel.h"

SortFilterProxyModel::SortFilterProxyModel(IRostersViewPlugin *ARostersViewPlugin, QObject *AParent) : QSortFilterProxyModel(AParent)
{
	FShowOffline = true;
	FSortByStatus = false;
	FRostersViewPlugin = ARostersViewPlugin;
}

SortFilterProxyModel::~SortFilterProxyModel()
{

}

void SortFilterProxyModel::invalidate()
{
	FShowOffline = Options::node(OPV_ROSTER_SHOWOFFLINE).value().toBool();
	FSortByStatus = Options::node(OPV_ROSTER_SORTBYSTATUS).value().toBool();
	QSortFilterProxyModel::invalidate();
}

bool SortFilterProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
	int leftTypeOrder = ALeft.data(RDR_TYPE_ORDER).toInt();
	int rightTypeOrder = ARight.data(RDR_TYPE_ORDER).toInt();
	if (leftTypeOrder == rightTypeOrder)
	{
		int leftShow = ALeft.data(RDR_SHOW).toInt();
		int rightShow = ARight.data(RDR_SHOW).toInt();
		if (FSortByStatus && leftTypeOrder!=RITO_STREAM_ROOT && leftShow!=rightShow)
		{
			const static int showOrders[] = {6,2,1,3,4,5,7,8};
			return showOrders[leftShow] < showOrders[rightShow];
		}
		else
			return QSortFilterProxyModel::lessThan(ALeft,ARight);
	}
	else
		return leftTypeOrder < rightTypeOrder;
}

bool SortFilterProxyModel::filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const
{
	QModelIndex index = sourceModel()->index(AModelRow,0,AModelParent);
	if (index.isValid())
	{
		if (index.data(RDR_ALLWAYS_INVISIBLE).toInt() > 0)
			return false;
		else if (index.data(RDR_ALLWAYS_VISIBLE).toInt() > 0)
			return true;

		int indexType = index.data(RDR_TYPE).toInt();
		switch (indexType)
		{
		case RIT_CONTACT:
		case RIT_METACONTACT:
			{
				if (!FShowOffline )
				{
					int indexShow = index.data(RDR_SHOW).toInt();
					return indexShow!=IPresence::Offline && indexShow!=IPresence::Error;
				}
				break;
			}
		case RIT_GROUP:
		case RIT_GROUP_AGENTS:
		case RIT_GROUP_BLANK:
		case RIT_GROUP_NOT_IN_ROSTER:
			{
				for (int childRow = 0; index.child(childRow,0).isValid(); childRow++)
					if (filterAcceptsRow(childRow,index))
						return true;
				return false;
			}
		case RIT_GROUP_MY_RESOURCES:
			return false;
		default:
			return true;
		}
	}
	return true;
}
