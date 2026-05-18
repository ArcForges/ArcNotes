#pragma once

#include "masterdialog.h"

class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QDialogButtonBox;

class NoteDiffDialog : public MasterDialog {
    Q_OBJECT

public:
    enum ButtonRole { Unset, Overwrite, Reload, Ignore, Cancel };

    explicit NoteDiffDialog(QWidget* parent = nullptr, const QString& html = QString());
    ~NoteDiffDialog() override;
    [[nodiscard]] int resultActionRole() const;

private slots:
    void dialogButtonClicked(QAbstractButton* button);
    void notificationButtonGroupPressed(QAbstractButton* button) const;
    void notificationNoneCheckBoxCheck();

private:
    int _actionRole = Unset;
    QDialogButtonBox* _buttonBox = nullptr;
    QCheckBox* _ignoreAllExternalChangesCheckBox = nullptr;
    QCheckBox* _acceptAllExternalChangesCheckBox = nullptr;
    QButtonGroup* _notificationButtonGroup = nullptr;
    QCheckBox* _notificationNoneCheckBox = nullptr;
};
