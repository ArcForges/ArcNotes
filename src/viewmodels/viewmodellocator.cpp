#include "viewmodellocator.h"

#include <QCoreApplication>
#include <QVariant>

#include "notefoldersettingsviewmodel.h"

namespace {
constexpr auto SettingsViewModelProperty = "settingsViewModel";
constexpr auto NoteFolderSettingsViewModelProperty = "noteFolderSettingsViewModel";
}  // namespace

void ViewModelLocator::setSettingsViewModel(SettingsViewModel* viewModel) {
    if (QCoreApplication::instance() == nullptr) {
        return;
    }

    QCoreApplication::instance()->setProperty(SettingsViewModelProperty, QVariant::fromValue<QObject*>(viewModel));
}

SettingsViewModel* ViewModelLocator::settingsViewModel() {
    if (QCoreApplication::instance() == nullptr) {
        return nullptr;
    }

    auto* object = QCoreApplication::instance()->property(SettingsViewModelProperty).value<QObject*>();
    return qobject_cast<SettingsViewModel*>(object);
}

void ViewModelLocator::setNoteFolderSettingsViewModel(NoteFolderSettingsViewModel* viewModel) {
    if (QCoreApplication::instance() == nullptr) {
        return;
    }

    QCoreApplication::instance()->setProperty(NoteFolderSettingsViewModelProperty,
                                              QVariant::fromValue<QObject*>(viewModel));
}

NoteFolderSettingsViewModel* ViewModelLocator::noteFolderSettingsViewModel() {
    if (QCoreApplication::instance() == nullptr) {
        return nullptr;
    }

    auto* object = QCoreApplication::instance()->property(NoteFolderSettingsViewModelProperty).value<QObject*>();
    return qobject_cast<NoteFolderSettingsViewModel*>(object);
}
