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
#include <core/data/notefolderdata.h>
#include <core/data/notesubfolderdata.h>
#include <core/data/tagdata.h>
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
#include <dialogs/notebookmarkdialog.h>
#include <dialogs/notedialog.h>
#include <dialogs/notediffdialog.h>
#include <dialogs/settingsdialog.h>
#include <dialogs/storedattachmentsdialog.h>
#include <dialogs/storedimagesdialog.h>
#include <dialogs/tabledialog.h>
#include <dialogs/tagadddialog.h>
#include <helpers/arcnotesmarkdownhighlighter.h>
#include <helpers/toolbarcontainer.h>
#include <models/navigationoutlinemodel.h>
#include <models/notelistmodel.h>
#include <models/notesubfoldertreemodel.h>
#include <models/tagtreemodel.h>
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
#include <QColorDialog>
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
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPageLayout>
#include <QPageSetupDialog>
#include <QPageSize>
#include <QPlainTextEdit>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
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
struct ActionSpec {
    const char* name;
    const char* text;
    const char* icon;
    const char* shortcut;
    bool checkable;
};

const ActionSpec kActionSpecs[] = {
    {"action_Quit", "&Quit", "application-exit.svg", "Ctrl+Q", false},
    {"action_Find_note", "&Find text in notes", "edit-find.svg", "Ctrl+Shift+F", false},
    {"action_Remove_note", "&Remove current note", "edit-delete.svg", "Alt+R", false},
    {"actionAbout_ArcNotes", "About &ArcNotes...", "help-about.svg", "", false},
    {"action_New_note", "&New note", "document-new.svg", "Ctrl+N", false},
    {"actionAlphabetical", "Alphabetical", "go-up.svg", "", true},
    {"actionBy_date", "By date", "go-down.svg", "", true},
    {"action_Settings", "&Settings", "configure.svg", "Ctrl+,", false},
    {"actionSelect_all_notes", "Select &all notes", "edit-guides.svg", "Ctrl+A", false},
    {"actionSelect_enclosed_text", "Select enclosed &text", "edit-guides.svg", "", false},
    {"actionInsert_text_link", "Insert text &link", "text-frame-link.svg", "Ctrl+L", false},
    {"action_Find_text_in_note", "&Find in current note", "edit-find-in-note.svg", "Ctrl+F", false},
    {"action_FormatTable", "Auto format table", "table.svg", "", false},
    {"action_DuplicateText", "&Duplicate text", "edit-copy.svg", "Ctrl+D", false},
    {"action_Back_in_note_history", "&Back in note history", "go-previous.svg", "Alt+Left", false},
    {"action_Forward_in_note_history", "Forward in note history", "go-next.svg", "Alt+Right", false},
    {"action_Previous_heading", "Previous &heading", "go-up.svg", "", false},
    {"action_Next_heading", "Next heading", "go-down.svg", "", false},
    {"actionFold_all_headings", "Fold all headings", "go-top.svg", "", false},
    {"actionUnfold_all_headings", "Unfold all headings", "go-bottom.svg", "", false},
    {"actionInsert_current_time", "Insert current &time", "appointment-new.svg", "", false},
    {"action_Export_note_as_PDF_markdown", "&Export note as PDF (preview)", "application-pdf.svg", "", false},
    {"action_Export_note_as_PDF_text", "&Export note as PDF (text)", "application-pdf.svg", "", false},
    {"action_Print_note_text", "&Print note (text)", "document-print.svg", "Ctrl+P", false},
    {"action_Print_note_markdown", "&Print note (preview)", "document-print.svg", "", false},
    {"actionInsert_image", "Insert &image", "insert-image.svg", "Ctrl+Shift+I", false},
    {"action_Open_note_in_external_editor", "&Open note in external editor", "story-editor.svg", "Ctrl+Shift+O", false},
    {"action_Export_note_as_markdown", "&Export note as Markdown file", "text-x-markdown.svg", "", false},
    {"actionInsert_code_block", "Insert &code block", "code-context.svg", "Ctrl+Shift+C", false},
    {"actionNext_note", "Move down in note list", "go-down.svg", "", false},
    {"actionPrevious_Note", "Move up in note list", "go-up.svg", "", false},
    {"actionToggle_distraction_free_mode", "&Distraction free mode", "window.svg", "F11", true},
    {"actionShow_toolbar", "Show &toolbar", "application-menu.svg", "", true},
    {"actionEditorWidthNarrow", "Narrow", "", "", true},
    {"actionEditorWidthMedium", "Medium", "", "", true},
    {"actionEditorWidthWide", "Wide", "", "", true},
    {"actionEditorWidthFull", "Full", "", "", true},
    {"actionPaste_image", "Paste html or media", "edit-paste.svg", "Ctrl+Shift+V", false},
    {"actionShow_note_in_file_manager", "Show note in &file manager", "document-open.svg", "", false},
    {"actionFormat_text_bold", "Format text &bold", "format-text-bold.svg", "Ctrl+B", false},
    {"actionFormat_text_italic", "Format text &italic", "format-text-italic.svg", "Ctrl+I", false},
    {"action_Increase_note_text_size", "&Increase note text size", "zoom-in.svg", "Ctrl++", false},
    {"action_Decrease_note_text_size", "&Decrease note text size", "zoom-out.svg", "Ctrl+-", false},
    {"action_Reset_note_text_size", "&Reset note text size", "zoom-original.svg", "Ctrl+0", false},
    {"action_new_tag", "&Add tag to note", "list-add.svg", "", false},
    {"action_Reload_note_folder", "&Reload note folder", "view-refresh.svg", "F5", false},
    {"actionUse_vertical_preview_layout", "Use &vertical preview layout", "", "", true},
    {"actionReplace_in_current_note", "Replace in current note", "edit-find-replace.svg", "Ctrl+R", false},
    {"actionAutocomplete", "Autocomplete, solve equation or open URL", "configure.svg", "Ctrl+Space", false},
    {"actionSelect_note_folder", "Select note folder", "folder.svg", "", false},
    {"actionShow_log", "Show log", "face-smile.svg", "", true},
    {"actionExport_preview_HTML", "Export note as HTML", "text-html.svg", "", false},
    {"actionInsert_headline_from_note_filename", "Insert headline from note filename", "", "", false},
    {"actionUse_softwrap_in_note_editor", "Use softwrap in note editor", "", "", true},
    {"actionShow_status_bar", "Show status bar", "", "", true},
    {"actionToggle_text_case", "Cycle text case", "format-text-capitalize.svg", "", false},
    {"actionStrike_out_text", "Strike out text", "format-text-strikethrough.svg", "Alt+Shift+S", false},
    {"actionUse_one_column_mode", "Use one column mode", "", "", true},
    {"actionShow_menu_bar", "Show menu bar", "", "", true},
    {"actionSplit_note_at_cursor_position", "Split note at cursor position", "split.svg", "", false},
    {"actionShow_note_list_under_tag_pane", "Show note list under tag panel", "", "", true},
    {"actionFind_notes_in_all_subfolders", "Find text in notes in all tags / subfolders", "edit-find.svg", "", true},
    {"actionImport_notes_from_text_files", "Import notes from text files", "document-import.svg", "", false},
    {"actionManage_stored_images", "Manage stored image files", "insert-image.svg", "", false},
    {"actionUnlock_panels", "Unlock panels", "configure.svg", "", true},
    {"actionReattach_panels", "Reattach floating panels", "window-duplicate.svg", "", false},
    {"actionRemove_current_layout", "Remove current layout", "edit-delete.svg", "", false},
    {"actionStore_as_new_layout", "Add new layout", "list-add.svg", "", false},
    {"actionRename_current_layout", "Rename current layout", "document-edit.svg", "", false},
    {"actionSwitch_to_previous_layout", "Switch to previous layout", "go-previous.svg", "", false},
    {"actionManage_layouts", "Manage layouts", "configure.svg", "", false},
    {"actionShow_all_panels", "Show all panels", "", "", false},
    {"actionFind_action", "Find action", "edit-find.svg", "Ctrl+Shift+P", false},
    {"actionRedo_action", "Redo last action", "go-next.svg", "", false},
    {"actionInsert_table", "Insert table", "insert-table.svg", "Alt+Shift+T", false},
    {"actionInsert_block_quote", "Insert block &quote", "format-text-blockquote.svg", "", false},
    {"actionInsert_checkbox_list_item", "Insert checkbox list item", "task-new.svg", "", false},
    {"actionSearch_text_on_the_web", "Search selected text on the web", "text-html.svg", "", false},
    {"actionDelete_line", "Delete line", "edit-delete.svg", "", false},
    {"actionDelete_word", "Delete word", "edit-delete.svg", "", false},
    {"actionCopy_headline", "Copy note headline", "edit-copy.svg", "", false},
    {"actionView_note_in_new_window", "Open note in different window", "window.svg", "", false},
    {"actionSave_modified_notes", "Save modified notes", "document-save.svg", "Ctrl+S", false},
    {"actionAscending", "Ascending", "go-up.svg", "", true},
    {"actionDescending", "Descending", "go-down.svg", "", true},
    {"actionInsert_attachment", "Insert attachment", "mail-attachment.svg", "", false},
    {"actionAllow_note_editing", "Allow all note editing", "document-edit.svg", "Alt+Shift+E", true},
    {"actionShow_local_trash", "Show local trash", "trash-empty.svg", "", false},
    {"actionJump_to_note_text_edit", "Jump to note edit panel", "go-next.svg", "", false},
    {"actionManage_stored_attachments", "Manage stored attachments", "mail-attachment.svg", "", false},
    {"actionJump_to_note_subfolder_panel", "Jump to note subfolder panel", "go-next.svg", "", false},
    {"actionJump_to_tags_panel", "Jump to tags panel", "go-next.svg", "", false},
    {"actionJump_to_note_list_panel", "Jump to note list panel", "go-next.svg", "", false},
    {"actionActivate_context_menu", "Activate context menu", "application-menu.svg", "", false},
    {"actionToggle_fullscreen", "Toggle full-screen mode", "window.svg", "F11", true},
    {"actionTypewriter_mode", "Typewriter mode", "", "", true},
    {"actionEditorWidthCustom", "Custom", "", "", true},
    {"actionShow_Hide_application", "Show/Hide application", "", "", false},
    {"actionPrevious_note_tab", "Previous note tab", "go-previous.svg", "", false},
    {"actionNext_note_tab", "Next note tab", "go-next.svg", "", false},
    {"actionClose_current_note_tab", "Close current note tab", "window-close.svg", "Ctrl+W", false},
    {"actionNew_note_in_new_tab", "New note in new tab", "document-new.svg", "Ctrl+Shift+N", false},
    {"actionToggle_note_stickiness_of_current_tab", "Toggle note pinning of current tab", "favorite-favorited.svg", "",
     true},
    {"actionFormat_text_underline", "Format text underline", "format-text-underline.svg", "Ctrl+U", false},
    {"actionJump_to_navigation_panel", "Jump to navigation panel", "go-next.svg", "", false},
    {"actionInsert_note_link", "Insert &note link", "text-frame-link.svg", "", false},
    {"actionToggle_Always_on_top", "Toggle always-on-top mode", "", "", true},
    {"actionCopy_path_to_note_to_clipboard", "Copy absolute path of note", "edit-copy.svg", "", false},
    {"actionMove_up_in_subfolder_list", "Move up in subfolder list", "go-up.svg", "", false},
    {"actionMove_down_in_subfolder_list", "Move down in subfolder list", "go-down.svg", "", false},
    {"actionMove_up_in_tag_list", "Move up in tag list", "go-up.svg", "", false},
    {"actionMove_down_in_tag_list", "Move down in tag list", "go-down.svg", "", false},
    {"actionToggle_checkboxes", "Toggle checkbox(es)", "task-new.svg", "", false},
    {"actionCreate_ordered_list", "1. 2. 3. list", "text-x-generic.svg", "", false},
    {"actionCreate_alphabetical_list", "a. b. c. list", "text-x-generic.svg", "", false},
    {"actionCreate_unordered_list", "- list", "text-x-generic.svg", "", false},
    {"actionCreate_checkbox_list", "Create checkbox list", "task-new.svg", "", false},
    {"actionClear_list_formatting", "Clear list formatting", "edit-clear.svg", "", false},
    {"actionOrder_checkboxes", "Order checkboxes", "task-new.svg", "", false},
    {"actionIncrease_heading_depth", "Increase heading depth", "format-font-size-more.svg", "", false},
    {"actionDecrease_heading_depth", "Decrease heading depth", "format-font-size-less.svg", "", false},
    {"actionCopy_code_block", "Copy code block", "edit-copy.svg", "", false},
};

const char* kStoreBookmarkActionNames[] = {
    "actionStore_note_bookmark_1", "actionStore_note_bookmark_2", "actionStore_note_bookmark_3",
    "actionStore_note_bookmark_4", "actionStore_note_bookmark_5", "actionStore_note_bookmark_6",
    "actionStore_note_bookmark_7", "actionStore_note_bookmark_8", "actionStore_note_bookmark_9",
};

const char* kGotoBookmarkActionNames[] = {
    "actionGoto_note_bookmark_1", "actionGoto_note_bookmark_2", "actionGoto_note_bookmark_3",
    "actionGoto_note_bookmark_4", "actionGoto_note_bookmark_5", "actionGoto_note_bookmark_6",
    "actionGoto_note_bookmark_7", "actionGoto_note_bookmark_8", "actionGoto_note_bookmark_9",
};

QString iconResourcePath(const QString& fileName) {
    if (fileName.isEmpty()) {
        return QString();
    }
    return QStringLiteral(":/icons/breeze-arcnotes/16x16/%1").arg(fileName);
}

QString readableActionText(const QString& objectName) {
    QString text = objectName;
    text.remove(QStringLiteral("action"));
    text.replace(QLatin1Char('_'), QLatin1Char(' '));
    return text.trimmed();
}

QString categoryForAction(const QString& objectName) {
    if (objectName.contains(QStringLiteral("tag"), Qt::CaseInsensitive)) {
        return QStringLiteral("tag");
    }
    if (objectName.contains(QStringLiteral("view"), Qt::CaseInsensitive) ||
        objectName.contains(QStringLiteral("show"), Qt::CaseInsensitive) ||
        objectName.contains(QStringLiteral("layout"), Qt::CaseInsensitive) ||
        objectName.contains(QStringLiteral("panel"), Qt::CaseInsensitive)) {
        return QStringLiteral("view");
    }
    if (objectName.contains(QStringLiteral("insert"), Qt::CaseInsensitive) ||
        objectName.contains(QStringLiteral("format"), Qt::CaseInsensitive) ||
        objectName.contains(QStringLiteral("text"), Qt::CaseInsensitive)) {
        return QStringLiteral("edit");
    }
    if (objectName.contains(QStringLiteral("about"), Qt::CaseInsensitive) ||
        objectName.contains(QStringLiteral("help"), Qt::CaseInsensitive)) {
        return QStringLiteral("help");
    }
    return QStringLiteral("note");
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

void MainWindow::setupActions() {
    for (const ActionSpec& spec : kActionSpecs) {
        configureAction(QString::fromLatin1(spec.name), tr(spec.text), iconResourcePath(QString::fromLatin1(spec.icon)),
                        QString::fromLatin1(spec.shortcut), spec.checkable);
    }
    for (int slot = 1; slot <= 9; ++slot) {
        configureAction(QString::fromLatin1(kStoreBookmarkActionNames[slot - 1]),
                        tr("Store bookmark at slot %1").arg(slot), QString(),
                        QStringLiteral("Ctrl+Shift+%1").arg(slot));
        configureAction(QString::fromLatin1(kGotoBookmarkActionNames[slot - 1]),
                        tr("Go to bookmark at slot %1").arg(slot), QString(), QStringLiteral("Ctrl+%1").arg(slot));
    }
    configureAction(QStringLiteral("actionOpen_note_bookmark_dialog"), tr("Note bookmarks"), QString());
    findAction(QStringLiteral("actionShow_toolbar"))->setChecked(true);
    findAction(QStringLiteral("actionShow_status_bar"))->setChecked(true);
    findAction(QStringLiteral("actionAllow_note_editing"))->setChecked(true);
    findAction(QStringLiteral("actionToggle_distraction_free_mode"))->setChecked(isInDistractionFreeMode());
}

void MainWindow::setupMenus() {
    auto* noteMenu = menuBar()->addMenu(tr("&Note"));
    noteMenu->setObjectName(QStringLiteral("menuArcNotes"));

    auto* navigationMenu = new QMenu(tr("Navigation"), noteMenu);
    navigationMenu->setObjectName(QStringLiteral("menuNavigation"));
    addMenuAction(navigationMenu, QStringLiteral("action_Back_in_note_history"));
    addMenuAction(navigationMenu, QStringLiteral("action_Forward_in_note_history"));
    navigationMenu->addSeparator();
    addMenuAction(navigationMenu, QStringLiteral("action_Previous_heading"));
    addMenuAction(navigationMenu, QStringLiteral("action_Next_heading"));
    addMenuAction(navigationMenu, QStringLiteral("actionFold_all_headings"));
    addMenuAction(navigationMenu, QStringLiteral("actionUnfold_all_headings"));
    navigationMenu->addSeparator();
    addMenuAction(navigationMenu, QStringLiteral("actionPrevious_Note"));
    addMenuAction(navigationMenu, QStringLiteral("actionNext_note"));
    addMenuAction(navigationMenu, QStringLiteral("actionPrevious_note_tab"));
    addMenuAction(navigationMenu, QStringLiteral("actionNext_note_tab"));
    addMenuAction(navigationMenu, QStringLiteral("actionMove_up_in_subfolder_list"));
    addMenuAction(navigationMenu, QStringLiteral("actionMove_down_in_subfolder_list"));
    addMenuAction(navigationMenu, QStringLiteral("actionMove_up_in_tag_list"));
    addMenuAction(navigationMenu, QStringLiteral("actionMove_down_in_tag_list"));
    addMenuAction(navigationMenu, QStringLiteral("actionClose_current_note_tab"));
    addMenuAction(navigationMenu, QStringLiteral("actionToggle_note_stickiness_of_current_tab"));
    navigationMenu->addSeparator();
    auto* storeBookmarkMenu = new QMenu(tr("Store note bookmark"), navigationMenu);
    storeBookmarkMenu->setObjectName(QStringLiteral("menuStore_note_bookmark"));
    auto* gotoBookmarkMenu = new QMenu(tr("Go to note bookmark"), navigationMenu);
    gotoBookmarkMenu->setObjectName(QStringLiteral("menuGoto_note_bookmark"));
    for (int slot = 1; slot <= 9; ++slot) {
        addMenuAction(storeBookmarkMenu, QString::fromLatin1(kStoreBookmarkActionNames[slot - 1]));
        addMenuAction(gotoBookmarkMenu, QString::fromLatin1(kGotoBookmarkActionNames[slot - 1]));
    }
    navigationMenu->addMenu(storeBookmarkMenu);
    navigationMenu->addMenu(gotoBookmarkMenu);
    navigationMenu->addSeparator();
    addMenuAction(navigationMenu, QStringLiteral("actionOpen_note_bookmark_dialog"));
    connect(navigationMenu, &QMenu::aboutToShow, this, [this, storeBookmarkMenu]() {
        storeBookmarkMenu->setEnabled(_noteTextEdit != nullptr && _noteTextEdit->hasFocus());
    });

    auto* trashMenu = new QMenu(tr("Trash"), noteMenu);
    trashMenu->setObjectName(QStringLiteral("menuTrash"));
    addMenuAction(trashMenu, QStringLiteral("actionShow_local_trash"));

    auto* sortMenu = new QMenu(tr("Sort by"), noteMenu);
    sortMenu->setObjectName(QStringLiteral("menuSort_by"));
    addMenuAction(sortMenu, QStringLiteral("actionAlphabetical"));
    addMenuAction(sortMenu, QStringLiteral("actionBy_date"));
    sortMenu->addSeparator();
    addMenuAction(sortMenu, QStringLiteral("actionAscending"));
    addMenuAction(sortMenu, QStringLiteral("actionDescending"));

    auto* exportMenu = new QMenu(tr("Export"), noteMenu);
    exportMenu->setObjectName(QStringLiteral("menuExport"));
    addMenuAction(exportMenu, QStringLiteral("action_Export_note_as_PDF_text"));
    addMenuAction(exportMenu, QStringLiteral("action_Export_note_as_PDF_markdown"));
    addMenuAction(exportMenu, QStringLiteral("actionExport_preview_HTML"));
    addMenuAction(exportMenu, QStringLiteral("action_Export_note_as_markdown"));

    auto* importMenu = new QMenu(tr("Import"), noteMenu);
    importMenu->setObjectName(QStringLiteral("menuImport"));
    addMenuAction(importMenu, QStringLiteral("actionImport_notes_from_text_files"));

    auto* printMenu = new QMenu(tr("Print"), noteMenu);
    printMenu->setObjectName(QStringLiteral("menuPrint"));
    addMenuAction(printMenu, QStringLiteral("action_Print_note_text"));
    addMenuAction(printMenu, QStringLiteral("action_Print_note_markdown"));

    auto* viewNoteMenu = new QMenu(tr("View note"), noteMenu);
    viewNoteMenu->setObjectName(QStringLiteral("menuView_note"));
    addMenuAction(viewNoteMenu, QStringLiteral("action_Open_note_in_external_editor"));
    addMenuAction(viewNoteMenu, QStringLiteral("actionShow_note_in_file_manager"));
    addMenuAction(viewNoteMenu, QStringLiteral("actionView_note_in_new_window"));

    addMenuAction(noteMenu, QStringLiteral("action_New_note"));
    addMenuAction(noteMenu, QStringLiteral("actionNew_note_in_new_tab"));
    addMenuAction(noteMenu, QStringLiteral("action_Find_note"));
    addMenuAction(noteMenu, QStringLiteral("actionFind_notes_in_all_subfolders"));
    noteMenu->addMenu(navigationMenu);
    noteMenu->addMenu(trashMenu);
    noteMenu->addMenu(sortMenu);
    addMenuAction(noteMenu, QStringLiteral("actionAllow_note_editing"));
    noteMenu->addSeparator();
    noteMenu->addMenu(exportMenu);
    noteMenu->addMenu(importMenu);
    noteMenu->addMenu(printMenu);
    noteMenu->addSeparator();
    noteMenu->addMenu(viewNoteMenu);
    addMenuAction(noteMenu, QStringLiteral("actionSave_modified_notes"));
    noteMenu->addSeparator();
    addMenuAction(noteMenu, QStringLiteral("action_Reload_note_folder"));
    addMenuAction(noteMenu, QStringLiteral("actionSelect_note_folder"));
    noteMenu->addSeparator();
    addMenuAction(noteMenu, QStringLiteral("action_Settings"));
    noteMenu->addSeparator();
    addMenuAction(noteMenu, QStringLiteral("action_Quit"));

    auto* editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->setObjectName(QStringLiteral("menuEdit"));
    auto* insertMenu = new QMenu(tr("Insert"), editMenu);
    insertMenu->setObjectName(QStringLiteral("menuInsert"));
    addMenuAction(insertMenu, QStringLiteral("actionInsert_text_link"));
    addMenuAction(insertMenu, QStringLiteral("actionInsert_note_link"));
    addMenuAction(insertMenu, QStringLiteral("actionInsert_image"));
    addMenuAction(insertMenu, QStringLiteral("actionInsert_attachment"));
    addMenuAction(insertMenu, QStringLiteral("actionInsert_table"));
    addMenuAction(insertMenu, QStringLiteral("actionInsert_current_time"));
    addMenuAction(insertMenu, QStringLiteral("actionInsert_code_block"));
    addMenuAction(insertMenu, QStringLiteral("actionInsert_block_quote"));
    addMenuAction(insertMenu, QStringLiteral("actionInsert_checkbox_list_item"));
    addMenuAction(insertMenu, QStringLiteral("actionInsert_headline_from_note_filename"));

    auto* formatMenu = new QMenu(tr("Format"), editMenu);
    formatMenu->setObjectName(QStringLiteral("menuFormat"));
    addMenuAction(formatMenu, QStringLiteral("actionFormat_text_bold"));
    addMenuAction(formatMenu, QStringLiteral("actionFormat_text_italic"));
    addMenuAction(formatMenu, QStringLiteral("actionFormat_text_underline"));
    addMenuAction(formatMenu, QStringLiteral("actionStrike_out_text"));
    addMenuAction(formatMenu, QStringLiteral("actionToggle_text_case"));

    auto* extraMenu = new QMenu(tr("Extra"), editMenu);
    extraMenu->setObjectName(QStringLiteral("menuExtra"));
    addMenuAction(extraMenu, QStringLiteral("actionCopy_headline"));
    addMenuAction(extraMenu, QStringLiteral("actionCopy_path_to_note_to_clipboard"));
    addMenuAction(extraMenu, QStringLiteral("actionSearch_text_on_the_web"));
    addMenuAction(extraMenu, QStringLiteral("actionAutocomplete"));
    addMenuAction(extraMenu, QStringLiteral("actionSplit_note_at_cursor_position"));
    addMenuAction(extraMenu, QStringLiteral("actionActivate_context_menu"));

    addMenuAction(editMenu, QStringLiteral("action_DuplicateText"));
    addMenuAction(editMenu, QStringLiteral("action_FormatTable"));
    addMenuAction(editMenu, QStringLiteral("action_Remove_note"));
    editMenu->addSeparator();
    addMenuAction(editMenu, QStringLiteral("action_Find_text_in_note"));
    addMenuAction(editMenu, QStringLiteral("actionReplace_in_current_note"));
    editMenu->addSeparator();
    editMenu->addMenu(insertMenu);
    editMenu->addMenu(formatMenu);
    addMenuAction(editMenu, QStringLiteral("actionSelect_enclosed_text"));
    addMenuAction(editMenu, QStringLiteral("actionSelect_all_notes"));
    addMenuAction(editMenu, QStringLiteral("actionPaste_image"));
    editMenu->addSeparator();
    addMenuAction(editMenu, QStringLiteral("actionManage_stored_images"));
    addMenuAction(editMenu, QStringLiteral("actionManage_stored_attachments"));
    editMenu->addMenu(extraMenu);

    auto* tagMenu = menuBar()->addMenu(tr("&Tag"));
    tagMenu->setObjectName(QStringLiteral("menuTag"));
    addMenuAction(tagMenu, QStringLiteral("action_new_tag"));

    auto* windowMenu = menuBar()->addMenu(tr("&Window"));
    windowMenu->setObjectName(QStringLiteral("menuWindow"));
    auto* showMenu = new QMenu(tr("Show"), windowMenu);
    showMenu->setObjectName(QStringLiteral("menuShow"));
    addMenuAction(showMenu, QStringLiteral("actionShow_menu_bar"));
    addMenuAction(showMenu, QStringLiteral("actionShow_toolbar"));
    addMenuAction(showMenu, QStringLiteral("actionShow_status_bar"));
    addMenuAction(showMenu, QStringLiteral("actionShow_log"));
    addMenuAction(showMenu, QStringLiteral("actionShow_all_panels"));
    addMenuAction(showMenu, QStringLiteral("actionShow_Hide_application"));

    auto* jumpMenu = new QMenu(tr("Jump to"), windowMenu);
    jumpMenu->setObjectName(QStringLiteral("menuJumpTo"));
    addMenuAction(jumpMenu, QStringLiteral("actionJump_to_note_text_edit"));
    addMenuAction(jumpMenu, QStringLiteral("actionJump_to_note_list_panel"));
    addMenuAction(jumpMenu, QStringLiteral("actionJump_to_note_subfolder_panel"));
    addMenuAction(jumpMenu, QStringLiteral("actionJump_to_tags_panel"));
    addMenuAction(jumpMenu, QStringLiteral("actionJump_to_navigation_panel"));

    auto* editorWidthMenu = new QMenu(tr("Editor width"), windowMenu);
    editorWidthMenu->setObjectName(QStringLiteral("menuEditor_width"));
    addMenuAction(editorWidthMenu, QStringLiteral("actionEditorWidthNarrow"));
    addMenuAction(editorWidthMenu, QStringLiteral("actionEditorWidthMedium"));
    addMenuAction(editorWidthMenu, QStringLiteral("actionEditorWidthWide"));
    addMenuAction(editorWidthMenu, QStringLiteral("actionEditorWidthFull"));
    addMenuAction(editorWidthMenu, QStringLiteral("actionEditorWidthCustom"));

    windowMenu->addMenu(showMenu);
    windowMenu->addMenu(jumpMenu);
    addMenuAction(windowMenu, QStringLiteral("actionUnlock_panels"));
    addMenuAction(windowMenu, QStringLiteral("actionReattach_panels"));
    windowMenu->addSeparator();
    addMenuAction(windowMenu, QStringLiteral("actionUse_softwrap_in_note_editor"));
    addMenuAction(windowMenu, QStringLiteral("actionTypewriter_mode"));
    windowMenu->addSeparator();
    addMenuAction(windowMenu, QStringLiteral("actionToggle_distraction_free_mode"));
    windowMenu->addMenu(editorWidthMenu);
    windowMenu->addSeparator();
    addMenuAction(windowMenu, QStringLiteral("actionToggle_fullscreen"));
    addMenuAction(windowMenu, QStringLiteral("actionToggle_Always_on_top"));
    windowMenu->addSeparator();
    addMenuAction(windowMenu, QStringLiteral("action_Increase_note_text_size"));
    addMenuAction(windowMenu, QStringLiteral("action_Decrease_note_text_size"));
    addMenuAction(windowMenu, QStringLiteral("action_Reset_note_text_size"));

    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->setObjectName(QStringLiteral("menu_Help"));
    addMenuAction(helpMenu, QStringLiteral("actionFind_action"));
    addMenuAction(helpMenu, QStringLiteral("actionRedo_action"));
    helpMenu->addSeparator();
    addMenuAction(helpMenu, QStringLiteral("actionAbout_ArcNotes"));
}

void MainWindow::setupToolBar() {
    auto* toolbar = addToolBar(tr("main toolbar"));
    toolbar->setObjectName(QStringLiteral("mainToolBar"));
    toolbar->setMovable(true);
    toolbar->setIconSize(QSize(24, 24));
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    const QStringList actionOrder = {
        QStringLiteral("action_New_note"),
        QStringLiteral("action_Find_note"),
        QStringLiteral("action_Remove_note"),
        QStringLiteral("action_Open_note_in_external_editor"),
        QStringLiteral("actionShow_local_trash"),
        QStringLiteral("actionAllow_note_editing"),
        QStringLiteral("separator"),
        QStringLiteral("action_Back_in_note_history"),
        QStringLiteral("action_Forward_in_note_history"),
        QStringLiteral("separator"),
        QStringLiteral("action_Find_text_in_note"),
        QStringLiteral("actionReplace_in_current_note"),
        QStringLiteral("separator"),
        QStringLiteral("actionFormat_text_bold"),
        QStringLiteral("actionFormat_text_italic"),
        QStringLiteral("actionStrike_out_text"),
        QStringLiteral("actionInsert_text_link"),
        QStringLiteral("actionInsert_note_link"),
        QStringLiteral("actionInsert_image"),
        QStringLiteral("actionInsert_attachment"),
        QStringLiteral("actionInsert_table"),
        QStringLiteral("separator"),
        QStringLiteral("actionToggle_distraction_free_mode"),
    };

    for (const QString& objectName : actionOrder) {
        if (objectName == QStringLiteral("separator")) {
            toolbar->addSeparator();
        } else {
            toolbar->addAction(findAction(objectName));
        }
    }

    toolbar->addSeparator();
    _layoutComboBox = new ComboBox(toolbar);
    _layoutComboBox->setObjectName(QStringLiteral("layoutComboBox"));
    _layoutComboBox->setToolTip(tr("Layouts"));
    toolbar->addWidget(_layoutComboBox);
    toolbar->addAction(findAction(QStringLiteral("actionManage_layouts")));
    toolbar->addAction(findAction(QStringLiteral("actionSwitch_to_previous_layout")));
    updateLayoutComboBox();
    connect(_layoutComboBox, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        if (_layoutComboBox == nullptr || index < 0) {
            return;
        }
        setCurrentLayout(_layoutComboBox->itemData(index).toString());
    });

    applySavedToolBarConfiguration();
}

void MainWindow::applySavedToolBarConfiguration() {
    const QVector<QVariantMap> toolbars = persistentSettingsArray(
        QStringLiteral("toolbar"), {QStringLiteral("name"), QStringLiteral("title"), QStringLiteral("items")});
    for (const QVariantMap& toolbarSettings : toolbars) {
        ToolbarContainer toolbar(toolbarSettings.value(QStringLiteral("name")).toString(),
                                 toolbarSettings.value(QStringLiteral("title")).toString(),
                                 toolbarSettings.value(QStringLiteral("items")).toStringList());
        if (toolbar.name.isEmpty()) {
            continue;
        }

        if (toolbar.toolbarFound(this)) {
            toolbar.updateToolbar(this);
        } else {
            toolbar.create(this);
        }
    }
}

void MainWindow::wireActions() {
    auto withEditor = [this](auto callback) {
        if (_noteTextEdit != nullptr) {
            callback(_noteTextEdit);
        }
    };
    auto applyFormatter = [this](const QString& formatter) {
        if (_noteTextEdit == nullptr) {
            return;
        }

        QTextCursor cursor = _noteTextEdit->textCursor();
        QString selectedText = cursor.selectedText();
        const bool endsWithLineBreak = !selectedText.isEmpty() && selectedText.endsWith(QChar::ParagraphSeparator);
        if (endsWithLineBreak) {
            selectedText.chop(1);
        }

        if (selectedText.isEmpty()) {
            cursor.insertText(formatter.repeated(2));
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, formatter.length());
            _noteTextEdit->setTextCursor(cursor);
            return;
        }

        const QRegularExpressionMatch match =
            QRegularExpression(QStringLiteral(R"(^(\s*)(.+?)(\s*)$)")).match(selectedText);
        if (!match.hasMatch()) {
            return;
        }

        QString formattedText = match.captured(1) + formatter + match.captured(2) + formatter + match.captured(3);
        if (endsWithLineBreak) {
            formattedText += QChar::ParagraphSeparator;
        }
        cursor.insertText(formattedText);
    };
    auto replaceFullLineSelection = [this](const QString& text) {
        if (_noteTextEdit != nullptr) {
            _noteTextEdit->replaceFullLineSelection(text);
        }
    };

    connect(findAction(QStringLiteral("action_New_note")), &QAction::triggered, this, &MainWindow::createDefaultNote);
    connect(findAction(QStringLiteral("actionNew_note_in_new_tab")), &QAction::triggered, this,
            &MainWindow::createDefaultNote);
    connect(findAction(QStringLiteral("action_Remove_note")), &QAction::triggered, this, [this]() {
        if (_coordinator != nullptr) {
            _coordinator->noteListViewModel()->deleteSelectedNotes(true);
        }
    });
    connect(findAction(QStringLiteral("action_Find_note")), &QAction::triggered, this, [this]() {
        if (_searchLineEdit != nullptr) {
            _searchLineEdit->setFocus();
            _searchLineEdit->selectAll();
        }
    });
    connect(findAction(QStringLiteral("action_Find_text_in_note")), &QAction::triggered, this,
            [this]() { doSearchInNote(QString()); });
    connect(findAction(QStringLiteral("action_Settings")), &QAction::triggered, this,
            [this]() { openSettingsDialog(); });
    connect(findAction(QStringLiteral("actionAbout_ArcNotes")), &QAction::triggered, this, [this]() {
        auto* dialog = new AboutDialog(this);
        dialog->exec();
        delete dialog;
    });
    connect(findAction(QStringLiteral("actionManage_layouts")), &QAction::triggered, this, [this]() {
        auto* dialog = new LayoutDialog(this);
        dialog->exec();
        delete dialog;
    });
    connect(findAction(QStringLiteral("action_new_tag")), &QAction::triggered, this, [this]() {
        TagAddDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted && !dialog.name().trimmed().isEmpty() && _coordinator != nullptr) {
            _coordinator->tagTreeViewModel()->createTag(dialog.name().trimmed());
        }
    });
    connect(findAction(QStringLiteral("action_Quit")), &QAction::triggered, this, &QWidget::close);
    connect(findAction(QStringLiteral("actionShow_toolbar")), &QAction::toggled, this, [this](bool visible) {
        if (auto* toolbar = findChild<QToolBar*>(QStringLiteral("mainToolBar"))) {
            toolbar->setVisible(visible);
        }
    });
    connect(findAction(QStringLiteral("actionShow_status_bar")), &QAction::toggled, statusBar(),
            &QStatusBar::setVisible);
    connect(findAction(QStringLiteral("actionToggle_fullscreen")), &QAction::triggered, this,
            [this]() { isFullScreen() ? showNormal() : showFullScreen(); });
    connect(findAction(QStringLiteral("actionToggle_distraction_free_mode")), &QAction::toggled, this,
            [this](bool enabled) { applyDistractionFreeMode(enabled); });

    connect(findAction(QStringLiteral("action_Print_note_text")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit != nullptr) {
            printTextDocument(_noteTextEdit->document(), true);
        }
    });
    connect(findAction(QStringLiteral("action_Print_note_markdown")), &QAction::triggered, this, [this]() {
        QTextDocument* document = createCurrentNotePreviewDocument(this);
        printTextDocument(document);
        delete document;
    });
    connect(findAction(QStringLiteral("action_Export_note_as_PDF_text")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit != nullptr) {
            exportNoteAsPDF(_noteTextEdit->document(), true);
        }
    });
    connect(findAction(QStringLiteral("action_Export_note_as_PDF_markdown")), &QAction::triggered, this, [this]() {
        QTextDocument* document = createCurrentNotePreviewDocument(this);
        exportNoteAsPDF(document);
        delete document;
    });
    connect(findAction(QStringLiteral("actionExport_preview_HTML")), &QAction::triggered, this, [this]() {
        if (_currentNote.id <= 0 || _coordinator == nullptr) {
            return;
        }

        FileDialog dialog(QStringLiteral("NoteHTMLExport"));
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setNameFilter(tr("HTML files") + QStringLiteral(" (*.html)"));
        dialog.setWindowTitle(tr("Export current note as HTML file"));
        dialog.selectFile(_currentNote.name + QStringLiteral(".html"));
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        QString fileName = dialog.selectedFile();
        if (fileName.isEmpty()) {
            return;
        }
        if (QFileInfo(fileName).suffix().isEmpty()) {
            fileName.append(QStringLiteral(".html"));
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return;
        }

        QTextStream out(&file);
        out << _coordinator->renderTextToHtml(_currentNote, _currentNote.noteText, currentNoteFolderPath(),
                                              getMaxImageWidth(), true, true);
        file.close();
        Utils::Misc::openFolderSelect(fileName, QStringLiteral("show-exported-note-html-in-file-manager"));
    });
    connect(findAction(QStringLiteral("action_Export_note_as_markdown")), &QAction::triggered, this, [this]() {
        if (_currentNote.id <= 0) {
            return;
        }

        FileDialog dialog(QStringLiteral("NoteMarkdownExport"));
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setNameFilter(tr("Markdown files") + QStringLiteral(" (*.md)"));
        dialog.setWindowTitle(tr("Export current note as Markdown file"));
        dialog.selectFile(_currentNote.name + QStringLiteral(".md"));
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        QString fileName = dialog.selectedFile();
        if (fileName.isEmpty()) {
            return;
        }
        if (QFileInfo(fileName).suffix().isEmpty()) {
            fileName.append(QStringLiteral(".md"));
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return;
        }
        QTextStream out(&file);
        out << _noteTextEdit->toPlainText();
        file.close();
        Utils::Misc::openFolderSelect(fileName, QStringLiteral("show-exported-markdown-in-file-manager"));
    });

    connect(findAction(QStringLiteral("actionFormat_text_bold")), &QAction::triggered, this,
            [applyFormatter]() { applyFormatter(QStringLiteral("**")); });
    connect(findAction(QStringLiteral("actionFormat_text_italic")), &QAction::triggered, this,
            [applyFormatter]() { applyFormatter(QStringLiteral("*")); });
    connect(findAction(QStringLiteral("actionFormat_text_underline")), &QAction::triggered, this,
            [applyFormatter]() { applyFormatter(QStringLiteral("__")); });
    connect(findAction(QStringLiteral("actionStrike_out_text")), &QAction::triggered, this,
            [applyFormatter]() { applyFormatter(QStringLiteral("~~")); });
    connect(findAction(QStringLiteral("action_DuplicateText")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->duplicateText(); }); });
    connect(findAction(QStringLiteral("actionToggle_checkboxes")), &QAction::triggered, this,
            [this, replaceFullLineSelection]() {
                if (_noteTextEdit != nullptr) {
                    replaceFullLineSelection(
                        Utils::ListUtils::toggleCheckboxes(_noteTextEdit->fullLineSelectionCursor().selectedText()));
                }
            });
    connect(findAction(QStringLiteral("actionCreate_ordered_list")), &QAction::triggered, this,
            [this, replaceFullLineSelection]() {
                if (_noteTextEdit != nullptr) {
                    replaceFullLineSelection(
                        Utils::ListUtils::createOrderedList(_noteTextEdit->fullLineSelectionCursor().selectedText()));
                }
            });
    connect(findAction(QStringLiteral("actionCreate_alphabetical_list")), &QAction::triggered, this,
            [this, replaceFullLineSelection]() {
                if (_noteTextEdit != nullptr) {
                    replaceFullLineSelection(Utils::ListUtils::createAlphabeticalList(
                        _noteTextEdit->fullLineSelectionCursor().selectedText()));
                }
            });
    connect(findAction(QStringLiteral("actionCreate_unordered_list")), &QAction::triggered, this,
            [this, replaceFullLineSelection]() {
                if (_noteTextEdit != nullptr) {
                    replaceFullLineSelection(
                        Utils::ListUtils::createUnorderedList(_noteTextEdit->fullLineSelectionCursor().selectedText()));
                }
            });
    connect(findAction(QStringLiteral("actionCreate_checkbox_list")), &QAction::triggered, this,
            [this, replaceFullLineSelection]() {
                if (_noteTextEdit != nullptr) {
                    replaceFullLineSelection(
                        Utils::ListUtils::createCheckboxList(_noteTextEdit->fullLineSelectionCursor().selectedText()));
                }
            });
    connect(findAction(QStringLiteral("actionClear_list_formatting")), &QAction::triggered, this,
            [this, replaceFullLineSelection]() {
                if (_noteTextEdit != nullptr) {
                    replaceFullLineSelection(
                        Utils::ListUtils::clearListFormatting(_noteTextEdit->fullLineSelectionCursor().selectedText()));
                }
            });
    connect(findAction(QStringLiteral("actionOrder_checkboxes")), &QAction::triggered, this,
            [this, replaceFullLineSelection]() {
                if (_noteTextEdit != nullptr) {
                    replaceFullLineSelection(
                        Utils::ListUtils::orderCheckboxes(_noteTextEdit->fullLineSelectionCursor().selectedText()));
                }
            });
    connect(findAction(QStringLiteral("actionIncrease_heading_depth")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->changeHeadingDepthOfSelection(1); }); });
    connect(findAction(QStringLiteral("actionDecrease_heading_depth")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->changeHeadingDepthOfSelection(-1); }); });
    connect(findAction(QStringLiteral("actionCopy_code_block")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit != nullptr) {
            Utils::Gui::copyCodeBlockText(_noteTextEdit->textCursor().block());
        }
    });
    connect(findAction(QStringLiteral("actionFold_all_headings")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->foldAllHeadings(); }); });
    connect(findAction(QStringLiteral("actionUnfold_all_headings")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->unfoldAllHeadings(); }); });
    connect(findAction(QStringLiteral("actionInsert_current_time")), &QAction::triggered, this,
            [this]() { writeToNoteTextEdit(QDateTime::currentDateTime().toString(Qt::ISODate)); });
    connect(findAction(QStringLiteral("actionInsert_checkbox_list_item")), &QAction::triggered, this,
            [this]() { writeToNoteTextEdit(QStringLiteral("- [ ] ")); });
    connect(findAction(QStringLiteral("actionAutocomplete")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->onAutoCompleteRequested(); }); });
    connect(findAction(QStringLiteral("actionPaste_image")), &QAction::triggered, this,
            [this]() { handleInsertingFromMimeData(QApplication::clipboard()->mimeData()); });
    connect(findAction(QStringLiteral("action_Increase_note_text_size")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { emit editor->zoomIn(); }); });
    connect(findAction(QStringLiteral("action_Decrease_note_text_size")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { emit editor->zoomOut(); }); });
    connect(findAction(QStringLiteral("action_Reset_note_text_size")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit != nullptr) {
            const int fontSize = _noteTextEdit->modifyFontSize(ArcNotesMarkdownTextEdit::Reset);
            showStatusBarMessage(tr("Reset font size to %1 pt").arg(fontSize), 3000);
            updatePreview();
        }
    });
    connect(findAction(QStringLiteral("actionInsert_text_link")), &QAction::triggered, this,
            [this]() { handleTextNoteLinking(LinkDialog::TextLinkPage); });
    connect(findAction(QStringLiteral("actionInsert_note_link")), &QAction::triggered, this,
            [this]() { handleTextNoteLinking(LinkDialog::NoteLinkPage); });
    connect(findAction(QStringLiteral("actionInsert_image")), &QAction::triggered, this,
            [this]() { handleImageInsertion(); });
    connect(findAction(QStringLiteral("actionInsert_attachment")), &QAction::triggered, this,
            [this]() { handleAttachmentInsertion(); });
    connect(findAction(QStringLiteral("actionManage_stored_images")), &QAction::triggered, this, [this]() {
        auto* dialog = new StoredImagesDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    });
    connect(findAction(QStringLiteral("actionManage_stored_attachments")), &QAction::triggered, this, [this]() {
        auto* dialog = new StoredAttachmentsDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    });
    connect(findAction(QStringLiteral("actionInsert_code_block")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit != nullptr) {
            _noteTextEdit->insertCodeBlock();
        }
    });
    connect(findAction(QStringLiteral("actionInsert_block_quote")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit != nullptr) {
            _noteTextEdit->insertBlockQuote();
        }
    });
    connect(findAction(QStringLiteral("actionInsert_table")), &QAction::triggered, this, [this]() {
        auto* dialog = new TableDialog(_noteTextEdit, this);
        dialog->exec();
        delete dialog;
    });
    connect(findAction(QStringLiteral("action_FormatTable")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit != nullptr) {
            Utils::Gui::autoFormatTableAtCursor(_noteTextEdit);
        }
    });
    connect(findAction(QStringLiteral("actionToggle_text_case")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit != nullptr) {
            _noteTextEdit->toggleCase();
        }
    });
    connect(findAction(QStringLiteral("actionSelect_enclosed_text")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit != nullptr) {
            _noteTextEdit->selectEnclosedText();
        }
    });
    connect(findAction(QStringLiteral("actionSave_modified_notes")), &QAction::triggered, this,
            &MainWindow::storeUpdatedNotesToDisk);
    connect(findAction(QStringLiteral("action_Reload_note_folder")), &QAction::triggered, this,
            [this]() { buildNotesIndexAndLoadNoteDirectoryList(true, true); });
    connect(findAction(QStringLiteral("actionShow_note_in_file_manager")), &QAction::triggered, this, [this]() {
        if (_currentNote.id > 0) {
            Utils::Misc::openFolderSelect(_currentNote.fullNoteFilePath, QStringLiteral("show-note-in-file-manager"));
        }
    });
    connect(findAction(QStringLiteral("action_Open_note_in_external_editor")), &QAction::triggered, this, [this]() {
        if (_currentNote.id > 0) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(_currentNote.fullNoteFilePath));
        }
    });
    connect(findAction(QStringLiteral("actionCopy_path_to_note_to_clipboard")), &QAction::triggered, this, [this]() {
        if (_currentNote.id > 0) {
            QApplication::clipboard()->setText(_currentNote.fullNoteFilePath);
        }
    });
    connect(findAction(QStringLiteral("actionCopy_headline")), &QAction::triggered, this, [this]() {
        if (_currentNote.id > 0) {
            QApplication::clipboard()->setText(_currentNote.name);
        }
    });
    connect(findAction(QStringLiteral("actionSearch_text_on_the_web")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit == nullptr) {
            return;
        }
        const QString selectedText = _noteTextEdit->textCursor().selectedText().trimmed();
        if (selectedText.isEmpty()) {
            return;
        }
        const int searchEngineId =
            persistentSetting(QStringLiteral("SearchEngineId"), Utils::Misc::getDefaultSearchEngineId()).toInt();
        const auto engines = Utils::Misc::getSearchEnginesHashMap();
        const QString searchUrl = engines.value(searchEngineId).searchUrl;
        QDesktopServices::openUrl(QUrl(searchUrl + QUrl::toPercentEncoding(selectedText)));
    });
    connect(findAction(QStringLiteral("actionDelete_line")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit == nullptr) {
            return;
        }
        QTextCursor cursor = _noteTextEdit->textCursor();
        cursor.select(QTextCursor::BlockUnderCursor);
        if (cursor.selectedText().isEmpty()) {
            cursor.deletePreviousChar();
        } else {
            cursor.removeSelectedText();
        }
        cursor.movePosition(QTextCursor::NextBlock);
        _noteTextEdit->setTextCursor(cursor);
    });
    connect(findAction(QStringLiteral("actionDelete_word")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit == nullptr) {
            return;
        }
        QTextCursor cursor = _noteTextEdit->textCursor();
        if (cursor.selectedText().isEmpty()) {
            cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
        }
        cursor.removeSelectedText();
    });
    connect(findAction(QStringLiteral("actionView_note_in_new_window")), &QAction::triggered, this, [this]() {
        if (_currentNote.id <= 0) {
            return;
        }
        auto* dialog = new NoteDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setNoteFolderPath(currentNoteFolderPath());
        dialog->setNote(_coordinator->appState()->currentNote());
        dialog->show();
    });
    connect(findAction(QStringLiteral("actionShow_local_trash")), &QAction::triggered, this, [this]() {
        auto* dialog = new LocalTrashDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    });
    connect(findAction(QStringLiteral("actionShow_log")), &QAction::toggled, this, [this](bool visible) {
        if (auto* dock = findChild<QDockWidget*>(QStringLiteral("logDockWidget"))) {
            dock->setVisible(visible);
        }
    });
    connect(findAction(QStringLiteral("actionShow_menu_bar")), &QAction::toggled, menuBar(), &QMenuBar::setVisible);
    connect(findAction(QStringLiteral("actionShow_all_panels")), &QAction::triggered, this, [this]() {
        const QList<QDockWidget*> docks = findChildren<QDockWidget*>();
        for (QDockWidget* dock : docks) {
            dock->show();
        }
        applyDockLocking(findAction(QStringLiteral("actionUnlock_panels"))->isChecked());
        updatePreview();
    });
    connect(findAction(QStringLiteral("actionReattach_panels")), &QAction::triggered, this, [this]() {
        const QList<QDockWidget*> docks = findChildren<QDockWidget*>();
        for (QDockWidget* dock : docks) {
            dock->setFloating(false);
        }
        applyDockLocking(findAction(QStringLiteral("actionUnlock_panels"))->isChecked());
    });
    connect(findAction(QStringLiteral("actionUnlock_panels")), &QAction::toggled, this,
            [this](bool unlocked) { applyDockLocking(unlocked); });
    connect(findAction(QStringLiteral("actionUse_softwrap_in_note_editor")), &QAction::toggled, this,
            [withEditor](bool enabled) {
                withEditor([enabled](auto* editor) {
                    editor->setLineWrapMode(enabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
                });
            });
    connect(findAction(QStringLiteral("actionTypewriter_mode")), &QAction::toggled, this, [withEditor](bool enabled) {
        withEditor([enabled](auto* editor) { editor->setCenterOnScroll(enabled); });
    });
    connect(findAction(QStringLiteral("actionToggle_Always_on_top")), &QAction::toggled, this, [this](bool enabled) {
        Qt::WindowFlags flags = windowFlags();
        flags.setFlag(Qt::WindowStaysOnTopHint, enabled);
        setWindowFlags(flags);
        show();
    });
    connect(findAction(QStringLiteral("actionFind_notes_in_all_subfolders")), &QAction::toggled, this,
            &MainWindow::setShowNotesFromAllNoteSubFolders);
    connect(findAction(QStringLiteral("actionJump_to_note_text_edit")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->setFocus(); }); });
    connect(findAction(QStringLiteral("actionJump_to_note_list_panel")), &QAction::triggered, this, [this]() {
        if (_noteListView != nullptr) {
            _noteListView->setFocus();
        }
    });
    connect(findAction(QStringLiteral("actionJump_to_note_subfolder_panel")), &QAction::triggered, this, [this]() {
        if (_noteSubFolderView != nullptr) {
            _noteSubFolderView->setFocus();
        }
    });
    connect(findAction(QStringLiteral("actionJump_to_tags_panel")), &QAction::triggered, this, [this]() {
        if (_tagTreeView != nullptr) {
            _tagTreeView->setFocus();
        }
    });
    connect(findAction(QStringLiteral("actionJump_to_navigation_panel")), &QAction::triggered, this, [this]() {
        if (_navigationView != nullptr) {
            _navigationView->setFocus();
        }
    });
    connect(findAction(QStringLiteral("actionSelect_all_notes")), &QAction::triggered, this, [this]() {
        if (_noteListView != nullptr) {
            _noteListView->selectAll();
        }
    });
    connect(_noteListView, &QWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { showNoteListContextMenu(pos); });
    connect(_tagTreeView, &QWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { showTagTreeContextMenu(pos); });
    connect(_noteSubFolderView, &QWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { showNoteSubFolderContextMenu(pos); });
    connect(_navigationView, &QWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { showNavigationContextMenu(pos); });
    connect(findAction(QStringLiteral("actionNext_note")), &QAction::triggered, this, [this]() {
        if (_noteListView == nullptr || _coordinator == nullptr) {
            return;
        }
        const QModelIndex current = _noteListView->currentIndex();
        const int nextRow = qMin(current.row() + 1, _coordinator->noteListViewModel()->model()->rowCount() - 1);
        if (nextRow >= 0) {
            _noteListView->setCurrentIndex(_coordinator->noteListViewModel()->model()->index(nextRow, 0));
        }
    });
    connect(findAction(QStringLiteral("actionPrevious_Note")), &QAction::triggered, this, [this]() {
        if (_noteListView == nullptr || _coordinator == nullptr) {
            return;
        }
        const QModelIndex current = _noteListView->currentIndex();
        const int previousRow = qMax(current.row() - 1, 0);
        if (previousRow >= 0) {
            _noteListView->setCurrentIndex(_coordinator->noteListViewModel()->model()->index(previousRow, 0));
        }
    });
    connect(findAction(QStringLiteral("actionClose_current_note_tab")), &QAction::triggered, this, [this]() {
        if (_noteEditTabWidget != nullptr && _noteEditTabWidget->count() > 1) {
            _noteEditTabWidget->removeTab(_noteEditTabWidget->currentIndex());
        }
    });
    connect(findAction(QStringLiteral("actionPrevious_note_tab")), &QAction::triggered, this, [this]() {
        if (_noteEditTabWidget == nullptr || _noteEditTabWidget->count() == 0) {
            return;
        }
        int index = _noteEditTabWidget->currentIndex() - 1;
        if (index < 0) {
            index = _noteEditTabWidget->count() - 1;
        }
        _noteEditTabWidget->setCurrentIndex(index);
    });
    connect(findAction(QStringLiteral("actionNext_note_tab")), &QAction::triggered, this, [this]() {
        if (_noteEditTabWidget == nullptr || _noteEditTabWidget->count() == 0) {
            return;
        }
        int index = _noteEditTabWidget->currentIndex() + 1;
        if (index >= _noteEditTabWidget->count()) {
            index = 0;
        }
        _noteEditTabWidget->setCurrentIndex(index);
    });
    for (int slot = 1; slot <= 9; ++slot) {
        connect(findAction(QString::fromLatin1(kStoreBookmarkActionNames[slot - 1])), &QAction::triggered, this,
                [this, slot]() { storeNoteBookmark(slot); });
        connect(findAction(QString::fromLatin1(kGotoBookmarkActionNames[slot - 1])), &QAction::triggered, this,
                [this, slot]() { gotoNoteBookmark(slot); });
    }
    connect(findAction(QStringLiteral("actionOpen_note_bookmark_dialog")), &QAction::triggered, this,
            &MainWindow::openNoteBookmarkDialog);
    connect(findAction(QStringLiteral("actionAlphabetical")), &QAction::triggered, this, [this]() {
        if (_coordinator != nullptr) {
            _coordinator->noteListViewModel()->sortBy(QStringLiteral("name"), QStringLiteral("ascending"));
        }
    });
    connect(findAction(QStringLiteral("actionBy_date")), &QAction::triggered, this, [this]() {
        if (_coordinator != nullptr) {
            _coordinator->noteListViewModel()->sortBy(QStringLiteral("modified"), QStringLiteral("descending"));
        }
    });
    connect(findAction(QStringLiteral("actionAscending")), &QAction::triggered, this, [this]() {
        if (_coordinator != nullptr) {
            _coordinator->noteListViewModel()->sortBy(_coordinator->noteListViewModel()->sortField(),
                                                      QStringLiteral("ascending"));
        }
    });
    connect(findAction(QStringLiteral("actionDescending")), &QAction::triggered, this, [this]() {
        if (_coordinator != nullptr) {
            _coordinator->noteListViewModel()->sortBy(_coordinator->noteListViewModel()->sortField(),
                                                      QStringLiteral("descending"));
        }
    });
    connect(findAction(QStringLiteral("actionReplace_in_current_note")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->searchWidget()->activateReplace(); }); });
    connect(findAction(QStringLiteral("actionFind_action")), &QAction::triggered, this, [this]() {
        auto* commandBar = new CommandBar(this);
        QVector<QPair<QString, QAction*>> actions;
        const QList<QMenu*> menus = menuList();
        for (QMenu* menu : menus) {
            const QString menuTitle = menu->title();
            for (QAction* action : menu->actions()) {
                if (action == nullptr || action->isSeparator() || action->menu() != nullptr ||
                    action->objectName().isEmpty()) {
                    continue;
                }
                actions.append(qMakePair(menuTitle, action));
            }
        }
        commandBar->updateBar(actions);
        connect(commandBar, &QMenu::aboutToHide, commandBar, &QObject::deleteLater);
        commandBar->popup(QCursor::pos());
    });
    connect(findAction(QStringLiteral("actionStore_as_new_layout")), &QAction::triggered, this, [this]() {
        const QString name = QInputDialog::getText(this, tr("Create new layout"), tr("Layout name:")).trimmed();
        if (name.isEmpty()) {
            return;
        }
        const QString uuid = Utils::Misc::createUuidString();
        QStringList layouts = persistentSetting(QStringLiteral("layouts")).toStringList();
        layouts.append(uuid);
        setPersistentSetting(QStringLiteral("layouts"), layouts);
        setPersistentSetting(QStringLiteral("layout-") + uuid + QStringLiteral("/name"), name);
        setPersistentSetting(QStringLiteral("layout-") + uuid + QStringLiteral("/windowState"), saveState());
        setCurrentLayout(uuid);
        updateLayoutComboBox();
    });
    connect(findAction(QStringLiteral("actionRemove_current_layout")), &QAction::triggered, this, [this]() {
        QStringList layouts = persistentSetting(QStringLiteral("layouts")).toStringList();
        const QString uuid = persistentSetting(QStringLiteral("currentLayout")).toString();
        if (uuid.isEmpty() || layouts.count() < 2) {
            return;
        }
        layouts.removeAll(uuid);
        setPersistentSetting(QStringLiteral("layouts"), layouts);
        removePersistentSetting(QStringLiteral("layout-") + uuid);
        setCurrentLayout(layouts.value(0));
        updateLayoutComboBox();
    });
    connect(findAction(QStringLiteral("actionRename_current_layout")), &QAction::triggered, this, [this]() {
        const QString uuid = persistentSetting(QStringLiteral("currentLayout")).toString();
        if (uuid.isEmpty()) {
            return;
        }
        const QString oldName =
            persistentSetting(QStringLiteral("layout-") + uuid + QStringLiteral("/name")).toString();
        const QString newName =
            QInputDialog::getText(this, tr("Rename layout"), tr("Layout name:"), QLineEdit::Normal, oldName).trimmed();
        if (!newName.isEmpty()) {
            setPersistentSetting(QStringLiteral("layout-") + uuid + QStringLiteral("/name"), newName);
            updateLayoutComboBox();
        }
    });
    connect(findAction(QStringLiteral("actionSwitch_to_previous_layout")), &QAction::triggered, this, [this]() {
        const QString uuid = persistentSetting(QStringLiteral("previousLayout")).toString();
        setCurrentLayout(uuid);
    });
    connect(findAction(QStringLiteral("actionInsert_headline_from_note_filename")), &QAction::triggered, this,
            [this]() {
                if (_currentNote.id > 0) {
                    writeToNoteTextEdit(QStringLiteral("# ") + _currentNote.name + QStringLiteral("\n\n"));
                }
            });
    connect(findAction(QStringLiteral("actionEditorWidthNarrow")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->setPaperMargins(500); }); });
    connect(findAction(QStringLiteral("actionEditorWidthMedium")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->setPaperMargins(700); }); });
    connect(findAction(QStringLiteral("actionEditorWidthWide")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->setPaperMargins(900); }); });
    connect(findAction(QStringLiteral("actionEditorWidthFull")), &QAction::triggered, this,
            [withEditor]() { withEditor([](auto* editor) { editor->setPaperMargins(0); }); });
    connect(findAction(QStringLiteral("actionEditorWidthCustom")), &QAction::triggered, this, [this, withEditor]() {
        bool ok = false;
        const int width =
            QInputDialog::getInt(this, tr("Editor width"), tr("Width:"), getMaxImageWidth(), 100, 5000, 50, &ok);
        if (ok) {
            withEditor([width](auto* editor) { editor->setPaperMargins(width); });
        }
    });
    auto jumpHeading = [this](bool forward) {
        if (_noteTextEdit == nullptr) {
            return;
        }
        QTextBlock block = _noteTextEdit->textCursor().block();
        while (block.isValid()) {
            block = forward ? block.next() : block.previous();
            if (!block.isValid()) {
                break;
            }
            if (block.text().trimmed().startsWith(QLatin1Char('#'))) {
                QTextCursor cursor(block);
                _noteTextEdit->setTextCursor(cursor);
                _noteTextEdit->centerCursor();
                return;
            }
        }
    };
    connect(findAction(QStringLiteral("action_Previous_heading")), &QAction::triggered, this,
            [jumpHeading]() { jumpHeading(false); });
    connect(findAction(QStringLiteral("action_Next_heading")), &QAction::triggered, this,
            [jumpHeading]() { jumpHeading(true); });
    connect(findAction(QStringLiteral("actionActivate_context_menu")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit != nullptr) {
            const QPoint position = _noteTextEdit->cursorRect().center();
            QContextMenuEvent event(QContextMenuEvent::Keyboard, position, _noteTextEdit->mapToGlobal(position));
            QApplication::sendEvent(_noteTextEdit, &event);
        }
    });
    connect(findAction(QStringLiteral("actionShow_Hide_application")), &QAction::triggered, this,
            [this]() { isVisible() ? hide() : show(); });
    connect(findAction(QStringLiteral("actionUse_vertical_preview_layout")), &QAction::toggled, this,
            [this](bool enabled) {
                if (_notePreviewDockWidget != nullptr) {
                    removeDockWidget(_notePreviewDockWidget);
                    addDockWidget(enabled ? Qt::BottomDockWidgetArea : Qt::RightDockWidgetArea, _notePreviewDockWidget,
                                  enabled ? Qt::Vertical : Qt::Horizontal);
                    _notePreviewDockWidget->show();
                    applyDockLocking(findAction(QStringLiteral("actionUnlock_panels"))->isChecked());
                    updatePreview();
                }
            });
    connect(findAction(QStringLiteral("actionUse_one_column_mode")), &QAction::toggled, this, [this](bool enabled) {
        if (_mainSplitter != nullptr) {
            _mainSplitter->setOrientation(enabled ? Qt::Vertical : Qt::Horizontal);
        }
    });
    connect(findAction(QStringLiteral("actionSplit_note_at_cursor_position")), &QAction::triggered, this, [this]() {
        if (_noteTextEdit == nullptr) {
            return;
        }
        QTextCursor cursor = _noteTextEdit->textCursor();
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        const QString tailText = cursor.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
        if (tailText.trimmed().isEmpty()) {
            return;
        }
        cursor.removeSelectedText();
        storeUpdatedNotesToDisk();
        createNewNote(
            tr("Split note %1").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH'h'mm's'ss"))),
            tailText, CursorAtEnd);
    });
    connect(findAction(QStringLiteral("actionImport_notes_from_text_files")), &QAction::triggered, this, [this]() {
        FileDialog dialog(QStringLiteral("ImportTextNotes"));
        dialog.setFileMode(QFileDialog::ExistingFiles);
        dialog.setNameFilter(tr("Text files") + QStringLiteral(" (*.txt *.md)"));
        dialog.setWindowTitle(tr("Import notes from text files"));
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        const QString notePath = currentNoteFolderPath();
        if (notePath.isEmpty()) {
            return;
        }
        for (const QString& fileName : dialog.selectedFiles()) {
            const QFileInfo fileInfo(fileName);
            QFile::copy(fileName, QDir(notePath).filePath(fileInfo.fileName()));
        }
        buildNotesIndexAndLoadNoteDirectoryList(true, true);
    });
}

void MainWindow::addMenuAction(QMenu* menu, const QString& objectName) {
    if (menu != nullptr) {
        menu->addAction(findAction(objectName));
    }
}

QVector<int> MainWindow::selectedNoteIds() const {
    QVector<int> noteIds;
    if (_noteListView == nullptr || _noteListView->selectionModel() == nullptr) {
        return noteIds;
    }

    const QModelIndexList rows = _noteListView->selectionModel()->selectedRows();
    for (const QModelIndex& index : rows) {
        const int noteId = index.data(NoteListModel::NoteIdRole).toInt();
        if (noteId > 0 && !noteIds.contains(noteId)) {
            noteIds.append(noteId);
        }
    }

    if (noteIds.isEmpty() && _noteListView->currentIndex().isValid()) {
        const int noteId = _noteListView->currentIndex().data(NoteListModel::NoteIdRole).toInt();
        if (noteId > 0) {
            noteIds.append(noteId);
        }
    }

    return noteIds;
}

QVector<int> MainWindow::selectedTagIds() const {
    QVector<int> tagIds;
    if (_tagTreeView == nullptr || _tagTreeView->selectionModel() == nullptr) {
        return tagIds;
    }

    const QModelIndexList rows = _tagTreeView->selectionModel()->selectedRows();
    for (const QModelIndex& index : rows) {
        const int tagId = index.data(TagTreeModel::TagIdRole).toInt();
        if (tagId > 0 && !tagIds.contains(tagId)) {
            tagIds.append(tagId);
        }
    }

    if (tagIds.isEmpty() && _tagTreeView->currentIndex().isValid()) {
        const int tagId = _tagTreeView->currentIndex().data(TagTreeModel::TagIdRole).toInt();
        if (tagId > 0) {
            tagIds.append(tagId);
        }
    }

    return tagIds;
}

QVector<int> MainWindow::selectedNoteSubFolderIds() const {
    QVector<int> subFolderIds;
    if (_noteSubFolderView == nullptr || _noteSubFolderView->selectionModel() == nullptr) {
        return subFolderIds;
    }

    const QModelIndexList rows = _noteSubFolderView->selectionModel()->selectedRows();
    for (const QModelIndex& index : rows) {
        const int subFolderId = index.data(NoteSubFolderTreeModel::SubFolderIdRole).toInt();
        if (subFolderId > 0 && !subFolderIds.contains(subFolderId)) {
            subFolderIds.append(subFolderId);
        }
    }

    if (subFolderIds.isEmpty() && _noteSubFolderView->currentIndex().isValid()) {
        const int subFolderId =
            _noteSubFolderView->currentIndex().data(NoteSubFolderTreeModel::SubFolderIdRole).toInt();
        if (subFolderId > 0) {
            subFolderIds.append(subFolderId);
        }
    }

    return subFolderIds;
}

void MainWindow::buildNoteSubFolderMenuTree(QMenu* parentMenu, bool copyNotes, const QVector<int>& selectedNotes,
                                            int parentId) {
    if (parentMenu == nullptr || _coordinator == nullptr) {
        return;
    }

    const QVector<NoteSubFolderData> subFolders = _coordinator->noteSubFolders(parentId);
    for (const NoteSubFolderData& subFolder : subFolders) {
        const QVector<NoteSubFolderData> children = _coordinator->noteSubFolders(subFolder.id);
        if (!children.isEmpty()) {
            QMenu* subFolderMenu = parentMenu->addMenu(subFolder.name);
            buildNoteSubFolderMenuTree(subFolderMenu, copyNotes, selectedNotes, subFolder.id);
        } else {
            QAction* action = parentMenu->addAction(subFolder.name);
            connect(action, &QAction::triggered, this, [this, copyNotes, selectedNotes, subFolder]() {
                if (_coordinator == nullptr) {
                    return;
                }
                copyNotes ? _coordinator->copyNotesToSubFolder(selectedNotes, subFolder.id)
                          : _coordinator->moveNotesToSubFolder(selectedNotes, subFolder.id);
            });
        }
    }

    parentMenu->addSeparator();
    QAction* thisFolderAction = parentMenu->addAction(
        parentId == 0 ? (copyNotes ? tr("Copy to note folder") : tr("Move to note folder"))
                      : (copyNotes ? tr("Copy to this subfolder") : tr("Move to this subfolder")));
    connect(thisFolderAction, &QAction::triggered, this, [this, copyNotes, selectedNotes, parentId]() {
        if (_coordinator == nullptr) {
            return;
        }
        copyNotes ? _coordinator->copyNotesToSubFolder(selectedNotes, parentId)
                  : _coordinator->moveNotesToSubFolder(selectedNotes, parentId);
    });
}

void MainWindow::buildTagMoveMenuTree(QMenu* parentMenu, const QVector<int>& selectedTags, int parentTagId) {
    if (parentMenu == nullptr || _coordinator == nullptr) {
        return;
    }

    for (const TagData& tag : _coordinator->tags(parentTagId)) {
        if (selectedTags.contains(tag.id)) {
            continue;
        }

        const QVector<TagData> childTags = _coordinator->tags(tag.id);
        if (!childTags.isEmpty()) {
            QMenu* tagMenu = parentMenu->addMenu(tag.name);
            buildTagMoveMenuTree(tagMenu, selectedTags, tag.id);
        } else {
            QAction* action = parentMenu->addAction(tag.name);
            connect(action, &QAction::triggered, this, [this, selectedTags, tag]() {
                if (_coordinator != nullptr) {
                    _coordinator->moveTags(selectedTags, tag.id);
                }
            });
        }
    }

    parentMenu->addSeparator();
    QAction* action = parentMenu->addAction(
        parentTagId == 0 ? tr("Move to the root", "to move a tag to the current tag in the tag context menu")
                         : tr("Move to this tag"));
    connect(action, &QAction::triggered, this, [this, selectedTags, parentTagId]() {
        if (_coordinator != nullptr) {
            _coordinator->moveTags(selectedTags, parentTagId);
        }
    });
}

void MainWindow::buildNoteTagMenuTree(QMenu* parentMenu, const QVector<int>& selectedNotes, int parentTagId) {
    if (parentMenu == nullptr || _coordinator == nullptr) {
        return;
    }

    for (const TagData& tag : _coordinator->tags(parentTagId)) {
        const QVector<TagData> childTags = _coordinator->tags(tag.id);
        if (!childTags.isEmpty()) {
            QMenu* tagMenu = parentMenu->addMenu(tag.name);
            buildNoteTagMenuTree(tagMenu, selectedNotes, tag.id);
        } else {
            QAction* action = parentMenu->addAction(tag.name);
            connect(action, &QAction::triggered, this, [this, selectedNotes, tag]() {
                if (_coordinator == nullptr) {
                    return;
                }
                if (Utils::Gui::question(
                        this, tr("Tag selected notes"),
                        tr("Tag %n selected note(s) with <strong>%2</strong>?", "", selectedNotes.count())
                            .arg(tag.name),
                        QStringLiteral("tag-notes")) == QMessageBox::Yes) {
                    _coordinator->tagNotes(selectedNotes, tag.id);
                }
            });
        }
    }

    if (parentTagId > 0) {
        parentMenu->addSeparator();
        QAction* action = parentMenu->addAction(tr("Tag this"));
        connect(action, &QAction::triggered, this, [this, selectedNotes, parentTagId]() {
            if (_coordinator != nullptr) {
                _coordinator->tagNotes(selectedNotes, parentTagId);
            }
        });
    }
}

void MainWindow::buildSubFolderMoveMenuTree(QMenu* parentMenu, const QVector<int>& selectedFolders, int parentId) {
    if (parentMenu == nullptr || _coordinator == nullptr) {
        return;
    }

    for (const NoteSubFolderData& subFolder : _coordinator->noteSubFolders(parentId)) {
        if (selectedFolders.contains(subFolder.id)) {
            continue;
        }

        QMenu* subFolderMenu = parentMenu->addMenu(subFolder.name);
        buildSubFolderMoveMenuTree(subFolderMenu, selectedFolders, subFolder.id);
        subFolderMenu->addSeparator();
        QAction* moveAction = subFolderMenu->addAction(tr("Move to this subfolder"));
        connect(moveAction, &QAction::triggered, this, [this, selectedFolders, subFolder]() {
            if (_coordinator != nullptr) {
                _coordinator->moveSubFolders(selectedFolders, subFolder.id);
            }
        });
    }
}

void MainWindow::copyNoteFilenameToClipboard() {
    if (_currentNote.id <= 0) {
        return;
    }

    QApplication::clipboard()->setText(_currentNote.fileName);
    showStatusBarMessage(tr("Note filename '%1' was copied to the clipboard").arg(_currentNote.fileName),
                         QStringLiteral("📋"), 3000);
}

void MainWindow::showNoteListContextMenu(const QPoint& pos) {
    if (_noteListView == nullptr || _coordinator == nullptr) {
        return;
    }

    const QModelIndex clickedIndex = _noteListView->indexAt(pos);
    if (!clickedIndex.isValid()) {
        return;
    }

    if (!_noteListView->selectionModel()->isSelected(clickedIndex)) {
        _noteListView->setCurrentIndex(clickedIndex);
        _noteListView->selectionModel()->select(clickedIndex,
                                                QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    const QVector<int> noteIds = selectedNoteIds();
    if (noteIds.isEmpty()) {
        return;
    }

    QVariantList noteIdValues;
    for (const int noteId : noteIds) {
        noteIdValues.append(noteId);
    }
    _coordinator->noteListViewModel()->setSelectedNoteIds(noteIdValues);
    _coordinator->tagTreeViewModel()->setSelectedNoteIds(noteIdValues);

    const bool multiNoteMenuEntriesOnly = noteIds.count() > 1;
    QMenu noteMenu(this);
    QAction* renameAction = nullptr;
    if (!multiNoteMenuEntriesOnly) {
        noteMenu.addAction(newNoteAction());
        renameAction = noteMenu.addAction(tr("Rename note"));
        renameAction->setToolTip(tr("Allows you to rename the filename of the note"));
    }

    QAction* removeAction = noteMenu.addAction(tr("&Remove notes"));
    noteMenu.addSeparator();

    QAction* jumpToSubFolderAction = nullptr;
    const bool showSubFolders =
        _coordinator->appState()->currentNoteFolder().showSubfolders || Utils::Misc::isEnableNoteTree();
    if (showSubFolders) {
        if (noteIds.count() == 1 && !Utils::Misc::isEnableNoteTree()) {
            jumpToSubFolderAction = noteMenu.addAction(tr("Jump to the note's subfolder"));
        }

        QMenu* subFolderMoveMenu = noteMenu.addMenu(tr("Move notes to subfolder…"));
        buildNoteSubFolderMenuTree(subFolderMoveMenu, false, noteIds);

        QMenu* subFolderCopyMenu = noteMenu.addMenu(tr("Copy notes to subfolder…"));
        buildNoteSubFolderMenuTree(subFolderCopyMenu, true, noteIds);
    }

    if (!_coordinator->tags().isEmpty()) {
        QMenu* tagMenu = noteMenu.addMenu(tr("&Tag selected notes with…"));
        buildNoteTagMenuTree(tagMenu, noteIds);
    }

    const QVector<TagData> noteTags = _coordinator->tagsForNotes(noteIds);
    if (!noteTags.isEmpty()) {
        QMenu* tagRemoveMenu = noteMenu.addMenu(tr("&Remove tag from selected notes…"));
        for (const TagData& tag : noteTags) {
            QAction* action = tagRemoveMenu->addAction(tag.name);
            connect(action, &QAction::triggered, this, [this, noteIds, tag]() {
                if (_coordinator != nullptr) {
                    _coordinator->untagNotes(noteIds, tag.id);
                }
            });
        }
    }

    if (!multiNoteMenuEntriesOnly) {
        noteMenu.addSeparator();
    }

    QAction* openNoteInTabAction = noteMenu.addAction(tr("Open selected notes in tabs"));
    QAction* openInExternalEditorAction = nullptr;
    QAction* openNoteWindowAction = nullptr;
    QAction* showInFileManagerAction = nullptr;
    QAction* copyNotePathToClipboardAction = nullptr;
    QAction* copyNoteFileNameToClipboardAction = nullptr;
    QAction* toggleFavoriteAction = nullptr;
    QAction* selectAllAction = nullptr;

    if (!multiNoteMenuEntriesOnly) {
        openInExternalEditorAction = noteMenu.addAction(tr("Open note in external editor"));
        openNoteWindowAction = noteMenu.addAction(tr("Open note in different window"));
        showInFileManagerAction = noteMenu.addAction(tr("Show note in file manager"));
        copyNotePathToClipboardAction = noteMenu.addAction(tr("Copy absolute path of note"));
        copyNoteFileNameToClipboardAction = noteMenu.addAction(tr("Copy note filename"));

        noteMenu.addSeparator();
        toggleFavoriteAction =
            noteMenu.addAction(_currentNote.isFavorite ? tr("Unmark as favorite") : tr("Mark as favorite"));

        noteMenu.addSeparator();
        selectAllAction = noteMenu.addAction(tr("Select &all notes"));
    }

    QAction* selectedItem = noteMenu.exec(_noteListView->viewport()->mapToGlobal(pos));
    if (selectedItem == nullptr) {
        return;
    }

    if (selectedItem == removeAction) {
        _coordinator->noteListViewModel()->deleteSelectedNotes();
    } else if (selectedItem == renameAction) {
        if (_coordinator->currentNoteFolderSettingValue(QStringLiteral("allowDifferentNoteFileName")).toBool()) {
            _noteListView->edit(clickedIndex);
        } else {
            QMessageBox msgBox(QMessageBox::Warning, tr("Note renaming not enabled!"),
                               tr("If you want to rename your note you have to enable "
                                  "the option to allow the note filename to be "
                                  "different from the headline."),
                               QMessageBox::NoButton, this);
            QPushButton* settingsButton = msgBox.addButton(tr("Open &settings"), QMessageBox::AcceptRole);
            msgBox.addButton(tr("&Cancel"), QMessageBox::RejectRole);
            msgBox.setDefaultButton(settingsButton);
            msgBox.exec();
            if (msgBox.clickedButton() == settingsButton) {
                openSettingsDialog(SettingsDialog::NoteFolderPage);
            }
        }
    } else if (selectedItem == jumpToSubFolderAction) {
        if (_currentNote.noteSubFolderId > 0) {
            jumpToNoteSubFolder(_currentNote.noteSubFolderId);
        }
    } else if (selectedItem == openNoteInTabAction) {
        for (const int noteId : noteIds) {
            openNoteInTab(_coordinator->note(noteId), false);
        }
    } else if (selectedItem == openInExternalEditorAction) {
        findAction(QStringLiteral("action_Open_note_in_external_editor"))->trigger();
    } else if (selectedItem == openNoteWindowAction) {
        findAction(QStringLiteral("actionView_note_in_new_window"))->trigger();
    } else if (selectedItem == showInFileManagerAction) {
        findAction(QStringLiteral("actionShow_note_in_file_manager"))->trigger();
    } else if (selectedItem == copyNotePathToClipboardAction) {
        findAction(QStringLiteral("actionCopy_path_to_note_to_clipboard"))->trigger();
    } else if (selectedItem == copyNoteFileNameToClipboardAction) {
        copyNoteFilenameToClipboard();
    } else if (selectedItem == toggleFavoriteAction) {
        _coordinator->toggleFavoriteNote(_currentNote.id);
    } else if (selectedItem == selectAllAction) {
        _noteListView->selectAll();
    }
}

void MainWindow::showTagTreeContextMenu(const QPoint& pos) {
    if (_tagTreeView == nullptr || _coordinator == nullptr) {
        return;
    }

    const QPoint globalPos = _tagTreeView->viewport()->mapToGlobal(pos);
    const QModelIndex clickedIndex = _tagTreeView->indexAt(pos);
    if (clickedIndex.isValid() && !_tagTreeView->selectionModel()->isSelected(clickedIndex)) {
        _tagTreeView->setCurrentIndex(clickedIndex);
        _tagTreeView->selectionModel()->select(clickedIndex,
                                               QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    const QModelIndex targetIndex = clickedIndex.isValid() ? clickedIndex : _tagTreeView->currentIndex();

    const QVector<int> tagIds = selectedTagIds();
    const bool hasSelected = !tagIds.isEmpty();
    QMenu menu(this);
    QAction* addAction = menu.addAction(tr("&Add tag"));
    QAction* renameAction = nullptr;
    QAction* assignColorAction = nullptr;
    QAction* disableColorAction = nullptr;
    QAction* removeAction = nullptr;
    if (hasSelected) {
        renameAction = menu.addAction(tr("Rename tag"));
        assignColorAction = menu.addAction(tr("Assign color"));
        disableColorAction = menu.addAction(tr("Disable color"));
        removeAction = menu.addAction(tr("&Remove tags"));
        QMenu* moveMenu = menu.addMenu(tr("&Move tags to…"));
        buildTagMoveMenuTree(moveMenu, tagIds);
    }

    QAction* selectedItem = menu.exec(globalPos);
    if (selectedItem == nullptr) {
        return;
    }

    if (selectedItem == addAction) {
        auto* dialog = new TagAddDialog(this);
        const int dialogResult = dialog->exec();
        if (dialogResult == QDialog::Accepted && !dialog->name().isEmpty()) {
            const int parentId = targetIndex.data(TagTreeModel::TagIdRole).toInt() < 0
                                     ? 0
                                     : targetIndex.data(TagTreeModel::TagIdRole).toInt();
            _coordinator->tagTreeViewModel()->createTag(dialog->name(), parentId);
        }
        delete dialog;
    } else if (selectedItem == renameAction && targetIndex.isValid()) {
        _tagTreeView->edit(targetIndex.sibling(targetIndex.row(), 0));
    } else if (selectedItem == assignColorAction) {
        auto color = targetIndex.data(TagTreeModel::ColorRole).value<QColor>();
        color = QColorDialog::getColor(color.isValid() ? color : QColor(Qt::white), this);
        if (color.isValid()) {
            _coordinator->setTagColor(tagIds, color);
        }
    } else if (selectedItem == disableColorAction) {
        _coordinator->setTagColor(tagIds, QColor());
    } else if (selectedItem == removeAction) {
        if (Utils::Gui::question(this, tr("Remove selected tags"),
                                 tr("Remove <strong>%n</strong> selected tag(s)? No notes will "
                                    "be removed in this process.",
                                    "", tagIds.count()),
                                 QStringLiteral("remove-tags")) == QMessageBox::Yes) {
            _coordinator->deleteTags(tagIds);
        }
    }
}

void MainWindow::showNoteSubFolderContextMenu(const QPoint& pos) {
    if (_noteSubFolderView == nullptr || _coordinator == nullptr) {
        return;
    }

    const QModelIndex clickedIndex = _noteSubFolderView->indexAt(pos);
    if (clickedIndex.isValid() && !_noteSubFolderView->selectionModel()->isSelected(clickedIndex)) {
        _noteSubFolderView->setCurrentIndex(clickedIndex);
        _noteSubFolderView->selectionModel()->select(clickedIndex,
                                                     QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    const QModelIndex targetIndex = clickedIndex.isValid() ? clickedIndex : _noteSubFolderView->currentIndex();

    const QVector<int> subFolderIds = selectedNoteSubFolderIds();
    QMenu menu(this);
    menu.addAction(newNoteAction());

    QAction* newAction = menu.addAction(tr("New subfolder"));
    QAction* renameAction = nullptr;
    QAction* removeAction = nullptr;
    if (!subFolderIds.isEmpty()) {
        renameAction = menu.addAction(tr("Rename subfolder"));
        removeAction = menu.addAction(tr("Remove selected folders"));

        QMenu* moveMenu = menu.addMenu(tr("Move selected folders to..."));
        QAction* moveToRootAction = moveMenu->addAction(tr("Move to note folder"));
        connect(moveToRootAction, &QAction::triggered, this, [this, subFolderIds]() {
            if (_coordinator != nullptr) {
                _coordinator->moveSubFolders(subFolderIds, 0);
            }
        });
        moveMenu->addSeparator();
        buildSubFolderMoveMenuTree(moveMenu, subFolderIds);
    }

    QAction* showInFileManagerAction = menu.addAction(tr("Show folder in file manager"));
    menu.addAction(reloadNoteFolderAction());

    QAction* selectedItem = menu.exec(_noteSubFolderView->viewport()->mapToGlobal(pos));
    if (selectedItem == nullptr) {
        return;
    }

    if (selectedItem == newAction) {
        createNewNoteSubFolder();
    } else if (selectedItem == renameAction && targetIndex.isValid()) {
        _noteSubFolderView->edit(targetIndex);
    } else if (selectedItem == removeAction) {
        QStringList subFolderPathList;
        for (const int subFolderId : subFolderIds) {
            subFolderPathList << _coordinator->noteSubFolderFullPath(subFolderId);
        }
        if (Utils::Gui::question(this, tr("Remove selected folders"),
                                 tr("Remove <strong>%n</strong> selected folder(s)?"
                                    "<ul><li>%1</li></ul>"
                                    "All files and folders in these folders will be removed as"
                                    " well!",
                                    "", subFolderIds.count())
                                     .arg(subFolderPathList.join(QStringLiteral("</li><li>"))),
                                 QStringLiteral("remove-folders")) == QMessageBox::Yes) {
            _coordinator->deleteSubFolders(subFolderIds);
        }
    } else if (selectedItem == showInFileManagerAction) {
        const int subFolderId = subFolderIds.isEmpty() ? 0 : subFolderIds.constFirst();
        Utils::Misc::openPath(_coordinator->noteSubFolderFullPath(subFolderId));
    }
}

void MainWindow::showNavigationContextMenu(const QPoint& pos) {
    if (_navigationView == nullptr) {
        return;
    }

    const QModelIndex clickedIndex = _navigationView->indexAt(pos);
    if (!clickedIndex.isValid()) {
        return;
    }
    _navigationView->setCurrentIndex(clickedIndex);

    QMenu menu(this);
    QAction* renameAction = menu.addAction(tr("&Rename heading"));
    QAction* selectedItem = menu.exec(_navigationView->viewport()->mapToGlobal(pos));
    if (selectedItem == renameAction) {
        renameCurrentHeadingFromNavigation();
    }
}

void MainWindow::renameCurrentHeadingFromNavigation() {
    if (_navigationView == nullptr || _noteTextEdit == nullptr) {
        return;
    }

    const QModelIndex index = _navigationView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    const QString oldText = index.data(NavigationOutlineModel::TitleRole).toString();
    bool ok = false;
    const QString newText =
        QInputDialog::getText(this, tr("Rename heading"), tr("Name:"), QLineEdit::Normal, oldText, &ok).trimmed();
    if (!ok || newText.isEmpty() || newText == oldText) {
        return;
    }

    const int line = index.data(NavigationOutlineModel::LineRole).toInt();
    QTextBlock block = _noteTextEdit->document()->findBlockByNumber(line);
    if (!block.isValid()) {
        return;
    }

    QString blockText = block.text();
    static const QRegularExpression headingExpression(QStringLiteral("^(#{1,6}\\s+)(.+)$"));
    const QRegularExpressionMatch match = headingExpression.match(blockText);
    blockText = match.hasMatch() ? match.captured(1) + newText : newText;

    QTextCursor cursor(block);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.insertText(blockText);
    _coordinator->noteEditorViewModel()->textChanged(_noteTextEdit->toPlainText());
    updatePreview();
}

NoteHistoryItemData MainWindow::currentNoteHistoryItem() const {
    NoteHistoryItemData item;
    if (_noteTextEdit == nullptr || _currentNote.id <= 0) {
        return item;
    }

    item.noteName = _currentNote.name;
    item.noteSubFolderPathData = _currentNote.relativeNoteSubFolderPath;
    item.cursorPosition = _noteTextEdit->textCursor().position();
    QScrollBar* scrollBar = _noteTextEdit->verticalScrollBar();
    item.relativeScrollBarPosition =
        scrollBar == nullptr || scrollBar->maximum() <= 0
            ? 0.0F
            : static_cast<float>(scrollBar->value()) / static_cast<float>(scrollBar->maximum());
    return item;
}

void MainWindow::storeNoteBookmark(int slot) {
    if (_coordinator == nullptr || _noteTextEdit == nullptr || !_noteTextEdit->hasFocus()) {
        return;
    }

    _coordinator->storeNoteBookmark(slot, currentNoteHistoryItem());
    showStatusBarMessage(tr("Bookmarked note position at slot %1").arg(slot), 3000);
    updateNoteBookmarkDisplay();
}

void MainWindow::gotoNoteBookmark(int slot) {
    if (_coordinator == nullptr || _noteTextEdit == nullptr) {
        return;
    }

    const NoteHistoryItemData item = _coordinator->noteBookmark(slot);
    const NoteData note = _coordinator->noteForHistoryItem(item);
    if (note.id <= 0) {
        return;
    }

    _noteTextEdit->setFocus();
    setCurrentNoteFromNoteId(note.id);

    QTextCursor cursor = _noteTextEdit->textCursor();
    cursor.setPosition(qBound(0, item.cursorPosition, _noteTextEdit->document()->characterCount() - 1));
    _noteTextEdit->setTextCursor(cursor);
    if (QScrollBar* scrollBar = _noteTextEdit->verticalScrollBar()) {
        scrollBar->setValue(
            static_cast<int>(item.relativeScrollBarPosition * static_cast<float>(scrollBar->maximum())));
    }
    showStatusBarMessage(tr("Jumped to bookmark position at slot %1").arg(slot), 3000);
    updateNoteBookmarkDisplay();
}

void MainWindow::deleteNoteBookmark(int slot) {
    if (_coordinator == nullptr) {
        return;
    }

    _coordinator->removeNoteBookmark(slot);
    updateNoteBookmarkDisplay();
}

void MainWindow::openNoteBookmarkDialog() {
    if (_coordinator == nullptr) {
        return;
    }

    auto* dialog = new NoteBookmarkDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setBookmarks(_coordinator->noteBookmarks());

    connect(dialog, &NoteBookmarkDialog::jumpToBookmarkRequested, this, &MainWindow::gotoNoteBookmark);
    connect(dialog, &NoteBookmarkDialog::deleteBookmarkRequested, this, [this, dialog](int slot) {
        deleteNoteBookmark(slot);
        if (_coordinator != nullptr) {
            dialog->setBookmarks(_coordinator->noteBookmarks());
        }
    });
    connect(dialog, &NoteBookmarkDialog::reloadRequested, this, [this, dialog]() {
        if (_coordinator != nullptr) {
            dialog->setBookmarks(_coordinator->noteBookmarks());
        }
    });

    dialog->show();
}

void MainWindow::updateNoteBookmarkDisplay() {
    if (_noteTextEdit == nullptr) {
        return;
    }

    QHash<int, int> bookmarkLines;
    if (_coordinator == nullptr || _currentNote.id <= 0) {
        _noteTextEdit->setBookmarkLines(bookmarkLines);
        return;
    }

    const QVector<BookmarkItemData> bookmarks = _coordinator->noteBookmarks();
    for (const BookmarkItemData& bookmark : bookmarks) {
        const NoteHistoryItemData& item = bookmark.historyItem;
        if (item.noteName != _currentNote.name ||
            item.noteSubFolderPathData != _currentNote.relativeNoteSubFolderPath) {
            continue;
        }

        QTextCursor cursor(_noteTextEdit->document());
        cursor.setPosition(qBound(0, item.cursorPosition, _noteTextEdit->document()->characterCount() - 1));
        bookmarkLines.insert(bookmark.slot, cursor.blockNumber() + 1);
    }
    _noteTextEdit->setBookmarkLines(bookmarkLines);
}

QVariant MainWindow::persistentSetting(const QString& key, const QVariant& defaultValue) const {
    return _coordinator == nullptr ? defaultValue : _coordinator->settingValue(key, defaultValue);
}

QVector<QVariantMap> MainWindow::persistentSettingsArray(const QString& arrayName, const QStringList& keys) const {
    return _coordinator == nullptr ? QVector<QVariantMap>() : _coordinator->settingsArrayValues(arrayName, keys);
}

void MainWindow::setPersistentSetting(const QString& key, const QVariant& value) {
    if (_coordinator != nullptr) {
        _coordinator->settingsViewModel()->setPersistentSetting(key, value);
    }
}

void MainWindow::removePersistentSetting(const QString& key) {
    if (_coordinator != nullptr) {
        _coordinator->settingsViewModel()->removePersistentSetting(key);
    }
}

void MainWindow::applyDistractionFreeMode(bool enabled, bool persistSetting) {
    if (persistSetting) {
        setPersistentSetting(QStringLiteral("DistractionFreeMode/isEnabled"), enabled);
    }

    if (_coordinator != nullptr) {
        _coordinator->uiState()->setDistractionFree(enabled);
    }

    if (auto* toolbar = findChild<QToolBar*>(QStringLiteral("mainToolBar"))) {
        toolbar->setVisible(!enabled && findAction(QStringLiteral("actionShow_toolbar"))->isChecked());
    }

    const QList<QDockWidget*> docks = findChildren<QDockWidget*>();
    for (QDockWidget* dock : docks) {
        if (enabled) {
            dock->setProperty("visible-before-dfm", dock->isVisible());
            dock->hide();
        } else if (dock->property("visible-before-dfm").isValid()) {
            dock->setVisible(dock->property("visible-before-dfm").toBool());
        }
    }

    if (_noteEditTabWidget != nullptr) {
        _noteEditTabWidget->tabBar()->setVisible(!enabled && _noteEditTabWidget->count() > 1);
    }
    if (_noteTextEdit != nullptr) {
        _noteTextEdit->setPaperMargins(enabled ? -1 : 0);
        _noteTextEdit->setFocus();
    }
}

void MainWindow::configureAction(const QString& objectName, const QString& text, const QString& iconPath,
                                 const QString& shortcut, bool checkable) {
    QAction* action = ensureAction(objectName, text);
    action->setText(text);
    action->setCheckable(checkable);
    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }
    if (!iconPath.isEmpty()) {
        action->setIcon(QIcon(iconPath));
    }
}

QAction* MainWindow::newNoteAction() {
    return ensureAction(QStringLiteral("action_New_note"), tr("New note"));
}

QAction* MainWindow::reloadNoteFolderAction() {
    return ensureAction(QStringLiteral("action_Reload_note_folder"), tr("Reload note folder"));
}

QAction* MainWindow::insertTextLinkAction() {
    return ensureAction(QStringLiteral("actionInsert_text_link"), tr("Insert link"));
}

QAction* MainWindow::toggleCheckboxesAction() {
    return ensureAction(QStringLiteral("actionToggle_checkboxes"), tr("Toggle checkboxes"));
}

QAction* MainWindow::createOrderedListAction() {
    return ensureAction(QStringLiteral("actionCreate_ordered_list"), tr("Create ordered list"));
}

QAction* MainWindow::createAlphabeticalListAction() {
    return ensureAction(QStringLiteral("actionCreate_alphabetical_list"), tr("Create alphabetical list"));
}

QAction* MainWindow::createUnorderedListAction() {
    return ensureAction(QStringLiteral("actionCreate_unordered_list"), tr("Create unordered list"));
}

QAction* MainWindow::createCheckboxListAction() {
    return ensureAction(QStringLiteral("actionCreate_checkbox_list"), tr("Create checkbox list"));
}

QAction* MainWindow::clearListFormattingAction() {
    return ensureAction(QStringLiteral("actionClear_list_formatting"), tr("Clear list formatting"));
}

QAction* MainWindow::orderCheckboxesAction() {
    return ensureAction(QStringLiteral("actionOrder_checkboxes"), tr("Order checkboxes"));
}

QAction* MainWindow::increaseHeadingDepthAction() {
    return ensureAction(QStringLiteral("actionIncrease_heading_depth"), tr("Increase heading depth"));
}

QAction* MainWindow::decreaseHeadingDepthAction() {
    return ensureAction(QStringLiteral("actionDecrease_heading_depth"), tr("Decrease heading depth"));
}

QAction* MainWindow::searchTextOnWebAction() {
    return ensureAction(QStringLiteral("actionSearch_text_on_the_web"), tr("Search text on web"));
}

QAction* MainWindow::findNoteAction() {
    return ensureAction(QStringLiteral("action_Find_note"), tr("Find note"));
}

QAction* MainWindow::selectEnclosedTextAction() {
    return ensureAction(QStringLiteral("actionSelect_enclosed_text"), tr("Select enclosed text"));
}

QAction* MainWindow::pasteImageAction() {
    return ensureAction(QStringLiteral("actionPaste_image"), tr("Paste image"));
}

QAction* MainWindow::autocompleteAction() {
    return ensureAction(QStringLiteral("actionAutocomplete"), tr("Autocomplete"));
}

QAction* MainWindow::splitNoteAtPosAction() {
    return ensureAction(QStringLiteral("actionSplit_note_at_cursor_position"), tr("Split note at cursor"));
}

QAction* MainWindow::ensureAction(const QString& objectName, const QString& text) {
    QAction* action = _actions.value(objectName, nullptr);
    if (action != nullptr) {
        if (!text.isEmpty()) {
            action->setText(text);
        }
        return action;
    }

    if (_coordinator != nullptr && _coordinator->actionRegistry() != nullptr) {
        action = _coordinator->actionRegistry()->registerAction(
            objectName, text.isEmpty() ? readableActionText(objectName) : text, categoryForAction(objectName));
    } else {
        action = new QAction(text.isEmpty() ? readableActionText(objectName) : text, this);
    }
    action->setObjectName(objectName);
    _actions.insert(objectName, action);
    return action;
}
