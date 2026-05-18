#include "dialogcoordinator.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QWidget>

DialogCoordinator::DialogCoordinator(QObject* parent) : QObject(parent) {}

QString DialogCoordinator::openFile(QWidget* parent, const QString& title, const QString& directory,
                                    const QString& filter) const {
    return QFileDialog::getOpenFileName(parent, title, directory, filter);
}

QString DialogCoordinator::saveFile(QWidget* parent, const QString& title, const QString& directory,
                                    const QString& filter) const {
    return QFileDialog::getSaveFileName(parent, title, directory, filter);
}

QString DialogCoordinator::selectDirectory(QWidget* parent, const QString& title, const QString& directory) const {
    return QFileDialog::getExistingDirectory(parent, title, directory);
}

bool DialogCoordinator::confirm(QWidget* parent, const QString& title, const QString& message) const {
    return QMessageBox::question(parent, title, message) == QMessageBox::Yes;
}

void DialogCoordinator::showError(QWidget* parent, const QString& title, const QString& message) const {
    QMessageBox::critical(parent, title, message);
}

void DialogCoordinator::showInformation(QWidget* parent, const QString& title, const QString& message) const {
    QMessageBox::information(parent, title, message);
}
