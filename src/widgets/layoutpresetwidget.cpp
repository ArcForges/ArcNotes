#include "layoutpresetwidget.h"

#include <utils/gui.h>
#include <utils/misc.h>
#include <viewmodels/settingsviewmodel.h>

#include <QColor>
#include <QComboBox>
#include <QDebug>
#include <QFrame>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QVariant>

LayoutPresetWidget::LayoutPresetWidget(QWidget* parent, SettingsViewModel* settingsViewModel)
    : QWidget(parent), _settingsViewModel(settingsViewModel) {
    auto* gridLayout = new QGridLayout(this);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    _layoutPresetComboBox = new QComboBox(this);
    gridLayout->addWidget(_layoutPresetComboBox, 0, 0, 1, 2);

    _layoutPresetDescriptionLabel = new QLabel(tr("Layout preset description"), this);
    _layoutPresetDescriptionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    _layoutPresetDescriptionLabel->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);
    _layoutPresetDescriptionLabel->setWordWrap(true);
    _layoutPresetDescriptionLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
    gridLayout->addWidget(_layoutPresetDescriptionLabel, 1, 0);

    _layoutPresetGraphicsView = new QGraphicsView(this);
    _layoutPresetGraphicsView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _layoutPresetGraphicsView->setFrameShape(QFrame::NoFrame);
    gridLayout->addWidget(_layoutPresetGraphicsView, 1, 1, 3, 1);

    auto* buttonFrame = new QFrame(this);
    buttonFrame->setFrameShape(QFrame::NoFrame);
    buttonFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto* buttonLayout = new QHBoxLayout(buttonFrame);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    _useLayoutPresetPushButton = new QPushButton(tr("Use preset"), buttonFrame);
    _useLayoutPresetPushButton->setIcon(QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/window.svg")));
    buttonLayout->addWidget(_useLayoutPresetPushButton);
    buttonLayout->addStretch();
    gridLayout->addWidget(buttonFrame, 2, 0);

    connect(_layoutPresetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &LayoutPresetWidget::on_layoutPresetComboBox_currentIndexChanged);
    connect(_useLayoutPresetPushButton, &QPushButton::clicked, this,
            &LayoutPresetWidget::on_useLayoutPresetPushButton_clicked);

    _manualSettingsStoring = true;
    loadLayoutPresets();
}

LayoutPresetWidget::~LayoutPresetWidget() {
    delete _layoutPresetSettings;
}

void LayoutPresetWidget::loadLayoutPresets() {
    delete _layoutPresetSettings;
    _layoutPresetSettings = new QSettings(QStringLiteral(":/configurations/layouts.ini"), QSettings::IniFormat);
    const auto layoutPresetIdentifiers =
        _layoutPresetSettings->value(QStringLiteral("LayoutPresetIdentifiers")).toStringList();

    {
        const QSignalBlocker blocker(_layoutPresetComboBox);
        Q_UNUSED(blocker)
        _layoutPresetComboBox->clear();

        for (const QString& layoutPresetIdentifier : layoutPresetIdentifiers) {
            _layoutPresetComboBox->addItem(getLayoutPresetName(layoutPresetIdentifier), layoutPresetIdentifier);
        }
    }

    _layoutPresetComboBox->setCurrentIndex(0);
    on_layoutPresetComboBox_currentIndexChanged(0);
}

void LayoutPresetWidget::on_layoutPresetComboBox_currentIndexChanged(int index) {
    Q_UNUSED(index)
    updateCurrentLayoutPreset();
}

void LayoutPresetWidget::updateCurrentLayoutPreset() {
    const QString layoutPresetIdentifier = _layoutPresetComboBox->currentData().toString();
    const QString layoutPresetSettingsPrefix =
        QStringLiteral("LayoutPreset-") + layoutPresetIdentifier + QStringLiteral("/");
    const QString screenshot =
        _layoutPresetSettings->value(layoutPresetSettingsPrefix + QStringLiteral("screenshot")).toString();
    QString layoutPresetDescription = getLayoutPresetDescription(layoutPresetIdentifier);

    if (_manualSettingsStoring) {
        layoutPresetDescription += QStringLiteral("\n\n") +
                                   tr("Keep in mind that layouts that demand that there is no central widget will not "
                                      "work properly if the central widget is enabled.");
    }

    _layoutPresetDescriptionLabel->setText(layoutPresetDescription);

    delete _layoutPresetGraphicsView->scene();
    auto* scene = new QGraphicsScene(_layoutPresetGraphicsView);

    const QColor bg = _layoutPresetGraphicsView->palette().window().color();
    _layoutPresetGraphicsView->setStyleSheet(QStringLiteral("background-color:") + bg.name(QColor::HexArgb));

    const QString filePath = QStringLiteral(":/images/layouts/") + screenshot;
    scene->addPixmap(QPixmap(filePath));
    _layoutPresetGraphicsView->setScene(scene);
    _layoutPresetGraphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void LayoutPresetWidget::storeLayoutPreset() {
    if (_manualSettingsStoring) {
        const QString title = tr("Use new layout preset");
        const QString text = tr("Do you want to use the selected layout preset?");

        if (Utils::Gui::question(this, title, text, QStringLiteral("layoutpresetwidget-use-layout-preset"),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
            return;
        }
    }

    const QString layoutPresetIdentifier = _layoutPresetComboBox->currentData().toString();
    const QString layoutPresetSettingsPrefix =
        QStringLiteral("LayoutPreset-") + layoutPresetIdentifier + QStringLiteral("/");
    QStringList layouts = settingValue(QStringLiteral("layouts")).toStringList();
    const QString layoutUuid =
        _manualSettingsStoring ? Utils::Misc::generateRandomString(12) : QStringLiteral("initial");

    if (!layouts.contains(layoutUuid)) {
        layouts << layoutUuid;
        setSettingValue(QStringLiteral("layouts"), layouts);
    }

    setSettingValue(QStringLiteral("initialLayoutPresetIdentifier"), layoutPresetIdentifier);

    if (!_manualSettingsStoring) {
        setSettingValue(
            QStringLiteral("noteEditIsCentralWidget"),
            _layoutPresetSettings->value(layoutPresetSettingsPrefix + QStringLiteral("noteEditIsCentralWidget")));
    }

    setSettingValue(
        QStringLiteral("layout-") + layoutUuid + QStringLiteral("/noteEditIsCentralWidget"),
        _layoutPresetSettings->value(layoutPresetSettingsPrefix + QStringLiteral("noteEditIsCentralWidget")));
    setSettingValue(QStringLiteral("layout-") + layoutUuid + QStringLiteral("/windowState"),
                    _layoutPresetSettings->value(layoutPresetSettingsPrefix + QStringLiteral("windowState")));
    setSettingValue(QStringLiteral("layout-") + layoutUuid + QStringLiteral("/name"),
                    getLayoutPresetName(layoutPresetIdentifier));
    setSettingValue(
        QStringLiteral("layout-") + layoutUuid + QStringLiteral("/noteSubFolderDockWidgetVisible"),
        _layoutPresetSettings->value(layoutPresetSettingsPrefix + QStringLiteral("noteSubFolderDockWidgetVisible")));

    setSettingValue(QStringLiteral("initialLayout"), true);

    emit layoutStored(layoutUuid);
}

void LayoutPresetWidget::setManualSettingsStoring(bool enabled) {
    _manualSettingsStoring = enabled;
    _useLayoutPresetPushButton->setVisible(enabled);

    if (!enabled) {
        updateCurrentLayoutPreset();
    }
}

QString LayoutPresetWidget::getLayoutPresetName(const QString& layoutPresetIdentifier) {
    if (layoutPresetIdentifier == QLatin1String("minimal")) {
        return tr("Minimal", "Layout preset name");
    }
    if (layoutPresetIdentifier == QLatin1String("full")) {
        return tr("Full", "Layout preset name");
    }
    if (layoutPresetIdentifier == QLatin1String("preview-only")) {
        return tr("Preview only", "Layout preset name");
    }
    if (layoutPresetIdentifier == QLatin1String("full-vertical")) {
        return tr("Full vertical", "Layout preset name");
    }
    if (layoutPresetIdentifier == QLatin1String("1col")) {
        return tr("Single column", "Layout preset name");
    }

    return QString();
}

QString LayoutPresetWidget::getLayoutPresetDescription(const QString& layoutPresetIdentifier) {
    const QString& centralWidgetAddText =
        QStringLiteral(" ") + tr("The note edit panel is the central widget that will be "
                                 "resized automatically.",
                                 "Layout preset description");

    const QString& noCentralWidgetAddText =
        QStringLiteral(" ") + tr("Because of this there is no central widget that will be "
                                 "resized automatically.",
                                 "Layout preset description");

    if (layoutPresetIdentifier == QLatin1String("minimal")) {
        return tr("Just the note list on the left and the note edit panel "
                  "on the right are enabled by default.",
                  "Layout preset description") +
               centralWidgetAddText;
    }
    if (layoutPresetIdentifier == QLatin1String("full")) {
        return tr("Most of the panels, like the note list on the left, the "
                  "tagging panels, the note edit panel in the center and the "
                  "preview panel on the right are enabled by default.",
                  "Layout preset description") +
               centralWidgetAddText;
    }
    if (layoutPresetIdentifier == QLatin1String("preview-only")) {
        return tr("Most of the panels, like the note list on the left, the "
                  "tagging panels, and only the preview panel on the right "
                  "are enabled by default. You will need another layout to "
                  "actually edit notes!",
                  "Layout preset description") +
               noCentralWidgetAddText;
    }
    if (layoutPresetIdentifier == QLatin1String("full-vertical")) {
        return tr("Most of the panels, like the note list on the left, the "
                  "tagging panels, the note edit panel on the right and the "
                  "preview panel on top of the note edit panel are enabled by "
                  "default.",
                  "Layout preset description") +
               noCentralWidgetAddText;
    }
    if (layoutPresetIdentifier == QLatin1String("1col")) {
        return tr("Tiny one column layout with note search, note list and note "
                  "edit on top of each other.",
                  "Layout preset description") +
               centralWidgetAddText;
    }

    return QString();
}

void LayoutPresetWidget::resizeEvent(QResizeEvent* event) {
    resizeLayoutPresetImage();
    QWidget::resizeEvent(event);
}

void LayoutPresetWidget::resizeLayoutPresetImage() const {
    if (_layoutPresetGraphicsView->scene() != nullptr) {
        _layoutPresetGraphicsView->fitInView(_layoutPresetGraphicsView->scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}

void LayoutPresetWidget::on_useLayoutPresetPushButton_clicked() {
    storeLayoutPreset();
}

QVariant LayoutPresetWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void LayoutPresetWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}
