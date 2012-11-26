/*
 * Copyright (C) 2004-2012 Geometer Plus <contact@geometerplus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <QtGui/QApplication>
#include <QtGui/QPixmap>
#include <QtGui/QImage>
#include <QtGui/QIcon>
#include <QtGui/QToolBar>
#include <QtGui/QMenuBar>
#include <QtGui/QMenu>
#include <QtGui/QToolButton>
#include <QtGui/QLayout>
#include <QtGui/QWheelEvent>
#include <QtGui/QDockWidget>
#include <QtCore/QObjectList>

#include <ZLibrary.h>
#include <ZLFile.h>
#include <ZLPopupData.h>

#include "ZLQtApplicationWindow.h"
#include "../dialogs/ZLQtDialogManager.h"
#include "../view/ZLQtViewWidget.h"
#include "../util/ZLQtKeyUtil.h"

void ZLQtDialogManager::createApplicationWindow(ZLApplication *application) const {
	myApplicationWindow = new ZLQtApplicationWindow(application);
}

ZLQtToolBarAction::ZLQtToolBarAction(ZLQtApplicationWindow *parent, ZLToolbar::AbstractButtonItem &item) : QAction(parent), Id(item.actionId()), myItem(item) {
	static std::string imagePrefix = ZLibrary::ApplicationImageDirectory() + ZLibrary::FileNameDelimiter;
	const QString path = QString::fromUtf8(ZLFile(imagePrefix + myItem.iconName() + ".png").path().c_str());
	QPixmap icon(path);
	setIcon(QIcon(icon));
	QSize size = icon.size();
	QString text = QString::fromUtf8(myItem.tooltip().c_str());
	setText(text);
	setToolTip(text);
	connect(this, SIGNAL(triggered()), this, SLOT(onActivated()));
}

void ZLQtToolBarAction::onActivated() {
	((ZLQtApplicationWindow*)parent())->onButtonPress(myItem);
}

ZLQtApplicationWindow::ZLQtApplicationWindow(ZLApplication *application) :
	ZLDesktopApplicationWindow(application),
	myFullscreenToolBar(0),
	myDocWidget(0),
	myFullScreen(false),
	myWasMaximized(false),
	myCursorIsHyperlink(false) {

	const std::string iconFileName = ZLibrary::ImageDirectory() + ZLibrary::FileNameDelimiter + ZLibrary::ApplicationName() + ".png";
	QPixmap icon(iconFileName.c_str());
	setWindowIcon(icon);

	myWindowToolBar = new QToolBar(this);
	myWindowToolBar->setFocusPolicy(Qt::NoFocus);
	myWindowToolBar->setMovable(false);
	addToolBar(myWindowToolBar);
	myWindowToolBar->setIconSize(QSize(32, 32));

	if (hasFullscreenToolbar()) {
		myFullscreenToolBar = new QToolBar();
		myFullscreenToolBar->setMovable(false);
		myFullscreenToolBar->setIconSize(QSize(32, 32));
		myFullscreenToolBar->hide();
	}

	resize(myWidthOption.value(), myHeightOption.value());
	move(myXOption.value(), myYOption.value());

	menuBar()->hide();
	show();
}

void ZLQtApplicationWindow::initMenu() {
	MenuBuilder(*this).processMenu(application());
}

void ZLQtApplicationWindow::init() {
	ZLDesktopApplicationWindow::init();
	if (application().toolbar(WINDOW_TOOLBAR).items().empty()) {
		myWindowToolBar->hide();
	}
	switch (myWindowStateOption.value()) {
		case NORMAL:
			break;
		case FULLSCREEN:
			setFullscreen(true);
			break;
		case MAXIMIZED:
			showMaximized();
			break;
	}
}

ZLQtApplicationWindow::~ZLQtApplicationWindow() {
	if (isFullscreen()) {
		myWindowStateOption.setValue(FULLSCREEN);
	} else if (isMaximized()) {
		myWindowStateOption.setValue(MAXIMIZED);
	} else {
		myWindowStateOption.setValue(NORMAL);
		QPoint position = pos();
		if (position.x() != -1) {
			myXOption.setValue(position.x());
		}
		if (position.y() != -1) {
			myYOption.setValue(position.y());
		}
		myWidthOption.setValue(width());
		myHeightOption.setValue(height());
	}
	for (std::map<std::string,QAction*>::iterator it = myActions.begin(); it != myActions.end(); ++it) {
		if (it->second != 0) {
			delete it->second;
		}
	}
}

void ZLQtApplicationWindow::setFullscreen(bool fullscreen) {
	if (fullscreen == myFullScreen) {
		return;
	}
	myFullScreen = fullscreen;
	if (myFullScreen) {
		myWasMaximized = isMaximized();
		myWindowToolBar->hide();
		showFullScreen();
		if (myFullscreenToolBar != 0) {
			if (myDocWidget == 0) {
				myDocWidget = new QDockWidget(this);
				myDocWidget->setWidget(myFullscreenToolBar);
				myDocWidget->setFloating(true);
				myDocWidget->setAllowedAreas(Qt::NoDockWidgetArea);
			}
			myDocWidget->show();
			myFullscreenToolBar->show();
			myDocWidget->setMinimumSize(myDocWidget->size());
			myDocWidget->setMaximumSize(myDocWidget->size());
		}
	} else {
		if (!application().toolbar(WINDOW_TOOLBAR).items().empty()) {
			myWindowToolBar->show();
		}
		showNormal();
		if (myWasMaximized) {
			showMaximized();
		}
		if (myDocWidget != 0) {
			//myFullscreenToolBar->hide();
			myDocWidget->hide();
		}
	}
}

bool ZLQtApplicationWindow::isFullscreen() const {
	return myFullScreen;
}

void ZLQtApplicationWindow::keyPressEvent(QKeyEvent *event) {
	application().doActionByKey(ZLQtKeyUtil::keyName(event));
}

void ZLQtApplicationWindow::wheelEvent(QWheelEvent *event) {
	if (event->orientation() == Qt::Vertical) {
		if (event->delta() > 0) {
			application().doActionByKey(ZLApplication::MouseScrollUpKey);
		} else {
			application().doActionByKey(ZLApplication::MouseScrollDownKey);
		}
	}
}

void ZLQtApplicationWindow::closeEvent(QCloseEvent *event) {
	if (application().closeView()) {
		event->accept();
	} else {
		event->ignore();
	}
}

void ZLQtApplicationWindow::addToolbarItem(ZLToolbar::ItemPtr item) {
	QToolBar *tb = toolbar(type(*item));
	QAction *action = 0;

	switch (item->type()) {
		case ZLToolbar::Item::PLAIN_BUTTON:
			action = new ZLQtToolBarAction(this, (ZLToolbar::AbstractButtonItem&)*item);
			tb->addAction(action);
			break;
		case ZLToolbar::Item::MENU_BUTTON:
		{
			ZLToolbar::MenuButtonItem &buttonItem = (ZLToolbar::MenuButtonItem&)*item;
			QToolButton *button = new QToolButton(tb);
			button->setFocusPolicy(Qt::NoFocus);
			button->setDefaultAction(new ZLQtToolBarAction(this, buttonItem));
			button->setMenu(new QMenu(button));
			button->setPopupMode(QToolButton::MenuButtonPopup);
			action = tb->addWidget(button);
			myMenuButtons[&buttonItem] = button;
			shared_ptr<ZLPopupData> popupData = buttonItem.popupData();
			myPopupIdMap[&buttonItem] =
				popupData.isNull() ? (size_t)-1 : (popupData->id() - 1);
			break;
		}
		case ZLToolbar::Item::TEXT_FIELD:
		case ZLToolbar::Item::SEARCH_FIELD:
		{
			ZLToolbar::ParameterItem &textFieldItem =
				(ZLToolbar::ParameterItem&)*item;
			LineEditParameter *para = new LineEditParameter(tb, *this, textFieldItem);
			addVisualParameter(textFieldItem.parameterId(), para);
			action = para->action();
			break;
		}
		case ZLToolbar::Item::SEPARATOR:
			action = tb->addSeparator();
			break;
	}

	if (action != 0) {
		myActions[item->actionId()] = action;
	}
}

ZLQtRunPopupAction::ZLQtRunPopupAction(QObject *parent, shared_ptr<ZLPopupData> data, size_t index) : QAction(parent), myData(data), myIndex(index) {
	setText(QString::fromUtf8(myData->text(myIndex).c_str()));
	connect(this, SIGNAL(triggered()), this, SLOT(onActivated()));
}

ZLQtRunPopupAction::~ZLQtRunPopupAction() {
}

void ZLQtRunPopupAction::onActivated() {
	myData->run(myIndex);
}

void ZLQtApplicationWindow::setToolbarItemState(ZLToolbar::ItemPtr item, bool visible, bool enabled) {
	QAction *action = myActions[item->actionId()];
	if (action != 0) {
		action->setEnabled(enabled);
		action->setVisible(visible);
	}
	switch (item->type()) {
		default:
			break;
		case ZLToolbar::Item::MENU_BUTTON:
		{
			ZLToolbar::MenuButtonItem &buttonItem = (ZLToolbar::MenuButtonItem&)*item;
			shared_ptr<ZLPopupData> data = buttonItem.popupData();
			if (!data.isNull() && (data->id() != myPopupIdMap[&buttonItem])) {
				myPopupIdMap[&buttonItem] = data->id();
				QToolButton *button = myMenuButtons[&buttonItem];
				QMenu *menu = button->menu();
				menu->clear();
				const size_t count = data->count();
				for (size_t i = 0; i < count; ++i) {
					menu->addAction(new ZLQtRunPopupAction(menu, data, i));
				}
			}
			break;
		}
	}
}

ZLViewWidget *ZLQtApplicationWindow::createViewWidget() {
	ZLQtViewWidget *viewWidget = new ZLQtViewWidget(this, &application());
	setCentralWidget(viewWidget->widget());
	viewWidget->widget()->show();
	return viewWidget;
}

void ZLQtApplicationWindow::close() {
	QMainWindow::close();
}

void ZLQtApplicationWindow::refresh() {
	QMetaObject::invokeMethod(this, "onRefresh", Qt::AutoConnection);
}

void ZLQtApplicationWindow::onRefresh() {
	for (std::list<ZLQtAction*>::const_iterator it = myMenuActions.begin(); it != myMenuActions.end(); ++it) {
		ZLQtAction *action = *it;
		action->setVisible(application().isActionVisible(action->Id));
		action->setEnabled(application().isActionEnabled(action->Id));
	}
	refreshToolbar(WINDOW_TOOLBAR);
	refreshToolbar(FULLSCREEN_TOOLBAR);
	qApp->processEvents();
}

void ZLQtApplicationWindow::grabAllKeys(bool) {
}

void ZLQtApplicationWindow::setCaption(const std::string &caption) {
	QMainWindow::setWindowTitle(QString::fromUtf8(caption.c_str()));
}

void ZLQtApplicationWindow::setHyperlinkCursor(bool hyperlink) {
	if (hyperlink == myCursorIsHyperlink) {
		return;
	}
	myCursorIsHyperlink = hyperlink;
	if (hyperlink) {
		myStoredCursor = cursor();
		setCursor(Qt::PointingHandCursor);
	} else {
		setCursor(myStoredCursor);
	}
}

void ZLQtApplicationWindow::setFocusToMainWidget() {
	centralWidget()->setFocus();
}

ZLQtApplicationWindow::MenuBuilder::MenuBuilder(ZLQtApplicationWindow &window) : myWindow(window), myCurrentMenu(0) {
}

void ZLQtApplicationWindow::MenuBuilder::processSubmenuBeforeItems(ZLMenubar::Submenu &submenu) {
	if (!myWindow.menuBar()->isVisible()) {
		myWindow.menuBar()->show();
	}
	myCurrentMenu = new QMenu(QString::fromUtf8(submenu.menuName().c_str()));
	myWindow.menuBar()->addMenu(myCurrentMenu);
}

void ZLQtApplicationWindow::MenuBuilder::processSubmenuAfterItems(ZLMenubar::Submenu &submenu) {
	myCurrentMenu = 0;
}

ZLQtAction::ZLQtAction(ZLApplication &application, const std::string &id, const std::string &title, QObject *parent) : QAction(QString::fromUtf8(title.c_str()), parent), myApplication(application), Id(id) {
	connect(this, SIGNAL(triggered()), this, SLOT(onActivated()));
}

void ZLQtAction::onActivated() {
	myApplication.doAction(Id);
}


void ZLQtApplicationWindow::MenuBuilder::processItem(ZLMenubar::PlainItem &item) {
	ZLQtAction *action = new ZLQtAction(myWindow.application(), item.actionId(), item.name(), myCurrentMenu);
	if (item.actionId() == "showLibrary") {
		action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
	}
	if (item.actionId() == "showNetworkLibrary") {
		action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L));
	}
	if (item.actionId() == "addBook") {
		action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
	}
	if (item.actionId() == "search") {
		action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F));
	}
	if (item.actionId() == "findNext") {
		action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
	}
	if (item.actionId() == "findPrevious") {
		action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
	}
	if (item.actionId() == "toggleFullscreen") {
		action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F));
	}
	if (item.actionId() == "increaseFont") {
		action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus));
	}
	if (item.actionId() == "decreaseFont") {
		action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));
	}
	myCurrentMenu->addAction(action);
	myWindow.addAction(action);
}

void ZLQtApplicationWindow::addAction(ZLQtAction *action) {
	myMenuActions.push_back(action);
}

void ZLQtApplicationWindow::MenuBuilder::processSepartor(ZLMenubar::Separator &separator) {
	myCurrentMenu->addSeparator();
}
