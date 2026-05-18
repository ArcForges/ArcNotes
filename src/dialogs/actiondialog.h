#pragma once

#include "masterdialog.h"

class QLineEdit;
class QMenu;
class QMenuBar;
class QTreeWidget;
class QTreeWidgetItem;

class ActionDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit ActionDialog(QMenuBar* menuBar, QWidget* parent = nullptr);
    ~ActionDialog() override;
    void refreshUi();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QMenuBar* _menuBar = nullptr;
    QLineEdit* _filterEdit = nullptr;
    QTreeWidget* _actionTree = nullptr;

    void buildActionTreeForMenu(QMenu* menu, QTreeWidgetItem* parentItem = nullptr);
};
