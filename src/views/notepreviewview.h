#pragma once

#include <QUrl>
#include <QWidget>

class QScrollBar;

#ifdef USE_QLITEHTML
class HtmlPreviewWidget;
#else
class NotePreviewWidget;
#endif

class NotePreviewView : public QWidget {
    Q_OBJECT

public:
    explicit NotePreviewView(QWidget* parent = nullptr);
    [[nodiscard]] QWidget* viewport() const;
    [[nodiscard]] QScrollBar* verticalScrollBar() const;
    [[nodiscard]] QScrollBar* horizontalScrollBar() const;

public slots:
    void setHtmlPreview(const QString& html);
    void updateBackground();

Q_SIGNALS:
    void anchorClicked(const QUrl& url);

private:
#ifdef USE_QLITEHTML
    HtmlPreviewWidget* _previewWidget = nullptr;
#else
    NotePreviewWidget* _previewWidget = nullptr;
#endif
};
