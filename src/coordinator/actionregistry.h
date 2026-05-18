#pragma once

#include <QHash>
#include <QIcon>
#include <QKeySequence>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

class QAction;

struct ActionDefinition {
    QString id;
    QString text;
    QString category;
    QKeySequence shortcut;
    QIcon icon;
};

class ActionRegistry : public QObject {
    Q_OBJECT

public:
    explicit ActionRegistry(QObject* parent = nullptr);

    QAction* registerAction(const QString& id, const QString& text, const QString& category,
                            const QKeySequence& shortcut = QKeySequence(), const QIcon& icon = QIcon());
    QAction* action(const QString& id) const;
    QList<QAction*> actionsForCategory(const QString& category) const;
    QStringList actionIds() const;
    QStringList categories() const;
    QKeySequence shortcut(const QString& id) const;

public slots:
    void registerDefaultActions();
    void setShortcut(const QString& id, const QKeySequence& shortcut);

signals:
    void actionRegistered(const QString& id, QAction* action);
    void shortcutChanged(const QString& id, const QKeySequence& shortcut);

private:
    QHash<QString, QAction*> _actionsById;
    QHash<QString, QString> _categoriesById;
};
