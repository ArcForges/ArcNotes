#pragma once

#include <QString>
#include <QStringList>
#include <utility>
class QToolBar;
class QMainWindow;

/**
 *  This class holds a string definition of toolbars and actions
 *
 *  Separators are stored as empty strings
 */
struct ToolbarContainer {
    QString name;
    QString title;
    QStringList actions;

    ToolbarContainer() = default;
    ToolbarContainer(QString name, QString title, QStringList actions)
        : name(std::move(name)), title(std::move(title)), actions(std::move(actions)) {}
    ToolbarContainer(QToolBar* toolbar);
    QToolBar* create(QMainWindow* w) const;

    void updateToolbar(QMainWindow* mainWindow) const;

    bool toolbarFound(QMainWindow* mainWindow) const;

    static void updateIconSize(QToolBar* toolbar);
};
