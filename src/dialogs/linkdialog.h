#pragma once

#include <core/data/notedata.h>

#include <QNetworkAccessManager>

#include "masterdialog.h"

class LinkDialogHost;
class QCheckBox;
class QListWidget;
class QLineEdit;
class QNetworkReply;
class QProgressBar;
class QPushButton;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QUrl;

class LinkDialog : public MasterDialog {
    Q_OBJECT

public:
    enum LinkDialogPages { TextLinkPage, NoteLinkPage };

    explicit LinkDialog(int page, const QString& dialogTitle = QString(), QWidget* parent = nullptr);
    ~LinkDialog() override;

    QString getSelectedNoteName() const;
    NoteData getSelectedNote() const;
    QString getURL() const;
    void setURL(const QString& text);
    QString getLinkName() const;
    void setLinkName(const QString& text);
    QString getLinkDescription() const;
    static QString getTitleFromHtml(const QString& html);
    QString getSelectedHeading() const;
    bool isWikiLink() const;
    void setCurrentNote(const NoteData& note);

private slots:
    void on_buttonBox_accepted();
    void on_searchLineEdit_textChanged(const QString& arg1);
    void on_notesListWidget_doubleClicked(const QModelIndex& index);
    void on_urlEdit_textChanged(const QString& arg1);
    void addFileUrl(bool relative = false);
    void addDirectoryUrl(bool relative = false);
    void slotReplyFinished(QNetworkReply* reply);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void on_headingSearchLineEdit_textChanged(const QString& arg1);
    void on_notesListWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void on_headingListWidget_doubleClicked(const QModelIndex& index);
    void on_tabWidget_currentChanged(int index);
    void on_refreshButton_clicked();

private:
    int firstVisibleNoteListRow = 0;
    bool eventFilter(QObject* obj, QEvent* event) override;

    NoteData _currentNote;
    QString selectedNoteText;
    QNetworkAccessManager* _networkManager = nullptr;
    LinkDialogHost* _host = nullptr;
    QLineEdit* _urlEdit = nullptr;
    QLineEdit* _linkNameEdit = nullptr;
    QLineEdit* _descriptionEdit = nullptr;
    QCheckBox* _wikiLinkCheckBox = nullptr;
    QTabWidget* _tabWidget = nullptr;
    QWidget* _urlTab = nullptr;
    QWidget* _noteTab = nullptr;
    QLineEdit* _searchLineEdit = nullptr;
    QTreeWidget* _notesListWidget = nullptr;
    QLineEdit* _headingSearchLineEdit = nullptr;
    QListWidget* _headingListWidget = nullptr;
    QProgressBar* _downloadProgressBar = nullptr;
    QPushButton* _fileUrlButton = nullptr;

    void buildUi();
    void setupFileUrlMenu();
    void loadNoteHeadings() const;
    void doAccept();
    void startTitleFetchRequest(const QUrl& url);
};
