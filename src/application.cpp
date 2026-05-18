#include "application.h"

#include "coordinator/appcoordinator.h"
#include "mainwindow.h"

Application::Application(QObject* parent) : QObject(parent), _coordinator(new AppCoordinator(this)) {}

Application::~Application() = default;

AppCoordinator* Application::coordinator() const {
    return _coordinator.get();
}

MainWindow* Application::mainWindow() const {
    return _mainWindow.data();
}

void Application::initialize() {
    _coordinator->initialize();
}

MainWindow* Application::createMainWindow(QWidget* parent) {
    if (_mainWindow == nullptr) {
        _mainWindow = new MainWindow(_coordinator.get(), parent);
    }
    return _mainWindow.data();
}
