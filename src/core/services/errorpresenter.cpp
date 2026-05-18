#include "errorpresenter.h"

ErrorPresenter::ErrorPresenter(QObject* parent) : QObject(parent) {}

void ErrorPresenter::present(const QString& title, const QString& message, Severity severity) {
    emit errorOccurred(title, message, severity);
}
