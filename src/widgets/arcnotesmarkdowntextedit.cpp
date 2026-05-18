#include "arcnotesmarkdowntextedit.h"

#include <utils/gui.h>
#include <utils/listutils.h>
#include <utils/misc.h>
#include <utils/schema.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QHash>
#include <QHelpEvent>
#include <QImageReader>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPixmap>
#include <QRegularExpression>
#include <QSet>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextEdit>
#include <QTextFormat>
#include <QTextLayout>
#include <QTimer>
#include <QToolTip>
#include <QUrl>
#include <QVariant>
#include <QVector>
#include <QtGlobal>
#include <algorithm>

#include "dialogs/markdowntabledialog.h"
#include "helpers/arcnotesmarkdownhighlighter.h"
#include "libraries/qmarkdowntextedit/linenumberarea.h"
#include "utils/urlhandler.h"
#include "version.h"
#include "widgets/arcnotesmarkdowntextedithost.h"

namespace {
constexpr int kFoldIndicatorPadding = 4;
QHash<QString, QSet<QString>> s_foldedHeadingStateByNoteReference;

ArcNotesMarkdownTextEditHost* editorHost(const QWidget* widget) {
    if (widget == nullptr) {
        return nullptr;
    }
    return dynamic_cast<ArcNotesMarkdownTextEditHost*>(const_cast<QWidget*>(widget)->window());
}

void showEditorStatusMessage(QWidget* widget, const QString& message, const QString& symbol, int timeout) {
    if (auto* host = editorHost(widget)) {
        host->editorShowStatusMessage(message, symbol, timeout);
    }
}

QAction* editorHostAction(QWidget* widget, const QString& objectName) {
    if (auto* host = editorHost(widget)) {
        return host->editorAction(objectName);
    }
    return nullptr;
}

QString editorCurrentNoteFolderPath(QWidget* widget) {
    if (auto* host = editorHost(widget)) {
        return host->editorCurrentNoteFolderPath();
    }
    return QString();
}

bool editorWikiLinkSupportEnabled(QWidget* widget) {
    if (auto* host = editorHost(widget)) {
        return host->editorWikiLinkSupportEnabled();
    }
    return false;
}

bool editorFileUrlIsNoteInCurrentFolder(QWidget* widget, const QUrl& url) {
    if (auto* host = editorHost(widget)) {
        return host->editorFileUrlIsNoteInCurrentFolder(url);
    }
    return false;
}

QVariant editorSettingValue(QWidget* widget, const QString& key, const QVariant& defaultValue = QVariant()) {
    if (auto* host = editorHost(widget)) {
        return host->editorSettingValue(key, defaultValue);
    }
    return defaultValue;
}

void editorSetSettingValue(QWidget* widget, const QString& key, const QVariant& value) {
    if (auto* host = editorHost(widget)) {
        host->editorSetSettingValue(key, value);
        return;
    }
    Q_UNUSED(key)
    Q_UNUSED(value)
}

void addEditorHostAction(QMenu* menu, QWidget* widget, const QString& objectName) {
    if (menu == nullptr) {
        return;
    }
    if (QAction* action = editorHostAction(widget, objectName)) {
        menu->addAction(action);
    }
}

class BasicExpressionParser {
public:
    explicit BasicExpressionParser(const QString& expression) : _expression(expression) {}

    bool parse(double& value) {
        if (!parseExpression(value)) {
            return false;
        }

        skipSpaces();
        return _position == _expression.length();
    }

private:
    const QString& _expression;
    int _position = 0;

    void skipSpaces() {
        while (_position < _expression.length() && _expression.at(_position).isSpace()) {
            ++_position;
        }
    }

    bool consume(QChar ch) {
        skipSpaces();
        if (_position < _expression.length() && _expression.at(_position) == ch) {
            ++_position;
            return true;
        }

        return false;
    }

    bool parseExpression(double& value) {
        if (!parseTerm(value)) {
            return false;
        }

        while (true) {
            if (consume(QLatin1Char('+'))) {
                double rhs = 0;
                if (!parseTerm(rhs)) {
                    return false;
                }
                value += rhs;
            } else if (consume(QLatin1Char('-'))) {
                double rhs = 0;
                if (!parseTerm(rhs)) {
                    return false;
                }
                value -= rhs;
            } else {
                return true;
            }
        }
    }

    bool parseTerm(double& value) {
        if (!parseFactor(value)) {
            return false;
        }

        while (true) {
            if (consume(QLatin1Char('*'))) {
                double rhs = 0;
                if (!parseFactor(rhs)) {
                    return false;
                }
                value *= rhs;
            } else if (consume(QLatin1Char('/'))) {
                double rhs = 0;
                if (!parseFactor(rhs) || rhs == 0) {
                    return false;
                }
                value /= rhs;
            } else {
                return true;
            }
        }
    }

    bool parseFactor(double& value) {
        if (consume(QLatin1Char('+'))) {
            return parseFactor(value);
        }

        if (consume(QLatin1Char('-'))) {
            if (!parseFactor(value)) {
                return false;
            }
            value = -value;
            return true;
        }

        if (consume(QLatin1Char('('))) {
            if (!parseExpression(value) || !consume(QLatin1Char(')'))) {
                return false;
            }
            return true;
        }

        return parseNumber(value);
    }

    bool parseNumber(double& value) {
        skipSpaces();
        const int start = _position;
        bool hasDigit = false;
        bool hasDecimalPoint = false;

        while (_position < _expression.length()) {
            const QChar ch = _expression.at(_position);
            if (ch.isDigit()) {
                hasDigit = true;
                ++_position;
            } else if (ch == QLatin1Char('.') && !hasDecimalPoint) {
                hasDecimalPoint = true;
                ++_position;
            } else {
                break;
            }
        }

        if (!hasDigit) {
            return false;
        }

        bool ok = false;
        value = _expression.mid(start, _position - start).toDouble(&ok);
        return ok;
    }
};

QChar accentForDeadKey(int key) {
    switch (key) {
        case Qt::Key_Dead_Acute:
            return QChar(0x0301);
        case Qt::Key_Dead_Grave:
            return QChar(0x0300);
        case Qt::Key_Dead_Circumflex:
            return QChar(0x0302);
        case Qt::Key_Dead_Diaeresis:
            return QChar(0x0308);
        case Qt::Key_Dead_Tilde:
            return QChar(0x0303);
        default:
            return QChar();
    }
}

QChar spacingAccentForCombiningMark(QChar accent) {
    switch (accent.unicode()) {
        case 0x0300:
            return QLatin1Char('`');
        case 0x0301:
            return QChar(0x00B4);
        case 0x0302:
            return QLatin1Char('^');
        case 0x0303:
            return QLatin1Char('~');
        case 0x0308:
            return QChar(0x00A8);
        default:
            return QChar();
    }
}

QString composeDeadKey(QChar accent, QChar character) {
    return QString(character).append(accent).normalized(QString::NormalizationForm_C);
}

struct SetextHeadingUnderline {
    int level = 0;
    int leadingSpaces = 0;
    int markerCount = 0;

    [[nodiscard]] bool isValid() const { return level > 0; }
};

int markdownHeadingIndent(const QString& line) {
    int i = 0;
    while (i < line.size() && i < 3 && line.at(i) == QLatin1Char(' ')) {
        ++i;
    }

    return i;
}

int atxHeadingLevel(const QString& line, int* headingMarkerStart = nullptr, int* headingMarkerEnd = nullptr) {
    int i = markdownHeadingIndent(line);
    const int markerStart = i;
    while (i < line.size() && line.at(i) == QLatin1Char('#')) {
        ++i;
    }

    const int headingLevel = i - markerStart;
    if (headingLevel <= 0 || headingLevel > 6) {
        return 0;
    }

    if (i < line.size() && line.at(i) != QLatin1Char(' ') && line.at(i) != QLatin1Char('\t')) {
        return 0;
    }

    if (headingMarkerStart) {
        *headingMarkerStart = markerStart;
    }

    if (headingMarkerEnd) {
        *headingMarkerEnd = i;
    }

    return headingLevel;
}

SetextHeadingUnderline parseSetextHeadingUnderline(const QString& line) {
    SetextHeadingUnderline underline;

    int i = markdownHeadingIndent(line);
    underline.leadingSpaces = i;

    if (i >= line.size()) {
        return underline;
    }

    const QChar marker = line.at(i);
    if (marker != QLatin1Char('=') && marker != QLatin1Char('-')) {
        return underline;
    }

    const int markerStart = i;
    while (i < line.size() && line.at(i) == marker) {
        ++i;
    }

    underline.markerCount = i - markerStart;

    while (i < line.size() && (line.at(i) == QLatin1Char(' ') || line.at(i) == QLatin1Char('\t'))) {
        ++i;
    }

    if (i != line.size()) {
        return {};
    }

    underline.level = (marker == QLatin1Char('=')) ? 1 : 2;
    return underline;
}

int boundedHeadingLevel(const int headingLevel, const int levelDelta) {
#if __cplusplus >= 201703L
    return std::clamp(headingLevel + levelDelta, 1, 6);
#else
    return qBound(1, headingLevel + levelDelta, 6);
#endif
}

QString changeHeadingDepth(const QString& text, const int levelDelta) {
    QString normalizedText = text;
    normalizedText.replace(QChar(0x2029), QLatin1Char('\n'));

    QStringList lines = normalizedText.split(QLatin1Char('\n'),
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                                             QString::KeepEmptyParts
#else
                                             Qt::KeepEmptyParts
#endif
    );
    QStringList updatedLines;
    updatedLines.reserve(lines.size());

    for (int lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        const QString& line = lines.at(lineIndex);

        int headingMarkerStart = 0;
        int headingMarkerEnd = 0;
        const int atxLevel = atxHeadingLevel(line, &headingMarkerStart, &headingMarkerEnd);
        if (atxLevel > 0) {
            const int newHeadingLevel = boundedHeadingLevel(atxLevel, levelDelta);
            if (newHeadingLevel == atxLevel) {
                updatedLines.append(line);
                continue;
            }

            updatedLines.append(line.left(headingMarkerStart) + QString(newHeadingLevel, QLatin1Char('#')) +
                                line.mid(headingMarkerEnd));
            continue;
        }

        if (lineIndex + 1 < lines.size()) {
            const SetextHeadingUnderline underline = parseSetextHeadingUnderline(lines.at(lineIndex + 1));
            const int titleIndent = markdownHeadingIndent(line);
            const QString titleText = line.mid(titleIndent).trimmed();

            if (underline.isValid() && !titleText.isEmpty()) {
                const int newHeadingLevel = boundedHeadingLevel(underline.level, levelDelta);
                if (newHeadingLevel <= 2) {
                    updatedLines.append(line);

                    if (newHeadingLevel == underline.level) {
                        updatedLines.append(lines.at(lineIndex + 1));
                    } else {
                        const int underlineWidth =
                            std::max(underline.markerCount, std::max(static_cast<int>(titleText.size()), 3));
                        const QChar marker = (newHeadingLevel == 1) ? QLatin1Char('=') : QLatin1Char('-');
                        updatedLines.append(QString(underline.leadingSpaces, QLatin1Char(' ')) +
                                            QString(underlineWidth, marker));
                    }
                } else {
                    updatedLines.append(line.left(titleIndent) + QString(newHeadingLevel, QLatin1Char('#')) +
                                        QStringLiteral(" ") + titleText);
                }

                ++lineIndex;
                continue;
            }
        }

        updatedLines.append(line);
    }

    return updatedLines.join(QLatin1Char('\n'));
}

struct InnerSelectionCandidate {
    int innerStart = -1;
    int innerEnd = -1;

    [[nodiscard]] bool isValid() const { return innerStart >= 0 && innerEnd >= innerStart; }
    [[nodiscard]] int length() const { return innerEnd - innerStart; }
};

void addSelectionCandidate(InnerSelectionCandidate& best, const int innerStart, const int innerEnd,
                           const int targetStart, const int targetEnd) {
    if (innerStart < 0 || innerEnd < innerStart || innerStart > targetStart || innerEnd < targetEnd) {
        return;
    }

    if (!best.isValid() || (innerEnd - innerStart) < best.length() ||
        ((innerEnd - innerStart) == best.length() && innerStart > best.innerStart)) {
        best.innerStart = innerStart;
        best.innerEnd = innerEnd;
    }
}

bool isEscapedToken(const QString& text, const int pos) {
    int backslashCount = 0;

    for (int i = pos - 1; i >= 0 && text.at(i) == QLatin1Char('\\'); --i) {
        ++backslashCount;
    }

    return (backslashCount % 2) == 1;
}

void addStackedTokenCandidates(const QString& text, const QString& openToken, const QString& closeToken,
                               const int targetStart, const int targetEnd, InnerSelectionCandidate& best) {
    QVector<int> openPositions;
    const int maxStart = text.size() - std::min(openToken.size(), closeToken.size());

    for (int i = 0; i <= maxStart;) {
        if (text.mid(i, closeToken.size()) == closeToken && !openPositions.isEmpty()) {
            const int openPos = openPositions.takeLast();
            addSelectionCandidate(best, openPos + openToken.size(), i, targetStart, targetEnd);
            i += closeToken.size();
            continue;
        }

        if (text.mid(i, openToken.size()) == openToken) {
            openPositions.append(i);
            i += openToken.size();
            continue;
        }

        ++i;
    }
}

void addRepeatedTokenCandidates(const QString& text, const QString& token, const int targetStart, const int targetEnd,
                                InnerSelectionCandidate& best, const bool escapable) {
    int openPos = -1;

    for (int i = 0; i <= text.size() - token.size();) {
        if (text.mid(i, token.size()) != token || (escapable && isEscapedToken(text, i))) {
            ++i;
            continue;
        }

        if (openPos < 0) {
            openPos = i;
        } else {
            addSelectionCandidate(best, openPos + token.size(), i, targetStart, targetEnd);
            openPos = -1;
        }

        i += token.size();
    }
}

void addMarkdownRangeCandidate(const QString& blockText, MarkdownHighlighter* markdownHighlighter,
                               const MarkdownHighlighter::RangeType rangeType, const int blockNumber,
                               const int probePosition, const int targetStart, const int targetEnd,
                               InnerSelectionCandidate& best) {
    if (!markdownHighlighter || probePosition < 0 || probePosition >= blockText.size()) {
        return;
    }

    const auto range = markdownHighlighter->getSpanRange(rangeType, blockNumber, probePosition);
    if (range.first < 0 || range.second < 0 || range.first >= blockText.size()) {
        return;
    }

    if (rangeType == MarkdownHighlighter::RangeType::CodeSpan) {
        int delimiterLength = 0;

        while ((range.first + delimiterLength) < blockText.size() &&
               blockText.at(range.first + delimiterLength) == QLatin1Char('`')) {
            ++delimiterLength;
        }

        if (delimiterLength > 0) {
            addSelectionCandidate(best, range.first + delimiterLength, range.second, targetStart, targetEnd);
        }

        return;
    }

    const QChar marker = blockText.at(range.first);
    if (rangeType != MarkdownHighlighter::RangeType::Emphasis ||
        (marker != QLatin1Char('*') && marker != QLatin1Char('_'))) {
        return;
    }

    int leftMarkerStart = range.first;
    while (leftMarkerStart > 0 && blockText.at(leftMarkerStart - 1) == marker) {
        --leftMarkerStart;
    }

    const int delimiterLength = range.first - leftMarkerStart + 1;
    int rightMarkerIndex = range.second;

    if (rightMarkerIndex >= blockText.size() || blockText.at(rightMarkerIndex) != marker) {
        --rightMarkerIndex;
    }

    const int rightMarkerStart = rightMarkerIndex - delimiterLength + 1;
    if (delimiterLength <= 0 || rightMarkerStart < 0 || rightMarkerIndex < 0 ||
        blockText.at(rightMarkerIndex) != marker) {
        return;
    }

    for (int i = rightMarkerStart; i <= rightMarkerIndex; ++i) {
        if (blockText.at(i) != marker) {
            return;
        }
    }

    addSelectionCandidate(best, leftMarkerStart + delimiterLength, rightMarkerStart, targetStart, targetEnd);
}

struct WikiLinkCompletionContext {
    QString filterText;
    int startPosition = -1;
    int cursorPosition = -1;
};

bool currentWikiLinkCompletionContext(const QPlainTextEdit* edit, WikiLinkCompletionContext& context) {
    if (!edit) {
        return false;
    }

    QTextCursor cursor = edit->textCursor();
    if (cursor.hasSelection()) {
        return false;
    }

    const QTextBlock block = cursor.block();
    const QString blockText = block.text();
    const int positionInBlock = cursor.positionInBlock();
    const QString leftText = blockText.left(positionInBlock);
    const int openPos = leftText.lastIndexOf(QStringLiteral("[["));
    if (openPos < 0) {
        return false;
    }

    const QString candidate = leftText.mid(openPos + 2);
    if (candidate.contains(QChar('|')) || candidate.contains(QStringLiteral("]]")) || candidate.contains(QChar('[')) ||
        candidate.contains(QChar(']')) || candidate.contains(QChar('\n'))) {
        return false;
    }

    context.filterText = candidate.trimmed();
    context.startPosition = block.position() + openPos + 2;
    context.cursorPosition = cursor.position();
    return true;
}
}  // namespace

ArcNotesMarkdownTextEdit::ArcNotesMarkdownTextEdit(QWidget* parent) : QMarkdownTextEdit(parent, false) {
    // We need to set the internal variable to true, because we start with a highlighter
    _highlightingEnabled = true;
    _highlighter = nullptr;
    if (!parent || parent->objectName() != QStringLiteral("LogWidget")) {
        _highlighter = new ArcNotesMarkdownHighlighter(document());

        connect(_highlighter, &ArcNotesMarkdownHighlighter::highlightingFinished, this,
                &ArcNotesMarkdownTextEdit::refreshFoldingSidebar);
        connect(_highlighter, &ArcNotesMarkdownHighlighter::highlightingFinished, this,
                &ArcNotesMarkdownTextEdit::scheduleRestoreCurrentFoldedHeadingState);

        setStyles();
        updateSettings();
    }

    MarkdownHighlighter::HighlightingOptions options;

    if (editorSettingValue(this, QStringLiteral("fullyHighlightedBlockquotes")).toBool()) {
        options |= MarkdownHighlighter::HighlightingOption ::FullyHighlightedBlockQuote;
    }
    if (editorSettingValue(this, QStringLiteral("MainWindow/noteTextView.underline")).toBool()) {
        options |= MarkdownHighlighter::HighlightingOption ::Underline;
    }

    // set the highlighting options
    if (_highlighter) {
        _highlighter->setHighlightingOptions(options);

        // re-initialize the highlighting rules if we are using some options
        if (options != MarkdownHighlighter::HighlightingOption::None) {
            _highlighter->initHighlightingRules();
        }
    }

    // ignores note clicks in QMarkdownTextEdit in the note text edit
    setIgnoredClickUrlSchemata(QStringList({"note", "wikilink"}));

    connect(this, &ArcNotesMarkdownTextEdit::zoomIn, this, [this]() { onZoom(/*in=*/true); });
    connect(this, &ArcNotesMarkdownTextEdit::zoomOut, this, [this]() { onZoom(/*in=*/false); });

    connect(this, &ArcNotesMarkdownTextEdit::urlClicked, this, [this](const QString& url) {
        // Use the openUrl method which properly handles the openInNewTab flag
        openUrl(url, _openLinkInNewTab);
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &ArcNotesMarkdownTextEdit::customContextMenuRequested, this,
            &ArcNotesMarkdownTextEdit::onContextMenu);

    refreshFoldingSidebar();
}

/*
 * Prevent infinite loops of QResizeEvents in conjunction with setPaperMargins()
 * when the widget is resized (and leave some space for line numbers too)
 * See: https://github.com/pbek/ArcNotes/issues/2679
 */
QSize ArcNotesMarkdownTextEdit::minimumSizeHint() const {
    int lineWidthLeftMargin = _lineNumArea->lineNumAreaWidth();

    // Let the min size be the defaultMinSize + lineNumAreaWidth + paper margin
    auto sizeHint = QMarkdownTextEdit::minimumSizeHint();
    sizeHint.rwidth() += lineWidthLeftMargin + 10;

    return sizeHint;
}

void ArcNotesMarkdownTextEdit::onZoom(bool in) {
    FontModificationMode mode = in ? Increase : Decrease;
    const int fontSize = modifyFontSize(mode);

    auto* host = editorHost(this);
    if (host != nullptr && host->editorIsInDistractionFreeMode()) {
        setPaperMargins();
        if (in) {
            host->editorShowStatusMessage(tr("Increased font size to %1 pt").arg(fontSize), QStringLiteral("🔤"), 3000);
        } else {
            host->editorShowStatusMessage(tr("Decreased font size to %1 pt").arg(fontSize), QStringLiteral("🔤"), 3000);
        }
    }

    setPaperMargins();
}

/**
 * Sets the format style
 *
 * @param index
 * @param styles
 */
void ArcNotesMarkdownTextEdit::setFormatStyle(MarkdownHighlighter::HighlighterState index) {
    QTextCharFormat format;
    Utils::Schema::schemaSettings->setFormatStyle(index, format);

    if (index == MarkdownHighlighter::HighlighterState::WikiLink) {
        format.setFontUnderline(true);
        format.setUnderlineStyle(QTextCharFormat::DotLine);
    } else if (index == MarkdownHighlighter::HighlighterState::WikiLinkBroken) {
        format.setFontUnderline(true);
        format.setUnderlineStyle(QTextCharFormat::DashUnderline);
    }

    if (_highlighter) {
        _highlighter->setTextFormat(index, format);
    }
}

/**
 * Overrides the font size style if overrideInterfaceFontSize was set to prevent
 * Utils::Gui::updateInterfaceFontSize from overriding the default text size on
 * Windows 10
 *
 * @param fontSize
 */
void ArcNotesMarkdownTextEdit::overrideFontSizeStyle(int fontSize) {
    bool overrideInterfaceFontSize =
        editorSettingValue(this, QStringLiteral("overrideInterfaceFontSize"), false).toBool();

    // remove old style
    QString stylesheet = styleSheet().remove(QRegularExpression(
        QRegularExpression::escape(ARCNOTESMARKDOWNTEXTEDIT_OVERRIDE_FONT_SIZE_STYLESHEET_PRE_STRING) + ".*" +
        QRegularExpression::escape(ARCNOTESMARKDOWNTEXTEDIT_OVERRIDE_FONT_SIZE_STYLESHEET_POST_STRING)));

    if (overrideInterfaceFontSize) {
        // using pt is important here, px didn't work properly
        stylesheet += QStringLiteral(ARCNOTESMARKDOWNTEXTEDIT_OVERRIDE_FONT_SIZE_STYLESHEET_PRE_STRING) +
                      "ArcNotesMarkdownTextEdit {font-size: " + QString::number(fontSize) + "pt;}" +
                      QStringLiteral(ARCNOTESMARKDOWNTEXTEDIT_OVERRIDE_FONT_SIZE_STYLESHEET_POST_STRING);
    }

    setStyleSheet(stylesheet);
}

/**
 * Sets the highlighting styles for the text edit
 */
void ArcNotesMarkdownTextEdit::setStyles() {
    QFont font = Utils::Schema::schemaSettings->getEditorTextFont();
    setFont(font);

    // workaround for Windows 10 if overrideInterfaceFontSize was set
    overrideFontSizeStyle(font.pointSize());

    // set the tab stop to the width of 4 spaces in the editor
    const int tabStop = 4;
    QFontMetrics metrics(font);

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
    setTabStopWidth(tabStop * metrics.width(' '));
#else
    setTabStopDistance(tabStop * metrics.horizontalAdvance(' '));
#endif

    setFormatStyle(MarkdownHighlighter::HighlighterState::H1);
    setFormatStyle(MarkdownHighlighter::HighlighterState::H2);
    setFormatStyle(MarkdownHighlighter::HighlighterState::H3);
    setFormatStyle(MarkdownHighlighter::HighlighterState::H4);
    setFormatStyle(MarkdownHighlighter::HighlighterState::H5);
    setFormatStyle(MarkdownHighlighter::HighlighterState::H6);
    setFormatStyle(MarkdownHighlighter::HighlighterState::HorizontalRule);
    setFormatStyle(MarkdownHighlighter::HighlighterState::List);
    setFormatStyle(MarkdownHighlighter::HighlighterState::CheckBoxChecked);
    setFormatStyle(MarkdownHighlighter::HighlighterState::CheckBoxUnChecked);
    setFormatStyle(MarkdownHighlighter::HighlighterState::Bold);
    setFormatStyle(MarkdownHighlighter::HighlighterState::Italic);
    setFormatStyle(MarkdownHighlighter::HighlighterState::StUnderline);
    setFormatStyle(MarkdownHighlighter::HighlighterState::BlockQuote);
    setFormatStyle(MarkdownHighlighter::HighlighterState::CodeBlock);
    setFormatStyle(MarkdownHighlighter::HighlighterState::Comment);
    setFormatStyle(MarkdownHighlighter::HighlighterState::MaskedSyntax);
    setFormatStyle(MarkdownHighlighter::HighlighterState::Image);
    setFormatStyle(MarkdownHighlighter::HighlighterState::InlineCodeBlock);
    setFormatStyle(MarkdownHighlighter::HighlighterState::Link);
    setFormatStyle(MarkdownHighlighter::HighlighterState::LinkInternal);
    setFormatStyle(MarkdownHighlighter::HighlighterState::WikiLink);
    setFormatStyle(MarkdownHighlighter::HighlighterState::WikiLinkBroken);
    setFormatStyle(MarkdownHighlighter::HighlighterState::Table);
    setFormatStyle(MarkdownHighlighter::HighlighterState::BrokenLink);
    setFormatStyle(MarkdownHighlighter::HighlighterState::TrailingSpace);

    setFormatStyle(MarkdownHighlighter::HighlighterState::CodeType);
    setFormatStyle(MarkdownHighlighter::HighlighterState::CodeKeyWord);
    setFormatStyle(MarkdownHighlighter::HighlighterState::CodeComment);
    setFormatStyle(MarkdownHighlighter::HighlighterState::CodeString);
    setFormatStyle(MarkdownHighlighter::HighlighterState::CodeNumLiteral);
    setFormatStyle(MarkdownHighlighter::HighlighterState::CodeBuiltIn);
    setFormatStyle(MarkdownHighlighter::HighlighterState::CodeOther);

#ifdef Q_OS_WIN32
    // set the selection background color to a light blue if not in dark mode
    if (!editorSettingValue(this, QStringLiteral("darkMode")).toBool()) {
        // light green (#9be29b) could be another choice, but be aware that
        // this color will be used for mouse and keyboard selections too
        setStyleSheet(styleSheet() +
                      "QWidget {selection-color: #ffffff;"
                      "selection-background-color: #3399ff}");
    }
#endif
}

/**
 * Modifies the font size of the text edit
 */
int ArcNotesMarkdownTextEdit::modifyFontSize(FontModificationMode mode) {
    QFont font = this->font();
    int fontSize = font.pointSize();
    bool doSetStyles = false;

    // modify the text edit default font
    QString fontString = editorSettingValue(this, QStringLiteral("MainWindow/noteTextEdit.font")).toString();
    if (!fontString.isEmpty()) {
        font.fromString(fontString);

        fontSize = font.pointSize();

        switch (mode) {
            case FontModificationMode::Increase:
                fontSize++;
                doSetStyles = true;
                break;
            case FontModificationMode::Decrease:
                fontSize--;

                if (fontSize < 5) {
                    fontSize = 5;
                } else {
                    doSetStyles = true;
                }
                break;
            default:
                QPlainTextEdit textEdit;
                int newFontSize = textEdit.font().pointSize();
                if (fontSize != newFontSize) {
                    fontSize = newFontSize;
                    doSetStyles = true;
                }
        }

        if (fontSize > 0) {
            font.setPointSize(fontSize);
        }

        // store the font settings
        editorSetSettingValue(this, QStringLiteral("MainWindow/noteTextEdit.font"), font.toString());
    }

    // modify the text edit code font
    fontString = editorSettingValue(this, QStringLiteral("MainWindow/noteTextEdit.code.font")).toString();
    if (!fontString.isEmpty()) {
        font.fromString(fontString);

        int codeFontSize = font.pointSize();

        switch (mode) {
            case FontModificationMode::Increase:
                codeFontSize++;
                doSetStyles = true;
                break;
            case FontModificationMode::Decrease:
                codeFontSize--;

                if (codeFontSize < 5) {
                    codeFontSize = 5;
                } else {
                    doSetStyles = true;
                }
                break;
            default:
                QPlainTextEdit textEdit;
                int newCodeFontSize = textEdit.font().pointSize();
                if (codeFontSize != newCodeFontSize) {
                    codeFontSize = newCodeFontSize;
                    doSetStyles = true;
                }
        }

        if (codeFontSize > 0) {
            font.setPointSize(codeFontSize);
        }

        // store the font settings
        editorSetSettingValue(this, QStringLiteral("MainWindow/noteTextEdit.code.font"), font.toString());
    }

    if (doSetStyles) {
        this->setStyles();
        if (_highlighter) {
            _highlighter->rehighlight();
        }
    }

    return fontSize;
}

/**
 * Handles clicked urls (including relative urls)
 *
 * examples:
 * - <https://www.arcnotes.org> opens the webpage
 * - <file:///path/to/my/file/ArcNotes.pdf> opens the file
 * "/path/to/my/file/ArcNotes.pdf" if the operating system
 * supports that handler
 */
void ArcNotesMarkdownTextEdit::openUrl(const QString& urlString, bool openInNewTab) {
    qDebug() << "ArcNotesMarkdownTextEdit " << __func__ << " - 'urlString': " << urlString
             << " - 'openInNewTab': " << openInNewTab;

    QString notesPath = editorCurrentNoteFolderPath(this);
    QString windowsSlash = QString();

#ifdef Q_OS_WIN32
    // we need another slash for Windows
    windowsSlash = QStringLiteral("/");
#endif

    auto urlCopy = urlString;

    // parse for relative file urls and make them absolute
    urlCopy.replace(QRegularExpression(QStringLiteral("^file:[\\/]{2}([^\\/].+)$")),
                    QStringLiteral("file://") + windowsSlash + notesPath + QStringLiteral("/\\1"));

    // Check if this is a note URL that should be handled specially
    QUrl url(urlCopy);
    const QString scheme = url.scheme();

    // If it's a note URL, noteid URL, file URL in note folder, or has no
    // scheme (relative link), use UrlHandler which knows how to handle these properly.
    if (scheme == QStringLiteral("note") || scheme == QStringLiteral("noteid") ||
        scheme == QStringLiteral("wikilink") || scheme == QStringLiteral("file") || scheme.isEmpty() ||
        editorFileUrlIsNoteInCurrentFolder(this, url)) {
        // Use UrlHandler for note URLs with the openInNewTab flag.
        UrlHandler(dynamic_cast<UrlHandlerContext*>(window())).openUrl(urlCopy, openInNewTab);
    } else {
        // For other URLs (http, https, etc.), use the base class implementation
        QMarkdownTextEdit::openUrl(urlCopy, openInNewTab);
    }
}

// void ArcNotesMarkdownTextEdit::setViewportMargins(
//        int left, int top, int right, int bottom) {
//    QMarkdownTextEdit::setViewportMargins(left, top, right, bottom);
//}

/**
 * Sets the viewport margins for the distraction free mode
 */
void ArcNotesMarkdownTextEdit::setPaperMargins(int width) {
    bool isInDistractionFreeMode = editorSettingValue(this, QStringLiteral("DistractionFreeMode/isEnabled")).toBool();
    bool editorWidthInDFMOnly = editorSettingValue(this, QStringLiteral("Editor/editorWidthInDFMOnly"), true).toBool();

    if (isInDistractionFreeMode || !editorWidthInDFMOnly) {
        int margin = 0;

        if (width == -1) {
            width = this->width();
        }

        int editorWidthMode = editorSettingValue(this, QStringLiteral("DistractionFreeMode/editorWidthMode")).toInt();

        if (editorWidthMode != Full) {
            QFontMetrics metrics(font());

            int characterAmount = 0;
            switch (editorWidthMode) {
                case Medium:
                    characterAmount = 80;
                    break;
                case Wide:
                    characterAmount = 100;
                    break;
                case Custom:
                    characterAmount =
                        editorSettingValue(this, QStringLiteral("DistractionFreeMode/editorWidthCustom"), 80).toInt();
                    break;
                default:
                case Narrow:
                    characterAmount = 60;
                    break;
            }

                // set the size of characterAmount times the size of "O"
                // characters
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
            int proposedEditorWidth = metrics.width(QStringLiteral("O").repeated(characterAmount));
#else
            int proposedEditorWidth = metrics.horizontalAdvance(QStringLiteral("O").repeated(characterAmount));
#endif

            // Apply a factor to correct the faulty calculated margin
            // Use a different factor for monospaced fonts
            // TODO(pbek): I don't know better way to get around this yet
            proposedEditorWidth /= usesMonospacedFont() ? 0.95 : 1.332;

            // calculate the margin to be applied
            margin = (width - proposedEditorWidth) / 2;

            if (margin < 0) {
                margin = 0;
            }
        }

        setViewportMargins(margin, 20, margin, 0);
    } else {
        int lineWidthLeftMargin = lineNumberArea()->lineNumAreaWidth();

        setLineNumberLeftMarginOffset(10);
        setViewportMargins(10 + lineWidthLeftMargin, 10, 10, 0);
    }
}

/**
 * Try to determine if the used font is monospaced
 *
 * @return
 */
bool ArcNotesMarkdownTextEdit::usesMonospacedFont() {
    QFontMetrics metrics(font());

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
    int widthNarrow = metrics.width(QStringLiteral("iiiii"));
    int widthWide = metrics.width(QStringLiteral("WWWWW"));
#else
    int widthNarrow = metrics.horizontalAdvance(QStringLiteral("iiiii"));
    int widthWide = metrics.horizontalAdvance(QStringLiteral("WWWWW"));
#endif

    return widthNarrow == widthWide;
}

void ArcNotesMarkdownTextEdit::toggleCase() {
    QTextCursor c = textCursor();
    // Save positions to restore everything at the end
    const int selectionStart = c.selectionStart();
    const int selectionEnd = c.selectionEnd();
    const int cPos = c.position();

    QString selectedText = c.selectedText();
    const bool textWasSelected = !selectedText.isEmpty();

    // if no text is selected: automatically select the Word under the Cursor
    if (selectedText.isEmpty()) {
        c.select(QTextCursor::WordUnderCursor);
        selectedText = c.selectedText();
    }

    // cycle text through lowercase, uppercase, start case, and sentence case
    c.insertText(Utils::Misc::cycleTextCase(selectedText));

    if (textWasSelected) {
        // select the text again to maybe do another operation on it
        // keep the original cursor position
        if (cPos == selectionStart) {
            c.setPosition(selectionEnd, QTextCursor::MoveAnchor);
            c.setPosition(selectionStart, QTextCursor::KeepAnchor);
        } else {
            c.setPosition(selectionStart, QTextCursor::MoveAnchor);
            c.setPosition(selectionEnd, QTextCursor::KeepAnchor);
        }
    } else {
        // Just restore the Cursor Position if no text was selected
        c.setPosition(cPos, QTextCursor::MoveAnchor);
    }
    // Restore the visible cursor
    setTextCursor(c);
}

void ArcNotesMarkdownTextEdit::selectEnclosedText() {
    QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    if (!block.isValid()) {
        return;
    }

    const QString blockText = block.text();
    if (blockText.isEmpty()) {
        return;
    }

    const int blockStart = block.position();
    int targetStart = cursor.positionInBlock();
    int targetEnd = targetStart;

    if (cursor.hasSelection()) {
        QTextCursor selectionStartCursor(document());
        selectionStartCursor.setPosition(cursor.selectionStart());

        const int selectionEndPos = std::max(cursor.selectionStart(), cursor.selectionEnd() - 1);
        QTextCursor selectionEndCursor(document());
        selectionEndCursor.setPosition(selectionEndPos);

        if (selectionStartCursor.block() != block || selectionEndCursor.block() != block) {
            return;
        }

        targetStart = cursor.selectionStart() - blockStart;
        targetEnd = cursor.selectionEnd() - blockStart;
    }

    InnerSelectionCandidate bestCandidate;
    const int lastBlockPosition = blockText.size() - 1;
    const int probePosition = qBound(0, std::min(targetStart, lastBlockPosition), lastBlockPosition);

    addMarkdownRangeCandidate(blockText, highlighter(), MarkdownHighlighter::RangeType::CodeSpan, block.blockNumber(),
                              probePosition, targetStart, targetEnd, bestCandidate);
    addMarkdownRangeCandidate(blockText, highlighter(), MarkdownHighlighter::RangeType::Emphasis, block.blockNumber(),
                              probePosition, targetStart, targetEnd, bestCandidate);

    addStackedTokenCandidates(blockText, QStringLiteral("[["), QStringLiteral("]]"), targetStart, targetEnd,
                              bestCandidate);
    addStackedTokenCandidates(blockText, QStringLiteral("("), QStringLiteral(")"), targetStart, targetEnd,
                              bestCandidate);
    addStackedTokenCandidates(blockText, QStringLiteral("["), QStringLiteral("]"), targetStart, targetEnd,
                              bestCandidate);
    addStackedTokenCandidates(blockText, QStringLiteral("{"), QStringLiteral("}"), targetStart, targetEnd,
                              bestCandidate);

    addRepeatedTokenCandidates(blockText, QStringLiteral("~~"), targetStart, targetEnd, bestCandidate, false);
    addRepeatedTokenCandidates(blockText, QStringLiteral("\""), targetStart, targetEnd, bestCandidate, true);
    addRepeatedTokenCandidates(blockText, QStringLiteral("'"), targetStart, targetEnd, bestCandidate, true);

    if (!bestCandidate.isValid()) {
        return;
    }

    cursor.setPosition(blockStart + bestCandidate.innerStart);
    cursor.setPosition(blockStart + bestCandidate.innerEnd, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
}

void ArcNotesMarkdownTextEdit::insertCodeBlock() {
    QTextCursor c = this->textCursor();
    QString selectedText = c.selection().toPlainText();

    if (selectedText.isEmpty()) {
        // insert multi-line code block if cursor is in an empty line
        if (c.atBlockStart() && c.atBlockEnd()) {
            c.insertText(QStringLiteral("```\n\n```"));
            c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 3);
        } else {
            c.insertText(QStringLiteral("``"));
        }

        c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor);
        setTextCursor(c);
    } else {
        bool addNewline = false;

        // if the selected text has multiple lines add a multi-line code block
        if (selectedText.contains(QStringLiteral("\n"))) {
            // add another newline if there is no newline at the end of the
            // selected text
            const QString endNewline = selectedText.endsWith(QLatin1String("\n")) ? QString() : QStringLiteral("\n");

            selectedText = QStringLiteral("``\n") + selectedText + endNewline + QStringLiteral("``");
            addNewline = true;
        }

        c.insertText(QStringLiteral("`") + selectedText + QStringLiteral("`"));

        if (addNewline) {
            c.insertText(QStringLiteral("\n"));
        }
    }
}

void ArcNotesMarkdownTextEdit::insertWikiLink() {
    QTextCursor cursor = textCursor();
    const QString selectedText = cursor.selectedText();

    if (!selectedText.isEmpty()) {
        cursor.insertText(QStringLiteral("[[") + selectedText + QStringLiteral("]]"));
        setTextCursor(cursor);
        return;
    }

    cursor.insertText(QStringLiteral("[[]]"));
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
    setTextCursor(cursor);

    if (editorWikiLinkSupportEnabled(this)) {
        QTimer::singleShot(0, this, &ArcNotesMarkdownTextEdit::onAutoCompleteRequested);
    }
}

void ArcNotesMarkdownTextEdit::onAutoCompleteRequested() {
    if (isReadOnly() || !Utils::Misc::isNoteEditingAllowed()) {
        if (openLinkAtCursorPosition()) {
            showEditorStatusMessage(this, tr("An url was opened at the current cursor position"), QStringLiteral("📃"),
                                    5000);
        }

        return;
    }

    // attempt to toggle a checkbox at the cursor position
    if (Utils::Gui::toggleCheckBoxAtCursor(this)) {
        return;
    }

    QString wikiFilterText;
    int wikiReplaceLength = 0;
    QStringList wikiResultList;
    const bool wikiContextActive =
        editorWikiLinkSupportEnabled(this) && wikiLinkAutoComplete(wikiResultList, wikiFilterText, wikiReplaceLength);

    // Don't treat typing inside a wiki-link target as an activation request.
    // This prevents completing the leading "[[" from opening or creating the note.
    if (!wikiContextActive && openLinkAtCursorPosition()) {
        showEditorStatusMessage(this, tr("An url was opened at the current cursor position"), QStringLiteral("📃"),
                                5000);
        return;
    }

    // attempt a Markdown table auto-format
    if (Utils::Gui::autoFormatTableAtCursor(this)) {
        return;
    }

    QMenu menu;

    if (wikiContextActive) {
        for (const QString& text : Utils::asConst(wikiResultList)) {
            auto* action = menu.addAction(text);
            action->setData(text);
            action->setWhatsThis(QStringLiteral("wikilink-autocomplete"));
        }
    }

    double resultValue;
    if (solveEquation(resultValue)) {
        const QString text = QString::number(resultValue);
        auto* action = menu.addAction(QStringLiteral("= ") + text);
        action->setData(text);
        action->setWhatsThis(QStringLiteral("equation"));
    }

    QStringList resultList;
    if (!wikiContextActive && autoComplete(resultList)) {
        for (const QString& text : Utils::asConst(resultList)) {
            auto* action = menu.addAction(text);
            action->setData(text);
            action->setWhatsThis(QStringLiteral("autocomplete"));
        }
    }

    QPoint globalPos = mapToGlobal(cursorRect().bottomRight());

    // compensate viewport margins
    globalPos.setY(globalPos.y() + viewportMargins().top());
    globalPos.setX(globalPos.x() + viewportMargins().left());

    if (menu.actions().count() > 0) {
        QAction* selectedItem = menu.exec(globalPos);
        if (selectedItem) {
            const QString text = selectedItem->data().toString();
            const QString type = selectedItem->whatsThis();

            if (text.isEmpty()) {
                return;
            }

            if (type == QStringLiteral("wikilink-autocomplete")) {
                WikiLinkCompletionContext context;
                if (!currentWikiLinkCompletionContext(this, context)) {
                    return;
                }

                QTextCursor c = textCursor();
                c.setPosition(context.startPosition, QTextCursor::MoveAnchor);
                c.setPosition(context.cursorPosition, QTextCursor::KeepAnchor);
                c.insertText(text);

                const QString rightText = toPlainText().mid(c.position(), 2);
                if (rightText != QStringLiteral("]]")) {
                    c.insertText(QStringLiteral("]]"));
                    c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
                }

                setTextCursor(c);
            } else if (type == QStringLiteral("autocomplete")) {
                // overwrite the currently written word
                QTextCursor c = textCursor();
                c.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
                c.insertText(text + QStringLiteral(" "));
            } else {
                insertPlainText(text);
            }
        }
    }
}

/**
 * Returns the text from the current cursor to the start of the word in the
 * note text edit
 *
 * @param withPreviousCharacters also get more characters at the beginning
 *                               to get characters like "@" that are not
 *                               word-characters
 * @return
 */
QString ArcNotesMarkdownTextEdit::currentWord(bool withPreviousCharacters) const {
    QTextCursor c = textCursor();

    // get the text from the current word
    c.movePosition(QTextCursor::EndOfWord);
    c.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);

    QString text = c.selectedText();

    if (withPreviousCharacters) {
        static const QRegularExpression re(QStringLiteral("^[\\s\\n][^\\s]*"));
        do {
            c.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
            text = c.selectedText();
        } while (!(re.match(text).hasMatch() || c.atBlockStart()));
    }

    return text.trimmed();
}

QString ArcNotesMarkdownTextEdit::currentBlock() const {
    QTextCursor cursor = textCursor();
    QTextBlock currentBlock = cursor.block();
    return currentBlock.text();
}

/**
 * Tries to find words that start with the current word in the note text edit
 *
 * @param resultList
 * @return
 */
bool ArcNotesMarkdownTextEdit::autoComplete(QStringList& resultList) const {
    // get the text from the current cursor to the start of the word
    const QString text = currentWord();
    qDebug() << __func__ << " - 'text': " << text;

    if (text.isEmpty()) {
        return false;
    }

    const QString noteText = toPlainText();

    // find all items that match our current word
    resultList =
        noteText
            .split(QRegularExpression(QStringLiteral("[^\\w\\d]"), QRegularExpression::UseUnicodePropertiesOption),
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                   QString::SkipEmptyParts)
#else
                   Qt::SkipEmptyParts)
#endif
            .filter(QRegularExpression(QStringLiteral("^") + QRegularExpression::escape(text),
                                       QRegularExpression::CaseInsensitiveOption));

    // we only want each word once
    resultList.removeDuplicates();

    // remove the text we already entered
    resultList.removeOne(text);

    if (resultList.count() == 0) {
        return false;
    }

    qDebug() << __func__ << " - 'resultList': " << resultList;

    return true;
}

bool ArcNotesMarkdownTextEdit::wikiLinkAutoComplete(QStringList& resultList, QString& filterText,
                                                    int& replaceLength) const {
    WikiLinkCompletionContext context;
    if (!currentWikiLinkCompletionContext(this, context)) {
        return false;
    }

    filterText = context.filterText;
    replaceLength = context.cursorPosition - context.startPosition;

    QSet<QString> results;
    QVector<NoteData> notes;
    if (auto* host = editorHost(this)) {
        notes = host->editorAllNotes();
    }
    for (const NoteData& note : notes) {
        results.insert(note.name);

        const QString subfolderPath = note.relativeNoteSubFolderPath;
        if (!subfolderPath.isEmpty()) {
            results.insert(subfolderPath + QStringLiteral("/") + note.name);
        }
    }

    resultList = results.values();
    std::sort(resultList.begin(), resultList.end(),
              [](const QString& a, const QString& b) { return a.toLower() < b.toLower(); });

    if (!filterText.isEmpty()) {
        resultList = resultList.filter(
            QRegularExpression(QRegularExpression::escape(filterText), QRegularExpression::CaseInsensitiveOption));
    }

    resultList.removeDuplicates();
    resultList.removeAll(filterText);
    return !resultList.isEmpty();
}

/**
 * Tries to find an equation in the current line and solves it
 *
 * @param returnValue
 * @return
 */
bool ArcNotesMarkdownTextEdit::solveEquation(double& returnValue) {
    QTextCursor c = textCursor();

    // get the text from the current cursor to the start of the line
    c.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    QString text = c.selectedText();
    qDebug() << __func__ << " - 'text': " << text;

    QString equation = text;

    // replace "," with "." to allow "," as coma
    equation.replace(QLatin1Char(','), QLatin1Char('.'));

    // remove leading list characters
    equation.remove(QRegularExpression(QStringLiteral(R"(^\s*[\-*+] )")));

    // match all numbers and basic operations like +, -, * and /
    QRegularExpressionMatch match = QRegularExpression(QStringLiteral(R"(([\d\.,+\-*\/\(\)\s]+)\s*=)")).match(equation);

    if (!match.hasMatch()) {
        if (equation.trimmed().endsWith(QChar('='))) {
            showEditorStatusMessage(this, tr("No equation was found in front of the cursor"), QStringLiteral("🧮"),
                                    5000);
        }

        return false;
    }

    equation = match.captured(1);
    qDebug() << __func__ << " - 'equation': " << equation;

    double resultValue = 0;
    if (!BasicExpressionParser(equation).parse(resultValue)) {
        showEditorStatusMessage(this, tr("No equation was found in front of the cursor"), QStringLiteral("🧮"), 5000);
        return false;
    }

    qDebug() << __func__ << " - 'resultValue': " << resultValue;

    // compensate for subtraction errors with 0
    if ((resultValue < 0.0001) && (resultValue > 0)) {
        resultValue = 0;
    }

    showEditorStatusMessage(this, tr("Result for equation: %1 = %2").arg(equation, QString::number(resultValue)),
                            QStringLiteral("🧮"), 10000);

    // check if cursor is after the "="
    match = QRegularExpression(QStringLiteral("=\\s*$")).match(text);
    if (!match.hasMatch()) {
        return false;
    }

    returnValue = resultValue;
    return true;
}

void ArcNotesMarkdownTextEdit::insertBlockQuote() {
    QTextCursor c = textCursor();
    QString selectedText = c.selectedText();

    if (selectedText.isEmpty()) {
        c.insertText(QStringLiteral("> "));
        setTextCursor(c);
    } else {
        // this only applies to the start of the selected block
        selectedText.replace(QRegularExpression(QStringLiteral("^")), QStringLiteral("> "));

        // transform Unicode line endings
        // this newline character seems to be used in multi-line selections
        const QString newLine = QString::fromUtf8(QByteArray::fromHex("e280a9"));
        selectedText.replace(newLine, QStringLiteral("\n> "));

        // remove the block quote if it was placed at the end of the text
        selectedText.remove(QRegularExpression(QStringLiteral("> $")));

        c.insertText(selectedText);
    }
}

QTextCursor ArcNotesMarkdownTextEdit::fullLineSelectionCursor() const {
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        return cursor;
    }

    const int selectionStart = cursor.selectionStart();
    int selectionEnd = cursor.selectionEnd();

    QTextCursor lineCursor(document());
    lineCursor.setPosition(selectionStart);
    lineCursor.movePosition(QTextCursor::StartOfBlock);

    QTextCursor endCursor(document());
    endCursor.setPosition(selectionEnd);
    if (selectionEnd > selectionStart && endCursor.atBlockStart()) {
        endCursor.movePosition(QTextCursor::PreviousCharacter);
    }
    endCursor.movePosition(QTextCursor::EndOfBlock);

    lineCursor.setPosition(endCursor.position(), QTextCursor::KeepAnchor);
    return lineCursor;
}

bool ArcNotesMarkdownTextEdit::replaceFullLineSelection(const QString& text) {
    QTextCursor cursor = fullLineSelectionCursor();
    if (!cursor.hasSelection()) {
        return false;
    }

    const int start = cursor.selectionStart();
    cursor.beginEditBlock();
    cursor.insertText(text);
    cursor.setPosition(start);
    cursor.setPosition(start + text.size(), QTextCursor::KeepAnchor);
    cursor.endEditBlock();
    setTextCursor(cursor);
    return true;
}

bool ArcNotesMarkdownTextEdit::changeHeadingDepthOfSelection(const int levelDelta) {
    QTextCursor cursor = fullLineSelectionCursor();
    if (!cursor.hasSelection()) {
        return false;
    }

    return replaceFullLineSelection(changeHeadingDepth(cursor.selectedText(), levelDelta));
}

QMargins ArcNotesMarkdownTextEdit::viewportMargins() {
    return QMarkdownTextEdit::viewportMargins();
}

void ArcNotesMarkdownTextEdit::setText(const QString& text) {
    // Set a search delay of 300ms for text with more than 20k characters
    setSearchWidgetDebounceDelay(text.size() > 20000 ? 300 : 0);
    _foldingStateRestorePending = !_currentNoteReference.isEmpty();
    _foldingStateRestoreAttempts = 0;

    QMarkdownTextEdit::setText(text);
}

void ArcNotesMarkdownTextEdit::setCurrentNoteReference(const QString& noteReference) {
    const QString effectiveNoteReference = _headingFoldingEnabled ? noteReference : QString();
    auto* qownNotesHighlighter = qobject_cast<ArcNotesMarkdownHighlighter*>(_highlighter);
    if (qownNotesHighlighter != nullptr) {
        const NoteData currentNote = editorHost(this) == nullptr ? NoteData() : editorHost(this)->editorCurrentNote();
        qownNotesHighlighter->updateCurrentNote(currentNote);
    }

    if (_currentNoteReference == effectiveNoteReference) {
        _foldingStateRestorePending = !effectiveNoteReference.isEmpty();
        _foldingStateRestoreAttempts = 0;
        return;
    }

    storeCurrentFoldedHeadingState();
    _currentNoteReference = effectiveNoteReference;
    _foldingStateRestorePending = !_currentNoteReference.isEmpty();
    _foldingStateRestoreAttempts = 0;
}

void ArcNotesMarkdownTextEdit::resizeEvent(QResizeEvent* event) {
    setPaperMargins();
    QMarkdownTextEdit::resizeEvent(event);
}

void ArcNotesMarkdownTextEdit::paintEvent(QPaintEvent* event) {
    if (_showMarkdownImagePreviews) {
        QMarkdownTextEdit::paintEvent(event);
        paintMarkdownImagePreviews();
    } else {
        QMarkdownTextEdit::paintEvent(event);
    }
}

int ArcNotesMarkdownTextEdit::sidebarAdditionalWidth() const {
    if (objectName() == QStringLiteral("logTextEdit") || !_headingFoldingEnabled || !_hasFoldableHeadings) {
        return 0;
    }

    return fontMetrics().height() + (kFoldIndicatorPadding * 2);
}

bool ArcNotesMarkdownTextEdit::isHeadingBlock(const QTextBlock& block, int* level) {
    if (!block.isValid()) {
        return false;
    }

    const int state = block.userState();
    if (state < MarkdownHighlighter::H1 || state > MarkdownHighlighter::H6) {
        return false;
    }

    if (level != nullptr) {
        *level = state - MarkdownHighlighter::H1 + 1;
    }

    return true;
}

bool ArcNotesMarkdownTextEdit::foldRegionForHeaderBlock(const QTextBlock& headerBlock, FoldRegion& region) const {
    int level = 0;
    if (!isHeadingBlock(headerBlock, &level)) {
        return false;
    }

    QTextBlock block = headerBlock.next();
    QTextBlock lastContentBlock;

    while (block.isValid()) {
        int nextLevel = 0;
        if (isHeadingBlock(block, &nextLevel) && nextLevel <= level) {
            break;
        }

        lastContentBlock = block;
        block = block.next();
    }

    if (!lastContentBlock.isValid()) {
        return false;
    }

    region.headerBlock = headerBlock;
    region.firstContentBlock = headerBlock.next();
    region.lastContentBlock = lastContentBlock;
    return true;
}

bool ArcNotesMarkdownTextEdit::isHeadingFolded(const QTextBlock& headerBlock) const {
    FoldRegion region;
    return foldRegionForHeaderBlock(headerBlock, region) && !region.firstContentBlock.isVisible();
}

QString ArcNotesMarkdownTextEdit::headingStateKey(const QTextBlock& headerBlock,
                                                  QHash<QString, int>& headingOccurrences) {
    int level = 0;
    if (!isHeadingBlock(headerBlock, &level)) {
        return QString();
    }

    const QString headingText = headerBlock.text();
    const int occurrence = ++headingOccurrences[headingText];
    return QStringLiteral("%1:%2:%3").arg(level).arg(occurrence).arg(headingText);
}

bool ArcNotesMarkdownTextEdit::setFoldRegionFolded(const FoldRegion& region, bool folded) {
    if (!region.firstContentBlock.isValid() || !region.lastContentBlock.isValid()) {
        return false;
    }

    const bool currentlyFolded = !region.firstContentBlock.isVisible();
    if (currentlyFolded == folded) {
        return false;
    }

    QTextCursor cursor = textCursor();
    const int firstPosition = region.firstContentBlock.position();
    const int lastPosition = region.lastContentBlock.position() + region.lastContentBlock.length();

    if (folded && cursor.position() >= firstPosition && cursor.position() < lastPosition) {
        cursor.setPosition(region.headerBlock.position());
        setTextCursor(cursor);
    }

    QTextBlock block = region.firstContentBlock;
    while (block.isValid()) {
        block.setVisible(!folded);
        if (folded) {
            block.setLineCount(0);
        } else {
            QTextLayout* layout = block.layout();
            block.setLineCount(layout ? qMax(1, layout->lineCount()) : 1);
        }

        if (block == region.lastContentBlock) {
            break;
        }

        block = block.next();
    }

    document()->markContentsDirty(firstPosition, lastPosition - firstPosition);
    viewport()->update();
    lineNumberArea()->update();
    ensureCursorVisible();
    storeCurrentFoldedHeadingState();
    return true;
}

bool ArcNotesMarkdownTextEdit::setHeadingFolded(const QTextBlock& headerBlock, bool folded) {
    FoldRegion region;
    return foldRegionForHeaderBlock(headerBlock, region) && setFoldRegionFolded(region, folded);
}

bool ArcNotesMarkdownTextEdit::hasFoldableHeadings() const {
    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        FoldRegion region;
        if (foldRegionForHeaderBlock(block, region)) {
            return true;
        }

        block = block.next();
    }

    return false;
}

void ArcNotesMarkdownTextEdit::refreshFoldingSidebar() {
    if (!_headingFoldingEnabled) {
        _hasFoldableHeadings = false;
        updateLineNumberAreaWidth(0);
        lineNumberArea()->update();
        return;
    }

    const bool hadFoldableHeadings = _hasFoldableHeadings;
    _hasFoldableHeadings = hasFoldableHeadings();

    if (hadFoldableHeadings != _hasFoldableHeadings) {
        updateLineNumberAreaWidth(0);
    }

    lineNumberArea()->update();
}

void ArcNotesMarkdownTextEdit::paintSidebar(QPainter* painter, const QRect& eventRect) {
    const int additionalWidth = sidebarAdditionalWidth();
    if (additionalWidth <= 0) {
        return;
    }

    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    top += viewportMargins().top();
    int bottom = top;

    const QColor iconColor = palette().color(QPalette::Active, QPalette::Text);
    const QColor borderColor = iconColor.lighter(130);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    while (block.isValid() && top <= eventRect.bottom()) {
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());

        if (block.isVisible() && bottom >= eventRect.top()) {
            FoldRegion region;
            if (foldRegionForHeaderBlock(block, region)) {
                const int indicatorSize = qMax(7, fontMetrics().height() - 8);
                const int x = kFoldIndicatorPadding;
                const int y = top + qMax(0, (qRound(blockBoundingRect(block).height()) - indicatorSize) / 2);
                const QRect indicatorRect(x, y, indicatorSize, indicatorSize);
                const bool folded = isHeadingFolded(block);

                painter->setPen(borderColor);
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(indicatorRect.adjusted(0, 0, -1, -1));

                painter->setPen(QPen(iconColor, 1.5));
                painter->drawLine(indicatorRect.left() + 2, indicatorRect.center().y(), indicatorRect.right() - 2,
                                  indicatorRect.center().y());

                if (folded) {
                    painter->drawLine(indicatorRect.center().x(), indicatorRect.top() + 2, indicatorRect.center().x(),
                                      indicatorRect.bottom() - 2);
                }
            }
        }

        block = block.next();
    }

    painter->restore();
}

bool ArcNotesMarkdownTextEdit::headerBlockAtSidebarPosition(const QPoint& pos, QTextBlock& headerBlock) const {
    if (pos.x() > sidebarAdditionalWidth()) {
        return false;
    }

    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    top += QPlainTextEdit::viewportMargins().top();
    int bottom = top;

    while (block.isValid()) {
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());

        if (block.isVisible() && pos.y() >= top && pos.y() <= bottom) {
            FoldRegion region;
            if (foldRegionForHeaderBlock(block, region)) {
                headerBlock = block;
                return true;
            }

            return false;
        }

        if (top > pos.y()) {
            return false;
        }

        block = block.next();
    }

    return false;
}

void ArcNotesMarkdownTextEdit::storeCurrentFoldedHeadingState() {
    if (!_headingFoldingEnabled || _currentNoteReference.isEmpty() || _isApplyingStoredFoldingState) {
        return;
    }

    QSet<QString> foldedHeadingState;
    QHash<QString, int> headingOccurrences;
    QTextBlock block = document()->firstBlock();

    while (block.isValid()) {
        if (isHeadingFolded(block)) {
            const QString headingKey = headingStateKey(block, headingOccurrences);
            if (!headingKey.isEmpty()) {
                foldedHeadingState.insert(headingKey);
            }
        } else if (isHeadingBlock(block)) {
            headingStateKey(block, headingOccurrences);
        }

        block = block.next();
    }

    if (foldedHeadingState.isEmpty()) {
        s_foldedHeadingStateByNoteReference.remove(_currentNoteReference);
    } else {
        s_foldedHeadingStateByNoteReference[_currentNoteReference] = foldedHeadingState;
    }
}

void ArcNotesMarkdownTextEdit::scheduleRestoreCurrentFoldedHeadingState() {
    if (!_foldingStateRestorePending || _currentNoteReference.isEmpty()) {
        return;
    }

    QTimer::singleShot(0, this, &ArcNotesMarkdownTextEdit::restoreCurrentFoldedHeadingState);
}

void ArcNotesMarkdownTextEdit::restoreCurrentFoldedHeadingState() {
    if (!_foldingStateRestorePending || _currentNoteReference.isEmpty() || !_headingFoldingEnabled) {
        return;
    }

    const QSet<QString> foldedHeadingState = s_foldedHeadingStateByNoteReference.value(_currentNoteReference);
    _foldingStateRestorePending = false;

    if (foldedHeadingState.isEmpty()) {
        return;
    }

    if (!_hasFoldableHeadings && _foldingStateRestoreAttempts < 5) {
        ++_foldingStateRestoreAttempts;
        QTimer::singleShot(0, this, &ArcNotesMarkdownTextEdit::restoreCurrentFoldedHeadingState);
        return;
    }

    _isApplyingStoredFoldingState = true;
    _foldingStateRestoreAttempts = 0;
    QHash<QString, int> headingOccurrences;
    QTextBlock block = document()->firstBlock();

    while (block.isValid()) {
        const QString headingKey = headingStateKey(block, headingOccurrences);
        if (!headingKey.isEmpty() && foldedHeadingState.contains(headingKey)) {
            setHeadingFolded(block, true);
        }

        block = block.next();
    }

    _isApplyingStoredFoldingState = false;
    refreshFoldingSidebar();
}

bool ArcNotesMarkdownTextEdit::sidebarMousePressEvent(QMouseEvent* event) {
    if (event == nullptr || event->button() != Qt::LeftButton || sidebarAdditionalWidth() <= 0) {
        return false;
    }

    QTextBlock headerBlock;
    if (!headerBlockAtSidebarPosition(event->pos(), headerBlock)) {
        return false;
    }

    const bool handled = setHeadingFolded(headerBlock, !isHeadingFolded(headerBlock));
    if (handled) {
        event->accept();
    }

    return handled;
}

void ArcNotesMarkdownTextEdit::foldAllHeadings() {
    if (!_headingFoldingEnabled) {
        return;
    }

    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        setHeadingFolded(block, true);
        block = block.next();
    }

    refreshFoldingSidebar();
}

void ArcNotesMarkdownTextEdit::unfoldAllHeadings() {
    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        setHeadingFolded(block, false);
        block = block.next();
    }

    refreshFoldingSidebar();
}

/**
 * Resolves a raw Markdown image source string to a canonical URL string
 * suitable for loading (file://, data:image/, or http(s)://).
 */
static QString resolveMarkdownImageSource(const QString& rawSource, const QString& noteDirectoryPath) {
    QString source = rawSource.trimmed();
    if (source.isEmpty()) {
        return QString();
    }

    // Handle angle-bracket wrapped paths: <path with spaces.png>
    if (source.startsWith(QLatin1Char('<')) && source.endsWith(QLatin1Char('>')) && source.size() > 2) {
        source = source.mid(1, source.size() - 2).trimmed();
    }

    // Data URIs are returned as-is
    if (source.startsWith(QLatin1String("data:image/"), Qt::CaseInsensitive)) {
        return source;
    }

    // Strip optional Markdown image title: path "title" or path 'title'
    if (!source.startsWith(QLatin1Char('<'))) {
        int splitPos = -1;
        bool inSingleQuote = false;
        bool inDoubleQuote = false;
        for (int i = 0; i < source.size(); ++i) {
            const QChar ch = source.at(i);
            if (ch == QLatin1Char('"') && !inSingleQuote) {
                inDoubleQuote = !inDoubleQuote;
                continue;
            }
            if (ch == QLatin1Char('\'') && !inDoubleQuote) {
                inSingleQuote = !inSingleQuote;
                continue;
            }
            if (!inSingleQuote && !inDoubleQuote && ch.isSpace()) {
                splitPos = i;
                break;
            }
        }
        if (splitPos > 0) {
            source = source.left(splitPos).trimmed();
        }
    }

    if (source.isEmpty()) {
        return QString();
    }

    const QUrl sourceUrl(source);
    if (sourceUrl.isLocalFile()) {
        return QLatin1String("file://") + sourceUrl.toLocalFile();
    }

    if (!sourceUrl.scheme().isEmpty()) {
        return source;
    }

    if (QFileInfo(source).isAbsolute()) {
        return QLatin1String("file://") + source;
    }

    return QLatin1String("file://") + QDir(noteDirectoryPath).absoluteFilePath(source);
}

/**
 * Loads a pixmap from a resolved image source string and caches the result.
 * Failed loads are also cached to avoid repeated expensive lookups.
 */
static QPixmap loadMarkdownImagePixmap(const QString& resolvedSource) {
    static QHash<QString, QPixmap> imageCache;
    static QSet<QString> failedImageCache;
    if (resolvedSource.isEmpty()) {
        return QPixmap();
    }

    if (imageCache.contains(resolvedSource)) {
        return imageCache.value(resolvedSource);
    }

    if (failedImageCache.contains(resolvedSource)) {
        return QPixmap();
    }

    QImage image;

    if (resolvedSource.startsWith(QLatin1String("file://"))) {
        const QString localPath = resolvedSource.mid(7);
        QFileInfo info(localPath);
        if (!info.exists() || !info.isFile()) {
            failedImageCache.insert(resolvedSource);
            return QPixmap();
        }

        QImageReader reader(localPath);
        reader.setAutoTransform(true);
        image = reader.read();
    } else if (resolvedSource.startsWith(QLatin1String("data:image/"), Qt::CaseInsensitive)) {
        const int markerPos = resolvedSource.indexOf(QStringLiteral(";base64,"));
        if (markerPos < 0) {
            failedImageCache.insert(resolvedSource);
            return QPixmap();
        }

        const QString base64 = resolvedSource.mid(markerPos + 8);
        image = QImage::fromData(QByteArray::fromBase64(base64.toLatin1()));
    } else {
        // Remote URL: do not block the paint event with a synchronous download.
        // Start an async fetch; the result will be stored in the cache and the
        // viewport will be refreshed once the download completes.
        static QSet<QString> pendingDownloadCache;
        if (!pendingDownloadCache.contains(resolvedSource)) {
            pendingDownloadCache.insert(resolvedSource);

            // Use a shared network manager to avoid spawning one per request.
            static QNetworkAccessManager* netManager = nullptr;
            if (!netManager) {
                netManager = new QNetworkAccessManager();
            }

            QNetworkRequest request((QUrl(resolvedSource)));
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
            // RedirectPolicyAttribute was introduced in Qt 5.9
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#endif
            request.setHeader(QNetworkRequest::UserAgentHeader, Utils::Misc::friendlyUserAgentString());

            QNetworkReply* reply = netManager->get(request);
            QObject::connect(reply, &QNetworkReply::finished, reply, [reply, resolvedSource]() {
                reply->deleteLater();
                pendingDownloadCache.remove(resolvedSource);

                if (reply->error() != QNetworkReply::NoError) {
                    failedImageCache.insert(resolvedSource);
                    return;
                }

                const QByteArray data = reply->readAll();
                const QImage img = QImage::fromData(data);
                if (img.isNull()) {
                    failedImageCache.insert(resolvedSource);
                    return;
                }

                const QPixmap pix = QPixmap::fromImage(img);
                if (pix.isNull()) {
                    failedImageCache.insert(resolvedSource);
                    return;
                }

                imageCache.insert(resolvedSource, pix);

                // Trigger a repaint on all markdown text edit widgets so the
                // newly downloaded image is shown without the user having to
                // interact with the editor.
                const auto widgets = QApplication::allWidgets();
                for (QWidget* w : widgets) {
                    if (auto* edit = qobject_cast<ArcNotesMarkdownTextEdit*>(w)) {
                        if (edit->viewport()) {
                            edit->viewport()->update();
                        }
                    }
                }
            });
        }
        // Return null for now; the next paint will pick up the cached pixmap.
        return QPixmap();
    }

    if (image.isNull()) {
        failedImageCache.insert(resolvedSource);
        return QPixmap();
    }

    QPixmap pixmap = QPixmap::fromImage(image);
    if (pixmap.isNull()) {
        failedImageCache.insert(resolvedSource);
        return QPixmap();
    }

    imageCache.insert(resolvedSource, pixmap);
    return pixmap;
}

void ArcNotesMarkdownTextEdit::paintMarkdownImagePreviews() {
    if (objectName() == QStringLiteral("logTextEdit")) {
        return;
    }

    QString noteDirectoryPath = editorCurrentNoteFolderPath(this);
    if (auto* host = editorHost(this)) {
        const QString currentNotePath = host->editorCurrentNote().fullNoteFilePath;
        if (!currentNotePath.isEmpty()) {
            noteDirectoryPath = QFileInfo(currentNotePath).absolutePath();
        }
    }

    static const QRegularExpression imageRegex(QStringLiteral(R"(!\[[^\]]*\]\(([^\n\)]*)\)(?:\s*\{[^}]*\})?)"));

    struct PreviewDrawItem {
        QRect targetRect;
        QPixmap pixmap;
    };
    QVector<PreviewDrawItem> drawItems;

    QTextBlock block = firstVisibleBlock();
    const qreal contentX = contentOffset().x();
    const QRect viewportRect = viewport()->rect();

    // Thumbnail size: as tall as one text line, square
    const int thumbSize = fontMetrics().height();
    const int thumbSpacing = 4;

    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid()) {
        if (!block.isVisible()) {
            block = block.next();
            continue;
        }

        if (bottom < 0) {
            block = block.next();
            top = bottom;
            bottom = top + qRound(blockBoundingRect(block).height());
            continue;
        }

        if (top > viewportRect.bottom()) {
            break;
        }

        const QString text = block.text();
        QTextLayout* layout = block.layout();
        if (!layout) {
            block = block.next();
            top = bottom;
            bottom = top + qRound(blockBoundingRect(block).height());
            continue;
        }

        QRegularExpressionMatchIterator iterator = imageRegex.globalMatch(text);

        while (iterator.hasNext()) {
            const QRegularExpressionMatch match = iterator.next();
            const int tagEnd = match.capturedEnd(0);
            const QTextLine line = layout->lineForTextPosition(tagEnd - 1);
            if (!line.isValid()) {
                continue;
            }

            // Only show inline image previews when no visible text follows the
            // image tag on the same layout line to avoid cursor/caret mismatch.
            const int lineTextEnd = line.textStart() + line.textLength();
            if (tagEnd < lineTextEnd) {
                const QString trailingText = text.mid(tagEnd, lineTextEnd - tagEnd).trimmed();
                if (!trailingText.isEmpty()) {
                    continue;
                }
            }

            const QString resolvedSource = resolveMarkdownImageSource(match.captured(1), noteDirectoryPath);
            const QPixmap pixmap = loadMarkdownImagePixmap(resolvedSource);
            if (pixmap.isNull()) {
                continue;
            }

            // Find the line that contains the end of the image tag and compute
            // the pixel X position right after the closing parenthesis
            // cursorToX gives us the X coordinate of the character at tagEnd
            // within the line's local coordinate system
            const qreal cursorX = line.cursorToX(tagEnd);
            QRectF lineRect = line.rect();
            lineRect.translate(contentX, top);

            const int x = qRound(lineRect.left() + cursorX) + thumbSpacing;
            const int y = qRound(lineRect.top() + (lineRect.height() - thumbSize) / 2.0);
            const QRect targetRect(x, y, thumbSize, thumbSize);

            if (!viewportRect.intersects(targetRect)) {
                continue;
            }

            drawItems.append({targetRect, pixmap});
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    for (const auto& item : drawItems) {
        // Draw a subtle rounded border around the thumbnail
        painter.setPen(QPen(QColor(120, 120, 120, 160)));
        painter.setBrush(QColor(255, 255, 255, 140));
        painter.drawRoundedRect(item.targetRect.adjusted(-1, -1, 1, 1), 2, 2);

        // Scale pixmap to fit the square thumbnail, keeping aspect ratio
        const QPixmap scaledPixmap =
            item.pixmap.scaled(thumbSize, thumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QRect pixmapRect(QPoint(0, 0), scaledPixmap.size());
        pixmapRect.moveCenter(item.targetRect.center());
        painter.drawPixmap(pixmapRect.topLeft(), scaledPixmap);
    }
}

bool ArcNotesMarkdownTextEdit::canInsertFromMimeData(const QMimeData* source) const {
    return (!source->hasUrls());
}

void ArcNotesMarkdownTextEdit::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return;
    }

    QMarkdownTextEdit::dragEnterEvent(event);
}

void ArcNotesMarkdownTextEdit::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return;
    }

    QMarkdownTextEdit::dragMoveEvent(event);
}

void ArcNotesMarkdownTextEdit::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        if (auto* host = editorHost(this)) {
            event->acceptProposedAction();
            host->editorHandleMimeData(event->mimeData());
            return;
        }
    }

    QMarkdownTextEdit::dropEvent(event);
}

/**
 * Handles pasting from clipboard
 */
void ArcNotesMarkdownTextEdit::insertFromMimeData(const QMimeData* source) {
    // if there is text in the clipboard do the normal pasting process
    if (source->hasText()) {
        QMarkdownTextEdit::insertFromMimeData(source);
    } else if (auto* host = editorHost(this)) {
        // to more complex pasting if there was no text (and a main window
        // was set)
        host->editorHandleMimeData(source);
    }
}

/**
 * Handles the settings of the Markdown textedit
 */
void ArcNotesMarkdownTextEdit::updateSettings() {
    // we need a blocker, otherwise the "change" events will fire
    const QSignalBlocker blocker(this);
    Q_UNUSED(blocker)

    QMarkdownTextEdit::AutoTextOptions options;

    if (editorSettingValue(this, QStringLiteral("Editor/autoBracketClosing"), true).toBool()) {
        options |= QMarkdownTextEdit::AutoTextOption::BracketClosing;
    }

    if (editorSettingValue(this, QStringLiteral("Editor/autoBracketRemoval"), true).toBool()) {
        options |= QMarkdownTextEdit::AutoTextOption::BracketRemoval;
    }

    setAutoTextOptions(options);

    // highlighting is always disabled for logTextEdit
    if (objectName() != QStringLiteral("logTextEdit")) {
        // enable or disable Markdown highlighting
        bool highlightingEnabled =
            editorSettingValue(this, QStringLiteral("markdownHighlightingEnabled"), true).toBool();

        setHighlightingEnabled(highlightingEnabled);

        if (highlightingEnabled) {
            // set the new highlighting styles
            setStyles();
            if (_highlighter) {
                auto* qownNotesHighlighter = qobject_cast<ArcNotesMarkdownHighlighter*>(_highlighter);
                if (qownNotesHighlighter != nullptr) {
                    qownNotesHighlighter->reloadNoteLinkSettings();
                }
                _highlighter->rehighlight();
            }
        }
    }

    const bool hlCurrLine = editorSettingValue(this, QStringLiteral("Editor/highlightCurrentLine"), true).toBool();
    setHighlightCurrentLine(hlCurrLine);

    // Hide formatting syntax on non-cursor blocks (Typora-like)
    const bool hideFormattingSyntax =
        editorSettingValue(this, QStringLiteral("Editor/hideFormattingSyntax"), false).toBool();
    if (_highlighter) {
        _highlighter->setHideFormattingSyntax(hideFormattingSyntax);
        _highlighter->setCurrentCursorBlockNumber(textCursor().blockNumber());
    }

    const bool hangingIndentEnabled = editorSettingValue(this, QStringLiteral("Editor/hangingIndent"), false).toBool();
    setHangingIndentEnabled(hangingIndentEnabled);
    _headingFoldingEnabled = editorSettingValue(this, QStringLiteral("Editor/headingFolding"), false).toBool();
    if (!_headingFoldingEnabled) {
        setCurrentNoteReference(QString());
        unfoldAllHeadings();
    }
    _showMarkdownImagePreviews =
        editorSettingValue(this, QStringLiteral("Editor/showMarkdownImagePreviews"), true).toBool();
    viewport()->update();
    refreshFoldingSidebar();
    const auto color = Utils::Schema::schemaSettings->getBackgroundColor(
        MarkdownHighlighter::HighlighterState::CurrentLineBackgroundColor);
    setCurrentLineHighlightColor(color);

    _centerCursor = editorSettingValue(this, QStringLiteral("Editor/centerCursor")).toBool();
    QMarkdownTextEdit::updateSettings();
}

void ArcNotesMarkdownTextEdit::onContextMenu(QPoint pos) {
    const QTextCursor cursorAtMouse = cursorForPosition(pos);
    Q_UNUSED(cursorAtMouse)

    const QPoint globalPos = this->viewport()->mapToGlobal(pos);
    QMenu* menu = this->createStandardContextMenu();

    const bool isAllowNoteEditing = Utils::Misc::isNoteEditingAllowed();
    const bool isTextSelected = textCursor().hasSelection();

    const QString linkTextActionName = isTextSelected ? tr("&Link selected text") : tr("Insert &link");
    QAction* linkTextAction = menu->addAction(linkTextActionName);
    connect(linkTextAction, &QAction::triggered, this, [this]() {
        if (QAction* action = editorHostAction(this, QStringLiteral("actionInsert_text_link"))) {
            action->trigger();
        }
    });
    linkTextAction->setEnabled(isAllowNoteEditing);

    QString blockQuoteTextActionName = isTextSelected ? tr("Block &quote selected text",
                                                           "Action to apply a block quote formatting to the "
                                                           "selected text")
                                                      : tr("Insert block &quote");
    QAction* blockQuoteTextAction = menu->addAction(blockQuoteTextActionName);
    connect(blockQuoteTextAction, &QAction::triggered, this, &ArcNotesMarkdownTextEdit::insertBlockQuote);
    blockQuoteTextAction->setEnabled(isAllowNoteEditing);

    if (isTextSelected) {
        QMenu* listOperationsMenu = menu->addMenu(tr("List operations"));
        listOperationsMenu->setEnabled(isAllowNoteEditing);

        addEditorHostAction(listOperationsMenu, this, QStringLiteral("actionToggle_checkboxes"));
        addEditorHostAction(listOperationsMenu, this, QStringLiteral("actionCreate_ordered_list"));
        addEditorHostAction(listOperationsMenu, this, QStringLiteral("actionCreate_alphabetical_list"));
        addEditorHostAction(listOperationsMenu, this, QStringLiteral("actionCreate_unordered_list"));
        addEditorHostAction(listOperationsMenu, this, QStringLiteral("actionCreate_checkbox_list"));
        addEditorHostAction(listOperationsMenu, this, QStringLiteral("actionClear_list_formatting"));
        addEditorHostAction(listOperationsMenu, this, QStringLiteral("actionOrder_checkboxes"));

        QMenu* markdownOperationsMenu = menu->addMenu(tr("Markdown operations"));
        markdownOperationsMenu->setEnabled(isAllowNoteEditing);

        addEditorHostAction(markdownOperationsMenu, this, QStringLiteral("actionIncrease_heading_depth"));
        addEditorHostAction(markdownOperationsMenu, this, QStringLiteral("actionDecrease_heading_depth"));

        addEditorHostAction(menu, this, QStringLiteral("actionSearch_text_on_the_web"));
        addEditorHostAction(menu, this, QStringLiteral("action_Find_note"));
    }

    //     searchAction->setEnabled(isTextSelected);
    //     QAction *searchAction =
    //         menu->addAction(searchTextOnWebAction->text());
    //     searchAction->setShortcut(searchTextOnWebAction->shortcut());

    QAction* copyCodeBlockAction = menu->addAction(tr("Copy code block"));
    copyCodeBlockAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy"),
                                                  QIcon(QStringLiteral(":icons/breeze-arcnotes/16x16/edit-copy.svg"))));
    const QTextBlock currentTextBlock = cursorForPosition(pos).block();
    const int userState = currentTextBlock.userState();
    const bool isCodeSpan =
        highlighter()->isPosInACodeSpan(currentTextBlock.blockNumber(), cursorForPosition(pos).positionInBlock());
    copyCodeBlockAction->setEnabled(MarkdownHighlighter::isCodeBlock(userState) || isCodeSpan);
    connect(copyCodeBlockAction, &QAction::triggered, this, [this, isCodeSpan, currentTextBlock, pos]() {
        // copy the text from a copy block around currentTextBlock to the
        // clipboard
        if (isCodeSpan) {
            const auto codeSpanRange =
                highlighter()->getSpanRange(MarkdownHighlighter::RangeType::CodeSpan, currentTextBlock.blockNumber(),
                                            cursorForPosition(pos).positionInBlock());
            QApplication::clipboard()->setText(
                currentTextBlock.text().mid(codeSpanRange.first + 1, codeSpanRange.second - codeSpanRange.first - 1));
        } else {
            Utils::Gui::copyCodeBlockText(currentTextBlock);
        }
    });

    menu->addSeparator();

    // add table column insertion actions if cursor is in a table
    if (Utils::Gui::isTableAtCursor(this)) {
        QAction* addColumnLeftAction = menu->addAction(tr("Add table column left"));
        addColumnLeftAction->setEnabled(isAllowNoteEditing);
        connect(addColumnLeftAction, &QAction::triggered, this, [this]() { Utils::Gui::insertTableColumnLeft(this); });

        QAction* addColumnRightAction = menu->addAction(tr("Add table column right"));
        addColumnRightAction->setEnabled(isAllowNoteEditing);
        connect(addColumnRightAction, &QAction::triggered, this,
                [this]() { Utils::Gui::insertTableColumnRight(this); });

        QAction* addRowAboveAction = menu->addAction(tr("Add table row above"));
        addRowAboveAction->setEnabled(isAllowNoteEditing);
        connect(addRowAboveAction, &QAction::triggered, this, [this]() { Utils::Gui::insertTableRowAbove(this); });

        QAction* addRowBelowAction = menu->addAction(tr("Add table row below"));
        addRowBelowAction->setEnabled(isAllowNoteEditing);
        connect(addRowBelowAction, &QAction::triggered, this, [this]() { Utils::Gui::insertTableRowBelow(this); });

        QAction* editTableAction = menu->addAction(tr("Edit table"));
        editTableAction->setEnabled(isAllowNoteEditing);
        connect(editTableAction, &QAction::triggered, this, [this]() {
            auto* dialog = new MarkdownTableDialog(this, this);
            dialog->exec();
            delete dialog;
        });

        menu->addSeparator();
    }

    // add the print menu
    QMenu* printMenu = menu->addMenu(tr("Print"));
    QIcon printIcon = QIcon::fromTheme(QStringLiteral("document-print"),
                                       QIcon(QStringLiteral(":icons/breeze-arcnotes/16x16/document-print.svg")));
    printMenu->setIcon(printIcon);

    // add the print selected text action
    QAction* printTextAction = printMenu->addAction(tr("Print selected text"));
    printTextAction->setEnabled(isTextSelected);
    printTextAction->setIcon(printIcon);
    connect(printTextAction, &QAction::triggered, this, [this]() {
        // print the selected text
        auto* host = editorHost(this);
        if (host == nullptr) {
            return;
        }
        auto* textEdit = new ArcNotesMarkdownTextEdit(this);
        textEdit->setPlainText(host->editorSelectedText());
        host->editorPrintDocument(textEdit->document(), true);
    });

    // add the print selected text (preview) action
    QAction* printHTMLAction = printMenu->addAction(tr("Print selected text (preview)"));
    printHTMLAction->setEnabled(isTextSelected);
    printHTMLAction->setIcon(printIcon);
    connect(printHTMLAction, &QAction::triggered, this, [this]() {
        // print the selected text (preview)
        auto* host = editorHost(this);
        if (host == nullptr) {
            return;
        }
        const QString html =
            host->editorRenderTextToHtml(host->editorSelectedText(), Utils::Misc::useInternalExportStylingForPreview());
        auto* textEdit = new QTextEdit(this);
        textEdit->setHtml(html);
        host->editorPrintDocument(textEdit->document(), false);
    });

    // add the export menu
    QMenu* exportMenu = menu->addMenu(tr("Export"));
    exportMenu->setIcon(QIcon::fromTheme(QStringLiteral("document-export"),
                                         QIcon(QStringLiteral(":icons/breeze-arcnotes/16x16/document-export.svg"))));

    QIcon pdfIcon = QIcon::fromTheme(QStringLiteral("application-pdf"),
                                     QIcon(QStringLiteral(":icons/breeze-arcnotes/16x16/application-pdf.svg")));

    // add the export selected text action
    QAction* exportTextAction = exportMenu->addAction(tr("Export selected text as PDF"));
    exportTextAction->setEnabled(isTextSelected);
    exportTextAction->setIcon(pdfIcon);
    connect(exportTextAction, &QAction::triggered, this, [this]() {
        // export the selected text as PDF
        auto* host = editorHost(this);
        if (host == nullptr) {
            return;
        }
        auto* textEdit = new ArcNotesMarkdownTextEdit(this);
        textEdit->setPlainText(host->editorSelectedText());
        host->editorExportNoteAsPDF(textEdit->document(), true);
    });

    // add the export selected text (preview) action
    QAction* exportHTMLAction = exportMenu->addAction(tr("Export selected text as PDF (preview)"));
    exportHTMLAction->setEnabled(isTextSelected);
    exportHTMLAction->setIcon(pdfIcon);
    connect(exportHTMLAction, &QAction::triggered, this, [this]() {
        // export the selected text (preview) as PDF
        auto* host = editorHost(this);
        if (host == nullptr) {
            return;
        }
        QString html =
            host->editorRenderTextToHtml(host->editorSelectedText(), Utils::Misc::useInternalExportStylingForPreview());
        html = Utils::Misc::parseTaskList(html, false);
        QTextDocument doc;
        doc.setHtml(html);
        host->editorExportNoteAsPDF(&doc, false);
    });

    menu->addSeparator();

    // add some other existing menu entries
    QMenu* selectMenu = menu->addMenu(tr("Select"));
    addEditorHostAction(selectMenu, this, QStringLiteral("actionSelect_enclosed_text"));
    addEditorHostAction(menu, this, QStringLiteral("actionPaste_image"));
    addEditorHostAction(menu, this, QStringLiteral("actionAutocomplete"));
    addEditorHostAction(menu, this, QStringLiteral("actionSplit_note_at_cursor_position"));

    menu->exec(globalPos);
}

bool ArcNotesMarkdownTextEdit::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);

        if (objectName() == QStringLiteral("noteTextEdit")) {
            // deactivating the search widget has priority
            if ((keyEvent->key() == Qt::Key_Escape) && _searchWidget->isVisible()) {
                _searchWidget->deactivate();
                return true;
            } else if ((keyEvent->key() == Qt::Key_R) && keyEvent->modifiers().testFlag(Qt::ControlModifier) &&
                       !Utils::Misc::isNoteEditingAllowed()) {
                if (auto* host = editorHost(this)) {
                    host->editorAllowNoteEditing();
                }
            } else if (!Utils::Misc::isNoteEditingAllowed()) {
                const auto noModifierKeys = QList<int>()
                                            << Qt::Key_Return << Qt::Key_Enter << Qt::Key_Space << Qt::Key_Backspace
                                            << Qt::Key_Delete << Qt::Key_Tab << Qt::Key_Backtab << Qt::Key_Minus
                                            << Qt::Key_ParenLeft << Qt::Key_BraceLeft << Qt::Key_BracketLeft
                                            << Qt::Key_Plus << Qt::Key_Comma << Qt::Key_Period;

                if ((keyEvent->key() == Qt::Key_Space) && keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
                    if (openLinkAtCursorPosition()) {
                        showEditorStatusMessage(this, tr("An url was opened at the current cursor position"),
                                                QStringLiteral("📃"), 5000);
                    }

                    return true;
                }

                const auto controlModifierKeys = QList<int>() << Qt::Key_V;

                // show notification if user tries to edit a note while
                // note editing is turned off
                if (((keyEvent->key() < 128 || noModifierKeys.contains(keyEvent->key())) &&
                     keyEvent->modifiers().testFlag(Qt::NoModifier)) ||
                    (controlModifierKeys.contains(keyEvent->key()) &&
                     keyEvent->modifiers().testFlag(Qt::ControlModifier) && isReadOnly())) {
                    auto* host = editorHost(this);
                    if (host != nullptr && host->editorDoNoteEditingCheck()) {
                        // If the answer is overridden to Yes ("Don't ask again" with "Yes"),
                        // what you type then only enables note editing, but is not typed in
                        // the editor. We need to re-send the event after enabling editing.
                        // BUT, we should do that only if the msgbox is overridden to Yes,
                        // not if manually answered.
                        // You may see: https://github.com/pbek/ArcNotes/issues/2421
                        // This check is partially copied from utils/gui.cpp showMessage()
                        const QString settingsKey = QStringLiteral("MessageBoxOverride/readonly-mode-allow");
                        auto overrideButton = static_cast<QMessageBox::StandardButton>(
                            editorSettingValue(this, settingsKey, QMessageBox::NoButton).toInt());
                        if (overrideButton == QMessageBox::Yes) {
                            // overridden to answer yes: re-send the event
                            return QMarkdownTextEdit::eventFilter(obj, event);
                        }
                    }

                    return true;
                }
            } else {
                // Disable note editing if Escape key was pressed and
                // read-only mode feature is enabled
                if (keyEvent->key() == Qt::Key_Escape && Utils::Misc::isReadOnlyModeEnabled()) {
                    if (auto* host = editorHost(this)) {
                        host->editorDisallowNoteEditing();
                    }

                    return true;
                } else if ((keyEvent->key() == Qt::Key_Tab) || (keyEvent->key() == Qt::Key_Backtab)) {
                    // handle entered tab and reverse tab keys
                    return handleTabEntered(keyEvent->key() == Qt::Key_Backtab, Utils::Misc::indentCharacters());
                }
            }
        }
    }

    return QMarkdownTextEdit::eventFilter(obj, event);
}

void ArcNotesMarkdownTextEdit::updateIgnoredClickUrlRegexps() {
    setIgnoredClickUrlRegexps({});
}

void ArcNotesMarkdownTextEdit::keyPressEvent(QKeyEvent* e) {
    if (!isReadOnly()) {
        const QChar deadKeyAccent = accentForDeadKey(e->key());
        if (!deadKeyAccent.isNull() && e->text().isEmpty()) {
            _pendingDeadKey = deadKeyAccent;
            e->accept();
            return;
        }

        if (!_pendingDeadKey.isNull()) {
            if (e->key() == Qt::Key_Escape) {
                _pendingDeadKey = QChar();
            } else if (!(e->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) &&
                       e->text().size() == 1) {
                const QChar accent = _pendingDeadKey;
                _pendingDeadKey = QChar();

                const QChar character = e->text().at(0);
                const QString composed = composeDeadKey(accent, character);
                QTextCursor cursor = textCursor();
                cursor.insertText(composed.size() == 1 ? composed : QString(spacingAccentForCombiningMark(accent)));
                if (composed.size() != 1 && character != QLatin1Char(' ')) {
                    cursor.insertText(e->text());
                }
                setTextCursor(cursor);
                e->accept();
                return;
            }
        }
    } else {
        _pendingDeadKey = QChar();
    }

    // Call parent implementation
    QMarkdownTextEdit::keyPressEvent(e);

    if (!e->isAccepted() || !editorWikiLinkSupportEnabled(this)) {
        return;
    }

    if (e->text() == QStringLiteral("[") && (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ShiftModifier)) {
        // Only trigger automatic note filename selection if the setting is enabled
        const bool autoSelect =
            editorSettingValue(this, QStringLiteral("Editor/wikiLinkFileNameAutoSelect"), false).toBool();
        if (!autoSelect) {
            return;
        }

        WikiLinkCompletionContext context;
        if (currentWikiLinkCompletionContext(this, context)) {
            QTimer::singleShot(0, this, &ArcNotesMarkdownTextEdit::onAutoCompleteRequested);
        }
    }
}

void ArcNotesMarkdownTextEdit::focusInEvent(QFocusEvent* e) {
    // Call parent implementation
    QMarkdownTextEdit::focusInEvent(e);
}

/**
 * Override inputMethodQuery to fix IME candidate window position on Windows.
 * When viewport margins are set (e.g. for paper margins), the cursor rectangle
 * returned to the IME must be offset by the top/left margin so that the
 * candidate window appears adjacent to the cursor rather than overlapping it.
 */
QVariant ArcNotesMarkdownTextEdit::inputMethodQuery(Qt::InputMethodQuery property) const {
    QVariant result = QPlainTextEdit::inputMethodQuery(property);

    const bool isCursorQuery = (property == Qt::ImCursorRectangle || property == Qt::ImAnchorRectangle);
    if (isCursorQuery) {
        // viewportMargins() is non-const in this class, so cast away const to call it
        const QMargins vm = const_cast<ArcNotesMarkdownTextEdit*>(this)->viewportMargins();
        if (vm.top() != 0 || vm.left() != 0) {
            if (result.userType() == QMetaType::QRectF) {
                result = result.toRectF().translated(qreal(vm.left()), qreal(vm.top()));
            }
        }
    }

    return result;
}

ArcNotesMarkdownTextEdit::~ArcNotesMarkdownTextEdit() {
    qDebug() << "*** ArcNotesMarkdownTextEdit DESTROYED ***" << this << objectName();
}
