#pragma once

#include <QFile>
#include <QNetworkReply>

#include "masterdialog.h"

class QFrame;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QProgressBar;
class QPushButton;
class QToolButton;

class AttachmentDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit AttachmentDialog(QWidget* parent = nullptr);
    ~AttachmentDialog() override;

    QFile* getFile();
    QString getTitle();

public slots:
    void accept() override;

private slots:
    void on_openButton_clicked();
    void on_fileEdit_textChanged(const QString& arg1);
    void on_downloadButton_clicked();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void slotReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* _networkManager = nullptr;
    bool _accept = false;
    QLineEdit* _fileEdit = nullptr;
    QLineEdit* _titleEdit = nullptr;
    QPushButton* _downloadButton = nullptr;
    QFrame* _downloadFrame = nullptr;
    QProgressBar* _downloadProgressBar = nullptr;
    QLabel* _downloadSizeLabel = nullptr;
    QToolButton* _downloadCancelButton = nullptr;
    QFrame* _infoFrame = nullptr;
    QLabel* _infoLabel = nullptr;

    void buildUi();
};
