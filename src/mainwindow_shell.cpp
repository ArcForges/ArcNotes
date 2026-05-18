/*
 * Copyright (c) 2014-2026 Patrizio Bekerle -- <patrizio@bekerle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

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
#include <views/noterelationview.h>
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
#include <QHBoxLayout>
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
#include <QPalette>
#include <QPlainTextEdit>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QtGlobal>

#include "mainwindow.h"
#include "version.h"

namespace {
QFrame* plainFrame(const QString& objectName, QWidget* parent) {
    auto* frame = new QFrame(parent);
    frame->setObjectName(objectName);
    frame->setFrameShape(QFrame::NoFrame);
    frame->setFrameShadow(QFrame::Plain);
    return frame;
}

QString searchLineEditStyleSheet() {
    return QStringLiteral(
        "QLineEdit {\n"
        "    border: none;\n"
        "    padding: 2px 5px 2px 27px;\n"
        "    background-image: url(:/images/search-notes.svg);\n"
        "    background-repeat: no-repeat;\n"
        "    background-position: center left;\n"
        "    margin-right: 0px;\n"
        "}\n");
}
}  // namespace

void MainWindow::setupShell() {
    setObjectName(QStringLiteral("MainWindow"));
    resize(1383, 1032);
    setWindowTitle(QStringLiteral("ArcNotes"));
    setWindowIcon(QIcon(QStringLiteral(":/images/icon.png")));

    setupActions();
    setupMenus();
    setupToolBar();
    setupCentralLayout();
    initStyling();
    setupStatusBarWidgets();
    wireViewModels();
    wireActions();
    restoreCurrentLayoutFromSettings();
    applyDockLocking(false);
    applyDistractionFreeMode(isInDistractionFreeMode(), false);

    statusBar()->clearMessage();
}

QDockWidget* MainWindow::createDockPanel(const QString& objectName, const QString& title, QWidget* widget,
                                         Qt::DockWidgetArea area, Qt::Orientation orientation) {
    auto* dock = new QDockWidget(title, this);
    dock->setObjectName(objectName);
    dock->setWidget(widget);
    dock->setContextMenuPolicy(Qt::PreventContextMenu);
    _dockTitleBars.insert(objectName, dock->titleBarWidget());

    QSizePolicy sizePolicy = dock->sizePolicy();
    sizePolicy.setHorizontalStretch(area == Qt::LeftDockWidgetArea ? 2 : 5);
    dock->setSizePolicy(sizePolicy);

    addDockWidget(area, dock, orientation);
    return dock;
}

void MainWindow::setupCentralLayout() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
#endif
    setDockNestingEnabled(true);

    _noteEditTabWidget = new QTabWidget(this);
    _noteEditTabWidget->setObjectName(QStringLiteral("noteEditTabWidget"));
    _noteEditTabWidget->setDocumentMode(true);
    _noteEditTabWidget->setTabBarAutoHide(true);
    _noteEditTabWidget->setTabsClosable(!persistentSetting(QStringLiteral("hideTabCloseButton")).toBool());

    _noteEditorView = new NoteEditorView(_noteEditTabWidget);
    _noteEditorView->setObjectName(QStringLiteral("noteEditorFrame"));
    _noteTextEdit = _noteEditorView->editor();
    _noteTextEdit->setObjectName(QStringLiteral("noteTextEdit"));
    connect(this, &MainWindow::settingsChanged, _noteTextEdit, &ArcNotesMarkdownTextEdit::updateSettings);
    _noteEditTabWidget->addTab(_noteEditorView, tr("Note"));
    setCentralWidget(_noteEditTabWidget);

    auto* subFolderFrame = plainFrame(QStringLiteral("noteSubFolderFrame"), this);
    auto* subFolderLayout = new QVBoxLayout(subFolderFrame);
    subFolderLayout->setObjectName(QStringLiteral("verticalLayout_4"));
    subFolderLayout->setContentsMargins(0, 0, 0, 0);
    subFolderLayout->setSpacing(3);
    _noteSubFolderLineEdit = new LineEdit(subFolderFrame);
    _noteSubFolderLineEdit->setObjectName(QStringLiteral("noteSubFolderLineEdit"));
    _noteSubFolderLineEdit->setStyleSheet(searchLineEditStyleSheet());
    _noteSubFolderLineEdit->setPlaceholderText(tr("Find or create note sub folder"));
    _noteSubFolderLineEdit->setClearButtonEnabled(true);
    _noteSubFolderView = new NoteSubFolderView(subFolderFrame);
    _noteSubFolderView->setObjectName(QStringLiteral("noteSubFolderTreeWidget"));
    subFolderLayout->addWidget(_noteSubFolderLineEdit);
    subFolderLayout->addWidget(_noteSubFolderView);

    auto* tagFrame = plainFrame(QStringLiteral("tagFrame"), this);
    auto* tagLayout = new QVBoxLayout(tagFrame);
    tagLayout->setObjectName(QStringLiteral("verticalLayout_3"));
    tagLayout->setContentsMargins(0, 0, 0, 0);
    tagLayout->setSpacing(3);
    _tagLineEdit = new LineEdit(tagFrame);
    _tagLineEdit->setObjectName(QStringLiteral("tagLineEdit"));
    _tagLineEdit->setStyleSheet(searchLineEditStyleSheet());
    _tagLineEdit->setPlaceholderText(tr("Find or create tag"));
    _tagLineEdit->setClearButtonEnabled(true);
    _tagTreeView = new TagTreeView(tagFrame);
    _tagTreeView->setObjectName(QStringLiteral("tagTreeWidget"));
    tagLayout->addWidget(_tagLineEdit);
    tagLayout->addWidget(_tagTreeView);

    _searchLineEdit = new LineEdit(this);
    _searchLineEdit->setObjectName(QStringLiteral("searchLineEdit"));
    _searchLineEdit->setToolTip(tr("Search for notes or create new notes by entering text and pressing return"));
    _searchLineEdit->setStyleSheet(searchLineEditStyleSheet());
    _searchLineEdit->setPlaceholderText(tr("Search or create note"));
    _searchLineEdit->setClearButtonEnabled(true);

    _noteFolderComboBox = new ComboBox(this);
    _noteFolderComboBox->setObjectName(QStringLiteral("noteFolderComboBox"));
    _noteFolderComboBox->setToolTip(tr("Note folder"));
    _noteFolderComboBox->setMaxVisibleItems(100);

    auto* notesListFrame = plainFrame(QStringLiteral("notesListFrame"), this);
    auto* notesListLayout = new QVBoxLayout(notesListFrame);
    notesListLayout->setObjectName(QStringLiteral("notesListLayout"));
    notesListLayout->setContentsMargins(0, 0, 0, 0);
    notesListLayout->setSpacing(3);
    _noteListView = new NoteListView(notesListFrame);
    _noteListView->setObjectName(QStringLiteral("noteTreeWidget"));
    notesListLayout->addWidget(_noteListView);

    auto* navigationFrame = plainFrame(QStringLiteral("navigationFrame"), this);
    auto* navigationLayout = new QVBoxLayout(navigationFrame);
    navigationLayout->setObjectName(QStringLiteral("verticalLayout_2"));
    navigationLayout->setContentsMargins(0, 0, 0, 0);
    navigationLayout->setSpacing(3);
    _navigationLineEdit = new LineEdit(navigationFrame);
    _navigationLineEdit->setObjectName(QStringLiteral("navigationLineEdit"));
    _navigationLineEdit->setStyleSheet(searchLineEditStyleSheet());
    _navigationLineEdit->setPlaceholderText(tr("Find navigation item"));
    _navigationLineEdit->setClearButtonEnabled(true);
    _navigationTabWidget = new QTabWidget(navigationFrame);
    _navigationTabWidget->setObjectName(QStringLiteral("navigationTabWidget"));
    _navigationTabWidget->setDocumentMode(true);
    _navigationView = new NavigationView(_navigationTabWidget);
    _navigationView->setObjectName(QStringLiteral("navigationWidget"));
    _navigationTabWidget->addTab(_navigationView, tr("Headings"));
    _searchPanelView = new SearchPanelView(_navigationTabWidget);
    _searchPanelView->setObjectName(QStringLiteral("fileNavigationWidget"));
    _navigationTabWidget->addTab(_searchPanelView, tr("Search"));
    navigationLayout->addWidget(_navigationLineEdit);
    navigationLayout->addWidget(_navigationTabWidget);

    auto* noteTagFrame = plainFrame(QStringLiteral("noteTagFrame"), this);
    auto* noteTagLayout = new QHBoxLayout(noteTagFrame);
    noteTagLayout->setContentsMargins(0, 0, 0, 0);
    noteTagLayout->setSpacing(3);
    noteTagLayout->addStretch();
    auto* newNoteTagButton = new QPushButton(tr("Add tag"), noteTagFrame);
    newNoteTagButton->setObjectName(QStringLiteral("newNoteTagButton"));
    newNoteTagButton->setIcon(QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/tag.svg")));
    connect(newNoteTagButton, &QPushButton::clicked, findAction(QStringLiteral("action_new_tag")), &QAction::trigger);
    noteTagLayout->addWidget(newNoteTagButton);

    auto* noteViewFrame = plainFrame(QStringLiteral("noteViewFrame"), this);
    auto* noteViewLayout = new QVBoxLayout(noteViewFrame);
    noteViewLayout->setContentsMargins(0, 0, 0, 0);
    noteViewLayout->setSpacing(0);
    _notePreviewView = new NotePreviewView(noteViewFrame);
    _notePreviewView->setObjectName(QStringLiteral("noteTextView"));
    noteViewLayout->addWidget(_notePreviewView);

    _noteRelationView = new NoteRelationView(this);
    _noteRelationView->setObjectName(QStringLiteral("noteGraphicsView"));

    _logView = new LogView(this);

    _noteSubFolderDockWidget = createDockPanel(QStringLiteral("noteSubFolderDockWidget"), tr("Subfolders"),
                                               subFolderFrame, Qt::LeftDockWidgetArea, Qt::Horizontal);
    _taggingDockWidget = createDockPanel(QStringLiteral("taggingDockWidget"), tr("Tags"), tagFrame,
                                         Qt::LeftDockWidgetArea, Qt::Vertical);
    _noteSearchDockWidget = createDockPanel(QStringLiteral("noteSearchDockWidget"), tr("Note search"), _searchLineEdit,
                                            Qt::LeftDockWidgetArea, Qt::Vertical);
    _noteFolderDockWidget = createDockPanel(QStringLiteral("noteFolderDockWidget"), tr("Note folder"),
                                            _noteFolderComboBox, Qt::LeftDockWidgetArea, Qt::Vertical);
    _noteListDockWidget = createDockPanel(QStringLiteral("noteListDockWidget"), tr("Note list"), notesListFrame,
                                          Qt::LeftDockWidgetArea, Qt::Vertical);
    _noteNavigationDockWidget = createDockPanel(QStringLiteral("noteNavigationDockWidget"), tr("Navigation"),
                                                navigationFrame, Qt::LeftDockWidgetArea, Qt::Vertical);
    splitDockWidget(_noteListDockWidget, _noteNavigationDockWidget, Qt::Vertical);
    _navigationTabWidget->setCurrentIndex(0);

    _noteTagDockWidget = createDockPanel(QStringLiteral("noteTagDockWidget"), tr("Note tags"), noteTagFrame,
                                         Qt::LeftDockWidgetArea, Qt::Vertical);
    _noteTagDockWidget->setMaximumHeight(40);
    QTimer::singleShot(250, this, [this]() {
        if (_noteTagDockWidget != nullptr) {
            _noteTagDockWidget->setMaximumHeight(QWIDGETSIZE_MAX);
        }
    });
    _notePreviewDockWidget = createDockPanel(QStringLiteral("notePreviewDockWidget"), tr("Note preview"), noteViewFrame,
                                             Qt::RightDockWidgetArea, Qt::Horizontal);
    _noteGraphicsViewDockWidget = createDockPanel(QStringLiteral("noteGraphicsViewDockWidget"), tr("Note relations"),
                                                  _noteRelationView, Qt::RightDockWidgetArea, Qt::Horizontal);
    _noteGraphicsViewDockWidget->hide();
    _noteGraphicsViewDockWidget->setMinimumHeight(20);
    _logDockWidget =
        createDockPanel(QStringLiteral("logDockWidget"), tr("Log"), _logView, Qt::RightDockWidgetArea, Qt::Vertical);
    _logDockWidget->hide();
    _logDockWidget->setMinimumHeight(20);
}

void MainWindow::setupStatusBarWidgets() {
    _filePathLabel = new QLabel(statusBar());
    _filePathLabel->setObjectName(QStringLiteral("noteFilePathLabel"));
    _filePathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statusBar()->addWidget(_filePathLabel, 1);

    _cursorPositionLabel = new QLabel(statusBar());
    _cursorPositionLabel->setObjectName(QStringLiteral("cursorPositionLabel"));
    statusBar()->addPermanentWidget(_cursorPositionLabel);
    updateCursorStatus();
}

void MainWindow::wireViewModels() {
    if (_coordinator == nullptr) {
        return;
    }

    _noteListView->setViewModel(_coordinator->noteListViewModel());
    _noteEditorView->setViewModel(_coordinator->noteEditorViewModel());
    _tagTreeView->setViewModel(_coordinator->tagTreeViewModel());
    _navigationView->setViewModel(_coordinator->navigationViewModel());
    _searchPanelView->setViewModel(_coordinator->searchViewModel());
    _noteSubFolderView->setViewModel(_coordinator->noteSubFolderViewModel());

    _noteFolderComboBox->setModel(_coordinator->noteFolderViewModel()->model());
    updateNoteFolderComboBoxVisibility();
    if (QAbstractItemModel* folderModel = _noteFolderComboBox->model()) {
        connect(folderModel, &QAbstractItemModel::rowsInserted, this, &MainWindow::updateNoteFolderComboBoxVisibility);
        connect(folderModel, &QAbstractItemModel::rowsRemoved, this, &MainWindow::updateNoteFolderComboBoxVisibility);
        connect(folderModel, &QAbstractItemModel::modelReset, this, &MainWindow::updateNoteFolderComboBoxVisibility);
    }

    connect(_coordinator->appState(), &AppState::currentNoteChanged, this, &MainWindow::onCurrentNoteChanged);
    connect(_coordinator->noteEditorViewModel(), &NoteEditorViewModel::noteTextChanged, this,
            &MainWindow::updatePreview);
    connect(_coordinator->noteEditorViewModel(), &NoteEditorViewModel::noteTextChanged,
            _coordinator->navigationViewModel(), &NavigationViewModel::parseDocument);
    connect(_coordinator->navigationViewModel(), &NavigationViewModel::headingSelected, this,
            &MainWindow::onNavigationWidgetPositionClicked);
    connect(_noteTextEdit, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateCursorStatus);
    connect(_noteTextEdit, &ArcNotesMarkdownTextEdit::zoomIn, this, &MainWindow::updatePreview);
    connect(_noteTextEdit, &ArcNotesMarkdownTextEdit::zoomOut, this, &MainWindow::updatePreview);
    connect(_notePreviewView, &NotePreviewView::anchorClicked, this, [this](const QUrl& url) {
        if (_noteTextEdit != nullptr) {
            _noteTextEdit->openUrl(url.toString(), false);
        }
    });
    connect(_searchLineEdit, &QLineEdit::textChanged, _coordinator->searchViewModel(), &SearchViewModel::search);
    connect(_searchLineEdit, &QLineEdit::returnPressed, this, [this]() {
        if (_searchLineEdit == nullptr || _searchLineEdit->text().trimmed().isEmpty()) {
            return;
        }
        createNewNote(_searchLineEdit->text().trimmed());
        _searchLineEdit->clear();
    });
    connect(_noteSubFolderLineEdit, &QLineEdit::returnPressed, this, [this]() {
        if (_noteSubFolderLineEdit == nullptr || _noteSubFolderLineEdit->text().trimmed().isEmpty()) {
            return;
        }
        createNewNoteSubFolder(_noteSubFolderLineEdit->text().trimmed());
        _noteSubFolderLineEdit->clear();
    });
    connect(_tagLineEdit, &QLineEdit::returnPressed, this, [this]() {
        if (_tagLineEdit == nullptr || _tagLineEdit->text().trimmed().isEmpty() || _coordinator == nullptr) {
            return;
        }
        _coordinator->tagTreeViewModel()->createTag(_tagLineEdit->text().trimmed());
        _tagLineEdit->clear();
    });
    connect(_noteFolderComboBox, qOverload<int>(&QComboBox::activated), this, [this](int row) {
        if (_coordinator == nullptr || row < 0) {
            return;
        }
        const QModelIndex index = _coordinator->noteFolderViewModel()->model()->index(row, 0);
        const int folderId = index.data(Qt::UserRole + 1).toInt();
        if (folderId > 0) {
            _coordinator->noteFolderViewModel()->switchFolder(folderId);
        }
    });

    if (_coordinator->appState()->currentNote().id > 0) {
        onCurrentNoteChanged(_coordinator->appState()->currentNote());
    }
    if (_noteTextEdit != nullptr) {
        _noteTextEdit->setFocus();
    }
}

void MainWindow::createDefaultNote() {
    if (_coordinator == nullptr) {
        createNewNote(QString(), true);
        return;
    }

    const QString name =
        tr("Note %1").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH'h'mm's'ss")));
    _coordinator->noteListViewModel()->createNote(name, _coordinator->appState()->currentNoteFolder().id,
                                                  _coordinator->noteSubFolderViewModel()->activeSubFolderId());
}

void MainWindow::onCurrentNoteChanged(const NoteData& note) {
    _currentNote = note;
    if (_noteEditTabWidget != nullptr) {
        _noteEditTabWidget->setTabText(0, note.name.isEmpty() ? tr("Note") : note.name);
        if (_noteEditorView != nullptr) {
            _noteEditorView->setProperty("note-id", note.id);
        }
    }
    if (_noteTextEdit != nullptr) {
        _noteTextEdit->setCurrentNoteReference(currentNoteReference(note));
    }
    if (_filePathLabel != nullptr) {
        _filePathLabel->setText(_currentNote.id > 0 ? _currentNote.fullNoteFilePath : note.fileName);
    }
    if (_coordinator != nullptr && _noteListView != nullptr) {
        const int row = _coordinator->noteListViewModel()->model()->rowForNoteId(note.id);
        if (row >= 0) {
            const QModelIndex index = _coordinator->noteListViewModel()->model()->index(row, 0);
            _noteListView->setCurrentIndex(index);
        }
    }
    if (note.name.isEmpty()) {
        setWindowTitle(QStringLiteral("ArcNotes - %1").arg(QStringLiteral(VERSION)));
    } else {
        setWindowTitle(QStringLiteral("%1 - ArcNotes - %2").arg(note.name, QStringLiteral(VERSION)));
    }
    emit currentNoteChanged(_currentNote);
    if (_coordinator != nullptr) {
        _coordinator->navigationViewModel()->parseDocument(note.noteText);
    }
    updatePreview();
    updateNoteBookmarkDisplay();
}

void MainWindow::updateNoteFolderComboBoxVisibility() {
    if (_noteFolderComboBox == nullptr) {
        return;
    }

    const QAbstractItemModel* model = _noteFolderComboBox->model();
    _noteFolderComboBox->setVisible(model != nullptr && model->rowCount() > 1);
}

void MainWindow::updateCursorStatus() {
    if (_cursorPositionLabel == nullptr || _noteTextEdit == nullptr) {
        return;
    }

    const QTextCursor cursor = _noteTextEdit->textCursor();
    _cursorPositionLabel->setText(tr("Ln %1, Col %2").arg(cursor.blockNumber() + 1).arg(cursor.positionInBlock() + 1));
}

void MainWindow::updatePreview() {
    if (_notePreviewView != nullptr && _noteTextEdit != nullptr) {
        const QString text = _noteTextEdit->toPlainText();
        const QString html = _coordinator == nullptr
                                 ? text.toHtmlEscaped()
                                 : _coordinator->renderTextToHtml(_currentNote, text, currentNoteFolderPath(),
                                                                  getMaxImageWidth(), false);
        _notePreviewView->setHtmlPreview(html);
    }
}

void MainWindow::initStyling() {
    const bool darkMode = persistentSetting(QStringLiteral("darkMode")).toBool();
    QString appStyleSheet;
    QString noteTagFrameColorName;

    if (darkMode) {
        QFile file(QStringLiteral(":qdarkstyle/style.qss"));
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream stream(&file);
            appStyleSheet = stream.readAll();
        }
        noteTagFrameColorName = QStringLiteral("#201F1F");
    } else {
        const QPalette palette;
        noteTagFrameColorName = palette.color(QPalette::Base).name();
    }

    const QString foregroundColor =
        Utils::Schema::schemaSettings->getForegroundColor(MarkdownHighlighter::HighlighterState::NoState).name();
    const QString backgroundColor =
        Utils::Schema::schemaSettings->getBackgroundColor(MarkdownHighlighter::HighlighterState::NoState).name();

    appStyleSheet +=
        QStringLiteral("QMarkdownTextEdit{color:%1;background-color:%2;}").arg(foregroundColor, backgroundColor);
    appStyleSheet += QStringLiteral(
                         "QFrame#noteTagFrame, QFrame#noteTagFrame QFrame "
                         "{background-color: %1;}")
                         .arg(noteTagFrameColorName);
    qApp->setStyleSheet(appStyleSheet);

    QTimer::singleShot(1, this, [] { Utils::Gui::updateInterfaceFontSize(); });

    if (_noteTextEdit != nullptr && !isInDistractionFreeMode()) {
        _noteTextEdit->setPaperMargins(0);
    }
    if (_notePreviewView != nullptr) {
        _notePreviewView->updateBackground();
    }

    Utils::Gui::fixDarkModeIcons(this);
}

void MainWindow::restoreCurrentLayoutFromSettings() {
    QString layoutUuid = persistentSetting(QStringLiteral("currentLayout")).toString();
    const QStringList layouts = persistentSetting(QStringLiteral("layouts")).toStringList();
    if (layoutUuid.isEmpty() && !layouts.isEmpty()) {
        layoutUuid = layouts.constFirst();
        setPersistentSetting(QStringLiteral("currentLayout"), layoutUuid);
    }
    if (layoutUuid.isEmpty()) {
        return;
    }

    const QByteArray windowState =
        persistentSetting(QStringLiteral("layout-") + layoutUuid + QStringLiteral("/windowState")).toByteArray();
    if (!windowState.isEmpty()) {
        restoreState(windowState);
    }
    if (_coordinator != nullptr) {
        _coordinator->uiState()->setCurrentLayoutUuid(layoutUuid);
    }
    updateLayoutComboBox();
}

void MainWindow::storeCurrentLayoutToSettings() {
    const QString layoutUuid = persistentSetting(QStringLiteral("currentLayout")).toString();
    if (layoutUuid.isEmpty()) {
        return;
    }
    setPersistentSetting(QStringLiteral("layout-") + layoutUuid + QStringLiteral("/windowState"), saveState());
}

void MainWindow::updateLayoutComboBox() {
    if (_layoutComboBox == nullptr) {
        return;
    }

    const QStringList layouts = persistentSetting(QStringLiteral("layouts")).toStringList();
    const QString currentUuid = persistentSetting(QStringLiteral("currentLayout")).toString();
    const QSignalBlocker blocker(_layoutComboBox);
    Q_UNUSED(blocker)

    _layoutComboBox->clear();
    int currentIndex = -1;
    for (const QString& uuid : layouts) {
        const QString name =
            persistentSetting(QStringLiteral("layout-") + uuid + QStringLiteral("/name"), uuid).toString();
        _layoutComboBox->addItem(name, uuid);
        if (uuid == currentUuid) {
            currentIndex = _layoutComboBox->count() - 1;
        }
    }
    if (currentIndex >= 0) {
        _layoutComboBox->setCurrentIndex(currentIndex);
    }
    _layoutComboBox->setVisible(_layoutComboBox->count() > 0);
}

void MainWindow::restoreDockTitleBars() {
    const QList<QDockWidget*> docks = findChildren<QDockWidget*>();
    for (QDockWidget* dock : docks) {
        dock->setTitleBarWidget(_dockTitleBars.value(dock->objectName(), nullptr));
        if (dock->widget() != nullptr) {
            dock->widget()->setContentsMargins(0, 0, 0, 0);
        }
    }
}

void MainWindow::applyDockLocking(bool unlocked) {
    QAction* unlockAction = findAction(QStringLiteral("actionUnlock_panels"));
    const QSignalBlocker blocker(unlockAction);
    Q_UNUSED(blocker)
    unlockAction->setChecked(unlocked);

    const QList<QDockWidget*> docks = findChildren<QDockWidget*>();
    if (unlocked) {
        restoreDockTitleBars();
    }

    for (QDockWidget* dock : docks) {
        dock->setFeatures(unlocked ? QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                                         QDockWidget::DockWidgetFloatable
                                   : QDockWidget::NoDockWidgetFeatures);
        if (!unlocked && !dock->isFloating()) {
            dock->setTitleBarWidget(new QWidget(dock));
#ifndef Q_OS_MAC
            if (dock->widget() != nullptr) {
                dock->widget()->setContentsMargins(0, 3, 0, 0);
            }
#endif
        }
    }
}

QString MainWindow::currentNoteReference(const NoteData& note) const {
    if (note.id <= 0) {
        return QString();
    }
    QString relativePath = note.fileName;
    if (!note.relativeNoteSubFolderPath.isEmpty()) {
        relativePath = note.relativeNoteSubFolderPath + QStringLiteral("/") + relativePath;
    }
    const int folderId = _coordinator == nullptr ? 0 : _coordinator->appState()->currentNoteFolder().id;
    return QStringLiteral("%1:%2").arg(folderId).arg(relativePath);
}
