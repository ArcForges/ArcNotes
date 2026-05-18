/*
 * Copyright (c) 2014-2026 Patrizio Bekerle -- <patrizio@bekerle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * QPlainTextEdit Markdown highlighter
 */

#include "arcnotesmarkdownhighlighter.h"

#include <core/repositories/noterepository.h>
#include <core/services/notelinkservice.h>

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QFont>
#include <QObject>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

ArcNotesMarkdownHighlighter::ArcNotesMarkdownHighlighter(QTextDocument* parent, HighlightingOptions highlightingOptions)
    : MarkdownHighlighter(parent, highlightingOptions) {
    reloadNoteLinkSettings();
}

void ArcNotesMarkdownHighlighter::updateCurrentNote(const NoteData& note) {
    _currentNote = note;
}

void ArcNotesMarkdownHighlighter::reloadNoteLinkSettings() {
    _defaultNoteFileExt = NoteRepository().defaultNoteFileExtension();
    updateCachedRegexes(_defaultNoteFileExt);
    clearWikiLinkCache();
}

/**
 * Does the Markdown highlighting
 * We need to override this method so our highlightMarkdown gets called
 *
 * @param text
 */
void ArcNotesMarkdownHighlighter::highlightBlock(const QString& text) {
    if (currentBlockState() == HeadlineEnd) {
        currentBlock().previous().setUserState(NoState);
        addDirtyBlock(currentBlock().previous());
    }
    setCurrentBlockState(HighlighterState::NoState);
    currentBlock().setUserState(HighlighterState::NoState);

    highlightMarkdown(text);

    if (text.contains(QLatin1String("note://")) || text.contains(QChar('.') + _defaultNoteFileExt)) {
        highlightBrokenNotesLink(text);
    }

    if (NoteRepository().isWikiLinkSupportEnabled() && text.contains(QStringLiteral("[["))) {
        highlightWikiLinks(text);
    }

    _highlightingFinished = true;
}

void ArcNotesMarkdownHighlighter::updateCachedRegexes(const QString& newExt) {
    _regexTagStyleLink = QRegularExpression(R"(<([^\s`][^`]*?\.)" + newExt + R"()>)");
    _regexBracketLink =
        QRegularExpression(R"(\[[^\[\]]+\]\((\S+\.)" + newExt + R"(|.+?\.)" + newExt + R"()(#[^\)]+)?\)\B)");
}

void ArcNotesMarkdownHighlighter::clearWikiLinkCache() {
    _wikiLinkCache.clear();
}

/**
 * Highlight internal note links (both valid and broken)
 *
 * @param text
 */
void ArcNotesMarkdownHighlighter::highlightBrokenNotesLink(const QString& text) {
    static const QRegularExpression regex(QStringLiteral(R"(note:\/\/[^\s\)>]+)"));
    QRegularExpressionMatch match = regex.match(text);
    bool noteExists = false;

    if (match.hasMatch()) {  // check legacy note:// links
        const QString noteLink = match.captured(0);

        // try to fetch a note from the url string
        const NoteData note = NoteRepository().findByUrlString(noteLink);
        noteExists = note.id > 0;
    } else {
        // don't make any further checks if no current note was set
        if (_currentNote.id <= 0) {
            return;
        }

        // check <note file.md> links
        // Example: <([^\s`][^`]*?\.md)>
        match = _regexTagStyleLink.match(text);

        if (match.hasMatch()) {
            const QString fileName = NoteRepository().decodeNoteUrl(match.captured(1));

            // skip urls
            if (fileName.contains(QStringLiteral("://"))) {
                return;
            }

            const NoteData note = NoteRepository().findByRelativeFileName(_currentNote, fileName);
            noteExists = note.id > 0;
        } else {  // check [note](note file.md) or [note](note file.md#heading) links
            // Example: R"(\[[^\[\]]+\]\((\S+\.md|.+?\.md)(#[^\)]+)?\)\B)")
            match = _regexBracketLink.match(text);

            if (match.hasMatch()) {
                const QString fileName = NoteRepository().decodeNoteUrl(match.captured(1));

                // skip urls
                if (fileName.contains(QStringLiteral("://"))) {
                    return;
                }

                const NoteData note = NoteRepository().findByRelativeFileName(_currentNote, fileName);
                noteExists = note.id > 0;
            } else {
                // no note link was found
                return;
            }
        }
    }

    auto state = noteExists ? HighlighterState::LinkInternal : HighlighterState::BrokenLink;

    setFormat(match.capturedStart(0), match.capturedLength(0), _formats[state]);
}

void ArcNotesMarkdownHighlighter::highlightWikiLinks(const QString& text) {
    if (_currentNote.id <= 0) {
        return;
    }

    static const QRegularExpression regex(QStringLiteral(R"(\[\[([^\[\]]+?)\]\])"));
    const QTextCharFormat maskedFormat = currentMaskedFormat();
    auto iterator = regex.globalMatch(text);

    while (iterator.hasNext()) {
        const QRegularExpressionMatch match = iterator.next();
        const QString innerText = match.captured(1).trimmed();
        if (innerText.isEmpty()) {
            continue;
        }

        bool exists = _wikiLinkCache.value(innerText, false);
        if (!_wikiLinkCache.contains(innerText)) {
            exists = NoteLinkService().resolveWikiLink(innerText, _currentNote.noteSubFolderId).id > 0;
            _wikiLinkCache.insert(innerText, exists);
        }

        QTextCharFormat openFormat = maskedFormat;
        QTextCharFormat closeFormat = maskedFormat;
        QTextCharFormat bodyFormat = _formats[exists ? HighlighterState::WikiLink : HighlighterState::WikiLinkBroken];
        if (bodyFormat.fontPointSize() > 0) {
            openFormat.setFontPointSize(bodyFormat.fontPointSize());
            closeFormat.setFontPointSize(bodyFormat.fontPointSize());
        }

        setFormat(match.capturedStart(0), 2, openFormat);
        setFormat(match.capturedStart(1), match.capturedLength(1), bodyFormat);
        setFormat(match.capturedStart(0) + match.capturedLength(0) - 2, 2, closeFormat);
    }
}
