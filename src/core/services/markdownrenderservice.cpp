#include "markdownrenderservice.h"

#include <core/repositories/noterepository.h>
#include <core/repositories/settingsrepository.h>
#include <core/services/notelinkservice.h>
#include <helpers/codetohtmlconverter.h>
#include <libraries/md4c/src/md4c-html.h>
#include <libraries/md4c/src/md4c.h>
#include <libraries/qmarkdowntextedit/markdownhighlighter.h>
#include <utils/misc.h>
#include <utils/schema.h>

#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QImage>
#include <QMimeDatabase>
#include <QMimeType>
#include <QRegularExpression>
#include <QUrl>
#include <QVariant>
#include <algorithm>
#include <vector>

namespace {
QString postProcessInternalNoteLinksHtml(QString html) {
    const QStringList extensions = NoteRepository().noteFileExtensionList();
    const QString extAlternation = extensions.join(QChar('|'));
    const QRegularExpression fileNoteRE(
        QStringLiteral(R"(<a (href="file://[^"]+\.(?:%1)(?:#[^"]*)?"))").arg(extAlternation),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression noteLinkRE(QStringLiteral(R"(<a (href="note://[^"]*"))"));
    html.replace(fileNoteRE, QStringLiteral(R"(<a class="notelink" \1)"));
    html.replace(noteLinkRE, QStringLiteral(R"(<a class="notelink" \1)"));
    return html;
}

QString postProcessWikiLinksHtml(QString html, int currentNoteSubFolderId) {
    static const QRegularExpression regex(
        QStringLiteral(R"wiki(<x-wikilink data-target="([^"]*?)">(.*?)</x-wikilink>)wiki"),
        QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator iterator = regex.globalMatch(html);
    QVector<QRegularExpressionMatch> matches;
    while (iterator.hasNext()) {
        matches.append(iterator.next());
    }

    for (int i = matches.count() - 1; i >= 0; --i) {
        const QRegularExpressionMatch& match = matches.at(i);
        const QString target = QUrl::fromPercentEncoding(match.captured(1).toUtf8());
        const QString linkText = Utils::Misc::unescapeHtml(match.captured(2), true);
        const NoteData resolvedNote = NoteLinkService().resolveWikiLink(target, currentNoteSubFolderId);
        const QString encodedTarget = QString::fromUtf8(QUrl::toPercentEncoding(target, QByteArrayLiteral("/-#")));
        const QString replacement =
            resolvedNote.id > 0
                ? QStringLiteral("<a class=\"wikilink\" href=\"wikilink:%1\">%2</a>").arg(encodedTarget, linkText)
                : QStringLiteral("<a class=\"wikilink broken\" href=\"wikilink:%1\">%2</a>")
                      .arg(encodedTarget, linkText);
        html.replace(match.capturedStart(0), match.capturedLength(0), replacement);
    }
    return html;
}

Utils::Misc::ExternalImageHash* externalImageHash() {
    auto* instance = qApp->property("externalImageHash").value<Utils::Misc::ExternalImageHash*>();
    if (instance == nullptr) {
        instance = new Utils::Misc::ExternalImageHash;
        qApp->setProperty("externalImageHash", QVariant::fromValue<Utils::Misc::ExternalImageHash*>(instance));
    }
    return instance;
}
void captureHtmlFragment(const MD_CHAR* data, MD_SIZE data_size, void* userData) {
    auto* array = static_cast<QByteArray*>(userData);

    if (data_size > 0) {
        array->append(data, int(data_size));
    }
}

/**
 * @brief Masks code blocks and inline code in the markdown text by replacing
 * their content with placeholders. This prevents link transformations from
 * modifying content inside code blocks (e.g., `<stdio.h>` being converted to a link).
 * @return A list of the original code block contents for later restoration.
 */
QStringList maskCodeBlocks(QString& str) {
    QStringList maskedBlocks;

    // Mask fenced code blocks (``` and ~~~)
    for (const QString& fence : {QStringLiteral("```"), QStringLiteral("~~~")}) {
        int pos = 0;
        while (true) {
            // Find opening fence
            int start = str.indexOf(fence, pos);
            if (start == -1) break;

            // Find end of opening fence line
            int endOfOpeningLine = str.indexOf(QChar('\n'), start);
            if (endOfOpeningLine == -1) break;

            // Check for inline code (closing fence on the same line as opening)
            const QString lang = str.mid(start + 3, endOfOpeningLine - (start + 3));
            if (lang.contains(fence)) {
                pos = endOfOpeningLine + 1;
                continue;
            }

            // Find closing fence
            int end = str.indexOf(fence, endOfOpeningLine + 1);
            if (end == -1) break;

            // Replace everything from opening fence to end of closing fence with placeholder
            int blockLen = end + fence.length() - start;
            const QString original = str.mid(start, blockLen);
            const QString placeholder = QStringLiteral(
                                            "\x01"
                                            "CODEBLOCK_%1\x01")
                                            .arg(maskedBlocks.size());
            maskedBlocks.append(original);
            str.replace(start, blockLen, placeholder);
            pos = start + placeholder.length();
        }
    }

    // Mask 4-space (or tab) indented code blocks
    // In Markdown, lines indented by 4+ spaces or a tab are code blocks.
    // We treat consecutive indented lines (including blank lines between them)
    // as a single code block.
    {
        static const QRegularExpression indentedCodeRE(QStringLiteral("(^|\\n)((?:(?:[ ]{4}|\\t).+(?:\\n|$))+)"),
                                                       QRegularExpression::MultilineOption);
        QRegularExpressionMatchIterator it = indentedCodeRE.globalMatch(str);

        // Collect matches in reverse order to replace from end to start
        QList<QRegularExpressionMatch> matches;
        while (it.hasNext()) {
            matches.append(it.next());
        }

        for (int i = matches.size() - 1; i >= 0; --i) {
            const QRegularExpressionMatch& match = matches[i];
            // Only replace the indented block part (group 2), not the
            // leading newline/start anchor (group 1)
            const QString original = match.captured(2);

            // Skip if this block already contains a masked placeholder from
            // Phase 1 (fenced code blocks). This happens when a fenced code
            // block has tab/space-indented fences — the placeholder line
            // itself starts with a tab, matching the indented-code regex.
            // See: https://github.com/pbek/ArcNotes/issues/2671
            if (original.contains(QLatin1String("\x01"
                                                "CODEBLOCK_"))) {
                continue;
            }

            const QString placeholder = QStringLiteral(
                                            "\x01"
                                            "CODEBLOCK_%1\x01")
                                            .arg(maskedBlocks.size());
            maskedBlocks.append(original);
            str.replace(match.capturedStart(2), match.capturedLength(2), placeholder);
        }
    }

    // Mask inline code (single/double backticks, but not triple)
    // Match `...` or ``...`` but not ```
    static const QRegularExpression inlineCodeRE(QStringLiteral("(?<!`)(`{1,2})(?!`)(.+?)(?<!`)\\1(?!`)"),
                                                 QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = inlineCodeRE.globalMatch(str);

    // Collect matches in reverse order to replace from end to start
    QList<QRegularExpressionMatch> matches;
    while (it.hasNext()) {
        matches.append(it.next());
    }

    for (int i = matches.size() - 1; i >= 0; --i) {
        const QRegularExpressionMatch& match = matches[i];
        const QString original = match.captured(0);
        const QString placeholder = QStringLiteral(
                                        "\x01"
                                        "CODEBLOCK_%1\x01")
                                        .arg(maskedBlocks.size());
        maskedBlocks.append(original);
        str.replace(match.capturedStart(), match.capturedLength(), placeholder);
    }

    return maskedBlocks;
}

/**
 * @brief Restores previously masked code blocks by replacing placeholders
 * with the original content.
 */
void unmaskCodeBlocks(QString& str, const QStringList& maskedBlocks) {
    // Unmask in reverse order because later phases (e.g. indented code block
    // detection) may re-mask lines that already contain a placeholder from an
    // earlier phase, creating nested placeholders. Replacing the outermost
    // (highest index) placeholders first ensures the inner ones are visible
    // in the string when their turn comes.
    // See: https://github.com/pbek/ArcNotes/issues/2671
    for (int i = maskedBlocks.size() - 1; i >= 0; --i) {
        const QString placeholder = QStringLiteral(
                                        "\x01"
                                        "CODEBLOCK_%1\x01")
                                        .arg(i);
        str.replace(placeholder, maskedBlocks[i]);
    }
}

/**
 * @brief Converts code blocks to highlighted code
 */
void highlightCode(QString& str, const QString& type, int cbCount, const QString& otherFenceType = QString()) {
    if (cbCount >= 1) {
        const int firstBlock = str.indexOf(type, 0);
        int currentCbPos = firstBlock;
        for (int i = 0; i < cbCount; ++i) {
            // find endline
            const int endline = str.indexOf(QChar('\n'), currentCbPos);
            // something invalid? => just skip it
            if (endline == -1) {
                break;
            }

            if (currentCbPos >= 4) {
                bool fourSpaces = std::all_of(str.cbegin() + (currentCbPos - 4), str.cbegin() + currentCbPos,
                                              [](QChar c) { return c == QChar(' '); });
                if (fourSpaces) {
                    continue;
                }
            }

            const QString lang = str.mid(currentCbPos + 3, endline - (currentCbPos + 3));
            // we skip it because it is inline code and not codeBlock
            if (lang.contains(type)) {
                int nextEnd = str.indexOf(type, currentCbPos + 3);
                nextEnd += 3;
                currentCbPos = str.indexOf(type, nextEnd);
                continue;
            }
            // move start pos to after the endline
            currentCbPos = endline + 1;
            // find the codeBlock end
            int next = str.indexOf(type, currentCbPos);
            if (next == -1) {
                break;
            }
            // extract the codeBlock
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 2)
            const QStringRef codeBlock = str.midRef(currentCbPos, next - currentCbPos);
#else
            QStringView str_view = str;
            QStringView codeBlock = str_view.mid(currentCbPos, next - currentCbPos);
#endif
            // Skip highlighting if the block's content contains a fence of the
            // other delimiter type (e.g. a tilde block whose content contains a
            // backtick code block). The outer block should not escape the inner
            // block's already-injected spans.
            // See: https://github.com/pbek/ArcNotes/issues/2671
            if (!otherFenceType.isEmpty() && codeBlock.contains(otherFenceType)) {
                next += 3;
                currentCbPos = str.indexOf(type, next);
                continue;
            }
            QString highlightedCodeBlock;
            if (!(codeBlock.isEmpty() && lang.isEmpty())) {
                const CodeToHtmlConverter c(lang);
                highlightedCodeBlock = c.process(codeBlock);
                // take care of the null char
                highlightedCodeBlock.replace(QChar('\u0000'), QLatin1String(""));
                str.replace(currentCbPos, next - currentCbPos, highlightedCodeBlock);
                // recalculate next because string has now changed
                next = str.indexOf(type, currentCbPos);
            }
            // move next pos to after the backticks
            next += 3;
            // find the start of the next code block
            currentCbPos = str.indexOf(type, next);
        }
    }
}

/**
 * @brief Temporarily mask all tilde-fenced code blocks (~~~...~~~) in @p str
 * with unique placeholders, storing the originals in @p masked.
 * Used to prevent backtick highlightCode from processing backtick fences that
 * appear inside tilde fences. See: https://github.com/pbek/ArcNotes/issues/2671
 */
QStringList maskTildeFences(QString& str) {
    QStringList masked;
    const QString fence = QStringLiteral("~~~");
    int pos = 0;
    for (;;) {
        int start = str.indexOf(fence, pos);
        if (start == -1) break;
        int endOfOpeningLine = str.indexOf(QChar('\n'), start);
        if (endOfOpeningLine == -1) break;
        // Skip inline (closing fence on the same line)
        const QString lang = str.mid(start + 3, endOfOpeningLine - (start + 3));
        if (lang.contains(fence)) {
            pos = endOfOpeningLine + 1;
            continue;
        }
        int end = str.indexOf(fence, endOfOpeningLine + 1);
        if (end == -1) break;
        int blockLen = end + fence.length() - start;
        const QString original = str.mid(start, blockLen);
        const QString placeholder = QStringLiteral("\x01TILDECB_%1\x01").arg(masked.size());
        masked.append(original);
        str.replace(start, blockLen, placeholder);
        pos = start + placeholder.length();
    }
    return masked;
}

/**
 * @brief Restore tilde-fenced code blocks previously masked by maskTildeFences().
 */
void unmaskTildeFences(QString& str, const QStringList& masked) {
    for (int i = 0; i < masked.size(); ++i) {
        const QString placeholder = QStringLiteral("\x01TILDECB_%1\x01").arg(i);
        str.replace(placeholder, masked[i]);
    }
}

/**
 * @brief Temporarily mask all 4-space/tab indented code blocks in @p str with
 * unique placeholders, storing the originals for later restoration.
 * Used to prevent highlightCode from processing fenced code blocks (e.g.
 * ```ini) that appear inside indented code blocks — which would inject
 * highlight spans into content that MD4C will render as verbatim text.
 * See: https://github.com/pbek/ArcNotes/issues/2671
 */
QStringList maskIndentedCodeBlocks(QString& str) {
    QStringList masked;
    // Match one or more consecutive lines that start with 4 spaces or a tab
    static const QRegularExpression indentedRE(QStringLiteral("(^|\\n)((?:(?:[ ]{4}|\\t)[^\\n]*(?:\\n|$))+)"),
                                               QRegularExpression::MultilineOption);

    // Build a list of (start, end) ranges for backtick-fenced code blocks so
    // we can skip indented lines that fall inside them.  Tilde fences are
    // already masked at this point, so we only need to handle backticks.
    QList<QPair<int, int>> fencedRanges;
    {
        const QString fence = QStringLiteral("```");
        int pos = 0;
        while (true) {
            int start = str.indexOf(fence, pos);
            if (start == -1) break;
            // Find the end of the opening fence line
            int eol = str.indexOf(QChar('\n'), start);
            if (eol == -1) break;
            // Find the closing fence
            int end = str.indexOf(fence, eol + 1);
            if (end == -1) break;
            // Find end of closing fence line (or end of string)
            int endLine = str.indexOf(QChar('\n'), end);
            if (endLine == -1) endLine = str.length();
            fencedRanges.append(qMakePair(start, endLine));
            pos = endLine + 1;
        }
    }

    // Collect all matches first (forward order)
    QList<QRegularExpressionMatch> matches;
    QRegularExpressionMatchIterator it = indentedRE.globalMatch(str);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        // Skip this match if it falls inside a backtick-fenced code block
        int matchStart = match.capturedStart(2);
        bool insideFence = false;
        for (const auto& range : fencedRanges) {
            if (matchStart >= range.first && matchStart < range.second) {
                insideFence = true;
                break;
            }
        }
        if (!insideFence) {
            matches.append(match);
        }
    }

    // Replace in reverse order to keep earlier positions valid, but assign
    // placeholder indices in forward order so masked[i] pairs with INDENTCB_i.
    for (int i = matches.size() - 1; i >= 0; --i) {
        const QRegularExpressionMatch& match = matches[i];
        // Group 2 is the indented block; group 1 is the preceding newline/start
        masked.prepend(match.captured(2));
        const QString placeholder = QStringLiteral("\x01INDENTCB_%1\x01").arg(i);
        str.replace(match.capturedStart(2), match.capturedLength(2), placeholder);
    }

    return masked;
}

/**
 * @brief Restore indented code blocks previously masked by maskIndentedCodeBlocks().
 */
void unmaskIndentedCodeBlocks(QString& str, const QStringList& masked) {
    for (int i = 0; i < masked.size(); ++i) {
        const QString placeholder = QStringLiteral("\x01INDENTCB_%1\x01").arg(i);
        str.replace(placeholder, masked[i]);
    }
}

inline int nonOverlapCount(const QString& str, const QChar c = '`') {
    const auto len = str.length();
    int count = 0;
    for (int i = 0; i < len; ++i) {
        if (str[i] == c && i + 2 < len && str[i + 1] == c && str[i + 2] == c) {
            ++count;
            i += 2;
        }
    }
    return count;
}

struct ImageSize {
    QString fileName;
    int size;
};
std::vector<ImageSize>* getImageSizeCache() {
    static std::vector<ImageSize> _imageSizesCache;
    if (_imageSizesCache.size() > 100) _imageSizesCache.erase(_imageSizesCache.begin());
    return &_imageSizesCache;
}

/**
 * Converts a Markdown string for a note to html
 *
 * @param str
 * @param notesPath
 * @param maxImageWidth
 * @param forExport
 * @param base64Images
 * @return
 */
}  // namespace

QString MarkdownRenderService::renderNoteToHtml(const NoteData& note, const QString& notesPath, int maxImageWidth,
                                                bool forExport, bool base64Images) const {
    return renderTextToHtml(note, note.noteText, notesPath, maxImageWidth, forExport, base64Images);
}

QString MarkdownRenderService::renderTextToHtml(const NoteData& note, QString str, const QString& notesPath,
                                                int maxImageWidth, bool forExport, bool base64Images) const {
    // MD4C flags
    unsigned flags = MD_DIALECT_GITHUB | MD_FLAG_WIKILINKS | MD_FLAG_LATEXMATHSPANS;
    // we parse the task lists ourselves

    // we render checkboxes when using qlitehtml
#ifndef USE_QLITEHTML
    flags &= ~MD_FLAG_TASKLISTS;
#endif

    const SettingsRepository settings;
    // Enable underline extension when the setting is on so that _text_ and
    // __text__ are rendered as <u> tags instead of italic/bold.
    // Note: MD_DIALECT_GITHUB does not include MD_FLAG_UNDERLINE, so we must
    // add it explicitly when the feature is enabled.
    if (settings.value(QStringLiteral("MainWindow/noteTextView.underline"), true).toBool()) {
        flags |= MD_FLAG_UNDERLINE;
    }
    if (!SettingsRepository().value(QStringLiteral("Editor/wikiLinkSupport"), false).toBool()) {
        flags &= ~MD_FLAG_WIKILINKS;
    }

    QString windowsSlash = QLatin1String("");

#ifdef Q_OS_WIN32
    // we need another slash for Windows
    windowsSlash = "/";
#endif

    // remove frontmatter from Markdown text
    if (str.startsWith(QLatin1String("---"))) {
        static const QRegularExpression re(
            QStringLiteral(R"(^---((\r\n)|(\n\r)|\r|\n).+?((\r\n)|(\n\r)|\r|\n)---((\r\n)|(\n\r)|\r|\n))"),
            QRegularExpression::DotMatchesEverythingOption);
        str.remove(re);
    }

    // Mask code blocks and inline code to prevent link transformations from
    // modifying content inside them (e.g., <stdio.h> being converted to a link)
    // See: https://github.com/pbek/ArcNotes/issues/3084
    const QStringList maskedBlocks = maskCodeBlocks(str);

    // parse for relative file urls and make them absolute
    // (for example to show images under the note path)
    static const QRegularExpression re(QStringLiteral(R"(([\(<])file:\/\/([^\/].+?)([\)>]))"));
    str.replace(re, QStringLiteral("\\1file://") + windowsSlash + QRegularExpression::escape(notesPath) +
                        QStringLiteral("/\\2\\3"));

    // transform images without "file://" urls to file-urls (but we better do
    // that in the html, not the Markdown!)
    //    str.replace(
    //            QRegularExpression(R"((\!\[.*\]\()((?!file:\/\/).+)(\)))"),
    //            "\\1file://" + windowsSlash +
    //            QRegularExpression::escape(notesPath)
    //            + "/\\2\\3");

    QRegularExpressionMatchIterator i;

    // Try to replace links like <my-note.md> or <file.pdf> with proper file
    // links We need to do that in the Markdown because Hoedown would not create
    // a link tag This is a "has not '\w+:\/\/' in it" regular expression see:
    // http://stackoverflow.com/questions/406230/regular-expression-to-match-line-that-doesnt-contain-a-word
    // TODO: maybe we could do that per QTextBlock to check if it's done in comment blocks?
    // Important: The `\n` is needed to not crash under Windows if there is just
    // an opening `<` and a lot of other text after it
    // Note: If we find an `@` in the link we don't replace it because it's an email address
    static const QRegularExpression linkRE(QStringLiteral("<(((?!\\w+:\\/\\/)[^\\*<>@\n])+\\.[\\w\\d]+)>"));
    i = linkRE.globalMatch(str);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        const QString fileLink = match.captured(1);
        const QString url = NoteLinkService().fileUrlFromFileName(note, fileLink, true);

        str.replace(match.captured(0),
                    QStringLiteral("[") + fileLink + QStringLiteral("](") + url + QStringLiteral(")"));
    }

    // Try to replace links like [my note](my-note.md) or [file](file.md) with
    // proper file links This is currently also is handling relative image and
    // attachment links! We are using `{1,500}` instead of `+` because there
    // were crashes with regular expressions running wild
    // TODO: In theory we could convert relative note links in the html (and not
    // in the Markdown) to prevent troubles with code blocks
    i = QRegularExpression(QStringLiteral(R"(\[(.*?)\]\((((?!\w+:\/\/)[^<>]){1,500}?)\))")).globalMatch(str);

    while (i.hasNext()) {
        const QRegularExpressionMatch match = i.next();
        const QString fileText = match.captured(1);
        const QString fileLink = match.captured(2);

        const QString url = NoteLinkService().fileUrlFromFileName(note, fileLink, true);

        str.replace(match.captured(0),
                    QStringLiteral("[") + fileText + QStringLiteral("](") + url + QStringLiteral(")"));
    }

    // Wrap bare note:// URLs (legacy links not already inside angle brackets,
    // parentheses or quotes) in <> so that MD4C renders them as clickable
    // autolinks in the preview.  The lookbehind excludes URLs that are already
    // part of <note://…> autolinks, [text](note://…) bracket links, or
    // href="note://…" HTML attributes.
    // Examples that are wrapped: note://MyNote  note://Note_2018_06_26T22_11_10
    // Examples that are left alone: <note://MyNote>  [t](note://MyNote)
    {
        static const QRegularExpression bareNoteUrlRE(QStringLiteral(R"((?<![<("'])(note:\/\/[^\s<>"')]+))"));
        str.replace(bareNoteUrlRE, QStringLiteral("<\\1>"));
    }

    // Restore masked code blocks after link transformations are done
    unmaskCodeBlocks(str, maskedBlocks);

    /*CODE HIGHLIGHTING*/
    int cbTildeCount = nonOverlapCount(str, '~');
    if (cbTildeCount % 2 != 0) --cbTildeCount;

    // divide by two to get actual number of code blocks
    cbTildeCount /= 2;

    // This will also add html in the code blocks, so we will do this at the very end.
    // Before processing backtick fences, temporarily mask:
    //   1. Tilde-fenced blocks (~~~...~~~) — so a backtick fence nested inside a
    //      tilde fence is not highlighted (the outer tilde block would HTML-escape
    //      the injected spans).
    //   2. 4-space/tab indented code blocks — so a backtick fence appearing as
    //      literal content inside an indented block is not highlighted (MD4C
    //      renders indented blocks verbatim, so injected spans would show as raw
    //      HTML in the preview).
    // See: https://github.com/pbek/ArcNotes/issues/2671
    const QStringList maskedTildeFences = maskTildeFences(str);
    const QStringList maskedIndentedBlocks = maskIndentedCodeBlocks(str);
    // Recount backtick blocks after masking (masked placeholders contain no
    // backtick triples, so the count is accurate)
    int cbCount = nonOverlapCount(str, '`');
    if (cbCount % 2 != 0) --cbCount;
    cbCount /= 2;
    highlightCode(str, QStringLiteral("```"), cbCount);
    unmaskIndentedCodeBlocks(str, maskedIndentedBlocks);
    unmaskTildeFences(str, maskedTildeFences);
    highlightCode(str, QStringLiteral("~~~"), cbTildeCount, QStringLiteral("```"));

    // Parse and strip image dimension attributes like { width=300 height=200 }
    // from Markdown image syntax before MD4C processing, since MD4C doesn't
    // support curly-brace attribute extensions.
    // We encode dimensions as a URL fragment (#qon-w=300&h=200) so that each
    // specific image occurrence carries its own dimensions through MD4C, even
    // when the same image URL appears multiple times with different attributes.
    // See: https://github.com/pbek/ArcNotes/issues/2898
    {
        static const QRegularExpression imgDimRE(QStringLiteral(
            R"(!\[([^\]]*)\]\(([^\)]*)\)\s*\{\s*((?:width=\d+\s*)?(?:height=\d+\s*)?(?:width=\d+\s*)?)\})"));
        static const QRegularExpression widthRE(QStringLiteral(R"(width=(\d+))"));
        static const QRegularExpression heightRE(QStringLiteral(R"(height=(\d+))"));

        QRegularExpressionMatchIterator dimIter = imgDimRE.globalMatch(str);
        while (dimIter.hasNext()) {
            const QRegularExpressionMatch dimMatch = dimIter.next();
            const QString imgUrl = dimMatch.captured(2);
            const QString attrs = dimMatch.captured(3);

            int w = 0, h = 0;
            QRegularExpressionMatch wMatch = widthRE.match(attrs);
            if (wMatch.hasMatch()) w = wMatch.captured(1).toInt();
            QRegularExpressionMatch hMatch = heightRE.match(attrs);
            if (hMatch.hasMatch()) h = hMatch.captured(1).toInt();

            if (w > 0 || h > 0) {
                // Encode dimensions as a URL fragment that MD4C passes through
                QString fragment = QStringLiteral("#qon-dim");
                if (w > 0) fragment += QStringLiteral("&w=%1").arg(w);
                if (h > 0) fragment += QStringLiteral("&h=%1").arg(h);

                const QString original = dimMatch.captured(0);
                const QString replacement = QStringLiteral("![%1](%2%3)").arg(dimMatch.captured(1), imgUrl, fragment);
                str.replace(original, replacement);
            }
        }
    }

    const auto data = str.toUtf8();
    if (data.size() == 0) {
        return QLatin1String("");
    }

    QByteArray array;
    const int renderResult = md_html(data.data(), MD_SIZE(data.size()), &captureHtmlFragment, &array, flags, 0);

    QString result;
    if (renderResult == 0) {
        result = QString::fromUtf8(array);
    } else {
        qWarning() << "MD4C Failure!";
        return QString();
    }

    // Inject width/height attributes from the #qon-dim URL fragment into
    // the HTML <img> tags and remove the fragment from the src attribute.
    // Only images that originally had { width=X height=Y } carry this marker.
    // Note: MD4C escapes '&' as '&amp;' in URLs, so we match both forms.
    {
        static const QRegularExpression dimFragRE(
            QStringLiteral(R"(<img src="([^"]*?)#qon-dim(?:(?:&amp;|&)w=(\d+))?(?:(?:&amp;|&)h=(\d+))?" )"));
        QRegularExpressionMatch dimMatch = dimFragRE.match(result);
        while (dimMatch.hasMatch()) {
            const QString fullMatch = dimMatch.captured(0);
            const QString cleanUrl = dimMatch.captured(1);
            const QString wStr = dimMatch.captured(2);
            const QString hStr = dimMatch.captured(3);

            QString replacement = QStringLiteral("<img src=\"%1\" ").arg(cleanUrl);
            if (!wStr.isEmpty()) replacement += QStringLiteral("width=\"%1\" ").arg(wStr);
            if (!hStr.isEmpty()) replacement += QStringLiteral("height=\"%1\" ").arg(hStr);

            result.replace(fullMatch, replacement);
            dimMatch = dimFragRE.match(result);
        }
    }

    if (SettingsRepository().value(QStringLiteral("Editor/wikiLinkSupport"), false).toBool()) {
        result = postProcessWikiLinksHtml(result, note.noteSubFolderId);
    }

    // Add the "notelink" CSS class to internal note links so the
    // LinkInternal highlighter state color is applied in the preview
    result = postProcessInternalNoteLinksHtml(result);

    // transform remote preview image tags
    Utils::Misc::transformRemotePreviewImages(result, maxImageWidth, externalImageHash());

    // transform images without "file://" urls to file-urls
    // Note: this is currently handled above in Markdown
    //       if we want to activate this code again we need to take care of
    //       remote http(s) links to images! see:
    //       https://github.com/pbek/ArcNotes/issues/1286
    /*
        const QString subFolderPath = getNoteSubFolder().relativePath("/");
        const QString notePath = notesPath + (subFolderPath.isEmpty() ? "" : "/"
       + subFolderPath); result.replace( QRegularExpression(R"((<img
       src=\")((?!file:\/\/).+)\")"),
                "\\1file://" + windowsSlash + notePath + "/\\2\"");
    */

    const QString fontString = Utils::Misc::previewCodeFontString();

    // set the stylesheet for the <code> blocks
    QString codeStyleSheet = QLatin1String("");
    if (!fontString.isEmpty()) {
        // set the note text view font
        QFont font;
        font.fromString(fontString);

        // add the font for the code block
        codeStyleSheet = QStringLiteral("pre, code { %1; }").arg(Utils::Schema::encodeCssFont(font));

        // ignore code font size to allow zooming (#1202)
        if (settings.value(QStringLiteral("MainWindow/noteTextView.ignoreCodeFontSize"), true).toBool()) {
            codeStyleSheet.remove(QRegularExpression(QStringLiteral(R"(font-size: \d+\w+;)")));
        }
    }

    const bool darkModeColors = !forExport && settings.value(QStringLiteral("darkModeColors")).toBool();

    const QString codeForegroundColor = darkModeColors ? QStringLiteral("#ffffff") : QStringLiteral("#000000");
    const QString codeBackgroundColor = darkModeColors ? QStringLiteral("#444444") : QStringLiteral("#f1f1f1");
    const QString tableBorderColor = darkModeColors ? QStringLiteral("#ffffff") : QStringLiteral("#000000");
    const QString wikiLinkStyle =
        QStringLiteral(
            "a.wikilink { color: %1; text-decoration: underline; "
            "text-decoration-style: dotted; }"
            "a.wikilink.broken { color: %2; text-decoration-style: dashed; }"
            "a.notelink { color: %3; text-decoration: underline; }")
            .arg(Utils::Schema::schemaSettings->getForegroundColor(MarkdownHighlighter::HighlighterState::Link).name(),
                 Utils::Schema::schemaSettings->getForegroundColor(MarkdownHighlighter::HighlighterState::BrokenLink)
                     .name(),
                 Utils::Schema::schemaSettings->getForegroundColor(MarkdownHighlighter::HighlighterState::LinkInternal)
                     .name());

    // do some more code formatting
    // the "pre" styles are for the full-width code block background color
    codeStyleSheet += QString(
                          "pre { display: block; background-color: %1;"
                          " white-space: pre-wrap } "
                          "code { padding: 3px; overflow: auto;"
                          " line-height: 1.45em; background-color: %1;"
                          " border-radius: 5px; color: %2; }")
                          .arg(codeBackgroundColor, codeForegroundColor);

    // TODO: We should probably make a stylesheet for this
    codeStyleSheet += QStringLiteral(" .code-comment { color: #75715E;}");
    codeStyleSheet += QStringLiteral(" .code-string { color: #E6DB74;}");
    codeStyleSheet += QStringLiteral(" .code-literal { color: #AE81FF;}");
    codeStyleSheet += QStringLiteral(" .code-type { color: #66D9EF;}");
    codeStyleSheet += QStringLiteral(" .code-builtin { color: #A6E22E;}");
    codeStyleSheet += QStringLiteral(" .code-keyword { color: #F92672;}");
    codeStyleSheet += QStringLiteral(" .code-other { color: #F92672;}");

    // correct the strikeout tag
    result.replace(QRegularExpression(QStringLiteral("<del>([^<]+)<\\/del>")), QStringLiteral("<s>\\1</s>"));
    const bool rtl = settings.value(QStringLiteral("MainWindow/noteTextView.rtl")).toBool();
    const QString rtlStyle = rtl ? QStringLiteral("body {text-align: right; direction: rtl;}") : QLatin1String("");

    if (forExport) {
        // get defined body font from settings
        const QString bodyFontString = Utils::Misc::previewFontString();

        // create export stylesheet
        QString exportStyleSheet = QLatin1String("");
        if (!bodyFontString.isEmpty()) {
            QFont bodyFont;
            bodyFont.fromString(bodyFontString);

            exportStyleSheet = QStringLiteral("body { %1; }").arg(Utils::Schema::encodeCssFont(bodyFont));
        }

        result = QString(
                     "<html><head><meta charset=\"utf-8\"/>"
                     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/>"
                     "<style>"
                     "h1 { margin: 5px 0 20px 0; }"
                     "h2, h3 { margin: 10px 0 15px 0; }"
                     "img { max-width: 100%; }"
                     "pre { background-color: %5; border-radius: 5px; padding: "
                     "10px; }"
                     "pre > code { padding: 0; }"
                     "table {border-spacing: 0; border-style: solid; "
                     "border-width: 1px; "
                     "border-collapse: collapse; margin-top: 0.5em;}"
                     "th, td {padding: 2px 5px; border: 1px solid %6;}"
                     "a { color: #FF9137; text-decoration: none; } %1 %2 %4 %7"
                     "</style></head><body class=\"export\">%3</body></html>")
                     .arg(codeStyleSheet, exportStyleSheet, result, rtlStyle, codeBackgroundColor, tableBorderColor,
                          wikiLinkStyle);

        // remove trailing newline in code blocks
        result.replace(QStringLiteral("\n</code>"), QStringLiteral("</code>"));
    } else {
        const QString schemaStyles =
            Utils::Misc::isPreviewUseEditorStyles() ? Utils::Schema::getSchemaStyles() : QLatin1String("");

        // for preview
        result = QStringLiteral(
                     "<html><head><style>"
                     "h1 { margin: 5px 0 20px 0; }"
                     "h2, h3 { margin: 10px 0 15px 0; }"
                     "table {border-spacing: 0; border-style: solid; border-width: "
                     "1px; border-collapse: collapse; margin-top: 0.5em;}"
                     "th, td {padding: 2px 5px; border: 1px solid %5;}"
                     "a { color: #FF9137; text-decoration: none; }"
                     "a.task-list-item-checkbox { color: #FF9137; text-decoration: none; cursor: "
                     "pointer; margin-right: 6px; }"
                     "a.task-list-item-checkbox:focus { outline: none; }"
                     "%1 %3 %4 %6"
                     "</style></head><body class=\"preview\">%2</body></html>")
                     .arg(codeStyleSheet, result, rtlStyle, schemaStyles, tableBorderColor, wikiLinkStyle);
        // remove trailing newline in code blocks
        result.replace(QStringLiteral("\n</code>"), QStringLiteral("</code>"));
    }

    // check if width of embedded local images is too high
    static const QRegularExpression imgRE(QStringLiteral("<img src=\"(file:\\/\\/[^\"]+)\""));
    i = imgRE.globalMatch(result);

    auto getImageSizeFromCache = [](const QString& image) {
        auto cache = getImageSizeCache();
        auto it =
            std::find_if(cache->begin(), cache->end(), [image](const ImageSize& i) { return i.fileName == image; });
        return it == cache->end() ? -1 : it->size;
    };

    while (i.hasNext()) {
        const QRegularExpressionMatch match = i.next();
        const QString fileUrl = match.captured(1);
        const QString fileName = QUrl(fileUrl).toLocalFile();

        // Skip images that already have explicit width/height from Markdown
        // dimension attributes like { width=300 height=200 }
        // (for https://github.com/pbek/ArcNotes/issues/2898)
        {
            const int matchEnd = match.capturedEnd(0);
            const int lookAhead = qMin(50, result.length() - matchEnd);
            const QString remainder = result.mid(matchEnd, lookAhead);
            if (remainder.contains(QStringLiteral("width=\"")) || remainder.contains(QStringLiteral("height=\""))) {
                continue;
            }
        }

        int imageWidth = 0;

        // If file is greater than 1MB just limit its width already
        constexpr int OneMB = 1000 * 1000;
        if (QFileInfo(fileName).size() > OneMB) {
            imageWidth = maxImageWidth;
        }

        // try the cache
        if (imageWidth == 0) {
            imageWidth = getImageSizeFromCache(fileName);
        }

        // get image size using QImage and cache it
        if (imageWidth == -1) {
            const QImage image(fileName);
            imageWidth = image.width();
            getImageSizeCache()->push_back({fileName, imageWidth});
        }

        auto fileNameWithPercentSpaces = fileName;
        fileNameWithPercentSpaces.replace(QChar(' '), QStringLiteral("%20"));

        if (forExport) {
            result.replace(QRegularExpression(QStringLiteral(R"(<img src="file:\/\/)") +
                                              QRegularExpression::escape(windowsSlash + fileNameWithPercentSpaces) +
                                              QStringLiteral("\"")),
                           QStringLiteral("<img src=\"file://%2\"").arg(windowsSlash + fileName));
        } else {
            // for preview
            // cap the image width at maxImageWidth (note text view width)
            const int originalWidth = imageWidth;
            const int displayWidth = (originalWidth > maxImageWidth) ? maxImageWidth : originalWidth;

            // Use a regex with a negative lookahead so that images which
            // already carry explicit width/height attributes (from Markdown
            // dimension syntax) are not touched by the auto-sizing replace.
            // Without this, a plain `QString::replace` would globally replace
            // ALL <img> tags sharing the same src, including those that already
            // have user-specified dimensions.
            const QRegularExpression autoSizeRE(QStringLiteral(R"(<img src="file://)") +
                                                QRegularExpression::escape(windowsSlash + fileNameWithPercentSpaces) +
                                                QStringLiteral(R"("(?! width=| height=))"));

            result.replace(autoSizeRE, QStringLiteral(R"(<img width="%1" src="file://%2")")
                                           .arg(QString::number(displayWidth), windowsSlash + fileName));
        }

        // encode the image base64
        if (base64Images) {
            QFile file(fileName);

            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << QObject::tr("Could not read image file: %1").arg(fileName);

                continue;
            }

            QMimeDatabase db;
            const QMimeType type = db.mimeTypeForFile(file.fileName());
            const QByteArray ba = file.readAll();

            result.replace(QRegularExpression(QStringLiteral("<img(.+?)src=\"") + QRegularExpression::escape(fileUrl) +
                                              QChar('"')),
                           QStringLiteral(R"(<img\1src="data:%1;base64,%2")").arg(type.name(), QString(ba.toBase64())));
        }
    }

    //    qDebug() << __func__ << " - 'result': " << result;
    return result;
}
