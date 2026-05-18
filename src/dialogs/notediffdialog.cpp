#include "notediffdialog.h"

#include <viewmodels/viewmodellocator.h>

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

NoteDiffDialog::NoteDiffDialog(QWidget* parent, const QString& html) : MasterDialog(parent) {
    setWindowModality(Qt::ApplicationModal);
    setWindowTitle(tr("Note was modified externally!"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("dialog-information"),
                                   QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/dialog-information.svg"))));
    setModal(true);
    resize(949, 613);

    auto* gridLayout = new QGridLayout(this);
    auto* verticalLayout = new QVBoxLayout;
    verticalLayout->setSpacing(6);
    verticalLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    gridLayout->addLayout(verticalLayout, 0, 0);

    auto* modifiedLabel = new QLabel(tr("The current note was modified outside of this application!"), this);
    modifiedLabel->setStyleSheet(QStringLiteral("QLabel {font-weight: bold;}"));
    verticalLayout->addWidget(modifiedLabel);
    verticalLayout->addWidget(new QLabel(tr("Differences:"), this));

    auto* textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setHtml(html);
    verticalLayout->addWidget(textEdit);

    _ignoreAllExternalChangesCheckBox = new QCheckBox(tr("Ignore all external modifications in the future"), this);
    verticalLayout->addWidget(_ignoreAllExternalChangesCheckBox);

    _acceptAllExternalChangesCheckBox = new QCheckBox(tr("Always accept external changes in the future"), this);
    verticalLayout->addWidget(_acceptAllExternalChangesCheckBox);

    verticalLayout->addWidget(new QLabel(tr("Accept external changes?"), this));

    _buttonBox = new QDialogButtonBox(this);
    _buttonBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _buttonBox->setMinimumSize(200, 0);
    _buttonBox->setMaximumSize(1000, 16777215);
    _buttonBox->setOrientation(Qt::Horizontal);
    verticalLayout->addWidget(_buttonBox);

    auto* yesButton = new QPushButton(tr("Yes"), _buttonBox);
    yesButton->setProperty("ActionRole", Reload);
    yesButton->setDefault(false);
    _buttonBox->addButton(yesButton, QDialogButtonBox::ActionRole);

    auto* noButton = new QPushButton(tr("No"), _buttonBox);
    noButton->setProperty("ActionRole", Overwrite);
    noButton->setDefault(false);
    _buttonBox->addButton(noButton, QDialogButtonBox::ActionRole);

    _notificationButtonGroup = new QButtonGroup(this);
    _notificationButtonGroup->addButton(_ignoreAllExternalChangesCheckBox);
    _notificationButtonGroup->addButton(_acceptAllExternalChangesCheckBox);

    _notificationNoneCheckBox = new QCheckBox(this);
    _notificationNoneCheckBox->setHidden(true);
    _notificationButtonGroup->addButton(_notificationNoneCheckBox);

    connect(_notificationButtonGroup, &QButtonGroup::buttonPressed, this,
            &NoteDiffDialog::notificationButtonGroupPressed);
    connect(_buttonBox, &QDialogButtonBox::clicked, this, &NoteDiffDialog::dialogButtonClicked);

    afterSetupUI();
}

NoteDiffDialog::~NoteDiffDialog() = default;

void NoteDiffDialog::notificationButtonGroupPressed(QAbstractButton* button) const {
    if (button->isChecked()) {
        QTimer::singleShot(100, this, &NoteDiffDialog::notificationNoneCheckBoxCheck);
    }
}

void NoteDiffDialog::notificationNoneCheckBoxCheck() {
    _notificationNoneCheckBox->setChecked(true);
}

void NoteDiffDialog::dialogButtonClicked(QAbstractButton* button) {
    _actionRole = button->property("ActionRole").toInt();

    SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel();
    if (_ignoreAllExternalChangesCheckBox->isChecked() && viewModel != nullptr) {
        viewModel->setPersistentSetting(QStringLiteral("ignoreAllExternalModifications"), true);
    }

    if (_acceptAllExternalChangesCheckBox->isChecked() && viewModel != nullptr) {
        viewModel->setPersistentSetting(QStringLiteral("acceptAllExternalModifications"), true);
    }

    close();
}

int NoteDiffDialog::resultActionRole() const {
    return _actionRole;
}
