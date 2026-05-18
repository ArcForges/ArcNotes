#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class UiState : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap panelVisibility READ panelVisibility WRITE setPanelVisibility NOTIFY panelVisibilityChanged)
    Q_PROPERTY(
        QString currentLayoutUuid READ currentLayoutUuid WRITE setCurrentLayoutUuid NOTIFY currentLayoutUuidChanged)
    Q_PROPERTY(bool isDistractionFree READ isDistractionFree WRITE setDistractionFree NOTIFY distractionFreeChanged)
    Q_PROPERTY(bool isFullScreen READ isFullScreen WRITE setFullScreen NOTIFY fullScreenChanged)
    Q_PROPERTY(bool isMenuBarVisible READ isMenuBarVisible WRITE setMenuBarVisible NOTIFY menuBarVisibleChanged)
    Q_PROPERTY(bool isStatusBarVisible READ isStatusBarVisible WRITE setStatusBarVisible NOTIFY statusBarVisibleChanged)

public:
    explicit UiState(QObject* parent = nullptr);

    QVariantMap panelVisibility() const;
    bool isPanelVisible(const QString& panelId) const;
    QString currentLayoutUuid() const;
    bool isDistractionFree() const;
    bool isFullScreen() const;
    bool isMenuBarVisible() const;
    bool isStatusBarVisible() const;

public slots:
    void setPanelVisibility(const QVariantMap& panelVisibility);
    void setPanelVisible(const QString& panelId, bool visible);
    void setCurrentLayoutUuid(const QString& uuid);
    void setDistractionFree(bool enabled);
    void setFullScreen(bool enabled);
    void setMenuBarVisible(bool visible);
    void setStatusBarVisible(bool visible);

signals:
    void panelVisibilityChanged(const QVariantMap& panelVisibility);
    void panelVisibleChanged(const QString& panelId, bool visible);
    void currentLayoutUuidChanged(const QString& uuid);
    void distractionFreeChanged(bool enabled);
    void fullScreenChanged(bool enabled);
    void menuBarVisibleChanged(bool visible);
    void statusBarVisibleChanged(bool visible);

private:
    QVariantMap _panelVisibility;
    QString _currentLayoutUuid;
    bool _distractionFree = false;
    bool _fullScreen = false;
    bool _menuBarVisible = true;
    bool _statusBarVisible = true;
};
