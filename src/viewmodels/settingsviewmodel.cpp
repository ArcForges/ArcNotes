#include "settingsviewmodel.h"

#include <core/commands/commandbus.h>
#include <core/data/commands.h>
#include <core/repositories/colormoderepository.h>
#include <core/repositories/settingsrepository.h>

SettingsViewModel::SettingsViewModel(CommandBus* commandBus, SettingsRepository* settingsRepository,
                                     ColorModeRepository* colorModeRepository, QObject* parent)
    : QObject(parent),
      _commandBus(commandBus),
      _settingsRepository(settingsRepository),
      _colorModeRepository(colorModeRepository) {}

QVariantMap SettingsViewModel::generalSettings() const {
    return _generalSettings;
}

QVariantMap SettingsViewModel::interfaceSettings() const {
    return _interfaceSettings;
}

QVariantMap SettingsViewModel::editorSettings() const {
    return _editorSettings;
}

QVariantMap SettingsViewModel::noteFolderSettings() const {
    return _noteFolderSettings;
}

QVariantMap SettingsViewModel::debugSettings() const {
    return _debugSettings;
}

bool SettingsViewModel::containsPersistentSetting(const QString& key) const {
    return _settingsRepository != nullptr && _settingsRepository->contains(key);
}

QStringList SettingsViewModel::persistentSettingKeys() const {
    return _settingsRepository == nullptr ? QStringList() : _settingsRepository->allKeys();
}

QVariant SettingsViewModel::persistentSetting(const QString& key, const QVariant& defaultValue) const {
    return _settingsRepository == nullptr ? defaultValue : _settingsRepository->value(key, defaultValue);
}

QVector<QVariantMap> SettingsViewModel::persistentSettingsArray(const QString& arrayName,
                                                                const QStringList& keys) const {
    return _settingsRepository == nullptr ? QVector<QVariantMap>() : _settingsRepository->arrayValues(arrayName, keys);
}

QList<ColorModeData> SettingsViewModel::colorModes() const {
    return _colorModeRepository == nullptr ? QList<ColorModeData>() : _colorModeRepository->findAll();
}

ColorModeData SettingsViewModel::colorModeById(const QString& id) const {
    return _colorModeRepository == nullptr ? ColorModeData() : _colorModeRepository->findById(id);
}

ColorModeData SettingsViewModel::currentColorMode() const {
    return _colorModeRepository == nullptr ? ColorModeData() : _colorModeRepository->current();
}

QString SettingsViewModel::currentColorModeId() const {
    return _colorModeRepository == nullptr ? QString() : _colorModeRepository->currentId();
}

bool SettingsViewModel::isBuiltInColorModeId(const QString& id) const {
    return _colorModeRepository != nullptr && _colorModeRepository->isBuiltInId(id);
}

QString SettingsViewModel::lightColorModeId() const {
    return _colorModeRepository == nullptr ? QString() : _colorModeRepository->lightModeId();
}

void SettingsViewModel::setGeneralSettings(const QVariantMap& settings) {
    if (_generalSettings == settings) {
        return;
    }
    _generalSettings = settings;
    emit generalSettingsChanged(_generalSettings);
}

void SettingsViewModel::setInterfaceSettings(const QVariantMap& settings) {
    if (_interfaceSettings == settings) {
        return;
    }
    _interfaceSettings = settings;
    emit interfaceSettingsChanged(_interfaceSettings);
}

void SettingsViewModel::setEditorSettings(const QVariantMap& settings) {
    if (_editorSettings == settings) {
        return;
    }
    _editorSettings = settings;
    emit editorSettingsChanged(_editorSettings);
}

void SettingsViewModel::setNoteFolderSettings(const QVariantMap& settings) {
    if (_noteFolderSettings == settings) {
        return;
    }
    _noteFolderSettings = settings;
    emit noteFolderSettingsChanged(_noteFolderSettings);
}

void SettingsViewModel::setDebugSettings(const QVariantMap& settings) {
    if (_debugSettings == settings) {
        return;
    }
    _debugSettings = settings;
    emit debugSettingsChanged(_debugSettings);
}

void SettingsViewModel::setSetting(const QString& group, const QString& key, const QVariant& value) {
    QVariantMap* settings = _settingsForGroup(group);
    if (settings == nullptr || settings->value(key) == value) {
        return;
    }

    settings->insert(key, value);
    emit settingChanged(group, key, value);
    emitGroupChanged(group);
}

void SettingsViewModel::setPersistentSetting(const QString& key, const QVariant& value) {
    if (_commandBus == nullptr) {
        return;
    }

    SetSettingCommand command;
    command.key = key;
    command.value = value;
    _commandBus->dispatch(command);
}

void SettingsViewModel::removePersistentSetting(const QString& key) {
    if (_commandBus == nullptr) {
        return;
    }

    RemoveSettingCommand command;
    command.key = key;
    _commandBus->dispatch(command);
}

void SettingsViewModel::clearPersistentSettings() {
    if (_commandBus == nullptr) {
        return;
    }

    ClearSettingsCommand command;
    _commandBus->dispatch(command);
}

bool SettingsViewModel::reinitializeDatabase() {
    if (_commandBus == nullptr) {
        return false;
    }

    ReinitializeDatabaseCommand command;
    return _commandBus->dispatch(command).success;
}

bool SettingsViewModel::checkDatabaseIntegrity() {
    if (_commandBus == nullptr) {
        return false;
    }

    CheckDatabaseIntegrityCommand command;
    const CommandResult result = _commandBus->dispatch(command);
    return result.success && result.resultData.toBool();
}

void SettingsViewModel::removeDiskDatabase() {
    if (_commandBus == nullptr) {
        return;
    }

    RemoveDiskDatabaseCommand command;
    _commandBus->dispatch(command);
}

void SettingsViewModel::ensureBuiltInColorModes() {
    if (_commandBus == nullptr) {
        return;
    }

    EnsureBuiltInColorModesCommand command;
    _commandBus->dispatch(command);
}

ColorModeData SettingsViewModel::createColorMode(const QString& name) {
    if (_commandBus == nullptr) {
        return ColorModeData();
    }

    CreateColorModeCommand command;
    command.name = name;
    const CommandResult result = _commandBus->dispatch(command);
    return result.success ? result.resultData.value<ColorModeData>() : ColorModeData();
}

bool SettingsViewModel::saveColorMode(const ColorModeData& colorMode) {
    if (_commandBus == nullptr) {
        return false;
    }

    SaveColorModeCommand command;
    command.colorMode = colorMode;
    return _commandBus->dispatch(command).success;
}

bool SettingsViewModel::removeColorMode(const QString& id) {
    if (_commandBus == nullptr) {
        return false;
    }

    RemoveColorModeCommand command;
    command.colorModeId = id;
    return _commandBus->dispatch(command).success;
}

bool SettingsViewModel::setCurrentColorModeId(const QString& id) {
    if (_commandBus == nullptr) {
        return false;
    }

    SetCurrentColorModeCommand command;
    command.colorModeId = id;
    return _commandBus->dispatch(command).success;
}

void SettingsViewModel::writePersistentSettingsArray(const QString& arrayName, const QVector<QVariantMap>& values) {
    if (_commandBus == nullptr) {
        return;
    }

    WriteSettingsArrayCommand command;
    command.arrayName = arrayName;
    command.values = values;
    _commandBus->dispatch(command);
}

QVariantMap* SettingsViewModel::_settingsForGroup(const QString& group) {
    if (group == QStringLiteral("general")) {
        return &_generalSettings;
    }
    if (group == QStringLiteral("interface")) {
        return &_interfaceSettings;
    }
    if (group == QStringLiteral("editor")) {
        return &_editorSettings;
    }
    if (group == QStringLiteral("noteFolder")) {
        return &_noteFolderSettings;
    }
    if (group == QStringLiteral("debug")) {
        return &_debugSettings;
    }
    return nullptr;
}

void SettingsViewModel::emitGroupChanged(const QString& group) {
    if (group == QStringLiteral("general")) {
        emit generalSettingsChanged(_generalSettings);
    } else if (group == QStringLiteral("interface")) {
        emit interfaceSettingsChanged(_interfaceSettings);
    } else if (group == QStringLiteral("editor")) {
        emit editorSettingsChanged(_editorSettings);
    } else if (group == QStringLiteral("noteFolder")) {
        emit noteFolderSettingsChanged(_noteFolderSettings);
    } else if (group == QStringLiteral("debug")) {
        emit debugSettingsChanged(_debugSettings);
    }
}
