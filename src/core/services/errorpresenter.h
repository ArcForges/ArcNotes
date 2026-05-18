#pragma once

#include <QObject>
#include <QString>

class ErrorPresenter : public QObject {
    Q_OBJECT

public:
    enum class Severity { Info, Warning, Error };
    Q_ENUM(Severity)

    explicit ErrorPresenter(QObject* parent = nullptr);
    void present(const QString& title, const QString& message, Severity severity = Severity::Error);

signals:
    void errorOccurred(const QString& title, const QString& message, ErrorPresenter::Severity severity);
};
