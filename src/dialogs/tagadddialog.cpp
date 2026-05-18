#include "tagadddialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QSizePolicy>

TagAddDialog::TagAddDialog(QWidget* parent) : MasterDialog(parent), _nameEdit(new QLineEdit(this)) {
    setWindowTitle(tr("Add a new tag"));
    setWindowIcon(
        QIcon::fromTheme(QStringLiteral("tag"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/tag.svg"))));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    auto* layout = new QGridLayout(this);
    layout->addWidget(new QLabel(tr("Add new tag"), this), 0, 0);

    _nameEdit->setPlaceholderText(tr("New tag"));
    layout->addWidget(_nameEdit, 1, 0);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox, 2, 0);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    afterSetupUI();
    _nameEdit->setFocus();
    resize(1, 1);
}

TagAddDialog::~TagAddDialog() = default;

QString TagAddDialog::name() {
    return _nameEdit == nullptr ? QString() : _nameEdit->text();
}
