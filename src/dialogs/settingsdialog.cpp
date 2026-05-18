#include "settingsdialog.h"

#include <helpers/toolbarcontainer.h>
#include <libraries/qkeysequencewidget/qkeysequencewidget/src/qkeysequencewidget.h>
#include <viewmodels/settingsviewmodel.h>
#include <viewmodels/viewmodellocator.h>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <functional>
#include <libraries/qttoolbareditor/src/toolbar_editor.hpp>
#include <utility>

#include "utils/misc.h"
#include "widgets/layoutpresetwidget.h"
#include "widgets/settings/colormodesettingswidget.h"
#include "widgets/settings/debugoptionsettingswidget.h"
#include "widgets/settings/debugsettingswidget.h"
#include "widgets/settings/editorfontcolorsettingswidget.h"
#include "widgets/settings/editorsettingswidget.h"
#include "widgets/settings/generalsettingswidget.h"
#include "widgets/settings/interfacesettingswidget.h"
#include "widgets/settings/localtrashsettingswidget.h"
#include "widgets/settings/notefoldersettingswidget.h"
#include "widgets/settings/panelssettingswidget.h"
#include "widgets/settings/previewfontsettingswidget.h"

SettingsDialog::SettingsDialog(int page, QWidget* parent, SettingsViewModel* settingsViewModel)
    : MasterDialog(parent),
      _pageList(new QListWidget(this)),
      _stack(new QStackedWidget(this)),
      _settingsViewModel(settingsViewModel) {
    _targetWindow = qobject_cast<QMainWindow*>(parent);
    setWindowTitle(tr("Settings"));
    resize(900, 640);

    auto* rootLayout = new QVBoxLayout(this);
    auto* contentLayout = new QHBoxLayout();
    rootLayout->addLayout(contentLayout);

    _pageList->setMinimumWidth(220);
    contentLayout->addWidget(_pageList);
    contentLayout->addWidget(_stack, 1);

    addPage(tr("Note folders"), new NoteFolderSettingsWidget(this, ViewModelLocator::noteFolderSettingsViewModel()));
    addPage(tr("Interface"), new InterfaceSettingsWidget(this, _settingsViewModel));
    addPage(tr("Shortcuts"), createShortcutPage());
    addPage(tr("General"), new GeneralSettingsWidget(this, _settingsViewModel));
    addPage(tr("Debug"), new DebugSettingsWidget(this, _settingsViewModel));
    addPage(tr("Editor fonts and colors"), new EditorFontColorSettingsWidget(this, _settingsViewModel));
    addPage(tr("Portable mode"), createPortableModePage());
    addPage(tr("Preview fonts"), new PreviewFontSettingsWidget(this, _settingsViewModel));
    addPage(tr("Toolbar"), createToolbarPage());
    addPage(tr("Debug options"), new DebugOptionSettingsWidget(this, _settingsViewModel));
    addPage(tr("Editor"), new EditorSettingsWidget(this, _settingsViewModel));
    addPage(tr("Panels"),
            new PanelsSettingsWidget(this, _settingsViewModel, ViewModelLocator::noteFolderSettingsViewModel()));
    addPage(tr("Local trash"), new LocalTrashSettingsWidget(this, _settingsViewModel));
    addPage(tr("Layout presets"), createLayoutPresetPage());
    addPage(tr("Experimental"), new QWidget(this));
    addPage(tr("Color modes"), new ColorModeSettingsWidget(this, _settingsViewModel));

    connect(_pageList, &QListWidget::currentRowChanged, _stack, &QStackedWidget::setCurrentIndex);
    connect(_pageList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row == ShortcutPage && _shortcutTreeWidget->topLevelItemCount() == 0) {
            loadShortcutSettings();
        } else if (row == PortableModePage) {
            initializePortableModePage();
        } else if (row == LayoutPresetsPage) {
            if (auto* widget = findChild<LayoutPresetWidget*>()) {
                widget->resizeLayoutPresetImage();
            }
        } else if (row == DebugPage) {
            if (auto* widget = findChild<DebugSettingsWidget*>()) {
                widget->outputSettings();
            }
        }
    });
    for (QWidget* settingsPage : std::as_const(_settingsPages)) {
        if (auto* panelsWidget = qobject_cast<PanelsSettingsWidget*>(settingsPage)) {
            connect(panelsWidget, &PanelsSettingsWidget::needRestart, this, &SettingsDialog::needRestart);
        } else if (auto* interfaceWidget = qobject_cast<InterfaceSettingsWidget*>(settingsPage)) {
            connect(interfaceWidget, &InterfaceSettingsWidget::needRestart, this, &SettingsDialog::needRestart);
        } else if (auto* generalWidget = qobject_cast<GeneralSettingsWidget*>(settingsPage)) {
            connect(generalWidget, &GeneralSettingsWidget::needRestart, this, &SettingsDialog::needRestart);
        } else if (auto* editorFontWidget = qobject_cast<EditorFontColorSettingsWidget*>(settingsPage)) {
            connect(editorFontWidget, &EditorFontColorSettingsWidget::needRestart, this, &SettingsDialog::needRestart);
        } else if (auto* noteFolderWidget = qobject_cast<NoteFolderSettingsWidget*>(settingsPage)) {
            connect(noteFolderWidget, &NoteFolderSettingsWidget::storeSettingsRequested, this,
                    &SettingsDialog::storeSettings);
        }
    }

    auto* interfaceSettingsWidget = findChild<InterfaceSettingsWidget*>();
    auto* generalSettingsWidget = findChild<GeneralSettingsWidget*>();
    if (interfaceSettingsWidget != nullptr && generalSettingsWidget != nullptr) {
        connect(interfaceSettingsWidget, &InterfaceSettingsWidget::systemTrayToggled, generalSettingsWidget,
                &GeneralSettingsWidget::setAllowOnlyOneAppInstance);
    }

    auto* buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
        storeSettings();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::storeSettings);
    rootLayout->addWidget(buttonBox);

    initializeSettingsPages();
    readSettings();
    setCurrentPage(page);
    afterSetupUI();
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::setCurrentPage(int page) {
    if (_pageList == nullptr || _stack == nullptr || _settingsPages.isEmpty()) {
        return;
    }

    const int index = page < 0 || page >= _settingsPages.size() ? 0 : page;
    _pageList->setCurrentRow(index);
    _stack->setCurrentIndex(index);
}

void SettingsDialog::readSettings() {
    for (QWidget* settingsPage : std::as_const(_settingsPages)) {
        if (auto* generalWidget = qobject_cast<GeneralSettingsWidget*>(settingsPage)) {
            generalWidget->readSettings();
        } else if (auto* interfaceWidget = qobject_cast<InterfaceSettingsWidget*>(settingsPage)) {
            interfaceWidget->readSettings();
        } else if (auto* noteFolderWidget = qobject_cast<NoteFolderSettingsWidget*>(settingsPage)) {
            noteFolderWidget->readSettings();
        } else if (auto* panelsWidget = qobject_cast<PanelsSettingsWidget*>(settingsPage)) {
            panelsWidget->readSettings();
        } else if (auto* localTrashWidget = qobject_cast<LocalTrashSettingsWidget*>(settingsPage)) {
            localTrashWidget->readSettings();
        } else if (auto* editorWidget = qobject_cast<EditorSettingsWidget*>(settingsPage)) {
            editorWidget->readSettings();
        } else if (auto* debugOptionsWidget = qobject_cast<DebugOptionSettingsWidget*>(settingsPage)) {
            debugOptionsWidget->readSettings();
        } else if (auto* editorFontWidget = qobject_cast<EditorFontColorSettingsWidget*>(settingsPage)) {
            editorFontWidget->readSettings();
        } else if (auto* previewFontWidget = qobject_cast<PreviewFontSettingsWidget*>(settingsPage)) {
            previewFontWidget->readSettings();
        }
    }
}

void SettingsDialog::closeEvent(QCloseEvent* event) {
    storeSettings();
    MasterDialog::closeEvent(event);
}

void SettingsDialog::storeSettings() {
    for (QWidget* settingsPage : std::as_const(_settingsPages)) {
        if (auto* generalWidget = qobject_cast<GeneralSettingsWidget*>(settingsPage)) {
            generalWidget->storeSettings();
        } else if (auto* interfaceWidget = qobject_cast<InterfaceSettingsWidget*>(settingsPage)) {
            interfaceWidget->storeSettings();
        } else if (auto* noteFolderWidget = qobject_cast<NoteFolderSettingsWidget*>(settingsPage)) {
            noteFolderWidget->storeSettings();
        } else if (auto* panelsWidget = qobject_cast<PanelsSettingsWidget*>(settingsPage)) {
            panelsWidget->storeSettings();
        } else if (auto* localTrashWidget = qobject_cast<LocalTrashSettingsWidget*>(settingsPage)) {
            localTrashWidget->storeSettings();
        } else if (auto* editorWidget = qobject_cast<EditorSettingsWidget*>(settingsPage)) {
            editorWidget->storeSettings();
        } else if (auto* debugOptionsWidget = qobject_cast<DebugOptionSettingsWidget*>(settingsPage)) {
            debugOptionsWidget->storeSettings();
        } else if (auto* editorFontWidget = qobject_cast<EditorFontColorSettingsWidget*>(settingsPage)) {
            editorFontWidget->storeSettings();
        } else if (auto* previewFontWidget = qobject_cast<PreviewFontSettingsWidget*>(settingsPage)) {
            previewFontWidget->storeSettings();
        }
    }

    storeShortcutSettings();
    applyToolbarConfiguration();
}

void SettingsDialog::addPage(const QString& title, QWidget* widget) {
    if (_pageList == nullptr || _stack == nullptr || widget == nullptr) {
        return;
    }

    _settingsPages.append(widget);
    _pageList->addItem(title);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(widget);
    _stack->addWidget(scrollArea);
}

QWidget* SettingsDialog::createShortcutPage() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    _shortcutSearchLineEdit = new QLineEdit(widget);
    _shortcutSearchLineEdit->setPlaceholderText(tr("Search shortcuts"));
    _shortcutSearchLineEdit->setClearButtonEnabled(true);
    layout->addWidget(_shortcutSearchLineEdit);

    auto* progressBar = new QProgressBar(widget);
    progressBar->setVisible(false);
    progressBar->setTextVisible(false);
    layout->addWidget(progressBar);

    _shortcutTreeWidget = new QTreeWidget(widget);
    _shortcutTreeWidget->setColumnCount(3);
    _shortcutTreeWidget->setHeaderLabels({tr("Action"), tr("Local shortcut"), tr("Global shortcut")});
    _shortcutTreeWidget->setRootIsDecorated(true);
    _shortcutTreeWidget->setAlternatingRowColors(true);
    layout->addWidget(_shortcutTreeWidget, 1);

    connect(_shortcutSearchLineEdit, &QLineEdit::textChanged, this, &SettingsDialog::filterShortcutTree);

    return widget;
}

QWidget* SettingsDialog::createPortableModePage() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    _portableModeInfoTextBrowser = new QTextBrowser(widget);
    _portableModeInfoTextBrowser->setStyleSheet(
        QStringLiteral("* {\n"
                       "    background-color: transparent;\n"
                       "    border: none;\n"
                       "}"));
    _portableModeInfoTextBrowser->setOpenExternalLinks(true);
    layout->addWidget(_portableModeInfoTextBrowser);
    return widget;
}

QWidget* SettingsDialog::createToolbarPage() {
    auto* widget = new QWidget(this);
    auto* layout = new QGridLayout(widget);

    _toolbarEditor = new Toolbar_Editor(widget);
    layout->addWidget(_toolbarEditor, 0, 0, 1, 4);

    _applyToolbarButton = new QPushButton(tr("Apply toolbar configuration"), widget);
    _applyToolbarButton->setIcon(QIcon::fromTheme(
        QStringLiteral("document-save"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/document-save.svg"))));
    layout->addWidget(_applyToolbarButton, 1, 1);

    _resetToolbarButton = new QPushButton(tr("Reset toolbars"), widget);
    _resetToolbarButton->setIcon(QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/window-close.svg")));
    layout->addWidget(_resetToolbarButton, 1, 2);
    layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 3);

    connect(_applyToolbarButton, &QPushButton::clicked, this, &SettingsDialog::applyToolbarConfiguration);
    connect(_resetToolbarButton, &QPushButton::clicked, this, &SettingsDialog::resetToolbarConfiguration);

    return widget;
}

QWidget* SettingsDialog::createLayoutPresetPage() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);

    auto* groupBox = new QGroupBox(tr("Layout preset selector"), widget);
    auto* groupLayout = new QVBoxLayout(groupBox);
    layout->addWidget(groupBox);

    auto* firstLabel = new QLabel(tr("You can create a new layout from a preset here. Keep in mind that you always can "
                                     "modify the position of the panels with the <i>Unlock panels</i> button."),
                                  groupBox);
    firstLabel->setWordWrap(true);
    groupLayout->addWidget(firstLabel);

    auto* secondLabel =
        new QLabel(tr("Panels can be turned on and off in the <i>Window / Panels</i> main menu and you "
                      "can also configure different <i>Layouts</i> for different panel configurations."),
                   groupBox);
    secondLabel->setWordWrap(true);
    groupLayout->addWidget(secondLabel);

    auto* layoutPresetWidget = new LayoutPresetWidget(groupBox, _settingsViewModel);
    layoutPresetWidget->setMinimumSize(0, 320);
    layoutPresetWidget->setMaximumSize(16777215, 800);
    groupLayout->addWidget(layoutPresetWidget);
    connect(layoutPresetWidget, &LayoutPresetWidget::layoutStored, this, &SettingsDialog::needRestart);

    layout->addStretch();
    return widget;
}

void SettingsDialog::initializePortableModePage() {
    if (_portableModeInfoTextBrowser == nullptr) {
        return;
    }

    const bool isInPortableMode = Utils::Misc::isInPortableMode();
    const QString status = isInPortableMode ? tr("enabled") : tr("disabled");

    QString text = QStringLiteral("<p>") + tr("Portable mode is currently:") + QStringLiteral(" <strong>") + status +
                   QStringLiteral("</strong></p>");

    text += tr("In portable mode") + QStringLiteral(":<ul><li>") +
            tr("the internal sqlite database and the settings will be stored inside a "
               "<code>Data</code> folder at the binary's location") +
            QStringLiteral("</li><li>") + tr("the settings will be stored in an ini file") +
            QStringLiteral("</li><li>") +
            tr("the note folders and path to an external editor will be automatically stored "
               "relative to the <code>Data</code> folder so that the correct note folders and "
               "external editor will be loaded regardless where your ArcNotes installation is "
               "currently located") +
            QStringLiteral("</li></ul>");

    if (!isInPortableMode) {
        text += QStringLiteral("<p>") +
                tr("It will be activated if you run ArcNotes with the parameter "
                   "<code>--portable</code>.") +
                QStringLiteral("</p>");

#ifdef Q_OS_WIN32
        text += QStringLiteral("<p>") +
                tr("You will find a <code>ArcNotesPortable.bat</code> in your release path to "
                   "start ArcNotes in portable mode.") +
                QStringLiteral("</p>");
#endif
    }

    _portableModeInfoTextBrowser->document()->setDefaultStyleSheet(Utils::Misc::genericCSS());
    _portableModeInfoTextBrowser->setHtml(text);
}

void SettingsDialog::loadShortcutSettings() {
    if (_shortcutTreeWidget == nullptr || _targetWindow == nullptr || _targetWindow->menuBar() == nullptr) {
        return;
    }

    _shortcutTreeWidget->clear();
    _shortcutWidgetMap.clear();
    _globalShortcutWidgetMap.clear();

    const QList<QMenu*> menus = _targetWindow->menuBar()->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
    for (QMenu* menu : menus) {
        buildShortcutTreeForMenu(menu, nullptr);
    }

    _shortcutTreeWidget->expandAll();
    _shortcutTreeWidget->resizeColumnToContents(0);
    _shortcutTreeWidget->resizeColumnToContents(1);
    _shortcutTreeWidget->resizeColumnToContents(2);
}

void SettingsDialog::buildShortcutTreeForMenu(QMenu* menu, QTreeWidgetItem* parentItem) {
    if (menu == nullptr) {
        return;
    }

    auto* menuItem = new QTreeWidgetItem();
    menuItem->setText(0, menu->title().remove(QStringLiteral("&")));
    menuItem->setToolTip(0, menu->objectName());

    if (parentItem == nullptr) {
        _shortcutTreeWidget->addTopLevelItem(menuItem);
    } else {
        parentItem->addChild(menuItem);
    }

    const QIcon clearButtonIcon =
        QIcon::fromTheme(QStringLiteral("edit-clear"), QIcon(QStringLiteral(":/icons/"
                                                                            "breeze-arcnotes/"
                                                                            "16x16/edit-clear.svg")));

    for (QAction* action : menu->actions()) {
        if (action->menu() != nullptr) {
            buildShortcutTreeForMenu(action->menu(), menuItem);
            continue;
        }

        const QString actionObjectName = action->objectName();
        if (actionObjectName.isEmpty() || action->isSeparator()) {
            continue;
        }

        auto* actionItem = new QTreeWidgetItem();
        actionItem->setText(0, action->text().remove(QStringLiteral("&")));
        actionItem->setData(0, Qt::UserRole, actionObjectName);
        menuItem->addChild(actionItem);

        const QString shortcutSettingKey = QStringLiteral("Shortcuts/MainWindow-") + actionObjectName;
        auto* keyWidget = new QKeySequenceWidget();
        keyWidget->setFixedWidth(240);
        keyWidget->setClearButtonIcon(clearButtonIcon);
        keyWidget->setNoneText(tr("Undefined shortcut"));
        keyWidget->setToolTip(tr("Assign a new shortcut"), tr("Reset to default shortcut"));
        keyWidget->setDefaultKeySequence(action->shortcut());
        keyWidget->setKeySequence(
            _settingsViewModel != nullptr && _settingsViewModel->containsPersistentSetting(shortcutSettingKey)
                ? QKeySequence(_settingsViewModel->persistentSetting(shortcutSettingKey).toString())
                : action->shortcut());
        _shortcutTreeWidget->setItemWidget(actionItem, 1, keyWidget);
        _shortcutWidgetMap[actionObjectName] = keyWidget;

        const QString globalShortcutSettingKey = QStringLiteral("GlobalShortcuts/MainWindow-") + actionObjectName;
        auto* globalShortcutKeyWidget = new QKeySequenceWidget();
        globalShortcutKeyWidget->setFixedWidth(240);
        globalShortcutKeyWidget->setClearButtonIcon(clearButtonIcon);
        globalShortcutKeyWidget->setNoneText(tr("Undefined shortcut"));
        globalShortcutKeyWidget->setToolTip(tr("Assign a new shortcut"), tr("Reset to default shortcut"));
        globalShortcutKeyWidget->setKeySequence(
            QKeySequence(_settingsViewModel == nullptr
                             ? QString()
                             : _settingsViewModel->persistentSetting(globalShortcutSettingKey).toString()));
        _shortcutTreeWidget->setItemWidget(actionItem, 2, globalShortcutKeyWidget);
        _globalShortcutWidgetMap[actionObjectName] = globalShortcutKeyWidget;
    }

    if (menuItem->childCount() == 0) {
        delete menuItem;
    }
}

void SettingsDialog::filterShortcutTree(const QString& filterText) {
    if (_shortcutTreeWidget == nullptr) {
        return;
    }

    std::function<bool(QTreeWidgetItem*)> filterItem = [&](QTreeWidgetItem* item) {
        bool visible = filterText.isEmpty() || item->text(0).contains(filterText, Qt::CaseInsensitive) ||
                       item->data(0, Qt::UserRole).toString().contains(filterText, Qt::CaseInsensitive);
        for (int i = 0; i < item->childCount(); ++i) {
            visible = filterItem(item->child(i)) || visible;
        }
        item->setHidden(!visible);
        if (!filterText.isEmpty() && visible) {
            item->setExpanded(true);
        }
        return visible;
    };

    for (int i = 0; i < _shortcutTreeWidget->topLevelItemCount(); ++i) {
        filterItem(_shortcutTreeWidget->topLevelItem(i));
    }
}

void SettingsDialog::storeShortcutSettings() {
    if (_targetWindow == nullptr) {
        return;
    }

    auto findActionByObjectName = [this](const QString& objectName) -> QAction* {
        std::function<QAction*(const QList<QAction*>&)> findInActions =
            [&](const QList<QAction*>& actions) -> QAction* {
            for (QAction* action : actions) {
                if (action->objectName() == objectName) {
                    return action;
                }
                if (action->menu() != nullptr) {
                    if (QAction* nestedAction = findInActions(action->menu()->actions())) {
                        return nestedAction;
                    }
                }
            }
            return nullptr;
        };

        if (_targetWindow->menuBar() == nullptr) {
            return nullptr;
        }
        return findInActions(_targetWindow->menuBar()->actions());
    };

    for (auto it = _shortcutWidgetMap.cbegin(); it != _shortcutWidgetMap.cend(); ++it) {
        QAction* action = findActionByObjectName(it.key());
        const QKeySequence shortcut = it.value()->keySequence();
        if (_settingsViewModel != nullptr) {
            _settingsViewModel->setPersistentSetting(QStringLiteral("Shortcuts/MainWindow-") + it.key(),
                                                     shortcut.toString());
        }
        if (action != nullptr) {
            action->setShortcut(shortcut);
        }
    }

    for (auto it = _globalShortcutWidgetMap.cbegin(); it != _globalShortcutWidgetMap.cend(); ++it) {
        if (_settingsViewModel != nullptr) {
            _settingsViewModel->setPersistentSetting(QStringLiteral("GlobalShortcuts/MainWindow-") + it.key(),
                                                     it.value()->keySequence().toString());
        }
    }
}

void SettingsDialog::applyToolbarConfiguration() {
    if (_toolbarEditor == nullptr || _targetWindow == nullptr) {
        return;
    }

    _toolbarEditor->apply();
    const QStringList toolbarObjectNames = _toolbarEditor->toolbarObjectNames();

    QList<ToolbarContainer> toolbarContainers;
    for (QToolBar* toolbar : _targetWindow->findChildren<QToolBar*>()) {
        if (!toolbarObjectNames.contains(toolbar->objectName())) {
            continue;
        }

        toolbarContainers.append(toolbar);
        ToolbarContainer::updateIconSize(toolbar);
    }

    if (toolbarContainers.isEmpty()) {
        return;
    }

    QVector<QVariantMap> toolbarValues;
    toolbarValues.reserve(toolbarContainers.size());
    for (const ToolbarContainer& toolbarContainer : toolbarContainers) {
        QVariantMap toolbarValue;
        toolbarValue.insert(QStringLiteral("name"), toolbarContainer.name);
        toolbarValue.insert(QStringLiteral("title"), toolbarContainer.title);
        toolbarValue.insert(QStringLiteral("items"), toolbarContainer.actions);
        toolbarValues.append(toolbarValue);
    }

    if (_settingsViewModel != nullptr) {
        _settingsViewModel->writePersistentSettingsArray(QStringLiteral("toolbar"), toolbarValues);
    }
}

void SettingsDialog::resetToolbarConfiguration() {
    if (QMessageBox::question(this, tr("Reset toolbars and exit"),
                              tr("Do you really want to reset all toolbars? "
                                 "The application will be closed in the process, the "
                                 "default toolbars will be restored when you start it again."),
                              QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes) {
        return;
    }

    if (_settingsViewModel != nullptr) {
        _settingsViewModel->removePersistentSetting(QStringLiteral("toolbar"));
    }

    qApp->quit();
}

void SettingsDialog::initializeSettingsPages() {
    for (QWidget* settingsPage : std::as_const(_settingsPages)) {
        if (auto* generalWidget = qobject_cast<GeneralSettingsWidget*>(settingsPage)) {
            generalWidget->initialize();
        } else if (auto* interfaceWidget = qobject_cast<InterfaceSettingsWidget*>(settingsPage)) {
            interfaceWidget->initialize();
        } else if (auto* noteFolderWidget = qobject_cast<NoteFolderSettingsWidget*>(settingsPage)) {
            noteFolderWidget->initialize();
        } else if (auto* panelsWidget = qobject_cast<PanelsSettingsWidget*>(settingsPage)) {
            panelsWidget->initialize();
        } else if (auto* editorWidget = qobject_cast<EditorSettingsWidget*>(settingsPage)) {
            editorWidget->initialize();
        } else if (auto* debugOptionsWidget = qobject_cast<DebugOptionSettingsWidget*>(settingsPage)) {
            debugOptionsWidget->initialize();
        } else if (auto* colorModeWidget = qobject_cast<ColorModeSettingsWidget*>(settingsPage)) {
            colorModeWidget->initialize();
        }
    }

    initializePortableModePage();

    if (_toolbarEditor != nullptr && _targetWindow != nullptr) {
        _toolbarEditor->setTargetWindow(_targetWindow);
        _toolbarEditor->setCustomToolbarRemovalOnly(true);
        _toolbarEditor->setDisabledToolbarNames(QStringList() << QStringLiteral("windowToolbar"));
        _toolbarEditor->setDisabledMenuNames(QStringList() << QStringLiteral("noteFoldersMenu"));
        _toolbarEditor->updateBars();
    }
}

void SettingsDialog::needRestart() {
    Utils::Misc::needRestart();
}
