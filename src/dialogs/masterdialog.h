#pragma once

#include <QDialog>
class QKeyEvent;

class MasterDialog : public QDialog {
public:
    explicit MasterDialog(QWidget* parent = nullptr);

public Q_SLOTS:
    int exec() override;

    virtual void show();

    void open() override;

protected:
    bool _ignoreReturnKey = false;

    void resizeEvent(QResizeEvent* event) override;

    [[nodiscard]] QString getGeometrySettingKey() const;

    void storeGeometrySettings() const;

    bool eventFilter(QObject* obj, QEvent* event) override;

    void closeEvent(QCloseEvent* event) override;

    void handleOpenDialog();

    void keyPressEvent(QKeyEvent* keyEvent) override;

    void afterSetupUI();

public:
    void setIgnoreReturnKey(bool ignore);
};
