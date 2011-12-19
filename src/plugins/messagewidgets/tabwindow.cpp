#include "tabwindow.h"

#include <QCursor>
#include <QMessageBox>
#include <QInputDialog>
#include <definitions/resources.h>
#include <definitions/graphicseffects.h>
#include <utils/graphicseffectsstorage.h>

#define ADR_TAB_INDEX               Action::DR_Parametr1
#define ADR_CLOSE_OTHER             Action::DR_Parametr2
#define ADR_OPEN_LAST               Action::DR_Parametr3
#define ADR_TABWINDOWID             Action::DR_Parametr1

TabWindow::TabWindow(IMessageWidgets *AMessageWidgets, const QUuid &AWindowId)
{
	ui.setupUi(this);
#ifdef Q_WS_MAC
	ui.lblCaption->setVisible(false);
	ui.lblStatusIcon->setVisible(false);
	ui.centralWidget->layout()->removeItem(ui.captionLayout);
#endif
	setAttribute(Qt::WA_DeleteOnClose,false);
	setMinimumSize(500, 400);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_MESSAGEWIDGETS_TABWINDOW);
	GraphicsEffectsStorage::staticStorage(RSR_STORAGE_GRAPHICSEFFECTS)->installGraphicsEffect(this, GFX_LABELS);

	FWindowId = AWindowId;
	FMessageWidgets = AMessageWidgets;

	FBorder = CustomBorderStorage::staticStorage(RSR_STORAGE_CUSTOMBORDER)->addBorder(this, CBS_MESSAGEWINDOW);
	loadWindowStateAndGeometry();

	FWindowMenu = new Menu(this);
	createActions();
	addActions(FWindowMenu->actions());

	connect(ui.twtTabs,SIGNAL(currentChanged(int)),SLOT(onTabChanged(int)));
	connect(ui.twtTabs,SIGNAL(tabMenuRequested(int)),SLOT(onTabMenuRequested(int)));
	connect(ui.twtTabs,SIGNAL(tabCloseRequested(int)),SLOT(onTabCloseRequested(int)));

	connect(FMessageWidgets->instance(),SIGNAL(tabWindowNameChanged(const QUuid &, const QString &)),
		SLOT(onTabWindowNameChanged(const QUuid &, const QString &)));
}

TabWindow::~TabWindow()
{
	clearTabs();
	saveWindowStateAndGeometry();
	emit windowDestroyed();
}

void TabWindow::showWindow()
{
//#ifdef Q_WS_MAC
//	(FBorder ? (QWidget *)FBorder : (QWidget *)this)->show();
//#else
	WidgetManager::showActivateRaiseWindow(window());
//#endif
}

void TabWindow::showMinimizedWindow()
{
	if (FBorder ? !FBorder->isVisible() : !isVisible())
	{
		if (FBorder)
			FBorder->showMinimized();
		else
			showMinimized();
	}
}

QUuid TabWindow::windowId() const
{
	return FWindowId;
}

QString TabWindow::windowName() const
{
	return FMessageWidgets->tabWindowName(FWindowId);
}

Menu *TabWindow::windowMenu() const
{
	return FWindowMenu;
}

void TabWindow::addTabPage(ITabPage *APage)
{
	if (ui.twtTabs->indexOf(APage->instance()) < 0)
	{
		int index = ui.twtTabs->addTab(APage->instance(),APage->instance()->windowTitle());
		connect(APage->instance(),SIGNAL(tabPageShow()),SLOT(onTabPageShow()));
		connect(APage->instance(),SIGNAL(tabPageShowMinimized()),SLOT(onTabPageShowMinimized()));
		connect(APage->instance(),SIGNAL(tabPageClose()),SLOT(onTabPageClose()));
		connect(APage->instance(),SIGNAL(tabPageChanged()),SLOT(onTabPageChanged()));
		connect(APage->instance(),SIGNAL(tabPageDestroyed()),SLOT(onTabPageDestroyed()));
		if (APage->tabPageNotifier())
			connect(APage->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onTabPageNotifierActiveNotifyChanged(int)));
		connect(APage->instance(),SIGNAL(tabPageNotifierChanged()),SLOT(onTabPageNotifierChanged()));
		updateTab(index);
		updateWindow();
		emit tabPageAdded(APage);
	}
}

bool TabWindow::hasTabPage(ITabPage *APage) const
{
	return ui.twtTabs->indexOf(APage->instance()) >= 0;
}

ITabPage *TabWindow::currentTabPage() const
{
	return qobject_cast<ITabPage *>(ui.twtTabs->currentWidget());
}

void TabWindow::setCurrentTabPage(ITabPage *APage)
{
	ui.twtTabs->setCurrentWidget(APage->instance());
}

void TabWindow::detachTabPage(ITabPage *APage)
{
	removeTabPage(APage);
	APage->instance()->show();
	if (APage->instance()->x()<=0 || APage->instance()->y()<0)
		APage->instance()->move(0,0);
	emit tabPageDetached(APage);
}

void TabWindow::removeTabPage(ITabPage *APage)
{
	int index = ui.twtTabs->indexOf(APage->instance());
	if (index >=0)
	{
		ui.twtTabs->removeTab(index);
		APage->instance()->close();
		APage->instance()->setParent(NULL);
		disconnect(APage->instance(),SIGNAL(tabPageShow()),this,SLOT(onTabPageShow()));
		disconnect(APage->instance(),SIGNAL(tabPageShowMinimized()),this,SLOT(onTabPageShowMinimized()));
		disconnect(APage->instance(),SIGNAL(tabPageClose()),this,SLOT(onTabPageClose()));
		disconnect(APage->instance(),SIGNAL(tabPageChanged()),this,SLOT(onTabPageChanged()));
		disconnect(APage->instance(),SIGNAL(tabPageDestroyed()),this,SLOT(onTabPageDestroyed()));
		if (APage->tabPageNotifier())
			disconnect(APage->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onTabPageNotifierActiveNotifyChanged(int)));
		disconnect(APage->instance(),SIGNAL(tabPageNotifierChanged()),this,SLOT(onTabPageNotifierChanged()));
		FLastClosedTab = APage->tabPageId();
		emit tabPageRemoved(APage);
		if (ui.twtTabs->count() == 0)
		{
			if (parentWidget())
				parentWidget()->close();
			else
				close();
		}
	}
}

void TabWindow::nextTab()
{
	ui.twtTabs->showNextTab();
}

void TabWindow::previousTab()
{
	ui.twtTabs->showPrevTab();
}

void TabWindow::closeCurrentTab()
{
	removeTabPage(currentTabPage());
}

void TabWindow::closeAllTabs()
{
	clearTabs();
	close();
}

void TabWindow::createActions()
{
#ifndef Q_WS_MAC
	FNextTab = new Action(FWindowMenu);
	FNextTab->setText(tr("Next Tab"));
	FNextTab->setShortcuts(QList<QKeySequence>() << tr("Ctrl+Tab") << tr("Ctrl+PgDown"));
	FWindowMenu->addAction(FNextTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FNextTab, SIGNAL(triggered()), SLOT(nextTab()));

	FPrevTab = new Action(FWindowMenu);
	FPrevTab->setText(tr("Prev. Tab"));
	FPrevTab->setShortcuts(QList<QKeySequence>() << tr("Ctrl+Shift+Tab") << tr("Ctrl+PgUp"));
	FWindowMenu->addAction(FPrevTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FPrevTab, SIGNAL(triggered()), SLOT(previousTab()));

	FCloseTab = new Action(FWindowMenu);
	FCloseTab->setText(tr("Close Tab"));
	FCloseTab->setShortcuts(QList<QKeySequence>() << tr("Ctrl+W") << tr("Ctrl+F4"));
	FWindowMenu->addAction(FCloseTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FCloseTab,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));

	FCloseAllTabs = new Action(FWindowMenu);
	FCloseAllTabs->setText(tr("Close All Tabs"));
	FCloseAllTabs->setShortcut(tr("Ctrl+Shift+W"));
	FWindowMenu->addAction(FCloseAllTabs,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FCloseAllTabs,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));

	FCloseWindow = new Action(FWindowMenu);
	FCloseWindow->setText(tr("Close Window"));
	FCloseWindow->setShortcuts(QList<QKeySequence>() << tr("Esc") << tr("Alt+F4"));
	FWindowMenu->addAction(FCloseWindow,AG_MWTW_MWIDGETS_TABWINDOW_OPTIONS);
	connect(FCloseWindow,SIGNAL(triggered(bool)),SLOT(onWindowMenuActionTriggered(bool)));
#endif
}

void TabWindow::saveWindowStateAndGeometry()
{
	if (FMessageWidgets->tabWindowList().contains(FWindowId))
	{
		QWidget *widget = FBorder!=NULL ? (QWidget *)FBorder : (QWidget *)this;
		if (widget->isWindow())
		{
			Options::setFileValue(FBorder ? FBorder->isMaximized() : isMaximized(),"messages.tabwindows.window.state-maximized",FWindowId.toString());
			if (FBorder && FBorder->isMaximized())
				FBorder->maximizeWidget();
			Options::setFileValue(widget->saveGeometry(),"messages.tabwindows.window.geometry",FWindowId.toString());
		}
	}
}

void TabWindow::loadWindowStateAndGeometry()
{
	if (FMessageWidgets->tabWindowList().contains(FWindowId))
	{
		QWidget *widget = FBorder!=NULL ? (QWidget *)FBorder : (QWidget *)this;
		if (widget->isWindow())
		{
			if (!widget->restoreGeometry(Options::fileValue("messages.tabwindows.window.geometry",FWindowId.toString()).toByteArray()))
				widget->setGeometry(WidgetManager::alignGeometry(QSize(640,480),this));
			if (Options::fileValue("messages.tabwindows.window.state-maximized",FWindowId.toString()).toBool())
				FBorder ? FBorder->maximizeWidget() : widget->setWindowState(widget->windowState() | Qt::WindowMaximized);
		}
	}
}

void TabWindow::clearTabs()
{
	while (ui.twtTabs->count() > 0)
	{
		ITabPage *page = qobject_cast<ITabPage *>(ui.twtTabs->widget(0));
		if (page)
			removeTabPage(page);
		else
			ui.twtTabs->removeTab(0);
	}
}

void TabWindow::updateWindow()
{
	QWidget *widget = ui.twtTabs->currentWidget();
	if (widget)
	{
		if (qobject_cast<CustomBorderContainer *>(parentWidget())!=NULL)
		{
			parentWidget()->setWindowIcon(widget->windowIcon());
			parentWidget()->setWindowTitle(widget->windowTitle());
		}
		else
		{
			setWindowIcon(widget->windowIcon());
			setWindowTitle(widget->windowTitle());
		}
		ui.lblStatusIcon->setPixmap(widget->windowIcon().pixmap(16, 16));
		ui.lblCaption->setText(widget->windowTitle());
		emit windowChanged();
	}
}

void TabWindow::updateTab(int AIndex)
{
	ITabPage *page = qobject_cast<ITabPage *>(ui.twtTabs->widget(AIndex));
	if (page)
	{
		ui.twtTabs->setTabIcon(AIndex,page->tabPageIcon());
		ui.twtTabs->setTabText(AIndex,page->tabPageCaption());
		ui.twtTabs->setTabToolTip(AIndex,page->tabPageToolTip());
	}
}

void TabWindow::onTabChanged(int AIndex)
{
	Q_UNUSED(AIndex);
	updateWindow();
	emit currentTabPageChanged(currentTabPage());
}

void TabWindow::onTabMenuRequested(int AIndex)
{
	Menu *tabMenu = new Menu(this);
	tabMenu->setAttribute(Qt::WA_DeleteOnClose, true);

	if (AIndex >= 0)
	{
		Action *action = new Action(tabMenu);
		action->setText(tr("Close Tab"));
		action->setData(ADR_TAB_INDEX, AIndex);
		action->setData(ADR_CLOSE_OTHER, false);
		connect(action,SIGNAL(triggered(bool)),SLOT(onTabMenuActionTriggered(bool)));
		tabMenu->addAction(action);

		action = new Action(tabMenu);
		action->setText(tr("Close Other Tabs"));
		action->setData(ADR_TAB_INDEX, AIndex);
		action->setData(ADR_CLOSE_OTHER, true);
		action->setEnabled(ui.twtTabs->count()>1);
		connect(action,SIGNAL(triggered(bool)),SLOT(onTabMenuActionTriggered(bool)));
		tabMenu->addAction(action);
	}
	else
	{
		Action *action = new Action(tabMenu);
		action->setText(tr("Close Window"));
		connect(action,SIGNAL(triggered()),SLOT(close()));
		tabMenu->addAction(action,AG_DEFAULT+1);
	}

	Action *action = new Action(tabMenu);
	action->setText(tr("Open Closed Tab"));
	action->setData(ADR_OPEN_LAST, true);
	action->setEnabled(!FLastClosedTab.isEmpty());
	connect(action,SIGNAL(triggered(bool)),SLOT(onTabMenuActionTriggered(bool)));
	tabMenu->addAction(action,AG_DEFAULT+1);

	tabMenu->popup(QCursor::pos());
}

void TabWindow::onTabCloseRequested(int AIndex)
{
	ITabPage *page = qobject_cast<ITabPage *>(ui.twtTabs->widget(AIndex));
	if (page)
		removeTabPage(page);
}

void TabWindow::onTabPageShow()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
	{
		setCurrentTabPage(page);
		showWindow();
	}
}

void TabWindow::onTabPageShowMinimized()
{
	showMinimizedWindow();
}

void TabWindow::onTabPageClose()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
		removeTabPage(page);
}

void TabWindow::onTabPageChanged()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
	{
		int index = ui.twtTabs->indexOf(page->instance());
		updateTab(index);
		if (index == ui.twtTabs->currentIndex())
			updateWindow();
	}
}

void TabWindow::onTabPageDestroyed()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
		removeTabPage(page);
}

void TabWindow::onTabPageNotifierChanged()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page && page->tabPageNotifier()!=NULL)
	{
		connect(page->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),SLOT(onTabPageNotifierActiveNotifyChanged(int)));
	}
}

void TabWindow::onTabPageNotifierActiveNotifyChanged(int ANotifyId)
{
	ITabPageNotifier *notifier = qobject_cast<ITabPageNotifier *>(sender());
	if (notifier)
	{
		int index = ui.twtTabs->indexOf(notifier->tabPage()->instance());
		ui.twtTabs->setTabNotify(index,notifier->notifyById(ANotifyId));
	}
}

void TabWindow::onTabWindowNameChanged(const QUuid &AWindowId, const QString &AName)
{
	Q_UNUSED(AName);
	if (AWindowId == FWindowId)
		updateWindow();
}

void TabWindow::onTabMenuActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		int index = action->data(ADR_TAB_INDEX).toInt();
		if (action->data(ADR_OPEN_LAST).toBool())
		{
			foreach(ITabPageHandler *handler, FMessageWidgets->tabPageHandlers())
			{
				ITabPage *page = handler->tabPageCreate(FLastClosedTab);
				if (page)
					page->showTabPage();
				break;
			}
			FLastClosedTab.clear();
		}
		else if (action->data(ADR_CLOSE_OTHER).toBool())
		{
			while (index+1 < ui.twtTabs->count())
				onTabCloseRequested(index+1);
			for (int i=0; i<index; i++)
				onTabCloseRequested(0);
		}
		else
		{
			onTabCloseRequested(index);
		}
	}
}

void TabWindow::onWindowMenuActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action == FCloseTab)
	{
		closeCurrentTab();
	}
	else if (action == FCloseAllTabs)
	{
		closeAllTabs();
	}
	else if (action == FCloseWindow)
	{
		window()->close();
	}
}
