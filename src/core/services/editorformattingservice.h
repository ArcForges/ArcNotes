#pragma once

#include <QString>

class EditorFormattingService {
public:
    QString wrapSelection(const QString& text, const QString& prefix, const QString& suffix = QString()) const;
    QString headingText(const QString& text, int level) const;
};
