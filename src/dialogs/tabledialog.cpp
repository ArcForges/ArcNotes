#include "tabledialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "filedialog.h"
#include "libraries/qtcsv/src/include/reader.h"

TableDialog::TableDialog(QWidget* parent) : TableDialog(nullptr, parent) {}

TableDialog::TableDialog(QPlainTextEdit* textEdit, QWidget* parent) : MasterDialog(parent), _targetTextEdit(textEdit) {
    buildUi();
    afterSetupUI();
    setIgnoreReturnKey(true);

    _tabWidget->setCurrentIndex(Tab::CreateTab);
    _createTableWidget->setColumnCount(50);
    _createTableWidget->setRowCount(100);
    _csvFileTextEdit->setVisible(false);
}

TableDialog::~TableDialog() {
    delete _tempFile;
}

void TableDialog::buildUi() {
    setWindowTitle(tr("Insert table"));
    resize(749, 647);

    auto* layout = new QGridLayout(this);
    _tabWidget = new QTabWidget(this);
    layout->addWidget(_tabWidget, 0, 0);

    auto* createTab = new QWidget(_tabWidget);
    auto* createLayout = new QGridLayout(createTab);
    auto* layoutLabel = new QLabel(tr("Select table layout or enter text:"), createTab);
    _createTableWidget = new QTableWidget(createTab);
    _createTableWidget->horizontalHeader()->setVisible(false);
    _createTableWidget->horizontalHeader()->setDefaultSectionSize(40);
    _createTableWidget->verticalHeader()->setVisible(false);
    _createTableWidget->verticalHeader()->setDefaultSectionSize(40);
    auto* rowLabel = new QLabel(tr("Rows:"), createTab);
    _rowSpinBox = new QSpinBox(createTab);
    _rowSpinBox->setRange(1, 100000000);
    _rowSpinBox->setValue(4);
    auto* columnLabel = new QLabel(tr("Columns:"), createTab);
    _columnSpinBox = new QSpinBox(createTab);
    _columnSpinBox->setRange(1, 100000000);
    _columnSpinBox->setValue(3);
    auto* widthLabel = new QLabel(tr("Column width:"), createTab);
    _columnWidthSpinBox = new QSpinBox(createTab);
    _columnWidthSpinBox->setToolTip(tr("The amount of spaces in a column"));
    _columnWidthSpinBox->setRange(0, 100000000);
    _columnWidthSpinBox->setValue(2);
    _headlineCheckBox = new QCheckBox(tr("Insert a table heading separator"), createTab);
    _headlineCheckBox->setChecked(true);
    _separatorColumnWidthLabel = new QLabel(tr("Separator column width:"), createTab);
    _separatorColumnWidthSpinBox = new QSpinBox(createTab);
    _separatorColumnWidthSpinBox->setToolTip(tr("The amount of dash characters in the separator"));
    _separatorColumnWidthSpinBox->setRange(3, 100000000);
    _separatorColumnWidthSpinBox->setValue(3);

    createLayout->addWidget(layoutLabel, 0, 0, 1, 2);
    createLayout->addWidget(_createTableWidget, 1, 0, 1, 2);
    createLayout->addWidget(rowLabel, 2, 0);
    createLayout->addWidget(_rowSpinBox, 2, 1);
    createLayout->addWidget(columnLabel, 3, 0);
    createLayout->addWidget(_columnSpinBox, 3, 1);
    createLayout->addWidget(widthLabel, 4, 0);
    createLayout->addWidget(_columnWidthSpinBox, 4, 1);
    createLayout->addWidget(_headlineCheckBox, 5, 0, 1, 2);
    createLayout->addWidget(_separatorColumnWidthLabel, 6, 0);
    createLayout->addWidget(_separatorColumnWidthSpinBox, 6, 1);
    createLayout->setRowStretch(1, 1);
    _tabWidget->addTab(createTab, tr("Create table"));

    auto* importTab = new QWidget(_tabWidget);
    auto* importLayout = new QGridLayout(importTab);
    auto* selectCsvLabel = new QLabel(tr("Please select the CSV file you want to import:"), importTab);
    _fileLineEdit = new QLineEdit(importTab);
    _fileLineEdit->setReadOnly(true);
    _fileLineEdit->setPlaceholderText(tr("CSV file"));
    auto* fileButton = new QPushButton(tr("Select file"), importTab);
    fileButton->setIcon(QIcon::fromTheme(QStringLiteral("document-open"),
                                         QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/document-open.svg"))));
    auto* clipboardButton = new QPushButton(tr("Import clipboard"), importTab);
    clipboardButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-paste"),
                                              QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/edit-paste.svg"))));
    _csvFileTextEdit = new QPlainTextEdit(importTab);
    _csvFileTextEdit->setUndoRedoEnabled(false);
    _csvFileTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    _csvFileTextEdit->setReadOnly(true);
    _firstLineHeadlineCheckBox = new QCheckBox(tr("First line is the table heading"), importTab);
    _firstLineHeadlineCheckBox->setChecked(true);
    auto* separatorLabel = new QLabel(tr("Separator:"), importTab);
    _separatorComboBox = new QComboBox(importTab);
    _separatorComboBox->setEditable(true);
    _separatorComboBox->addItems({QStringLiteral(","), QStringLiteral(";"), QStringLiteral("\\t")});
    auto* textDelimiterLabel = new QLabel(tr("Text-delimiter:"), importTab);
    _textDelimiterComboBox = new QComboBox(importTab);
    _textDelimiterComboBox->setEditable(true);
    _textDelimiterComboBox->addItems({QStringLiteral("\""), QStringLiteral("'")});

    importLayout->addWidget(selectCsvLabel, 0, 0, 1, 4);
    importLayout->addWidget(_fileLineEdit, 1, 0, 1, 2);
    importLayout->addWidget(fileButton, 1, 2);
    importLayout->addWidget(clipboardButton, 1, 3);
    importLayout->addWidget(_csvFileTextEdit, 2, 0, 1, 4);
    importLayout->addWidget(_firstLineHeadlineCheckBox, 3, 0, 1, 4);
    importLayout->addWidget(separatorLabel, 4, 0, 1, 3);
    importLayout->addWidget(_separatorComboBox, 4, 3);
    importLayout->addWidget(textDelimiterLabel, 5, 0, 1, 3);
    importLayout->addWidget(_textDelimiterComboBox, 5, 3);
    importLayout->setRowStretch(6, 1);
    _tabWidget->addTab(importTab, tr("Import CSV file / clipboard"));

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox, 1, 0);

    connect(_createTableWidget, &QTableWidget::itemSelectionChanged, this,
            &TableDialog::on_createTableWidget_itemSelectionChanged);
    connect(_createTableWidget, &QTableWidget::itemChanged, this, &TableDialog::on_createTableWidget_itemChanged);
    connect(_headlineCheckBox, &QCheckBox::toggled, this, &TableDialog::on_headlineCheckBox_toggled);
    connect(fileButton, &QPushButton::clicked, this, &TableDialog::on_fileButton_clicked);
    connect(clipboardButton, &QPushButton::clicked, this, &TableDialog::on_clipboardButton_clicked);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        on_buttonBox_accepted();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    on_headlineCheckBox_toggled(_headlineCheckBox->isChecked());
}

void TableDialog::insertMarkdown(const QString& text) {
    if (_targetTextEdit == nullptr || text.isEmpty()) {
        return;
    }

    _targetTextEdit->textCursor().insertText(text);
    _targetTextEdit->setFocus();
}

void TableDialog::on_createTableWidget_itemSelectionChanged() {
    updateMaxItems();
    const QList<QTableWidgetSelectionRange> ranges = _createTableWidget->selectedRanges();
    if (ranges.isEmpty()) {
        return;
    }

    const QTableWidgetSelectionRange range = ranges.constFirst();
    _rowSpinBox->setValue(std::max<int>(_maxRows, range.rowCount()));
    _columnSpinBox->setValue(std::max<int>(_maxColumns, range.columnCount()));
}

void TableDialog::updateMaxItems() {
    for (int row = 0; row < _createTableWidget->rowCount(); ++row) {
        for (int col = 0; col < _createTableWidget->columnCount(); ++col) {
            const auto* item = _createTableWidget->item(row, col);
            const bool hasText = item != nullptr && !item->text().isEmpty();

            if (hasText) {
                _maxRows = std::max<int>(_maxRows, row + 1);
                _maxColumns = std::max<int>(_maxColumns, col + 1);
            }
        }
    }
}

void TableDialog::on_buttonBox_accepted() {
    switch (_tabWidget->currentIndex()) {
        case Tab::ImportTab:
            importCSV();
            break;

        case Tab::CreateTab:
        default:
            createMarkdownTable();
            break;
    }
}

void TableDialog::importCSV() {
    const QString filePath = _tempFile != nullptr ? _tempFile->fileName() : _fileLineEdit->text();

    if (filePath.isEmpty()) {
        return;
    }

    QString text = QStringLiteral("\n\n");
    QString separator = _separatorComboBox->currentText();
    separator.replace(QStringLiteral("\\t"), QStringLiteral("\t"));

    const QList<QStringList> readData =
        QtCSV::Reader::readToList(filePath, separator, _textDelimiterComboBox->currentText());

    for (int row = 0; row < readData.size(); ++row) {
        const QStringList& rowData = readData.at(row);
        text += QStringLiteral("| ") + rowData.join(QStringLiteral(" | ")) + QStringLiteral(" |\n");

        if (row == 0 && _firstLineHeadlineCheckBox->isChecked()) {
            for (int col = 0; col < rowData.count(); ++col) {
                text += QLatin1String("| --- ");
            }
            text += QLatin1String("|\n");
        }
    }

    insertMarkdown(text);
}

void TableDialog::createMarkdownTable() {
    if (_rowSpinBox->value() == 0 || _columnSpinBox->value() == 0) {
        return;
    }

    QString text = QStringLiteral("\n\n");
    const int colWidth = _columnWidthSpinBox->value();
    const QString space = QStringLiteral(" ").repeated(colWidth);
    const QString headline = QStringLiteral("-").repeated(_separatorColumnWidthSpinBox->value());

    for (int row = 0; row < _rowSpinBox->value(); ++row) {
        for (int col = 0; col < _columnSpinBox->value(); ++col) {
            const auto* item = _createTableWidget->item(row, col);
            const QString itemText = item != nullptr ? item->text() : QString();
            text +=
                QStringLiteral("|") + (itemText.isEmpty() ? space : itemText.leftJustified(colWidth, QLatin1Char(' ')));
        }

        text += QStringLiteral("|\n");

        if (row == 0 && _headlineCheckBox->isChecked()) {
            for (int col = 0; col < _columnSpinBox->value(); ++col) {
                text += QStringLiteral("|") + headline;
            }
            text += QStringLiteral("|\n");
        }
    }

    insertMarkdown(text);
}

void TableDialog::on_headlineCheckBox_toggled(bool checked) {
    _separatorColumnWidthSpinBox->setVisible(checked);
    _separatorColumnWidthLabel->setVisible(checked);
}

void TableDialog::on_fileButton_clicked() {
    _csvFileTextEdit->clear();
    QStringList filters = QStringList() << tr("CSV files") + QStringLiteral(" (*.csv)")
                                        << tr("All files") + QStringLiteral(" (*)");
    FileDialog dialog(QStringLiteral("CSVTableImport"));
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setNameFilters(filters);
    dialog.setWindowTitle(tr("Select CSV file to import"));

    if (dialog.exec() == QDialog::Accepted) {
        const QString fileName = dialog.selectedFile();

        if (!fileName.isEmpty()) {
            _fileLineEdit->setText(fileName);

            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qCritical() << file.errorString();
                return;
            }

            _csvFileTextEdit->show();
            const QByteArray text = file.readAll();
            _csvFileTextEdit->setPlainText(QString::fromUtf8(text));
            updateSeparator(QString::fromUtf8(text));

            delete _tempFile;
            _tempFile = nullptr;
        }
    }
}

void TableDialog::on_createTableWidget_itemChanged(QTableWidgetItem* item) {
    if (item == nullptr) {
        return;
    }

    const int columns = item->column() + 1;
    if (columns > _columnSpinBox->value()) {
        _columnSpinBox->setValue(columns);
    }

    const int rows = item->row() + 1;
    if (rows > _rowSpinBox->value()) {
        _rowSpinBox->setValue(rows);
    }

    const int length = item->text().length();
    if (length > _columnWidthSpinBox->value()) {
        _columnWidthSpinBox->setValue(length);
        _separatorColumnWidthSpinBox->setValue(length);
    }
}

void TableDialog::on_clipboardButton_clicked() {
    QClipboard* clipboard = QApplication::clipboard();
    const QString text = clipboard->text().trimmed();

    if (text.isEmpty()) {
        return;
    }

    _fileLineEdit->clear();
    _csvFileTextEdit->show();
    _csvFileTextEdit->setPlainText(text);

    delete _tempFile;
    _tempFile = new QTemporaryFile(QDir::tempPath() + QDir::separator() + QStringLiteral("table-XXXXXX.csv"));

    if (!_tempFile->open()) {
        delete _tempFile;
        _tempFile = nullptr;
        return;
    }

    updateSeparator(text);
    _tempFile->write(text.toUtf8());
    _tempFile->close();
}

void TableDialog::updateSeparator(const QString& text) const {
    const QStringList characters = {QStringLiteral("\t"), QStringLiteral(";"), QStringLiteral(",")};

    for (const QString& character : characters) {
        if (text.contains(character)) {
            _separatorComboBox->setCurrentText(character == QStringLiteral("\t") ? QStringLiteral("\\t") : character);
            return;
        }
    }
}
