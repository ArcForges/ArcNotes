#pragma once

#include <QHash>
#include <QList>

#include "masterdialog.h"

class QKeySequenceWidget;
class QLineEdit;
class QListWidget;
class QMainWindow;
class QMenu;
class QPushButton;
class QStackedWidget;
class QTextBrowser;
class QTreeWidget;
class QTreeWidgetItem;
class QWidget;
class SettingsViewModel;
class Toolbar_Editor;

class SettingsDialog : public MasterDialog {
    Q_OBJECT

public:
    enum SettingsPages {
        NoteFolderPage,
        InterfacePage,
        ShortcutPage,
        GeneralPage,
        DebugPage,
        EditorFontColorPage,
        PortableModePage,
        PreviewFontPage,
        ToolbarPage,
        DebugOptionPage,
        EditorPage,
        PanelsPage,
        LocalTrashPage,
        LayoutPresetsPage,
        ExperimentalPage,
        ColorModesPage
    };

    explicit SettingsDialog(int page = 0, QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr);
    ~SettingsDialog() override;

    void setCurrentPage(int page);
    void readSettings();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QListWidget* _pageList = nullptr;
    QStackedWidget* _stack = nullptr;
    QList<QWidget*> _settingsPages;
    QMainWindow* _targetWindow = nullptr;
    QLineEdit* _shortcutSearchLineEdit = nullptr;
    QTreeWidget* _shortcutTreeWidget = nullptr;
    QTextBrowser* _portableModeInfoTextBrowser = nullptr;
    Toolbar_Editor* _toolbarEditor = nullptr;
    QPushButton* _applyToolbarButton = nullptr;
    QPushButton* _resetToolbarButton = nullptr;
    QHash<QString, QKeySequenceWidget*> _shortcutWidgetMap;
    QHash<QString, QKeySequenceWidget*> _globalShortcutWidgetMap;
    SettingsViewModel* _settingsViewModel = nullptr;

    void storeSettings();
    void addPage(const QString& title, QWidget* widget);
    QWidget* createShortcutPage();
    QWidget* createPortableModePage();
    QWidget* createToolbarPage();
    QWidget* createLayoutPresetPage();
    void initializeSettingsPages();
    void initializePortableModePage();
    void loadShortcutSettings();
    void buildShortcutTreeForMenu(QMenu* menu, QTreeWidgetItem* parentItem);
    void filterShortcutTree(const QString& filterText);
    void storeShortcutSettings();
    void applyToolbarConfiguration();
    void resetToolbarConfiguration();

private slots:
    void needRestart();
};
