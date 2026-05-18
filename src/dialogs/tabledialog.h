#pragma once

#include <QTemporaryFile>

#include "masterdialog.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QTableWidget;
class QTableWidgetItem;
class QTabWidget;

class TableDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit TableDialog(QWidget* parent = nullptr);
    explicit TableDialog(QPlainTextEdit* textEdit, QWidget* parent = nullptr);
    ~TableDialog() override;

private slots:
    void on_createTableWidget_itemSelectionChanged();
    void on_buttonBox_accepted();
    void on_headlineCheckBox_toggled(bool checked);
    void on_fileButton_clicked();
    void on_createTableWidget_itemChanged(QTableWidgetItem* item);
    void on_clipboardButton_clicked();

private:
    enum Tab { CreateTab, ImportTab };

    QPlainTextEdit* _targetTextEdit = nullptr;
    QTabWidget* _tabWidget = nullptr;
    QTableWidget* _createTableWidget = nullptr;
    QSpinBox* _rowSpinBox = nullptr;
    QSpinBox* _columnSpinBox = nullptr;
    QSpinBox* _columnWidthSpinBox = nullptr;
    QSpinBox* _separatorColumnWidthSpinBox = nullptr;
    QLabel* _separatorColumnWidthLabel = nullptr;
    QCheckBox* _headlineCheckBox = nullptr;
    QLineEdit* _fileLineEdit = nullptr;
    QPlainTextEdit* _csvFileTextEdit = nullptr;
    QComboBox* _separatorComboBox = nullptr;
    QComboBox* _textDelimiterComboBox = nullptr;
    QCheckBox* _firstLineHeadlineCheckBox = nullptr;
    int _maxColumns = 0;
    int _maxRows = 0;
    QTemporaryFile* _tempFile = nullptr;

    void buildUi();
    void insertMarkdown(const QString& text);
    void createMarkdownTable();
    void importCSV();
    void updateMaxItems();
    void updateSeparator(const QString& text) const;
};
