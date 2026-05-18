#include "attachmentdialog.h"

#include <utils/misc.h>

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QTemporaryFile>
#include <QToolButton>
#include <QUrl>

#include "filedialog.h"

AttachmentDialog::AttachmentDialog(QWidget* parent)
    : MasterDialog(parent), _networkManager(new QNetworkAccessManager(this)) {
    buildUi();
    afterSetupUI();
    _fileEdit->setFocus();
    _downloadButton->hide();
    _downloadFrame->hide();
    _infoFrame->hide();

    connect(_networkManager, &QNetworkAccessManager::finished, this, &AttachmentDialog::slotReplyFinished);

    const QString text = QApplication::clipboard()->text();
    const QUrl url(text);
    const QFile file(text);

    if (url.isValid() && (!url.scheme().isEmpty() || file.exists())) {
        _fileEdit->setText(text);
    }
}

AttachmentDialog::~AttachmentDialog() = default;

void AttachmentDialog::buildUi() {
    setWindowTitle(tr("Insert attachment"));
    resize(580, 231);

    auto* layout = new QGridLayout(this);
    _fileEdit = new QLineEdit(this);
    _fileEdit->setStatusTip(tr("Image filename or URL"));
    _fileEdit->setPlaceholderText(tr("Path to file or URL"));
    _fileEdit->setClearButtonEnabled(true);
    _downloadButton = new QPushButton(this);
    _downloadButton->setToolTip(tr("Download URL"));
    _downloadButton->setIcon(QIcon::fromTheme(
        QStringLiteral("edit-download"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/edit-download.svg"))));
    auto* openButton = new QToolButton(this);
    openButton->setToolTip(tr("Select file"));
    openButton->setText(QStringLiteral("..."));

    _downloadFrame = new QFrame(this);
    auto* downloadLayout = new QHBoxLayout(_downloadFrame);
    downloadLayout->setContentsMargins(0, 0, 0, 0);
    _downloadProgressBar = new QProgressBar(_downloadFrame);
    _downloadProgressBar->setValue(0);
    _downloadSizeLabel = new QLabel(QStringLiteral("0 kB"), _downloadFrame);
    _downloadCancelButton = new QToolButton(_downloadFrame);
    _downloadCancelButton->setToolTip(tr("Cancel download"));
    _downloadCancelButton->setIcon(QIcon::fromTheme(
        QStringLiteral("dialog-cancel"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/dialog-cancel.svg"))));
    downloadLayout->addWidget(_downloadProgressBar);
    downloadLayout->addWidget(_downloadSizeLabel);
    downloadLayout->addWidget(_downloadCancelButton);

    _infoFrame = new QFrame(this);
    auto* infoLayout = new QGridLayout(_infoFrame);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    _infoLabel = new QLabel(QStringLiteral("Info"), _infoFrame);
    infoLayout->addWidget(_infoLabel, 0, 0);

    _titleEdit = new QLineEdit(this);
    _titleEdit->setStatusTip(tr("Title of the image link"));
    _titleEdit->setPlaceholderText(tr("Title"));
    _titleEdit->setClearButtonEnabled(true);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(_fileEdit, 0, 0);
    layout->addWidget(_downloadButton, 0, 1);
    layout->addWidget(openButton, 0, 2);
    layout->addWidget(_downloadFrame, 1, 0, 1, 3);
    layout->addWidget(_infoFrame, 2, 0, 1, 3);
    layout->addWidget(_titleEdit, 3, 0, 1, 3);
    layout->setRowStretch(4, 1);
    layout->addWidget(buttonBox, 5, 0, 1, 3);

    connect(openButton, &QToolButton::clicked, this, &AttachmentDialog::on_openButton_clicked);
    connect(_fileEdit, &QLineEdit::textChanged, this, &AttachmentDialog::on_fileEdit_textChanged);
    connect(_downloadButton, &QPushButton::clicked, this, &AttachmentDialog::on_downloadButton_clicked);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AttachmentDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AttachmentDialog::on_openButton_clicked() {
    FileDialog dialog(QStringLiteral("InsertAttachment"));
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setWindowTitle(tr("Select file to insert"));

    if (dialog.exec() == QDialog::Accepted) {
        const QString fileName = dialog.selectedFile();

        if (!fileName.isEmpty()) {
            _fileEdit->setText(fileName);
        }
    }
}

void AttachmentDialog::on_fileEdit_textChanged(const QString& arg1) {
    QString pathOrUrl = arg1.trimmed();

    if (pathOrUrl.isEmpty()) {
        _infoFrame->hide();
        _downloadButton->hide();
        return;
    }

    const QUrl url(pathOrUrl);
    _infoFrame->hide();

    if (!url.isValid()) {
        _downloadButton->hide();
        return;
    }

    _downloadButton->setVisible(url.scheme().startsWith(QLatin1String("http")));

    if (url.scheme() == QLatin1String("file")) {
        _fileEdit->setText(url.toLocalFile());
    } else if (url.scheme().isEmpty()) {
        _infoFrame->show();
        QFileInfo fileInfo(pathOrUrl);

        QMimeDatabase db;
        QMimeType type = db.mimeTypeForFile(pathOrUrl);

        _infoLabel->setText(Utils::Misc::toHumanReadableByteSize(fileInfo.size()) + QStringLiteral(" - ") +
                            type.comment());
    }
}

void AttachmentDialog::on_downloadButton_clicked() {
    _downloadButton->setDisabled(true);
    _downloadProgressBar->setValue(0);
    _downloadSizeLabel->setText(QString());
    _downloadFrame->show();

    const QUrl url(_fileEdit->text());
    QNetworkRequest networkRequest(url);

#if QT_VERSION < QT_VERSION_CHECK(5, 9, 0)
    networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#else
    networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
#endif

#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    _networkManager->setNetworkAccessible(QNetworkAccessManager::Accessible);
#endif

    QNetworkReply* reply = _networkManager->get(networkRequest);
    connect(reply, &QNetworkReply::downloadProgress, this, &AttachmentDialog::downloadProgress);
    connect(_downloadCancelButton, &QToolButton::clicked, reply, &QNetworkReply::abort);
}

void AttachmentDialog::downloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    _downloadProgressBar->setMaximum(static_cast<int>(bytesTotal / 1000));
    _downloadProgressBar->setValue(static_cast<int>(bytesReceived / 1000));
    _downloadSizeLabel->setText(Utils::Misc::toHumanReadableByteSize(bytesReceived) + QStringLiteral(" / ") +
                                Utils::Misc::toHumanReadableByteSize(bytesTotal));
}

void AttachmentDialog::slotReplyFinished(QNetworkReply* reply) {
    if (reply == nullptr) {
        return;
    }

    reply->deleteLater();
    _downloadFrame->hide();
    _downloadButton->setDisabled(false);

    const QByteArray data = reply->readAll();
    if (reply->error() != QNetworkReply::NoError && reply->error() != QNetworkReply::OperationCanceledError) {
        _accept = false;
        QMessageBox::critical(nullptr, tr("Download error"),
                              tr("Error while downloading:\n%1").arg(reply->errorString()));
        qWarning() << QStringLiteral("Network error: %1").arg(reply->errorString());
        return;
    }

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForData(data);
    QString suffix;

    if (type.isValid()) {
        const QStringList suffixes = type.suffixes();
        if (!suffixes.isEmpty()) {
            suffix = suffixes.constFirst();
        }
    }

    auto* tempFile = new QTemporaryFile(QDir::tempPath() + QStringLiteral("/ArcNotes-XXXXXX.") + suffix);
    tempFile->setAutoRemove(false);

    if (!tempFile->open()) {
        QMessageBox::critical(nullptr, tr("File error"),
                              tr("Could not open temporary file:\n%1").arg(tempFile->errorString()));
        return;
    }

    const QString filePath = tempFile->fileName();
    tempFile->close();
    tempFile->deleteLater();

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(nullptr, tr("File error"),
                              tr("Could not store downloaded file:\n%1").arg(file.errorString()));
        return;
    }

    file.write(data);
    file.close();
    QCoreApplication::processEvents();
    _fileEdit->setText(filePath);

    if (_accept) {
        _accept = false;
        MasterDialog::accept();
    }
}

QFile* AttachmentDialog::getFile() {
    return new QFile(_fileEdit->text());
}

QString AttachmentDialog::getTitle() {
    return _titleEdit->text();
}

void AttachmentDialog::accept() {
    if (QUrl(_fileEdit->text()).scheme().startsWith(QLatin1String("http"))) {
        _accept = true;
        on_downloadButton_clicked();
    } else {
        MasterDialog::accept();
    }
}
