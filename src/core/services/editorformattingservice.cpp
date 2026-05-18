#include "editorformattingservice.h"

QString EditorFormattingService::wrapSelection(const QString& text, const QString& prefix,
                                               const QString& suffix) const {
    return prefix + text + (suffix.isEmpty() ? prefix : suffix);
}

QString EditorFormattingService::headingText(const QString& text, int level) const {
    level = qBound(1, level, 6);
    return QString(level, QLatin1Char('#')) + QLatin1Char(' ') + text;
}
