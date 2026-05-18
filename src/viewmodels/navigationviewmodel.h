#pragma once

#include <models/navigationoutlinemodel.h>

#include <QObject>
#include <QString>

class NavigationViewModel : public QObject {
    Q_OBJECT

public:
    explicit NavigationViewModel(QObject* parent = nullptr);

    NavigationOutlineModel* model();
    const NavigationOutlineModel* model() const;

public slots:
    void parseDocument(const QString& text);
    void selectHeading(int position);

signals:
    void headingSelected(int position);

private:
    NavigationOutlineModel _model;
};
