#pragma once

#include "settingsviewmodel.h"

class NoteFolderSettingsViewModel;

class ViewModelLocator {
public:
    static void setSettingsViewModel(SettingsViewModel* viewModel);
    static SettingsViewModel* settingsViewModel();
    static void setNoteFolderSettingsViewModel(NoteFolderSettingsViewModel* viewModel);
    static NoteFolderSettingsViewModel* noteFolderSettingsViewModel();
};
