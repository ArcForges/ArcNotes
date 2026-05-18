#include "qtexteditsearchwidget.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCursor>

QTextEditSearchWidget::QTextEditSearchWidget(QTextEdit* parent) : QWidget(parent), _textEdit(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    _searchEdit = new QLineEdit(this);
    _searchEdit->setPlaceholderText(tr("Search"));
    _searchEdit->installEventFilter(this);
    layout->addWidget(_searchEdit);

    auto* previousButton = new QPushButton(tr("Previous"), this);
    connect(previousButton, &QPushButton::clicked, this, &QTextEditSearchWidget::doSearchUp);
    layout->addWidget(previousButton);

    auto* nextButton = new QPushButton(tr("Next"), this);
    connect(nextButton, &QPushButton::clicked, this, &QTextEditSearchWidget::doSearchDown);
    layout->addWidget(nextButton);

    _replaceEdit = new QLineEdit(this);
    _replaceEdit->setPlaceholderText(tr("Replace"));
    _replaceEdit->hide();
    layout->addWidget(_replaceEdit);

    auto* replaceButton = new QPushButton(tr("Replace"), this);
    connect(replaceButton, &QPushButton::clicked, this, [this] { doReplace(false); });
    layout->addWidget(replaceButton);

    connect(_searchEdit, &QLineEdit::textChanged, this, &QTextEditSearchWidget::searchLineEditTextChanged);
    connect(&_debounceTimer, &QTimer::timeout, this, &QTextEditSearchWidget::doSearchDown);
    _debounceTimer.setSingleShot(true);

    setVisible(false);
}

QTextEditSearchWidget::~QTextEditSearchWidget() = default;

void QTextEditSearchWidget::setReplaceEnabled(bool enabled) {
    _replaceEnabled = enabled;
    if (_replaceEdit != nullptr) {
        _replaceEdit->setVisible(enabled);
    }
}

void QTextEditSearchWidget::activate() {
    show();
    if (_searchEdit != nullptr) {
        _searchEdit->setFocus();
        _searchEdit->selectAll();
    }
}

void QTextEditSearchWidget::activateReplace() {
    setReplaceMode(true);
    activate();
}

void QTextEditSearchWidget::deactivate() {
    hide();
}

void QTextEditSearchWidget::setReplaceMode(bool enabled) {
    setReplaceEnabled(enabled);
}

bool QTextEditSearchWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == _searchEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            deactivate();
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void QTextEditSearchWidget::searchLineEditTextChanged(const QString&) {
    _debounceTimer.start(150);
}

void QTextEditSearchWidget::doSearchUp() {
    doSearch(false);
}

void QTextEditSearchWidget::doSearchDown() {
    doSearch(true);
}

bool QTextEditSearchWidget::doReplace(bool forAll) {
    if (_textEdit == nullptr || _searchEdit == nullptr || _replaceEdit == nullptr || _searchEdit->text().isEmpty()) {
        return false;
    }

    bool replaced = false;
    do {
        QTextCursor cursor = _textEdit->textCursor();
        if (!cursor.hasSelection() && !doSearch(true, false)) {
            break;
        }

        cursor = _textEdit->textCursor();
        if (cursor.hasSelection()) {
            cursor.insertText(_replaceEdit->text());
            replaced = true;
        }
    } while (forAll && doSearch(true, false));

    return replaced;
}

void QTextEditSearchWidget::doReplaceAll() {
    doReplace(true);
}

bool QTextEditSearchWidget::doSearch(bool searchDown, bool allowRestartAtTop) {
    if (_textEdit == nullptr || _searchEdit == nullptr || _searchEdit->text().isEmpty()) {
        return false;
    }

    QTextDocument::FindFlags flags;
    if (!searchDown) {
        flags |= QTextDocument::FindBackward;
    }

    if (_textEdit->find(_searchEdit->text(), flags)) {
        return true;
    }

    if (!allowRestartAtTop) {
        return false;
    }

    QTextCursor cursor = _textEdit->textCursor();
    cursor.movePosition(searchDown ? QTextCursor::Start : QTextCursor::End);
    _textEdit->setTextCursor(cursor);
    return _textEdit->find(_searchEdit->text(), flags);
}

void QTextEditSearchWidget::setDarkMode(bool enabled) {
    _darkMode = enabled;
}
