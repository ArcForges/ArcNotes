#pragma once

#include "masterdialog.h"

class QAbstractButton;
class QCheckBox;
class QDialogButtonBox;
class QPlainTextEdit;
class QTextEdit;

class TextDiffDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit TextDiffDialog(QWidget* parent = nullptr, const QString& title = QString(),
                            const QString& labelText = QString(), QString text1 = QString(),
                            const QString& text2 = QString(), QString identifier = QString());
    ~TextDiffDialog() override;
    [[nodiscard]] bool resultAccepted() const;
    QString resultText();

private slots:
    void dialogButtonClicked(QAbstractButton* button);
    void on_plainTextEdit_textChanged();

private:
    QPlainTextEdit* _plainTextEdit = nullptr;
    QTextEdit* _textEdit = nullptr;
    QCheckBox* _dontShowAgainCheckBox = nullptr;
    QDialogButtonBox* _buttonBox = nullptr;
    bool _accepted = false;
    QString _text1;
    QString _identifier;
};
