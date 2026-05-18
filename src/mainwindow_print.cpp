/*
 * Copyright (c) 2014-2026 Patrizio Bekerle -- <patrizio@bekerle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <coordinator/actionregistry.h>
#include <coordinator/appcoordinator.h>
#include <core/data/notedata.h>
#include <core/state/appstate.h>
#include <core/state/uistate.h>
#include <dialogs/aboutdialog.h>
#include <dialogs/attachmentdialog.h>
#include <dialogs/commandbar.h>
#include <dialogs/filedialog.h>
#include <dialogs/imagedialog.h>
#include <dialogs/layoutdialog.h>
#include <dialogs/linkdialog.h>
#include <dialogs/localtrashdialog.h>
#include <dialogs/notedialog.h>
#include <dialogs/notediffdialog.h>
#include <dialogs/settingsdialog.h>
#include <dialogs/storedattachmentsdialog.h>
#include <dialogs/storedimagesdialog.h>
#include <dialogs/tabledialog.h>
#include <dialogs/tagadddialog.h>
#include <helpers/arcnotesmarkdownhighlighter.h>
#include <helpers/toolbarcontainer.h>
#include <utils/gui.h>
#include <utils/listutils.h>
#include <utils/misc.h>
#include <utils/schema.h>
#include <viewmodels/navigationviewmodel.h>
#include <viewmodels/noteeditorviewmodel.h>
#include <viewmodels/notefolderviewmodel.h>
#include <viewmodels/notelistviewmodel.h>
#include <viewmodels/notesubfolderviewmodel.h>
#include <viewmodels/searchviewmodel.h>
#include <viewmodels/settingsviewmodel.h>
#include <viewmodels/tagtreeviewmodel.h>
#include <views/logview.h>
#include <views/navigationview.h>
#include <views/noteeditorview.h>
#include <views/notelistview.h>
#include <views/notepreviewview.h>
#include <views/notesubfolderview.h>
#include <views/searchpanelview.h>
#include <views/tagtreeview.h>
#include <widgets/arcnotesmarkdowntextedit.h>
#include <widgets/combobox.h>
#include <widgets/lineedit.h>

#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QPageLayout>
#include <QPageSetupDialog>
#include <QPageSize>
#include <QPlainTextEdit>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QtGlobal>

#include "mainwindow.h"
#include "version.h"

namespace {
void setHighlighterFormatStyle(ArcNotesMarkdownHighlighter* highlighter, int highlighterState,
                               const QString& schemaKey) {
    const auto state = static_cast<MarkdownHighlighter::HighlighterState>(highlighterState);
    QTextCharFormat format;
    Utils::Schema::schemaSettings->setFormatStyle(state, format, schemaKey);

    if (state == MarkdownHighlighter::HighlighterState::WikiLink) {
        format.setFontUnderline(true);
        format.setUnderlineStyle(QTextCharFormat::DotLine);
    } else if (state == MarkdownHighlighter::HighlighterState::WikiLinkBroken) {
        format.setFontUnderline(true);
        format.setUnderlineStyle(QTextCharFormat::DashUnderline);
    }

    highlighter->setTextFormat(state, format);
}

void setHighlighterSchemaStyles(ArcNotesMarkdownHighlighter* highlighter, const QString& schemaKey) {
    const int states[] = {
        MarkdownHighlighter::HighlighterState::H1,
        MarkdownHighlighter::HighlighterState::H2,
        MarkdownHighlighter::HighlighterState::H3,
        MarkdownHighlighter::HighlighterState::H4,
        MarkdownHighlighter::HighlighterState::H5,
        MarkdownHighlighter::HighlighterState::H6,
        MarkdownHighlighter::HighlighterState::HorizontalRule,
        MarkdownHighlighter::HighlighterState::List,
        MarkdownHighlighter::HighlighterState::CheckBoxChecked,
        MarkdownHighlighter::HighlighterState::CheckBoxUnChecked,
        MarkdownHighlighter::HighlighterState::Bold,
        MarkdownHighlighter::HighlighterState::Italic,
        MarkdownHighlighter::HighlighterState::StUnderline,
        MarkdownHighlighter::HighlighterState::BlockQuote,
        MarkdownHighlighter::HighlighterState::CodeBlock,
        MarkdownHighlighter::HighlighterState::Comment,
        MarkdownHighlighter::HighlighterState::MaskedSyntax,
        MarkdownHighlighter::HighlighterState::Image,
        MarkdownHighlighter::HighlighterState::InlineCodeBlock,
        MarkdownHighlighter::HighlighterState::Link,
        MarkdownHighlighter::HighlighterState::LinkInternal,
        MarkdownHighlighter::HighlighterState::WikiLink,
        MarkdownHighlighter::HighlighterState::WikiLinkBroken,
        MarkdownHighlighter::HighlighterState::Table,
        MarkdownHighlighter::HighlighterState::BrokenLink,
        MarkdownHighlighter::HighlighterState::TrailingSpace,
        MarkdownHighlighter::HighlighterState::CodeType,
        MarkdownHighlighter::HighlighterState::CodeKeyWord,
        MarkdownHighlighter::HighlighterState::CodeComment,
        MarkdownHighlighter::HighlighterState::CodeString,
        MarkdownHighlighter::HighlighterState::CodeNumLiteral,
        MarkdownHighlighter::HighlighterState::CodeBuiltIn,
        MarkdownHighlighter::HighlighterState::CodeOther,
    };

    for (const int state : states) {
        setHighlighterFormatStyle(highlighter, state, schemaKey);
    }
}

QTextDocument* createLightEditorSchemaDocument(QTextDocument* sourceDocument) {
    if (sourceDocument == nullptr || Utils::Schema::schemaSettings == nullptr ||
        !Utils::Schema::schemaSettings->currentSchemaIsDark()) {
        return sourceDocument;
    }

    auto* document = new QTextDocument();
    document->setDefaultFont(Utils::Schema::schemaSettings->getEditorTextFont());
    document->setPlainText(sourceDocument->toPlainText());

    auto* highlighter = new ArcNotesMarkdownHighlighter(document);
    setHighlighterSchemaStyles(highlighter, Utils::Schema::lightEditorSchemaKey());
    highlighter->rehighlight();

    return document;
}

QTextDocument* createCurrentNotePreviewDocument(MainWindow* window) {
    if (window == nullptr || window->getCurrentNote().id <= 0) {
        return nullptr;
    }

    const NoteData note = window->getCurrentNote();
    QString html = window->editorRenderTextToHtml(note.noteText, Utils::Misc::useInternalExportStylingForPreview());
    html = Utils::Misc::parseTaskList(html, false);

    auto* document = new QTextDocument(window);
    document->setHtml(html);
    return document;
}
}  // namespace

int MainWindow::getMaxImageWidth() const {
    return 980;
}

void MainWindow::printTextDocument(QTextDocument* document, bool useLightEditorSchemaForDarkSchema) {
    if (document == nullptr) {
        return;
    }

    QPrinter printer;
    Utils::Misc::loadPrinterSettings(&printer, QStringLiteral("Printer/NotePrinting"));

    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Print note"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Utils::Misc::storePrinterSettings(&printer, QStringLiteral("Printer/NotePrinting"));
    QTextDocument* printDocument =
        useLightEditorSchemaForDarkSchema ? createLightEditorSchemaDocument(document) : document;
    printDocument->print(&printer);

    if (printDocument != document) {
        delete printDocument;
    }
}

void MainWindow::exportNoteAsPDF(QTextDocument* document, bool useLightEditorSchemaForDarkSchema) {
    if (document == nullptr) {
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setColorMode(QPrinter::Color);

#ifdef Q_OS_LINUX
    Utils::Misc::loadPrinterSettings(&printer, QStringLiteral("Printer/NotePDFExport"));
    QPageSetupDialog pageSetupDialog(&printer, this);
    if (pageSetupDialog.exec() != QDialog::Accepted) {
        return;
    }
    Utils::Misc::storePrinterSettings(&printer, QStringLiteral("Printer/NotePDFExport"));
#else
    const QStringList pageSizeStrings = {QStringLiteral("A0"), QStringLiteral("A1"), QStringLiteral("A2"),
                                         QStringLiteral("A3"), QStringLiteral("A4"), QStringLiteral("A5"),
                                         QStringLiteral("A6"), QStringLiteral("A7"), QStringLiteral("A8"),
                                         QStringLiteral("A9"), tr("Letter")};
    const QList<QPageSize::PageSizeId> pageSizes = {QPageSize::A0, QPageSize::A1, QPageSize::A2,    QPageSize::A3,
                                                    QPageSize::A4, QPageSize::A5, QPageSize::A6,    QPageSize::A7,
                                                    QPageSize::A8, QPageSize::A9, QPageSize::Letter};

    bool ok = false;
    const QString pageSizeString = QInputDialog::getItem(
        this, tr("Page size"), tr("Page size:"), pageSizeStrings,
        persistentSetting(QStringLiteral("Printer/NotePDFExportPageSize"), 4).toInt(), false, &ok);
    const int pageSizeIndex = pageSizeStrings.indexOf(pageSizeString);
    if (!ok || pageSizeIndex < 0) {
        return;
    }
    setPersistentSetting(QStringLiteral("Printer/NotePDFExportPageSize"), pageSizeIndex);
    printer.setPageSize(QPageSize(pageSizes.at(pageSizeIndex)));

    const QStringList orientationStrings = {tr("Portrait"), tr("Landscape")};
    const QList<QPageLayout::Orientation> orientations = {QPageLayout::Portrait, QPageLayout::Landscape};
    const QString orientationString = QInputDialog::getItem(
        this, tr("Orientation"), tr("Orientation:"), orientationStrings,
        persistentSetting(QStringLiteral("Printer/NotePDFExportOrientation"), 0).toInt(), false, &ok);
    const int orientationIndex = orientationStrings.indexOf(orientationString);
    if (!ok || orientationIndex < 0) {
        return;
    }
    setPersistentSetting(QStringLiteral("Printer/NotePDFExportOrientation"), orientationIndex);
    printer.setPageOrientation(orientations.at(orientationIndex));
#endif

    FileDialog dialog(QStringLiteral("NotePDFExport"));
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(tr("PDF files") + QStringLiteral(" (*.pdf)"));
    dialog.setWindowTitle(tr("Export current note as PDF"));
    dialog.selectFile((_currentNote.id > 0 ? _currentNote.name : tr("note")) + QStringLiteral(".pdf"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString fileName = dialog.selectedFile();
    if (fileName.isEmpty()) {
        return;
    }
    if (QFileInfo(fileName).suffix().isEmpty()) {
        fileName.append(QStringLiteral(".pdf"));
    }

    printer.setColorMode(QPrinter::Color);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QTextDocument* printDocument =
        useLightEditorSchemaForDarkSchema ? createLightEditorSchemaDocument(document) : document;
    printDocument->print(&printer);

    if (printDocument != document) {
        delete printDocument;
    }

    Utils::Misc::openFolderSelect(fileName, QStringLiteral("show-exported-pdf-in-file-manager"));
}
