#include "uistate.h"

UiState::UiState(QObject* parent) : QObject(parent) {}

QVariantMap UiState::panelVisibility() const {
    return _panelVisibility;
}

bool UiState::isPanelVisible(const QString& panelId) const {
    return _panelVisibility.value(panelId, true).toBool();
}

QString UiState::currentLayoutUuid() const {
    return _currentLayoutUuid;
}

bool UiState::isDistractionFree() const {
    return _distractionFree;
}

bool UiState::isFullScreen() const {
    return _fullScreen;
}

bool UiState::isMenuBarVisible() const {
    return _menuBarVisible;
}

bool UiState::isStatusBarVisible() const {
    return _statusBarVisible;
}

void UiState::setPanelVisibility(const QVariantMap& panelVisibility) {
    if (_panelVisibility == panelVisibility) {
        return;
    }

    _panelVisibility = panelVisibility;
    emit panelVisibilityChanged(_panelVisibility);
}

void UiState::setPanelVisible(const QString& panelId, bool visible) {
    if (_panelVisibility.value(panelId, true).toBool() == visible && _panelVisibility.contains(panelId)) {
        return;
    }

    _panelVisibility.insert(panelId, visible);
    emit panelVisibleChanged(panelId, visible);
    emit panelVisibilityChanged(_panelVisibility);
}

void UiState::setCurrentLayoutUuid(const QString& uuid) {
    if (_currentLayoutUuid == uuid) {
        return;
    }

    _currentLayoutUuid = uuid;
    emit currentLayoutUuidChanged(_currentLayoutUuid);
}

void UiState::setDistractionFree(bool enabled) {
    if (_distractionFree == enabled) {
        return;
    }

    _distractionFree = enabled;
    emit distractionFreeChanged(_distractionFree);
}

void UiState::setFullScreen(bool enabled) {
    if (_fullScreen == enabled) {
        return;
    }

    _fullScreen = enabled;
    emit fullScreenChanged(_fullScreen);
}

void UiState::setMenuBarVisible(bool visible) {
    if (_menuBarVisible == visible) {
        return;
    }

    _menuBarVisible = visible;
    emit menuBarVisibleChanged(_menuBarVisible);
}

void UiState::setStatusBarVisible(bool visible) {
    if (_statusBarVisible == visible) {
        return;
    }

    _statusBarVisible = visible;
    emit statusBarVisibleChanged(_statusBarVisible);
}
