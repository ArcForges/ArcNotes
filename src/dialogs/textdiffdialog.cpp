#include "textdiffdialog.h"

#include <diff_match_patch.h>
#include <viewmodels/viewmodellocator.h>

#include <QAbstractButton>
#include <QCheckBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSizePolicy>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

TextDiffDialog::TextDiffDialog(QWidget* parent, const QString& title, const QString& labelText, QString text1,
                               const QString& text2, QString identifier)
    : MasterDialog(parent), _text1(std::move(text1)), _identifier(std::move(identifier)) {
    if (!_identifier.isEmpty()) {
        const QString settingsKey = QStringLiteral("MessageBoxOverride/") + _identifier;
        SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel();
        const QVariant storedResult = viewModel == nullptr ? QVariant() : viewModel->persistentSetting(settingsKey);

        if (storedResult.isValid()) {
            _accepted = storedResult.toBool();
            QTimer::singleShot(0, this, &TextDiffDialog::close);
            return;
        }
    }

    setWindowModality(Qt::ApplicationModal);
    setWindowTitle(title.isEmpty() ? tr("Text difference") : title);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("dialog-information"),
                                   QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/dialog-information.svg"))));
    setModal(true);
    resize(949, 613);

    auto* gridLayout = new QGridLayout(this);
    auto* verticalLayout = new QVBoxLayout;
    verticalLayout->setSpacing(6);
    verticalLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    gridLayout->addLayout(verticalLayout, 0, 0);

    auto* label = new QLabel(labelText, this);
    label->setStyleSheet(QStringLiteral("QLabel {font-weight: bold;}"));
    verticalLayout->addWidget(label);

    _plainTextEdit = new QPlainTextEdit(this);
    _plainTextEdit->setPlainText(text2);
    verticalLayout->addWidget(_plainTextEdit);

    verticalLayout->addWidget(new QLabel(tr("Differences:"), this));

    _textEdit = new QTextEdit(this);
    _textEdit->setReadOnly(true);
    verticalLayout->addWidget(_textEdit);

    _dontShowAgainCheckBox = new QCheckBox(tr("Don't show dialog again"), this);
    _dontShowAgainCheckBox->setVisible(!_identifier.isEmpty());
    verticalLayout->addWidget(_dontShowAgainCheckBox);

    verticalLayout->addWidget(new QLabel(tr("Accept change?"), this));

    _buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    _buttonBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _buttonBox->setMinimumSize(200, 0);
    _buttonBox->setMaximumSize(1000, 16777215);
    _buttonBox->setOrientation(Qt::Horizontal);
    verticalLayout->addWidget(_buttonBox);

    connect(_buttonBox, &QDialogButtonBox::clicked, this, &TextDiffDialog::dialogButtonClicked);
    connect(_plainTextEdit, &QPlainTextEdit::textChanged, this, &TextDiffDialog::on_plainTextEdit_textChanged);

    afterSetupUI();
    on_plainTextEdit_textChanged();
}

TextDiffDialog::~TextDiffDialog() = default;

void TextDiffDialog::dialogButtonClicked(QAbstractButton* button) {
    const QDialogButtonBox::ButtonRole buttonRole = _buttonBox->buttonRole(button);
    _accepted = buttonRole == QDialogButtonBox::AcceptRole;

    if (!_identifier.isEmpty() && _dontShowAgainCheckBox->isChecked()) {
        const QString settingsKey = QStringLiteral("MessageBoxOverride/") + _identifier;
        if (SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel()) {
            viewModel->setPersistentSetting(settingsKey, _accepted);
        }
    }

    close();
}

bool TextDiffDialog::resultAccepted() const {
    return _accepted;
}

QString TextDiffDialog::resultText() {
    return _plainTextEdit == nullptr ? QString() : _plainTextEdit->toPlainText();
}

void TextDiffDialog::on_plainTextEdit_textChanged() {
    if (_plainTextEdit == nullptr || _textEdit == nullptr) {
        return;
    }

    diff_match_patch diff;
    const QList<Diff> diffList = diff.diff_main(_text1, _plainTextEdit->toPlainText());
    const QString html = diff.diff_prettyHtml(diffList);
    _textEdit->setHtml(html);
}
