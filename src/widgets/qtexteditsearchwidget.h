#pragma once

#include <QTextEdit>
#include <QTimer>
#include <QWidget>

class QLineEdit;

class QTextEditSearchWidget : public QWidget {
    Q_OBJECT

public:
    enum SearchMode { PlainTextMode, WholeWordsMode, RegularExpressionMode };

    explicit QTextEditSearchWidget(QTextEdit* parent = nullptr);
    ~QTextEditSearchWidget() override;

    bool doSearch(bool searchDown = true, bool allowRestartAtTop = true);
    void setDarkMode(bool enabled);
    void setReplaceEnabled(bool enabled);

public slots:
    void activate();
    void deactivate();
    void doSearchDown();
    void doSearchUp();
    void setReplaceMode(bool enabled);
    void activateReplace();
    bool doReplace(bool forAll = false);
    void doReplaceAll();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void searchLineEditTextChanged(const QString& text);

private:
    QTextEdit* _textEdit = nullptr;
    QLineEdit* _searchEdit = nullptr;
    QLineEdit* _replaceEdit = nullptr;
    QTimer _debounceTimer;
    bool _darkMode = false;
    bool _replaceEnabled = false;
};
