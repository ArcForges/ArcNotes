#include "toolbarcontainer.h"

#include <QComboBox>
#include <QDebug>
#include <QMainWindow>
#include <QMenu>
#include <QToolBar>
#include <QWidgetAction>

#include "services/settingsservice.h"

ToolbarContainer::ToolbarContainer(QToolBar* toolbar) : name(toolbar->objectName()), title(toolbar->windowTitle()) {
    foreach (QAction* a, toolbar->actions()) actions.push_back(a->objectName());
}

QToolBar* ToolbarContainer::create(QMainWindow* w) const {
    auto* toolbar = new QToolBar(title, w);
    toolbar->setObjectName(name);

    w->addToolBar(Qt::TopToolBarArea, toolbar);

    foreach (const QString& item, actions) {
        if (item.isEmpty()) {
            toolbar->addSeparator();
        } else {
            auto* act = w->findChild<QAction*>(item);

            if (!act) {
                auto* menu = w->findChild<QMenu*>(item);
                if (menu) {
                    act = menu->menuAction();
                }
            }

            if (act) {
                toolbar->addAction(act);
            } else {
                qWarning() << QObject::tr("Unknown action %1").arg(item);
            }

            updateIconSize(toolbar);
        }
    }

    return toolbar;
}

bool ToolbarContainer::toolbarFound(QMainWindow* mainWindow) const {
    if (mainWindow == nullptr) {
        return false;
    }

    auto* toolbar = mainWindow->findChild<QToolBar*>(name);
    return toolbar != nullptr;
}

void ToolbarContainer::updateToolbar(QMainWindow* mainWindow) const {
    if (mainWindow == nullptr) {
        return;
    }

    auto* toolbar = mainWindow->findChild<QToolBar*>(name);
    if (toolbar == nullptr) {
        return;
    }

    toolbar->clear();

    foreach (const QString& item, actions) {
        if (item.isEmpty()) {
            toolbar->addSeparator();
        } else {
            // TODO(pbek): we will enable that again later
            if (false) {
                //            if (item == "actionLayoutComboBox") {
                qDebug() << __func__ << " - 'actionLayoutComboBox': " << item;

                // TODO(pbek): for some reason we can't find the combobox
                auto* layoutComboBox = mainWindow->findChild<QComboBox*>(QStringLiteral("layoutComboBox"));

                qDebug() << __func__ << " - 'layoutComboBox': " << layoutComboBox;

                auto* widgetAction = mainWindow->findChild<QWidgetAction*>(item);

                qDebug() << __func__ << " - 'widgetAction': " << widgetAction;

                if (widgetAction == nullptr) {
                    widgetAction = new QWidgetAction(mainWindow);
                    widgetAction->setObjectName(QStringLiteral("actionLayoutComboBox"));
                    widgetAction->setText(QObject::tr("Layout selector"));
                }

                widgetAction->setDefaultWidget(layoutComboBox);
                toolbar->addAction(widgetAction);
            } else {
                auto* action = mainWindow->findChild<QAction*>(item);

                if (!action) {
                    auto* menu = mainWindow->findChild<QMenu*>(item);
                    if (menu) {
                        action = menu->menuAction();
                    }
                }

                if (action != nullptr) {
                    toolbar->addAction(action);
                } else {
                    qWarning() << QObject::tr("Unknown action %1").arg(item);
                }
            }

            updateIconSize(toolbar);
        }
    }
}

/**
 * Updates the icon size of a toolbar
 *
 * @param toolbar
 */
void ToolbarContainer::updateIconSize(QToolBar* toolbar) {
    SettingsService settings;
    int toolBarIconSize = settings.value(QStringLiteral("MainWindow/mainToolBar.iconSize")).toInt();
    QSize size(toolBarIconSize, toolBarIconSize);
    toolbar->setIconSize(size);
}
