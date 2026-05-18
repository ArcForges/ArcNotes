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

#pragma once

#include <core/data/notedata.h>
#include <libraries/qmarkdowntextedit/markdownhighlighter.h>

#include <QHash>

QT_BEGIN_NAMESPACE
class QTextDocument;

QT_END_NAMESPACE

class ArcNotesMarkdownHighlighter : public MarkdownHighlighter {
    Q_OBJECT

public:
    ArcNotesMarkdownHighlighter(QTextDocument* parent = nullptr,
                                HighlightingOptions highlightingOptions = HighlightingOption::None);

    void updateCurrentNote(const NoteData& note);
    void reloadNoteLinkSettings();

protected:
    void highlightBlock(const QString& text) Q_DECL_OVERRIDE;

private:
    void highlightBrokenNotesLink(const QString& text);
    void highlightWikiLinks(const QString& text);
    void clearWikiLinkCache();

    void updateCachedRegexes(const QString& newExt);

private:
    NoteData _currentNote;
    QString _defaultNoteFileExt;
    QRegularExpression _regexTagStyleLink;
    QRegularExpression _regexBracketLink;
    QHash<QString, bool> _wikiLinkCache;
};
