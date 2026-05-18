#pragma once

#include <core/data/colormodedata.h>

#include <QList>
#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QVector>

class CommandBus;
class ColorModeRepository;
class SettingsRepository;

class SettingsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap generalSettings READ generalSettings WRITE setGeneralSettings NOTIFY generalSettingsChanged)
    Q_PROPERTY(
        QVariantMap interfaceSettings READ interfaceSettings WRITE setInterfaceSettings NOTIFY interfaceSettingsChanged)
    Q_PROPERTY(QVariantMap editorSettings READ editorSettings WRITE setEditorSettings NOTIFY editorSettingsChanged)
    Q_PROPERTY(QVariantMap noteFolderSettings READ noteFolderSettings WRITE setNoteFolderSettings NOTIFY
                   noteFolderSettingsChanged)
    Q_PROPERTY(QVariantMap debugSettings READ debugSettings WRITE setDebugSettings NOTIFY debugSettingsChanged)

public:
    explicit SettingsViewModel(CommandBus* commandBus = nullptr, SettingsRepository* settingsRepository = nullptr,
                               ColorModeRepository* colorModeRepository = nullptr, QObject* parent = nullptr);

    QVariantMap generalSettings() const;
    QVariantMap interfaceSettings() const;
    QVariantMap editorSettings() const;
    QVariantMap noteFolderSettings() const;
    QVariantMap debugSettings() const;
    bool containsPersistentSetting(const QString& key) const;
    QStringList persistentSettingKeys() const;
    QVariant persistentSetting(const QString& key, const QVariant& defaultValue = QVariant()) const;
    QVector<QVariantMap> persistentSettingsArray(const QString& arrayName, const QStringList& keys) const;
    QList<ColorModeData> colorModes() const;
    ColorModeData colorModeById(const QString& id) const;
    ColorModeData currentColorMode() const;
    QString currentColorModeId() const;
    bool isBuiltInColorModeId(const QString& id) const;
    QString lightColorModeId() const;

public slots:
    void setGeneralSettings(const QVariantMap& settings);
    void setInterfaceSettings(const QVariantMap& settings);
    void setEditorSettings(const QVariantMap& settings);
    void setNoteFolderSettings(const QVariantMap& settings);
    void setDebugSettings(const QVariantMap& settings);
    void setSetting(const QString& group, const QString& key, const QVariant& value);
    void setPersistentSetting(const QString& key, const QVariant& value);
    void removePersistentSetting(const QString& key);
    void clearPersistentSettings();
    bool reinitializeDatabase();
    bool checkDatabaseIntegrity();
    void removeDiskDatabase();
    void ensureBuiltInColorModes();
    ColorModeData createColorMode(const QString& name);
    bool saveColorMode(const ColorModeData& colorMode);
    bool removeColorMode(const QString& id);
    bool setCurrentColorModeId(const QString& id);
    void writePersistentSettingsArray(const QString& arrayName, const QVector<QVariantMap>& values);

signals:
    void generalSettingsChanged(const QVariantMap& settings);
    void interfaceSettingsChanged(const QVariantMap& settings);
    void editorSettingsChanged(const QVariantMap& settings);
    void noteFolderSettingsChanged(const QVariantMap& settings);
    void debugSettingsChanged(const QVariantMap& settings);
    void settingChanged(const QString& group, const QString& key, const QVariant& value);

private:
    QVariantMap* _settingsForGroup(const QString& group);
    void emitGroupChanged(const QString& group);

    CommandBus* _commandBus = nullptr;
    SettingsRepository* _settingsRepository = nullptr;
    ColorModeRepository* _colorModeRepository = nullptr;
    QVariantMap _generalSettings;
    QVariantMap _interfaceSettings;
    QVariantMap _editorSettings;
    QVariantMap _noteFolderSettings;
    QVariantMap _debugSettings;
};
