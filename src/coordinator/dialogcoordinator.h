#pragma once

#include <QObject>
#include <QString>

class QWidget;

class DialogCoordinator : public QObject {
    Q_OBJECT

public:
    explicit DialogCoordinator(QObject* parent = nullptr);

    QString openFile(QWidget* parent, const QString& title, const QString& directory = QString(),
                     const QString& filter = QString()) const;
    QString saveFile(QWidget* parent, const QString& title, const QString& directory = QString(),
                     const QString& filter = QString()) const;
    QString selectDirectory(QWidget* parent, const QString& title, const QString& directory = QString()) const;
    bool confirm(QWidget* parent, const QString& title, const QString& message) const;

public slots:
    void showError(QWidget* parent, const QString& title, const QString& message) const;
    void showInformation(QWidget* parent, const QString& title, const QString& message) const;
};
