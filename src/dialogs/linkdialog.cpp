#include "linkdialog.h"

#include <utils/gui.h>
#include <utils/misc.h>
#include <widgets/arcnotesmarkdowntextedit.h>
#include <widgets/navigationwidget.h>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHeaderView>
#include <QIcon>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTabWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <memory>

#include "linkdialoghost.h"

namespace {
enum NoteListDataRoles {
    NoteIdRole = Qt::UserRole,
    NoteNameRole,
    NoteModifiedRole,
};

QString noteTagLookupKey(const QString& subFolderPath, const QString& noteName) {
    return subFolderPath + QStringLiteral("/") + noteName;
}

class NoteListTreeWidgetItem : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;

    bool operator<(const QTreeWidgetItem& other) const override {
        if (treeWidget() != nullptr && treeWidget()->sortColumn() == 3) {
            const QDateTime thisModified = data(3, NoteModifiedRole).toDateTime();
            const QDateTime otherModified = other.data(3, NoteModifiedRole).toDateTime();
            return thisModified < otherModified;
        }

        return QTreeWidgetItem::operator<(other);
    }
};
}  // namespace

LinkDialog::LinkDialog(int page, const QString& dialogTitle, QWidget* parent)
    : MasterDialog(parent),
      _networkManager(new QNetworkAccessManager(this)),
      _host(dynamic_cast<LinkDialogHost*>(parent)) {
    buildUi();
    afterSetupUI();
    _tabWidget->setCurrentIndex(page);
    on_tabWidget_currentChanged(page);
    _downloadProgressBar->hide();

    connect(_networkManager, &QNetworkAccessManager::finished, this, &LinkDialog::slotReplyFinished);

    _wikiLinkCheckBox->setVisible(_host != nullptr && _host->linkDialogWikiLinkSupportEnabled());
    _linkNameEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(R"([^\]]*)"), this));

    if (!dialogTitle.isEmpty()) {
        setWindowTitle(dialogTitle);
    }

    _searchLineEdit->installEventFilter(this);
    _headingSearchLineEdit->installEventFilter(this);
    _notesListWidget->installEventFilter(this);
    _notesListWidget->setRootIsDecorated(false);

    const QString searchIconFileName = _host != nullptr && _host->linkDialogDarkModeColors()
                                           ? QStringLiteral("search-notes-dark.svg")
                                           : QStringLiteral("search-notes.svg");
    const QString searchStyle = QStringLiteral(
        "QLineEdit { border: 1px solid lightgray; border-radius: 5px; padding: 2px 5px 2px "
        "25px; background-image: url(:/images/%1); background-repeat: no-repeat; "
        "background-position: center left; margin-right: 0px; }");
    _searchLineEdit->setStyleSheet(searchStyle.arg(searchIconFileName));
    _headingSearchLineEdit->setStyleSheet(searchStyle.arg(searchIconFileName));

    const bool showSubfolders = _host != nullptr && _host->linkDialogShowSubfolders();
    _notesListWidget->setColumnHidden(1, !showSubfolders);
    _notesListWidget->setSortingEnabled(true);
    _notesListWidget->sortByColumn(3, Qt::DescendingOrder);
    _notesListWidget->header()->setSortIndicatorShown(true);
    _notesListWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    _notesListWidget->header()->setSectionResizeMode(1, QHeaderView::Interactive);
    _notesListWidget->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    _notesListWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    _notesListWidget->header()->setStretchLastSection(false);
    _notesListWidget->setColumnWidth(1, 180);
    _notesListWidget->setColumnWidth(2, 180);
    Utils::Gui::initTreeWidgetHeaderOrderPersistence(_notesListWidget,
                                                     QStringLiteral("LinkDialog/notesListWidgetHeaderOrder"));

    const auto tagNamesByNoteFilePath =
        _host == nullptr ? QHash<QString, QStringList>() : _host->linkDialogTagNamesByNoteFilePath();
    const QVector<NoteData> notes = _host == nullptr ? QVector<NoteData>() : _host->linkDialogAllNotes();
    for (const NoteData& note : notes) {
        const QString noteName = note.name;
        const QString subFolderPath = note.relativeNoteSubFolderPath;
        const QString tagText =
            tagNamesByNoteFilePath.value(noteTagLookupKey(subFolderPath, noteName)).join(QStringLiteral(", "));
        const QDateTime modified = note.fileLastModified;
        const QString modifiedDisplay = QLocale().toString(modified, QLocale::ShortFormat);

        auto* item = new NoteListTreeWidgetItem(_notesListWidget);
        item->setText(0, noteName);
        item->setText(1, subFolderPath);
        item->setText(2, tagText);
        item->setText(3, modifiedDisplay);
        item->setToolTip(2, tagText);
        item->setData(0, NoteIdRole, note.id);
        item->setData(0, NoteNameRole, noteName);
        item->setData(3, NoteModifiedRole, modified);
    }

    if (_notesListWidget->topLevelItemCount() > 0) {
        _notesListWidget->setCurrentItem(_notesListWidget->topLevelItem(0));
    }

    if (page == LinkDialog::TextLinkPage) {
        const QString text = QApplication::clipboard()->text().remove(QStringLiteral("\n")).trimmed();
        const QUrl url(text);
        if (url.isValid() && !url.scheme().isEmpty()) {
            _urlEdit->setText(text);
        }
    }

    setupFileUrlMenu();
}

LinkDialog::~LinkDialog() = default;

void LinkDialog::buildUi() {
    setWindowTitle(tr("Link to a URL or note"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("insert-link"),
                                   QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/insert-link.svg"))));
    resize(524, 501);

    auto* gridLayout = new QGridLayout(this);
    auto* verticalLayout = new QVBoxLayout();
    gridLayout->addLayout(verticalLayout, 0, 0);

    _tabWidget = new QTabWidget(this);
    verticalLayout->addWidget(_tabWidget);

    _urlTab = new QWidget(_tabWidget);
    auto* urlLayout = new QGridLayout(_urlTab);
    urlLayout->setContentsMargins(0, 0, 0, 0);
    _urlEdit = new QLineEdit(_urlTab);
    _urlEdit->setStatusTip(tr("Enter URL"));
    _urlEdit->setPlaceholderText(tr("Enter an URL to link to"));
    _urlEdit->setClearButtonEnabled(true);
    auto* refreshButton = new QPushButton(_urlTab);
    refreshButton->setToolTip(tr("Clear link name and fetch title of webpage again"));
    refreshButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh"),
                                            QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/view-refresh.svg"))));
    _fileUrlButton = new QPushButton(QStringLiteral("..."), _urlTab);
    _fileUrlButton->setToolTip(tr("Select local file or directory to link to"));
    _downloadProgressBar = new QProgressBar(_urlTab);
    _downloadProgressBar->setValue(0);
    urlLayout->addWidget(_urlEdit, 1, 1);
    urlLayout->addWidget(refreshButton, 1, 2);
    urlLayout->addWidget(_fileUrlButton, 1, 3);
    urlLayout->addWidget(_downloadProgressBar, 2, 1, 1, 3);
    urlLayout->setRowStretch(3, 1);
    _tabWidget->addTab(_urlTab, QStringLiteral("URL"));

    _noteTab = new QWidget(_tabWidget);
    auto* noteLayout = new QVBoxLayout(_noteTab);
    noteLayout->setContentsMargins(0, 0, 0, 0);
    _searchLineEdit = new QLineEdit(_noteTab);
    _searchLineEdit->setPlaceholderText(tr("Search for a note to link to"));
    _searchLineEdit->setClearButtonEnabled(true);
    _notesListWidget = new QTreeWidget(_noteTab);
    _notesListWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _notesListWidget->setHeaderLabels({tr("Note"), tr("Folder"), tr("Tags"), tr("Modified")});
    _headingSearchLineEdit = new QLineEdit(_noteTab);
    _headingSearchLineEdit->setPlaceholderText(tr("Search for a heading to link to"));
    _headingSearchLineEdit->setClearButtonEnabled(true);
    _headingListWidget = new QListWidget(_noteTab);
    _wikiLinkCheckBox = new QCheckBox(tr("Create wiki-style link ([[...]])"), _noteTab);
    _wikiLinkCheckBox->setToolTip(
        tr("Insert the link as a wiki-style link ([[Note Name]]) instead of a Markdown link"));
    _wikiLinkCheckBox->setChecked(true);
    noteLayout->addWidget(_searchLineEdit);
    noteLayout->addWidget(_notesListWidget, 3);
    noteLayout->addWidget(_headingSearchLineEdit);
    noteLayout->addWidget(_headingListWidget, 1);
    noteLayout->addWidget(_wikiLinkCheckBox);
    _tabWidget->addTab(_noteTab, tr("Note"));

    _linkNameEdit = new QLineEdit(this);
    _linkNameEdit->setStatusTip(tr("Enter the name of the link (optional)"));
    _linkNameEdit->setPlaceholderText(tr("Name of link (optional)"));
    _linkNameEdit->setClearButtonEnabled(true);
    _descriptionEdit = new QLineEdit(this);
    _descriptionEdit->setStatusTip(tr("Enter a description for the link (optional)"));
    _descriptionEdit->setPlaceholderText(tr("Description of link (optional)"));
    _descriptionEdit->setClearButtonEnabled(true);
    verticalLayout->addWidget(_linkNameEdit);
    verticalLayout->addWidget(_descriptionEdit);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    gridLayout->addWidget(buttonBox, 1, 0);

    connect(_urlEdit, &QLineEdit::textChanged, this, &LinkDialog::on_urlEdit_textChanged);
    connect(refreshButton, &QPushButton::clicked, this, &LinkDialog::on_refreshButton_clicked);
    connect(_searchLineEdit, &QLineEdit::textChanged, this, &LinkDialog::on_searchLineEdit_textChanged);
    connect(_headingSearchLineEdit, &QLineEdit::textChanged, this, &LinkDialog::on_headingSearchLineEdit_textChanged);
    connect(_notesListWidget, &QTreeWidget::doubleClicked, this, &LinkDialog::on_notesListWidget_doubleClicked);
    connect(_notesListWidget, &QTreeWidget::currentItemChanged, this,
            &LinkDialog::on_notesListWidget_currentItemChanged);
    connect(_headingListWidget, &QListWidget::doubleClicked, this, &LinkDialog::on_headingListWidget_doubleClicked);
    connect(_tabWidget, &QTabWidget::currentChanged, this, &LinkDialog::on_tabWidget_currentChanged);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        on_buttonBox_accepted();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void LinkDialog::on_searchLineEdit_textChanged(const QString& arg1) {
    if (arg1.size() >= 2) {
        QStringList noteNameList = _host == nullptr ? QStringList() : _host->linkDialogSearchNoteNames(arg1);
        firstVisibleNoteListRow = -1;

        for (int i = 0; i < _notesListWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = _notesListWidget->topLevelItem(i);
            if (!noteNameList.contains(item->data(0, NoteNameRole).toString())) {
                item->setHidden(true);
            } else {
                if (firstVisibleNoteListRow < 0) {
                    firstVisibleNoteListRow = i;
                }
                item->setHidden(false);
            }
        }
    } else {
        firstVisibleNoteListRow = 0;

        for (int i = 0; i < _notesListWidget->topLevelItemCount(); ++i) {
            _notesListWidget->topLevelItem(i)->setHidden(false);
        }
    }
}

QString LinkDialog::getSelectedNoteName() const {
    return _notesListWidget->currentItem() != nullptr
               ? _notesListWidget->currentItem()->data(0, NoteNameRole).toString()
               : QString();
}

NoteData LinkDialog::getSelectedNote() const {
    if (_notesListWidget->currentItem() == nullptr) {
        return {};
    }

    const int noteId = _notesListWidget->currentItem()->data(0, NoteIdRole).toInt();
    return _host == nullptr ? NoteData() : _host->linkDialogNoteById(noteId);
}

QString LinkDialog::getSelectedHeading() const {
    return _headingListWidget->selectedItems().isEmpty() ? QString()
                                                         : _headingListWidget->currentItem()->text().trimmed();
}

QString LinkDialog::getURL() const {
    QString url = _urlEdit->text().trimmed();

    if (!url.isEmpty() && !url.contains(QStringLiteral("://")) && !url.startsWith(QStringLiteral("./"))) {
        url = QStringLiteral("https://") + url;
    }

    return url;
}

void LinkDialog::setURL(const QString& text) {
    if (_urlEdit != nullptr) {
        _urlEdit->setText(text);
    }
}

QString LinkDialog::getLinkName() const {
    return _linkNameEdit == nullptr ? QString() : _linkNameEdit->text();
}

void LinkDialog::setLinkName(const QString& text) {
    if (_linkNameEdit != nullptr) {
        _linkNameEdit->setText(text);
    }
}

QString LinkDialog::getLinkDescription() const {
    return _descriptionEdit == nullptr ? QString() : _descriptionEdit->text();
}

QString LinkDialog::getTitleFromHtml(const QString& html) {
    if (html.isEmpty()) {
        return {};
    }

    QRegularExpression regex(QStringLiteral(R"(<title.*>(.*)<\/title>)"),
                             QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption |
                                 QRegularExpression::InvertedGreedinessOption);
    QRegularExpressionMatch match = regex.match(html);
    QString title = match.captured(1);

    title = Utils::Misc::unescapeHtml(std::move(title));
    title.replace(QStringLiteral("["), QStringLiteral("("))
        .replace(QStringLiteral("]"), QStringLiteral(")"))
        .replace(QStringLiteral("&#8211;"), QStringLiteral("-"))
        .replace(QStringLiteral("&#124;"), QStringLiteral("-"))
        .replace(QStringLiteral("&#038;"), QStringLiteral("&"))
        .replace(QStringLiteral("&#39;"), QStringLiteral("'"));

    return title.simplified();
}

bool LinkDialog::isWikiLink() const {
    return _host != nullptr && _host->linkDialogWikiLinkSupportEnabled() && _wikiLinkCheckBox->isChecked();
}

void LinkDialog::setCurrentNote(const NoteData& note) {
    _currentNote = note;
}

bool LinkDialog::eventFilter(QObject* obj, QEvent* event) {
    if (obj == _searchLineEdit) {
        if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = dynamic_cast<QKeyEvent*>(event);

            if (keyEvent != nullptr && (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Tab)) {
                auto* item = _notesListWidget->currentItem();
                if (item != nullptr && item->isHidden() && firstVisibleNoteListRow >= 0) {
                    _notesListWidget->setCurrentItem(_notesListWidget->topLevelItem(firstVisibleNoteListRow));
                }

                _notesListWidget->setFocus();
                return true;
            }
        }
        return false;
    }

    if (obj == _headingSearchLineEdit) {
        if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = dynamic_cast<QKeyEvent*>(event);

            if (keyEvent != nullptr && (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Tab)) {
                _headingListWidget->setFocus();
                return true;
            }
        }
        return false;
    }

    if (obj == _notesListWidget) {
        if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = dynamic_cast<QKeyEvent*>(event);

            if (keyEvent != nullptr && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Tab)) {
                _headingSearchLineEdit->setFocus();
                return true;
            }
        }
        return false;
    }

    return MasterDialog::eventFilter(obj, event);
}

void LinkDialog::doAccept() {
    _urlEdit->clear();
    close();
    setResult(QDialog::Accepted);
}

void LinkDialog::on_notesListWidget_doubleClicked(const QModelIndex& index) {
    Q_UNUSED(index)
    doAccept();
}

void LinkDialog::on_headingListWidget_doubleClicked(const QModelIndex& index) {
    Q_UNUSED(index)
    doAccept();
}

void LinkDialog::downloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    _downloadProgressBar->setMaximum(static_cast<int>(bytesTotal / 1000));
    _downloadProgressBar->setValue(static_cast<int>(bytesReceived / 1000));
    _downloadProgressBar->setToolTip(Utils::Misc::toHumanReadableByteSize(bytesReceived) + QStringLiteral(" / ") +
                                     Utils::Misc::toHumanReadableByteSize(bytesTotal));
}

void LinkDialog::slotReplyFinished(QNetworkReply* reply) {
    if (reply == nullptr) {
        return;
    }

    reply->deleteLater();
    _downloadProgressBar->hide();

    const QByteArray replyData = reply->readAll();
    if (reply->error() != QNetworkReply::NoError && reply->error() != QNetworkReply::OperationCanceledError) {
        qWarning() << QStringLiteral("Network error: %1").arg(reply->errorString());
        return;
    }

    if (getLinkName().isEmpty()) {
        const QString title = getTitleFromHtml(QString::fromUtf8(replyData));

        if (!title.isEmpty()) {
            setLinkName(title);
        }
    }
}

void LinkDialog::addFileUrl(bool relative) {
    QUrl fileUrl = _host == nullptr ? QUrl() : _host->linkDialogLastSelectedFileUrl();

    if (Utils::Misc::isInPortableMode()) {
        fileUrl = QUrl(QStringLiteral("file://") +
                       Utils::Misc::prependPortableDataPathIfNeeded(
                           Utils::Misc::removeIfStartsWith(fileUrl.toLocalFile(), QStringLiteral("/"))));
    }

    fileUrl = QFileDialog::getOpenFileUrl(this, tr("Select file to link to"), fileUrl);
    QString fileUrlString = fileUrl.toString(QUrl::FullyEncoded);

    if (Utils::Misc::isInPortableMode()) {
        fileUrlString =
            QStringLiteral("file://") +
            QUrl(QStringLiteral("../") + Utils::Misc::makePathRelativeToPortableDataPathIfNeeded(fileUrl.toLocalFile()))
                .toString(QUrl::FullyEncoded);
    }

    if (fileUrlString.isEmpty()) {
        return;
    }

    if (_host != nullptr) {
        _host->linkDialogSetLastSelectedFileUrl(fileUrlString);
    }
    fileUrl = QUrl(fileUrlString);

    if (relative && fileUrl.isLocalFile() && _currentNote.id > 0) {
        const QString relativePath =
            _host == nullptr ? QString() : _host->linkDialogRelativeFilePathFromCurrentNote(fileUrl.toLocalFile());
        fileUrlString = QStringLiteral("./") + relativePath;

        if (_linkNameEdit->text().isEmpty()) {
            QFileInfo fileInfo(relativePath);
            _linkNameEdit->setText(fileInfo.fileName());
        }
    }

    _urlEdit->setText(fileUrlString);
}

void LinkDialog::addDirectoryUrl(bool relative) {
    QUrl directoryUrl = _host == nullptr ? QUrl() : _host->linkDialogLastSelectedDirectoryUrl();

    if (Utils::Misc::isInPortableMode()) {
        directoryUrl = QUrl(QStringLiteral("file://") +
                            Utils::Misc::prependPortableDataPathIfNeeded(
                                Utils::Misc::removeIfStartsWith(directoryUrl.toLocalFile(), QStringLiteral("/"))));
    }

    directoryUrl = QFileDialog::getExistingDirectoryUrl(this, tr("Select directory to link to"), directoryUrl);
    QString directoryUrlString = directoryUrl.toString(QUrl::FullyEncoded);

    if (Utils::Misc::isInPortableMode()) {
        directoryUrlString = QStringLiteral("file://") +
                             QUrl(QStringLiteral("../") +
                                  Utils::Misc::makePathRelativeToPortableDataPathIfNeeded(directoryUrl.toLocalFile()))
                                 .toString(QUrl::FullyEncoded);
    }

    if (directoryUrlString.isEmpty()) {
        return;
    }

    if (_host != nullptr) {
        _host->linkDialogSetLastSelectedDirectoryUrl(directoryUrlString);
    }
    directoryUrl = QUrl(directoryUrlString);

    if (relative && directoryUrl.isLocalFile() && _currentNote.id > 0) {
        const QString relativePath =
            _host == nullptr ? QString() : _host->linkDialogRelativeFilePathFromCurrentNote(directoryUrl.toLocalFile());
        directoryUrlString = QStringLiteral("./") + relativePath;

        if (_linkNameEdit->text().isEmpty()) {
            QFileInfo directoryInfo(directoryUrl.toLocalFile());
            _linkNameEdit->setText(directoryInfo.fileName());
        }
    }

    _urlEdit->setText(directoryUrlString);
}

void LinkDialog::on_urlEdit_textChanged(const QString& arg1) {
    const QUrl url(arg1);

    if (!url.isValid()) {
        return;
    }

    if (url.scheme().startsWith(QStringLiteral("http")) && _linkNameEdit->text().isEmpty()) {
        startTitleFetchRequest(url);
    }
}

void LinkDialog::setupFileUrlMenu() {
    auto addMenu = std::make_unique<QMenu>(this);

    QAction* addFileRelativeAction = addMenu->addAction(tr("Select file to link to (relative)"));
    addFileRelativeAction->setIcon(QIcon::fromTheme(
        QStringLiteral("document-open"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/document-open.svg"))));
    connect(addFileRelativeAction, &QAction::triggered, this, [this]() { addFileUrl(true); });

    QAction* addFileAbsoluteAction = addMenu->addAction(tr("Select file to link to (absolute)"));
    addFileAbsoluteAction->setIcon(QIcon::fromTheme(
        QStringLiteral("document-open"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/document-open.svg"))));
    connect(addFileAbsoluteAction, &QAction::triggered, this, [this]() { addFileUrl(false); });

    QAction* addDirectoryRelativeAction = addMenu->addAction(tr("Select directory to link to (relative)"));
    addDirectoryRelativeAction->setIcon(
        QIcon::fromTheme(QStringLiteral("folder"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/folder.svg"))));
    connect(addDirectoryRelativeAction, &QAction::triggered, this, [this]() { addDirectoryUrl(true); });

    QAction* addDirectoryAbsoluteAction = addMenu->addAction(tr("Select directory to link to (absolute)"));
    addDirectoryAbsoluteAction->setIcon(
        QIcon::fromTheme(QStringLiteral("folder"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/folder.svg"))));
    connect(addDirectoryAbsoluteAction, &QAction::triggered, this, [this]() { addDirectoryUrl(false); });

    _fileUrlButton->setMenu(addMenu.release());
}

void LinkDialog::on_buttonBox_accepted() {
    if (_tabWidget->currentWidget() == _noteTab) {
        _urlEdit->clear();
    }
}

void LinkDialog::on_headingSearchLineEdit_textChanged(const QString& arg1) {
    Utils::Gui::searchForTextInListWidget(_headingListWidget, arg1);
}

void LinkDialog::loadNoteHeadings() const {
    const NoteData note = getSelectedNote();
    ArcNotesMarkdownTextEdit markdownTextEdit;
    markdownTextEdit.setPlainText(note.noteText);
    const auto nodes = NavigationWidget::parseDocument(markdownTextEdit.document());

    _headingListWidget->clear();
    for (const Node& node : nodes) {
        _headingListWidget->addItem(node.text);
    }
}

void LinkDialog::on_notesListWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    Q_UNUSED(current)
    Q_UNUSED(previous)

    loadNoteHeadings();
}

void LinkDialog::on_tabWidget_currentChanged(int index) {
    if (index == 0) {
        _urlEdit->setFocus();
    } else {
        _searchLineEdit->setFocus();
    }
}

void LinkDialog::startTitleFetchRequest(const QUrl& url) {
    _downloadProgressBar->show();
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
    connect(reply, &QNetworkReply::downloadProgress, this, &LinkDialog::downloadProgress);
}

void LinkDialog::on_refreshButton_clicked() {
    _linkNameEdit->clear();
    on_urlEdit_textChanged(_urlEdit->text());
}
