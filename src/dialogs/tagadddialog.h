#pragma once

#include "masterdialog.h"

class QLineEdit;

class TagAddDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit TagAddDialog(QWidget* parent = nullptr);
    ~TagAddDialog() override;
    QString name();

private:
    QLineEdit* _nameEdit = nullptr;
};
