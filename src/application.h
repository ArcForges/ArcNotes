#pragma once

#include <QObject>
#include <QPointer>
#include <memory>

class AppCoordinator;
class MainWindow;
class QWidget;

class Application : public QObject {
    Q_OBJECT

public:
    explicit Application(QObject* parent = nullptr);
    ~Application() override;

    AppCoordinator* coordinator() const;
    MainWindow* mainWindow() const;

public slots:
    void initialize();
    MainWindow* createMainWindow(QWidget* parent = nullptr);

private:
    std::unique_ptr<AppCoordinator> _coordinator;
    QPointer<MainWindow> _mainWindow;
};
