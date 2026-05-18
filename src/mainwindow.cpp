/*
 * Copyright (c) 2014-2026 Patrizio Bekerle -- <patrizio@bekerle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include "mainwindow.h"

#include <coordinator/actionregistry.h>
#include <coordinator/appcoordinator.h>
#include <core/data/notedata.h>
#include <core/state/appstate.h>
#include <core/state/uistate.h>
#include <dialogs/aboutdialog.h>
#include <dialogs/attachmentdialog.h>
#include <dialogs/commandbar.h>
#include <dialogs/filedialog.h>
#include <dialogs/imagedialog.h>
#include <dialogs/layoutdialog.h>
#include <dialogs/linkdialog.h>
#include <dialogs/localtrashdialog.h>
#include <dialogs/notedialog.h>
#include <dialogs/notediffdialog.h>
#include <dialogs/settingsdialog.h>
#include <dialogs/storedattachmentsdialog.h>
#include <dialogs/storedimagesdialog.h>
#include <dialogs/tabledialog.h>
#include <dialogs/tagadddialog.h>
#include <helpers/arcnotesmarkdownhighlighter.h>
#include <helpers/toolbarcontainer.h>
#include <utils/gui.h>
#include <utils/listutils.h>
#include <utils/misc.h>
#include <utils/schema.h>
#include <viewmodels/navigationviewmodel.h>
#include <viewmodels/noteeditorviewmodel.h>
#include <viewmodels/notefolderviewmodel.h>
#include <viewmodels/notelistviewmodel.h>
#include <viewmodels/notesubfolderviewmodel.h>
#include <viewmodels/searchviewmodel.h>
#include <viewmodels/settingsviewmodel.h>
#include <viewmodels/tagtreeviewmodel.h>
#include <views/logview.h>
#include <views/navigationview.h>
#include <views/noteeditorview.h>
#include <views/notelistview.h>
#include <views/notepreviewview.h>
#include <views/notesubfolderview.h>
#include <views/searchpanelview.h>
#include <views/tagtreeview.h>
#include <widgets/arcnotesmarkdowntextedit.h>
#include <widgets/combobox.h>
#include <widgets/lineedit.h>

#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QPageLayout>
#include <QPageSetupDialog>
#include <QPageSize>
#include <QPlainTextEdit>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QtGlobal>

#include "version.h"

MainWindow::MainWindow(QWidget* parent) : MainWindow(nullptr, parent) {}

MainWindow::MainWindow(AppCoordinator* coordinator, QWidget* parent) : QMainWindow(parent), _coordinator(coordinator) {
    if (_coordinator == nullptr) {
        _ownedCoordinator = std::make_unique<AppCoordinator>(this);
        _coordinator = _ownedCoordinator.get();
        _coordinator->initialize();
    }
    if (_coordinator != nullptr) {
        _coordinator->uiState()->setDistractionFree(
            _coordinator->settingValue(QStringLiteral("DistractionFreeMode/isEnabled")).toBool());
    }

    setupShell();
}

MainWindow::~MainWindow() = default;

bool MainWindow::isInDistractionFreeMode() const {
    return _coordinator != nullptr && _coordinator->uiState()->isDistractionFree();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    storeCurrentLayoutToSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    return QMainWindow::eventFilter(watched, event);
}
