#pragma once

#include <QHash>
#include <QTextBlock>
#include <QTextEdit>

#include "helpers/arcnotesmarkdownhighlighter.h"
#include "libraries/qmarkdowntextedit/qmarkdowntextedit.h"
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QAction;

#define ARCNOTESMARKDOWNTEXTEDIT_OVERRIDE_FONT_SIZE_STYLESHEET_PRE_STRING "/* BEGIN FONT SIZE OVERRIDE STYLESHEET */"
#define ARCNOTESMARKDOWNTEXTEDIT_OVERRIDE_FONT_SIZE_STYLESHEET_POST_STRING "/* END FONT SIZE OVERRIDE STYLESHEET */"

class ArcNotesMarkdownTextEdit : public QMarkdownTextEdit {
    Q_OBJECT

public:
    enum EditorWidthMode { Narrow = 1, Medium, Wide, Full, Custom };
    Q_ENUMS(EditorWidthMode)

    enum FontModificationMode { Increase = 1, Decrease, Reset };
    Q_ENUMS(FontModificationMode)

    explicit ArcNotesMarkdownTextEdit(QWidget* parent = nullptr);
    ~ArcNotesMarkdownTextEdit() override;

    void setStyles();
    void openUrl(const QString& urlString, bool openInNewTab) override;
    //    void setViewportMargins(int left, int top, int right, int bottom);
    void setPaperMargins(int width = -1);
    int modifyFontSize(FontModificationMode mode);
    void updateSettings();
    QMargins viewportMargins();
    void setText(const QString& text);
    void setCurrentNoteReference(const QString& noteReference);
    bool usesMonospacedFont();
    void foldAllHeadings();
    void unfoldAllHeadings();

    /**
     * Toggles the case of the word under the Cursor or the selected text
     */
    void toggleCase();

    /**
     * Selects the innermost text enclosed by surrounding delimiters
     */
    void selectEnclosedText();

    [[nodiscard]] QTextCursor fullLineSelectionCursor() const;
    bool replaceFullLineSelection(const QString& text);
    bool changeHeadingDepthOfSelection(int levelDelta);

    /**
     * Inserts an empty code block
     */
    void insertCodeBlock();
    void insertWikiLink();

    /**
     * Handles auto completion
     */
    void onAutoCompleteRequested();

    /**
     * Tries to find an equation in the current line and solves it
     * @return true on success
     */
    bool solveEquation(double& returnValue);

    /**
     * Inserts a block quote character or formats the selected text as block quote
     */
    void insertBlockQuote();

    /**
     * Returns the text from the current cursor to the start of the word in the
     * note text edit
     *
     * @param withPreviousCharacters also get more characters at the beginning
     *                               to get characters like "@" that are not
     *                               word-characters
     * @return
     */
    [[nodiscard]] QString currentWord(bool withPreviousCharacters = false) const;

    /**
     * Tries to find words that start with the current word in the note text edit
     *
     * @param resultList
     * @return
     */
    bool autoComplete(QStringList& resultList) const;
    bool wikiLinkAutoComplete(QStringList& resultList, QString& filterText, int& replaceLength) const;

    [[nodiscard]] QString currentBlock() const;

    [[nodiscard]] QSize minimumSizeHint() const override;

    void updateIgnoredClickUrlRegexps();

protected:
    // we must not override _highlighter or Windows will create a
    // ArcNotesMarkdownHighlighter and MarkdownHighlighter instance
    //    ArcNotesMarkdownHighlighter *_highlighter;
    bool canInsertFromMimeData(const QMimeData* source) const override;
    void insertFromMimeData(const QMimeData* source) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void keyPressEvent(QKeyEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    [[nodiscard]] int sidebarAdditionalWidth() const override;
    void paintSidebar(QPainter* painter, const QRect& eventRect) override;
    bool sidebarMousePressEvent(QMouseEvent* event) override;
    [[nodiscard]] QVariant inputMethodQuery(Qt::InputMethodQuery property) const override;

private:
    struct FoldRegion {
        QTextBlock headerBlock;
        QTextBlock firstContentBlock;
        QTextBlock lastContentBlock;
    };

    /// @param in is true if zoom-in, false otherwise
    void onZoom(bool in);

    void setFormatStyle(MarkdownHighlighter::HighlighterState index);

    void onContextMenu(QPoint pos);

    void overrideFontSizeStyle(int fontSize);
    void paintMarkdownImagePreviews();
    void refreshFoldingSidebar();
    static bool isHeadingBlock(const QTextBlock& block, int* level = nullptr);
    bool foldRegionForHeaderBlock(const QTextBlock& headerBlock, FoldRegion& region) const;
    bool setHeadingFolded(const QTextBlock& headerBlock, bool folded);
    bool setFoldRegionFolded(const FoldRegion& region, bool folded);
    [[nodiscard]] bool isHeadingFolded(const QTextBlock& headerBlock) const;
    static QString headingStateKey(const QTextBlock& headerBlock, QHash<QString, int>& headingOccurrences);
    [[nodiscard]] bool hasFoldableHeadings() const;
    bool headerBlockAtSidebarPosition(const QPoint& pos, QTextBlock& headerBlock) const;
    void storeCurrentFoldedHeadingState();
    void scheduleRestoreCurrentFoldedHeadingState();
    void restoreCurrentFoldedHeadingState();

    bool _showMarkdownImagePreviews = true;
    bool _headingFoldingEnabled = true;
    bool _hasFoldableHeadings = false;
    QString _currentNoteReference;
    bool _foldingStateRestorePending = false;
    bool _isApplyingStoredFoldingState = false;
    int _foldingStateRestoreAttempts = 0;
    QChar _pendingDeadKey;
};
