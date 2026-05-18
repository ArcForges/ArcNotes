#include "navigationviewmodel.h"

#include <QRegularExpression>
#include <QStringList>

namespace {
QString stripMarkdown(const QString& input) {
    static const QRegularExpression boldRegex(QStringLiteral(R"(\*{2}([^*]+)\*{2})"));
    static const QRegularExpression italicRegex(QStringLiteral(R"(\*{1}([^*]+)\*{1})"));
    static const QRegularExpression strikethroughRegex(QStringLiteral(R"(\~{2}([^~]+)\~{2})"));
    static const QRegularExpression linkRegex(QStringLiteral(R"(\[([^]]+)\]\(([^)]+)\))"));
    static const QRegularExpression angleBracketLinkRegex(QStringLiteral(R"(<([^>]+)>)"));
    static const QRegularExpression codeRegex(QStringLiteral(R"(`([^`]+)`+)"));

    QString strippedText = input;
    strippedText.replace(boldRegex, QStringLiteral("\\1"));
    strippedText.replace(italicRegex, QStringLiteral("\\1"));
    strippedText.replace(strikethroughRegex, QStringLiteral("\\1"));
    strippedText.replace(linkRegex, QStringLiteral("\\1"));
    strippedText.replace(angleBracketLinkRegex, QStringLiteral("\\1"));
    strippedText.replace(codeRegex, QStringLiteral("\\1"));
    return strippedText.trimmed();
}
}  // namespace

NavigationViewModel::NavigationViewModel(QObject* parent) : QObject(parent), _model(this) {}

NavigationOutlineModel* NavigationViewModel::model() {
    return &_model;
}

const NavigationOutlineModel* NavigationViewModel::model() const {
    return &_model;
}

void NavigationViewModel::parseDocument(const QString& text) {
    QVector<NavigationOutlineItemData> headings;
    int position = 0;
    int lineNumber = 0;
    const QRegularExpression headingExpression(QStringLiteral("^(#{1,6})\\s+(.+)$"));
    const QRegularExpression h1UnderlineExpression(QStringLiteral("^\\s*=+\\s*$"));
    const QRegularExpression h2UnderlineExpression(QStringLiteral("^\\s*-+\\s*$"));

    const QStringList lines = text.split(QLatin1Char('\n'));
    for (int index = 0; index < lines.count(); ++index) {
        const QString& line = lines.at(index);
        const QRegularExpressionMatch match = headingExpression.match(line);
        if (match.hasMatch()) {
            NavigationOutlineItemData item;
            item.level = match.captured(1).length();
            item.title = stripMarkdown(match.captured(2));
            item.position = position;
            item.line = lineNumber;
            if (!item.title.isEmpty()) {
                headings.append(item);
            }
        } else if (index + 1 < lines.count()) {
            const QString& nextLine = lines.at(index + 1);
            const bool isH1 = h1UnderlineExpression.match(nextLine).hasMatch();
            const bool isH2 = h2UnderlineExpression.match(nextLine).hasMatch();
            if ((isH1 || isH2) && !line.trimmed().isEmpty()) {
                NavigationOutlineItemData item;
                item.level = isH1 ? 1 : 2;
                item.title = stripMarkdown(line);
                item.position = position;
                item.line = lineNumber;
                if (!item.title.isEmpty()) {
                    headings.append(item);
                }
            }
        }
        position += line.length() + 1;
        ++lineNumber;
    }

    _model.setHeadings(headings);
}

void NavigationViewModel::selectHeading(int position) {
    emit headingSelected(position);
}
