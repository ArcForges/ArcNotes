#pragma once

#include "masterdialog.h"

class AboutDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog() override;
};
