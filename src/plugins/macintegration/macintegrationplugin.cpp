#include "macintegrationplugin.h"

#include "macintegration_p.h"

#include <QApplication>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextDocumentFragment>
#include <QTextEdit>
#include <QWebView>
#include <QClipboard>

#include <utils/custombordercontainer.h>

extern void qt_mac_set_dock_menu(QMenu *); // Qt internal function

MacIntegrationPlugin::MacIntegrationPlugin()
{
	_dockMenu = _fileMenu = _editMenu = _contactsMenu = _windowMenu = NULL;
	_menuBar = NULL;
}

MacIntegrationPlugin::~MacIntegrationPlugin()
{
	delete _dockMenu;
	delete _menuBar;
}

void MacIntegrationPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Mac OS X Integration");
	APluginInfo->description = tr("Allow integration in Mac OS X Dock and Menu Bar");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Valentine Gorshkov aka Silvansky";
	APluginInfo->homePage = "http://contacts.rambler.ru";
}

bool MacIntegrationPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(APluginManager)
	Q_UNUSED(AInitOrder)

	return true;
}

bool MacIntegrationPlugin::initObjects()
{
	// menus
	initMenus();

	// dock
	_dockMenu = new Menu();

	qt_mac_set_dock_menu(_dockMenu); // setting dock menu

	connect(MacIntegrationPrivate::instance(), SIGNAL(dockClicked()), SIGNAL(dockClicked()));
	connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), SLOT(onFocusChanged(QWidget*,QWidget*)));

	return true;
}

Menu * MacIntegrationPlugin::dockMenu()
{
	return _dockMenu;
}

QMenuBar * MacIntegrationPlugin::menuBar()
{
	return _menuBar;
}

Menu * MacIntegrationPlugin::fileMenu()
{
	return _fileMenu;
}

Menu * MacIntegrationPlugin::editMenu()
{
	return _editMenu;
}

Menu * MacIntegrationPlugin::contactsMenu()
{
	return _contactsMenu;
}

Menu * MacIntegrationPlugin::windowMenu()
{
	return _windowMenu;
}

void MacIntegrationPlugin::initMenus()
{
	_menuBar = new QMenuBar(NULL); // this must be the first parentless QMenuBar

	// File
	_fileMenu = new Menu();
	_fileMenu->setTitle(tr("File"));
	_menuBar->addMenu(_fileMenu);

	// Edit
	_editMenu = new Menu;
	_editMenu->setTitle(tr("Edit"));
	_menuBar->addMenu(_editMenu);

	copyAction = new Action;
	copyAction->setText(tr("Copy"));
	copyAction->setShortcut(QKeySequence("Ctrl+C"));
	connect(copyAction, SIGNAL(triggered()), SLOT(onCopyAction()));
	_editMenu->addAction(copyAction);

	cutAction = new Action;
	cutAction->setText(tr("Cut"));
	cutAction->setShortcut(QKeySequence("Ctrl+X"));
	connect(cutAction, SIGNAL(triggered()), SLOT(onCutAction()));
	_editMenu->addAction(cutAction);

	pasteAction = new Action;
	pasteAction->setText(tr("Paste"));
	pasteAction->setShortcut(QKeySequence("Ctrl+V"));
	connect(pasteAction, SIGNAL(triggered()), SLOT(onPasteAction()));
	_editMenu->addAction(pasteAction);

	undoAction = new Action;
	undoAction->setText(tr("Undo"));
	undoAction->setShortcut(QKeySequence("Ctrl+Z"));
	connect(undoAction, SIGNAL(triggered()), SLOT(onUndoAction()));
	_editMenu->addAction(undoAction, 400);

	redoAction = new Action;
	redoAction->setText(tr("Redo"));
	redoAction->setShortcut(QKeySequence("Shift+Ctrl+Z"));
	connect(redoAction, SIGNAL(triggered()), SLOT(onRedoAction()));
	_editMenu->addAction(redoAction, 400);

	selectallAction = new Action;
	selectallAction->setText(tr("Select All"));
	selectallAction->setShortcut(QKeySequence("Ctrl+A"));
	connect(selectallAction, SIGNAL(triggered()), SLOT(onSelectAllAction()));
	_editMenu->addAction(selectallAction);

	// Contacts
	_contactsMenu = new Menu;
	_contactsMenu->setTitle(tr("Contacts"));
	_menuBar->addMenu(_contactsMenu);

	// Window
	_windowMenu = new Menu;
	_windowMenu->setTitle(tr("Window"));
	_menuBar->addMenu(_windowMenu);

	Action * minimizeAction = new Action();
	minimizeAction->setText(tr("Minimize"));
	minimizeAction->setShortcut(QKeySequence("Ctrl+M"));
	connect(minimizeAction, SIGNAL(triggered()), SLOT(onMinimizeAction()));
	_windowMenu->addAction(minimizeAction);

	Action * closeAction = new Action;
	closeAction->setText(tr("Close"));
	closeAction->setShortcut(QKeySequence("Ctrl+W"));
	connect(closeAction, SIGNAL(triggered()), SLOT(onCloseAction()));
	_windowMenu->addAction(closeAction);
}

void MacIntegrationPlugin::onFocusChanged(QWidget * old, QWidget * now)
{
	Q_UNUSED(old)
	if (now)
	{
		bool copyEnabled = false;
		bool cutEnabled = false;
		bool pasteEnabled = false;
		bool undoEnabled = false;
		bool redoEnabled = false;
		bool selectallEnabled = false;
		if (QLineEdit * le = qobject_cast<QLineEdit*>(now))
		{
			copyEnabled = !le->selectedText().isEmpty();
			cutEnabled = copyEnabled && !le->isReadOnly();
			pasteEnabled = !le->isReadOnly();
			undoEnabled = le->isUndoAvailable();
			redoEnabled = le->isRedoAvailable();
			selectallEnabled = !le->text().isEmpty();
		}
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(now))
		{
			copyEnabled = !pte->textCursor().selection().isEmpty();
			cutEnabled = copyEnabled && !pte->isReadOnly();
			pasteEnabled = !pte->isReadOnly();
			undoEnabled = redoEnabled = pte->isUndoRedoEnabled();
			selectallEnabled = !pte->toPlainText().isEmpty();
		}
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(now))
		{
			copyEnabled = !te->textCursor().selection().isEmpty();
			cutEnabled = copyEnabled && !te->isReadOnly();
			pasteEnabled = !te->isReadOnly();
			undoEnabled = redoEnabled = te->isUndoRedoEnabled();
			selectallEnabled = !(te->toPlainText().isEmpty() && te->toHtml().isEmpty());
		}
		else if (QWebView * wv = qobject_cast<QWebView*>(now))
		{
			copyEnabled = !wv->page()->action(QWebPage::Copy)->isEnabled();
			cutEnabled = !wv->page()->action(QWebPage::Cut)->isEnabled();
			pasteEnabled = !wv->page()->action(QWebPage::Paste)->isEnabled();
			undoEnabled = !wv->page()->action(QWebPage::Undo)->isEnabled();
			redoEnabled = !wv->page()->action(QWebPage::Redo)->isEnabled();
			selectallEnabled = !wv->page()->action(QWebPage::SelectAll)->isEnabled();
		}
		copyAction->setEnabled(copyEnabled);
		cutAction->setEnabled(cutEnabled);
		pasteAction->setEnabled(pasteEnabled);
		undoAction->setEnabled(undoEnabled);
		redoAction->setEnabled(undoEnabled);
		selectallAction->setEnabled(selectallEnabled);
	}
}

void MacIntegrationPlugin::onMinimizeAction()
{
	QWidget * activeWindow = QApplication::activeWindow();
	if (activeWindow)
		activeWindow->showMinimized();
}

void MacIntegrationPlugin::onCloseAction()
{
	QWidget * activeWindow = QApplication::activeWindow();
	if (CustomBorderContainer* border = qobject_cast<CustomBorderContainer*>(activeWindow))
	{
		border->closeWidget();
	}
	else if (activeWindow)
		activeWindow->close();
}

// copy/cut/paste/undo/redo/selectall are handled by widgets, these slots do nothing for now
void MacIntegrationPlugin::onCopyAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->copy();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->copy();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->copy();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::Copy);
	}
}

void MacIntegrationPlugin::onPasteAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->paste();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->paste();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->paste();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::Paste);
	}
}

void MacIntegrationPlugin::onCutAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->cut();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->cut();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->cut();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::Cut);
	}
}

void MacIntegrationPlugin::onUndoAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->undo();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->undo();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->undo();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::Undo);
	}
}

void MacIntegrationPlugin::onRedoAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->redo();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->redo();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->redo();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::Redo);
	}
}

void MacIntegrationPlugin::onSelectAllAction()
{
	if (QWidget * w = QApplication::focusWidget())
	{
		if (QLineEdit * le = qobject_cast<QLineEdit*>(w))
			le->selectAll();
		else if (QPlainTextEdit * pte = qobject_cast<QPlainTextEdit*>(w))
			pte->selectAll();
		else if (QTextEdit * te = qobject_cast<QTextEdit*>(w))
			te->selectAll();
		else if (QWebView * wv = qobject_cast<QWebView*>(w))
			wv->page()->triggerAction(QWebPage::SelectAll);
	}
}

Q_EXPORT_PLUGIN2(plg_macintegration, MacIntegrationPlugin)
