#include "notedialog.h"

#include <dialogs/notedialoghost.h>
#include <utils/misc.h>
#include <utils/urlhandler.h>
#include <viewmodels/viewmodellocator.h>
#include <widgets/arcnotesmarkdowntextedit.h>
#include <widgets/notepreviewwidget.h>

#include <QDebug>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

NoteDialog::NoteDialog(QWidget* parent) : MasterDialog(parent), _host(dynamic_cast<NoteDialogHost*>(parent)) {
    setWindowTitle(tr("Note"));
    resize(686, 424);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 3, 0, 0);

    _tabWidget = new QTabWidget(this);
    _tabWidget->setDocumentMode(true);
    mainLayout->addWidget(_tabWidget);

    auto* noteTextTab = new QWidget(_tabWidget);
    auto* noteTextLayout = new QVBoxLayout(noteTextTab);
    noteTextLayout->setSpacing(0);
    noteTextLayout->setContentsMargins(0, 0, 0, 0);
    _textEdit = new ArcNotesMarkdownTextEdit(noteTextTab);
    noteTextLayout->addWidget(_textEdit);
    _searchFrame = new QFrame(noteTextTab);
    _searchFrame->setFrameShape(QFrame::NoFrame);
    noteTextLayout->addWidget(_searchFrame);
    _tabWidget->addTab(noteTextTab, tr("Note text"));

    auto* previewTab = new QWidget(_tabWidget);
    auto* previewLayout = new QVBoxLayout(previewTab);
    previewLayout->setSpacing(0);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    _noteTextView = new NotePreviewWidget(previewTab);
    _noteTextView->setEnabled(true);
    _noteTextView->setStyleSheet(QString());
    _noteTextView->setFrameShape(QFrame::NoFrame);
    _noteTextView->setReadOnly(true);
    _noteTextView->setAcceptRichText(false);
    _noteTextView->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse |
                                           Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard |
                                           Qt::TextSelectableByMouse);
    _noteTextView->setOpenExternalLinks(true);
    _noteTextView->setOpenLinks(false);
    previewLayout->addWidget(_noteTextView);
    _tabWidget->addTab(previewTab, tr("Preview"));

    auto* buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(6);
    buttonLayout->setContentsMargins(9, 6, 9, 9);
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttonLayout->addWidget(buttonBox);
    mainLayout->addLayout(buttonLayout);

    _textEdit->initSearchFrame(_searchFrame);
    _textEdit->setReadOnly(true);
    if (SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel()) {
        _tabWidget->setCurrentIndex(viewModel->persistentSetting(QStringLiteral("NoteDialog/tabWidgetIndex")).toInt());
    }

    QFont font;
    font.fromString(Utils::Misc::previewFontString());
    _noteTextView->setFont(font);

    QPushButton* reloadButton = buttonBox->addButton(tr("Reload"), QDialogButtonBox::ActionRole);
    reloadButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh"),
                                           QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/view-refresh.svg"))));
    reloadButton->setToolTip(tr("Reload the note text"));

    QPushButton* jumpToNoteButton = buttonBox->addButton(tr("Jump to note"), QDialogButtonBox::ActionRole);
    jumpToNoteButton->setIcon(QIcon::fromTheme(QStringLiteral("go-next"),
                                               QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/go-next.svg"))));
    jumpToNoteButton->setToolTip(tr("Jump to the note in the main window"));

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(_noteTextView, &QTextBrowser::anchorClicked, this, &NoteDialog::on_noteTextView_anchorClicked);
    connect(_tabWidget, &QTabWidget::currentChanged, this, &NoteDialog::on_tabWidget_currentChanged);
    connect(reloadButton, &QPushButton::clicked, this, &NoteDialog::onReloadButtonClicked);
    connect(jumpToNoteButton, &QPushButton::clicked, this, &NoteDialog::onJumpToNoteButtonClicked);

    afterSetupUI();
}

NoteDialog::~NoteDialog() = default;

void NoteDialog::setNoteFolderPath(const QString& noteFolderPath) {
    _noteFolderPath = noteFolderPath;
}

void NoteDialog::setNote(const NoteData& note) {
    _note = note;
    setWindowTitle(note.name);

    _textEdit->setPlainText(note.noteText);
    _noteTextView->setHtml(_host == nullptr ? QString() : _host->noteDialogRenderNoteToHtml(note, _noteFolderPath));
}

void NoteDialog::on_noteTextView_anchorClicked(const QUrl& url) {
    qDebug() << __func__ << " - 'url': " << url;

    if (UrlHandler::isUrlSchemeLocal(url)) {
        return;
    }

    QDesktopServices::openUrl(url);
}

void NoteDialog::on_tabWidget_currentChanged(int index) {
    if (SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel()) {
        viewModel->setPersistentSetting(QStringLiteral("NoteDialog/tabWidgetIndex"), index);
    }
}

void NoteDialog::onReloadButtonClicked() {
    if (_host == nullptr || _note.id <= 0) {
        return;
    }

    const NoteData note = _host->noteDialogNoteById(_note.id);
    if (note.id > 0) {
        setNote(note);
    }
}

void NoteDialog::onJumpToNoteButtonClicked() {
    Q_EMIT jumpToNoteRequested(_note.id);
}
