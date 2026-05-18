#include "notepreviewview.h"

#ifdef USE_QLITEHTML
#include <widgets/htmlpreviewwidget.h>
#else
#include <widgets/notepreviewwidget.h>
#endif

#include <QFrame>
#include <QScrollBar>
#include <QTextBrowser>
#include <QVBoxLayout>

NotePreviewView::NotePreviewView(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("notePreviewView"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

#ifdef USE_QLITEHTML
    _previewWidget = new HtmlPreviewWidget(this);
#else
    _previewWidget = new NotePreviewWidget(this);
    _previewWidget->setFrameShape(QFrame::NoFrame);
    _previewWidget->setReadOnly(true);
    _previewWidget->setAcceptRichText(false);
    _previewWidget->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse |
                                            Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard |
                                            Qt::TextSelectableByMouse);
    _previewWidget->setOpenExternalLinks(true);
    _previewWidget->setOpenLinks(false);
#endif

    layout->addWidget(_previewWidget);
#ifdef USE_QLITEHTML
    connect(_previewWidget, &HtmlPreviewWidget::anchorClicked, this, &NotePreviewView::anchorClicked);
#else
    connect(_previewWidget, &QTextBrowser::anchorClicked, this, &NotePreviewView::anchorClicked);
#endif
}

void NotePreviewView::setHtmlPreview(const QString& html) {
    _previewWidget->setHtml(html);
}

void NotePreviewView::updateBackground() {
#ifdef USE_QLITEHTML
    _previewWidget->updateBackground();
#endif
}

QWidget* NotePreviewView::viewport() const {
    return _previewWidget->viewport();
}

QScrollBar* NotePreviewView::verticalScrollBar() const {
    return _previewWidget->verticalScrollBar();
}

QScrollBar* NotePreviewView::horizontalScrollBar() const {
    return _previewWidget->horizontalScrollBar();
}
