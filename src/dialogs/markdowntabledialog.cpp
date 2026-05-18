#include "markdowntabledialog.h"

#include <utils/gui.h>

#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTableWidget>
#include <QTextBlock>
#include <QTextCursor>
#include <QVBoxLayout>

MarkdownTableDialog::MarkdownTableDialog(QPlainTextEdit* textEdit, QWidget* parent)
    : MasterDialog(parent), _textEdit(textEdit) {
    buildUi();
    afterSetupUI();
    setIgnoreReturnKey(true);

    _tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    _tableWidget->horizontalHeader()->setStretchLastSection(true);
    _tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed |
                                  QAbstractItemView::AnyKeyPressed);

    connect(_tableWidget->horizontalHeader(), &QHeaderView::sectionClicked, this,
            &MarkdownTableDialog::onColumnHeaderClicked);

    loadTableFromCursor();
}

MarkdownTableDialog::~MarkdownTableDialog() = default;

void MarkdownTableDialog::buildUi() {
    setWindowTitle(tr("Edit Markdown table"));
    resize(820, 580);

    auto* layout = new QVBoxLayout(this);
    _tableWidget = new QTableWidget(this);
    _tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    layout->addWidget(_tableWidget);

    auto* controlsLayout = new QHBoxLayout();

    auto* rowGroupBox = new QGroupBox(tr("Row"), this);
    auto* rowLayout = new QHBoxLayout(rowGroupBox);
    auto* addRowButton = new QPushButton(tr("Add row"), rowGroupBox);
    addRowButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add"),
                                           QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/list-add.svg"))));
    auto* removeRowButton = new QPushButton(tr("Remove row"), rowGroupBox);
    removeRowButton->setIcon(QIcon::fromTheme(QStringLiteral("list-remove"),
                                              QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/list-remove.svg"))));
    rowLayout->addWidget(addRowButton);
    rowLayout->addWidget(removeRowButton);
    controlsLayout->addWidget(rowGroupBox);

    auto* columnGroupBox = new QGroupBox(tr("Column"), this);
    auto* columnLayout = new QHBoxLayout(columnGroupBox);
    auto* addColumnButton = new QPushButton(tr("Add column"), columnGroupBox);
    addColumnButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add"),
                                              QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/list-add.svg"))));
    auto* removeColumnButton = new QPushButton(tr("Remove column"), columnGroupBox);
    removeColumnButton->setIcon(QIcon::fromTheme(
        QStringLiteral("list-remove"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/list-remove.svg"))));
    auto* alignmentLabel = new QLabel(tr("Alignment:"), columnGroupBox);
    _alignmentComboBox = new QComboBox(columnGroupBox);
    _alignmentComboBox->setToolTip(tr("Alignment of the selected column. Click a column header to select it."));
    _alignmentComboBox->addItems({tr("Default (left)"), tr("Left"), tr("Center"), tr("Right")});
    auto* applyAlignmentButton = new QPushButton(tr("Apply alignment"), columnGroupBox);
    applyAlignmentButton->setToolTip(
        tr("Apply the selected alignment to the currently selected column(s). Click a column "
           "header first to select it."));
    columnLayout->addWidget(addColumnButton);
    columnLayout->addWidget(removeColumnButton);
    columnLayout->addWidget(alignmentLabel);
    columnLayout->addWidget(_alignmentComboBox);
    columnLayout->addWidget(applyAlignmentButton);
    controlsLayout->addWidget(columnGroupBox);
    layout->addLayout(controlsLayout);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(addRowButton, &QPushButton::clicked, this, &MarkdownTableDialog::on_addRowButton_clicked);
    connect(removeRowButton, &QPushButton::clicked, this, &MarkdownTableDialog::on_removeRowButton_clicked);
    connect(addColumnButton, &QPushButton::clicked, this, &MarkdownTableDialog::on_addColumnButton_clicked);
    connect(removeColumnButton, &QPushButton::clicked, this, &MarkdownTableDialog::on_removeColumnButton_clicked);
    connect(applyAlignmentButton, &QPushButton::clicked, this, &MarkdownTableDialog::on_applyAlignmentButton_clicked);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        on_buttonBox_accepted();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}

void MarkdownTableDialog::loadTableFromCursor() {
    if (_textEdit == nullptr) {
        return;
    }

    QTextCursor cursor = _textEdit->textCursor();
    QTextBlock block = cursor.block();

    if (!block.text().trimmed().startsWith(QLatin1String("|"))) {
        return;
    }

    while (true) {
        QTextBlock prev = block.previous();
        if (!prev.isValid() || !prev.text().trimmed().startsWith(QLatin1String("|"))) {
            break;
        }
        block = prev;
    }

    _tableStartPosition = block.position();

    QList<QStringList> rows;
    QTextBlock current = block;
    int endPos = block.position();

    while (current.isValid() && current.text().trimmed().startsWith(QLatin1String("|"))) {
        const QString line = current.text().trimmed();
        rows << line.split(QLatin1Char('|'));
        endPos = current.position() + current.text().size();
        current = current.next();
    }

    _tableEndPosition = endPos;
    _colAlignments.clear();
    static const QRegularExpression sepRe(QStringLiteral(R"(^(:)?-+(:)?$)"));

    if (rows.size() >= 2) {
        const QStringList& sepRow = rows.at(1);
        for (int col = 1; col < sepRow.size() - 1; ++col) {
            const QString text = sepRow.at(col).trimmed();
            if (!sepRe.match(text).hasMatch()) {
                _colAlignments << Qt::AlignLeft;
                continue;
            }
            const bool left = text.startsWith(QLatin1Char(':'));
            const bool right = text.endsWith(QLatin1Char(':'));
            if (left && right) {
                _colAlignments << Qt::AlignHCenter;
            } else if (right) {
                _colAlignments << Qt::AlignRight;
            } else {
                _colAlignments << Qt::AlignLeft;
            }
        }
    }

    populateGridFromParsed(rows);
}

void MarkdownTableDialog::populateGridFromParsed(const QList<QStringList>& rows) {
    if (rows.isEmpty()) {
        return;
    }

    int maxCols = 0;
    for (const QStringList& row : rows) {
        maxCols = std::max(maxCols, static_cast<int>(row.size()) - 2);
    }

    if (maxCols <= 0) {
        return;
    }

    QList<QStringList> displayRows;
    for (int i = 0; i < rows.size(); ++i) {
        if (i != 1) {
            displayRows << rows.at(i);
        }
    }

    _tableWidget->setRowCount(displayRows.size());
    _tableWidget->setColumnCount(maxCols);

    for (int row = 0; row < displayRows.size(); ++row) {
        const QStringList& parts = displayRows.at(row);
        for (int col = 0; col < maxCols; ++col) {
            QString cellText;
            const int partIndex = col + 1;
            if (partIndex < parts.size() - 1) {
                cellText = parts.at(partIndex).trimmed();
            }
            _tableWidget->setItem(row, col, new QTableWidgetItem(cellText));
        }
    }

    while (_colAlignments.size() < maxCols) {
        _colAlignments << Qt::AlignLeft;
    }
    if (_colAlignments.size() > maxCols) {
        _colAlignments.resize(maxCols);
    }

    updateColumnHeaderLabels();
    _tableWidget->resizeColumnsToContents();
}

void MarkdownTableDialog::updateColumnHeaderLabels() {
    QStringList labels;
    for (int col = 0; col < _tableWidget->columnCount(); ++col) {
        const Qt::AlignmentFlag align = _colAlignments.value(col, Qt::AlignLeft);
        QString indicator = QStringLiteral(" L");
        if (align == Qt::AlignHCenter) {
            indicator = QStringLiteral(" C");
        } else if (align == Qt::AlignRight) {
            indicator = QStringLiteral(" R");
        } else if (align == static_cast<Qt::AlignmentFlag>(0)) {
            indicator = QStringLiteral(" D");
        }
        labels << QString::number(col + 1) + indicator;
    }
    _tableWidget->setHorizontalHeaderLabels(labels);
}

void MarkdownTableDialog::onColumnHeaderClicked(int logicalIndex) {
    _tableWidget->selectColumn(logicalIndex);

    const Qt::AlignmentFlag align = _colAlignments.value(logicalIndex, static_cast<Qt::AlignmentFlag>(0));
    int comboIndex = 0;
    if (align == Qt::AlignLeft) {
        comboIndex = 1;
    } else if (align == Qt::AlignHCenter) {
        comboIndex = 2;
    } else if (align == Qt::AlignRight) {
        comboIndex = 3;
    }
    _alignmentComboBox->setCurrentIndex(comboIndex);
}

int MarkdownTableDialog::currentColumn() const {
    const QList<QTableWidgetSelectionRange> ranges = _tableWidget->selectedRanges();
    if (!ranges.isEmpty()) {
        return ranges.constFirst().leftColumn();
    }
    const QTableWidgetItem* current = _tableWidget->currentItem();
    if (current != nullptr) {
        return current->column();
    }
    return -1;
}

QString MarkdownTableDialog::buildMarkdownTable() const {
    const int rows = _tableWidget->rowCount();
    const int cols = _tableWidget->columnCount();

    if (rows == 0 || cols == 0) {
        return {};
    }

    QVector<int> colWidths(cols, 3);
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const QTableWidgetItem* item = _tableWidget->item(row, col);
            const int length = item != nullptr ? item->text().length() : 0;
            colWidths[col] = std::max(colWidths.at(col), length);
        }
    }

    auto buildRow = [&](int row) {
        QString line = QStringLiteral("|");
        for (int col = 0; col < cols; ++col) {
            const QTableWidgetItem* item = _tableWidget->item(row, col);
            const QString text = item != nullptr ? item->text() : QString();
            const Qt::AlignmentFlag align = _colAlignments.value(col, Qt::AlignLeft);
            const int width = colWidths.at(col);
            QString cell;
            if (align == Qt::AlignHCenter) {
                const int pad = width - text.length();
                const int leftPad = pad / 2;
                const int rightPad = pad - leftPad;
                cell = QString(leftPad, QLatin1Char(' ')) + text + QString(rightPad, QLatin1Char(' '));
            } else if (align == Qt::AlignRight) {
                cell = text.rightJustified(width);
            } else {
                cell = text.leftJustified(width);
            }
            line += QStringLiteral(" ") + cell + QStringLiteral(" |");
        }
        return line;
    };

    QString result = buildRow(0) + QStringLiteral("\n");

    QString separator = QStringLiteral("|");
    for (int col = 0; col < cols; ++col) {
        const Qt::AlignmentFlag align = _colAlignments.value(col, Qt::AlignLeft);
        QString dashes = QString(colWidths.at(col), QLatin1Char('-'));
        if (align == Qt::AlignHCenter) {
            separator += QStringLiteral(" :") + dashes + QStringLiteral(": |");
        } else if (align == Qt::AlignRight) {
            separator += QStringLiteral(" ") + dashes + QStringLiteral(": |");
        } else if (align == Qt::AlignLeft) {
            separator += QStringLiteral(" :") + dashes + QStringLiteral("- |");
        } else {
            separator += QStringLiteral(" ") + dashes + QStringLiteral(" |");
        }
    }
    result += separator + QStringLiteral("\n");

    for (int row = 1; row < rows; ++row) {
        result += buildRow(row);
        if (row < rows - 1) {
            result += QStringLiteral("\n");
        }
    }

    return result;
}

void MarkdownTableDialog::on_buttonBox_accepted() {
    if (_textEdit == nullptr) {
        return;
    }

    const QString newTable = buildMarkdownTable();
    if (newTable.isEmpty()) {
        return;
    }

    QTextCursor cursor = _textEdit->textCursor();
    cursor.setPosition(_tableStartPosition);
    cursor.setPosition(_tableEndPosition, QTextCursor::KeepAnchor);
    cursor.insertText(newTable);
    _textEdit->setTextCursor(cursor);
    Utils::Gui::autoFormatTableAtCursor(_textEdit);
}

void MarkdownTableDialog::on_addRowButton_clicked() {
    const int cols = _tableWidget->columnCount();
    const int newRow = _tableWidget->rowCount();
    _tableWidget->insertRow(newRow);
    for (int col = 0; col < cols; ++col) {
        _tableWidget->setItem(newRow, col, new QTableWidgetItem());
    }
}

void MarkdownTableDialog::on_removeRowButton_clicked() {
    const int row = _tableWidget->currentRow();
    if (row < 0) {
        return;
    }
    if (row == 0) {
        QMessageBox::information(this, tr("Cannot remove header"), tr("The header row cannot be removed."));
        return;
    }
    if (_tableWidget->rowCount() <= 2) {
        QMessageBox::information(this, tr("Cannot remove row"), tr("The table must have at least one data row."));
        return;
    }
    _tableWidget->removeRow(row);
}

void MarkdownTableDialog::on_addColumnButton_clicked() {
    const int newCol = _tableWidget->columnCount();
    _tableWidget->insertColumn(newCol);
    _colAlignments << Qt::AlignLeft;
    for (int row = 0; row < _tableWidget->rowCount(); ++row) {
        _tableWidget->setItem(row, newCol, new QTableWidgetItem());
    }
    updateColumnHeaderLabels();
}

void MarkdownTableDialog::on_removeColumnButton_clicked() {
    const int col = currentColumn();
    if (col < 0) {
        return;
    }
    if (_tableWidget->columnCount() <= 1) {
        QMessageBox::information(this, tr("Cannot remove column"), tr("The table must have at least one column."));
        return;
    }
    _tableWidget->removeColumn(col);
    if (col < _colAlignments.size()) {
        _colAlignments.removeAt(col);
    }
    updateColumnHeaderLabels();
}

void MarkdownTableDialog::on_applyAlignmentButton_clicked() {
    QSet<int> selectedCols;
    const QList<QTableWidgetSelectionRange> ranges = _tableWidget->selectedRanges();
    for (const QTableWidgetSelectionRange& range : ranges) {
        for (int col = range.leftColumn(); col <= range.rightColumn(); ++col) {
            selectedCols.insert(col);
        }
    }

    if (selectedCols.isEmpty()) {
        const int col = currentColumn();
        if (col >= 0) {
            selectedCols.insert(col);
        }
    }

    if (selectedCols.isEmpty()) {
        return;
    }

    Qt::AlignmentFlag align = Qt::AlignLeft;
    switch (_alignmentComboBox->currentIndex()) {
        case 1:
            align = Qt::AlignLeft;
            break;
        case 2:
            align = Qt::AlignHCenter;
            break;
        case 3:
            align = Qt::AlignRight;
            break;
        default:
            align = static_cast<Qt::AlignmentFlag>(0);
            break;
    }

    for (int col : selectedCols) {
        if (col < _colAlignments.size()) {
            _colAlignments[col] = align;
        }
    }

    updateColumnHeaderLabels();
}
