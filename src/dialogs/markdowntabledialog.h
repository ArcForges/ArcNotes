#pragma once

#include <QList>
#include <QPlainTextEdit>
#include <QStringList>
#include <QVector>

#include "masterdialog.h"

class QComboBox;
class QTableWidget;

class MarkdownTableDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit MarkdownTableDialog(QPlainTextEdit* textEdit, QWidget* parent = nullptr);
    ~MarkdownTableDialog() override;

private slots:
    void on_buttonBox_accepted();
    void on_addRowButton_clicked();
    void on_removeRowButton_clicked();
    void on_addColumnButton_clicked();
    void on_removeColumnButton_clicked();
    void on_applyAlignmentButton_clicked();
    void onColumnHeaderClicked(int logicalIndex);

private:
    QPlainTextEdit* _textEdit = nullptr;
    QTableWidget* _tableWidget = nullptr;
    QComboBox* _alignmentComboBox = nullptr;
    int _tableStartPosition = 0;
    int _tableEndPosition = 0;
    QVector<Qt::AlignmentFlag> _colAlignments;

    void buildUi();
    void loadTableFromCursor();
    void populateGridFromParsed(const QList<QStringList>& rows);
    [[nodiscard]] QString buildMarkdownTable() const;
    [[nodiscard]] int currentColumn() const;
    void updateColumnHeaderLabels();
};
