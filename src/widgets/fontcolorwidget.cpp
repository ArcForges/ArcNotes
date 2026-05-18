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
 */

#include "fontcolorwidget.h"

#include <dialogs/filedialog.h>
#include <utils/gui.h>
#include <utils/misc.h>
#include <viewmodels/settingsviewmodel.h>

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFontComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QIcon>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUuid>
#include <QVariant>

#include "utils/schema.h"
struct FontColorControls {
    QGroupBox* fontGroupBox = nullptr;
    QFontComboBox* fontFamilyComboBox = nullptr;
    QSpinBox* fontSizeSpinBox = nullptr;
    QGroupBox* colorGroupBox = nullptr;
    QComboBox* colorSchemeComboBox = nullptr;
    QPushButton* copySchemeButton = nullptr;
    QPushButton* deleteSchemeButton = nullptr;
    QPushButton* exportSchemeButton = nullptr;
    QPushButton* importSchemeButton = nullptr;
    QTreeWidget* textTreeWidget = nullptr;
    QFrame* schemeEditFrame = nullptr;
    QCheckBox* fontCheckBox = nullptr;
    QFontComboBox* fontComboBox = nullptr;
    QCheckBox* foregroundColorCheckBox = nullptr;
    QPushButton* foregroundColorButton = nullptr;
    QCheckBox* backgroundColorCheckBox = nullptr;
    QPushButton* backgroundColorButton = nullptr;
    QLabel* label = nullptr;
    QSpinBox* fontSizeAdaptionSpinBox = nullptr;
    QCheckBox* boldCheckBox = nullptr;
    QCheckBox* italicCheckBox = nullptr;
    QCheckBox* underlineCheckBox = nullptr;
    QPushButton* shareSchemaPushButton = nullptr;

    void build(FontColorWidget* parent) {
        parent->setFrameShape(QFrame::NoFrame);
        auto* rootLayout = new QGridLayout(parent);
        rootLayout->setContentsMargins(0, 0, 0, 0);

        fontGroupBox = new QGroupBox(QObject::tr("Text font"), parent);
        auto* fontLayout = new QGridLayout(fontGroupBox);
        fontLayout->addWidget(new QLabel(QObject::tr("Family:"), fontGroupBox), 0, 0);
        fontFamilyComboBox = new QFontComboBox(fontGroupBox);
        fontLayout->addWidget(fontFamilyComboBox, 0, 1);
        fontLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Fixed, QSizePolicy::Minimum), 0, 2);
        fontLayout->addWidget(new QLabel(QObject::tr("Size:"), fontGroupBox), 0, 3);
        fontSizeSpinBox = new QSpinBox(fontGroupBox);
        fontSizeSpinBox->setRange(5, 72);
        fontLayout->addWidget(fontSizeSpinBox, 0, 4);
        fontLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 5);
        rootLayout->addWidget(fontGroupBox, 0, 0);

        colorGroupBox = new QGroupBox(QObject::tr("Color schema"), parent);
        auto* colorLayout = new QGridLayout(colorGroupBox);
        colorSchemeComboBox = new QComboBox(colorGroupBox);
        colorSchemeComboBox->setMaxVisibleItems(15);
        colorLayout->addWidget(colorSchemeComboBox, 0, 0);
        copySchemeButton = new QPushButton(QObject::tr("Copy"), colorGroupBox);
        copySchemeButton->setToolTip(QObject::tr("Copy schema"));
        copySchemeButton->setIcon(QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/edit-copy.svg")));
        colorLayout->addWidget(copySchemeButton, 0, 1);
        deleteSchemeButton = new QPushButton(QObject::tr("Delete"), colorGroupBox);
        deleteSchemeButton->setToolTip(QObject::tr("Delete schema"));
        deleteSchemeButton->setIcon(QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/edit-delete.svg")));
        colorLayout->addWidget(deleteSchemeButton, 0, 2);
        exportSchemeButton = new QPushButton(QObject::tr("Export"), colorGroupBox);
        exportSchemeButton->setToolTip(QObject::tr("Export schema"));
        exportSchemeButton->setIcon(
            QIcon::fromTheme(QStringLiteral("document-export"),
                             QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/document-export.svg"))));
        colorLayout->addWidget(exportSchemeButton, 0, 3);
        importSchemeButton = new QPushButton(QObject::tr("Import"), colorGroupBox);
        importSchemeButton->setToolTip(QObject::tr("Import schema"));
        importSchemeButton->setIcon(
            QIcon::fromTheme(QStringLiteral("document-import"),
                             QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/document-import.svg"))));
        colorLayout->addWidget(importSchemeButton, 0, 4);

        textTreeWidget = new QTreeWidget(colorGroupBox);
        textTreeWidget->setRootIsDecorated(false);
        textTreeWidget->setHeaderHidden(true);
        textTreeWidget->setHeaderLabels({QStringLiteral("1")});
        colorLayout->addWidget(textTreeWidget, 1, 0);

        schemeEditFrame = new QFrame(colorGroupBox);
        schemeEditFrame->setFrameShape(QFrame::NoFrame);
        auto* editLayout = new QGridLayout(schemeEditFrame);
        fontCheckBox = new QCheckBox(QObject::tr("Font:"), schemeEditFrame);
        editLayout->addWidget(fontCheckBox, 1, 1);
        fontComboBox = new QFontComboBox(schemeEditFrame);
        editLayout->addWidget(fontComboBox, 1, 2, 1, 2);
        foregroundColorCheckBox = new QCheckBox(QObject::tr("Foreground:"), schemeEditFrame);
        editLayout->addWidget(foregroundColorCheckBox, 2, 1);
        foregroundColorButton = new QPushButton(schemeEditFrame);
        foregroundColorButton->setStyleSheet(QStringLiteral("* {background: black; border: none}"));
        editLayout->addWidget(foregroundColorButton, 2, 2);
        backgroundColorCheckBox = new QCheckBox(QObject::tr("Background:"), schemeEditFrame);
        editLayout->addWidget(backgroundColorCheckBox, 3, 1);
        backgroundColorButton = new QPushButton(schemeEditFrame);
        backgroundColorButton->setStyleSheet(QStringLiteral("* {background: white; border: none}"));
        editLayout->addWidget(backgroundColorButton, 3, 2);
        label = new QLabel(QObject::tr("Adapt font size:"), schemeEditFrame);
        editLayout->addWidget(label, 4, 1);
        fontSizeAdaptionSpinBox = new QSpinBox(schemeEditFrame);
        fontSizeAdaptionSpinBox->setSuffix(QStringLiteral("%"));
        fontSizeAdaptionSpinBox->setRange(10, 10000);
        fontSizeAdaptionSpinBox->setSingleStep(10);
        fontSizeAdaptionSpinBox->setValue(100);
        editLayout->addWidget(fontSizeAdaptionSpinBox, 4, 2);
        boldCheckBox = new QCheckBox(QObject::tr("Bold"), schemeEditFrame);
        italicCheckBox = new QCheckBox(QObject::tr("Italic"), schemeEditFrame);
        underlineCheckBox = new QCheckBox(QObject::tr("Underline"), schemeEditFrame);
        editLayout->addWidget(boldCheckBox, 5, 1);
        editLayout->addWidget(italicCheckBox, 5, 2);
        editLayout->addWidget(underlineCheckBox, 5, 3);
        editLayout->addItem(new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Preferred), 6, 1, 1, 3);
        auto* shareLabel = new QLabel(
            QObject::tr("If you have created a nice color schema please export it and share it with everyone!"),
            schemeEditFrame);
        shareLabel->setWordWrap(true);
        editLayout->addWidget(shareLabel, 7, 1, 1, 3);
        shareSchemaPushButton = new QPushButton(QObject::tr("Share schema"), schemeEditFrame);
        shareSchemaPushButton->setIcon(QIcon::fromTheme(
            QStringLiteral("share"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/document-share.svg"))));
        editLayout->addWidget(shareSchemaPushButton, 9, 1, 1, 3);
        editLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 10, 2);
        colorLayout->addWidget(schemeEditFrame, 1, 1, 1, 4);
        rootLayout->addWidget(colorGroupBox, 1, 0);
    }
};

FontColorWidget::FontColorWidget(QWidget* parent, SettingsViewModel* settingsViewModel)
    : QFrame(parent), _controls(new FontColorControls), _settingsViewModel(settingsViewModel) {
    _controls->build(this);

    connect(_controls->foregroundColorButton, &QPushButton::clicked, this,
            &FontColorWidget::on_foregroundColorButton_clicked);
    connect(_controls->colorSchemeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &FontColorWidget::on_colorSchemeComboBox_currentIndexChanged);
    connect(_controls->textTreeWidget, &QTreeWidget::currentItemChanged, this,
            &FontColorWidget::on_textTreeWidget_currentItemChanged);
    connect(_controls->copySchemeButton, &QPushButton::clicked, this, &FontColorWidget::on_copySchemeButton_clicked);
    connect(_controls->backgroundColorButton, &QPushButton::clicked, this,
            &FontColorWidget::on_backgroundColorButton_clicked);
    connect(_controls->foregroundColorCheckBox, &QCheckBox::toggled, this,
            &FontColorWidget::on_foregroundColorCheckBox_toggled);
    connect(_controls->backgroundColorCheckBox, &QCheckBox::toggled, this,
            &FontColorWidget::on_backgroundColorCheckBox_toggled);
    connect(_controls->deleteSchemeButton, &QPushButton::clicked, this,
            &FontColorWidget::on_deleteSchemeButton_clicked);
    connect(_controls->boldCheckBox, &QCheckBox::toggled, this, &FontColorWidget::on_boldCheckBox_toggled);
    connect(_controls->italicCheckBox, &QCheckBox::toggled, this, &FontColorWidget::on_italicCheckBox_toggled);
    connect(_controls->underlineCheckBox, &QCheckBox::toggled, this, &FontColorWidget::on_underlineCheckBox_toggled);
    connect(_controls->exportSchemeButton, &QPushButton::clicked, this,
            &FontColorWidget::on_exportSchemeButton_clicked);
    connect(_controls->importSchemeButton, &QPushButton::clicked, this,
            &FontColorWidget::on_importSchemeButton_clicked);
    connect(_controls->fontSizeAdaptionSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &FontColorWidget::on_fontSizeAdaptionSpinBox_valueChanged);
    connect(_controls->shareSchemaPushButton, &QPushButton::clicked, this,
            &FontColorWidget::on_shareSchemaPushButton_clicked);
    connect(_controls->fontCheckBox, &QCheckBox::toggled, this, &FontColorWidget::on_fontCheckBox_toggled);
    connect(_controls->fontComboBox, &QFontComboBox::currentFontChanged, this,
            &FontColorWidget::on_fontComboBox_currentFontChanged);

    // initialize the schema selector
    initSchemaSelector();

    // initialize the text tree widget items
    initTextTreeWidgetItems();

    // we currently are using the font selector from the settings dialog
    _controls->fontGroupBox->setVisible(false);
    applySchemaByIndex(_controls->colorSchemeComboBox->currentIndex());

    // disable the button under macOS for Qt < 5.8 because of a Qt bug
    // see: https://github.com/pbek/ArcNotes/issues/503
    // see: https://bugreports.qt.io/browse/QTBUG-56565
#ifdef Q_OS_MAC
#if (QT_VERSION < QT_VERSION_CHECK(5, 8, 0))
    _controls->copySchemeButton->setEnabled(false);
    _controls->copySchemeButton->setToolTip(
        "Disabled because of a Qt bug, see "
        "https://github.com/pbek/ArcNotes/issues/503");
#endif
#endif

    // initialize the font selectors
    //    initFontSelectors();

    // Schema changes are applied live via the schemaChanged() signal
}

/**
 * Initializes the schema selector
 */
void FontColorWidget::initSchemaSelector() {
    _controls->colorSchemeComboBox->clear();
    _controls->fontComboBox->setEnabled(false);

    //
    // Load the default schemes
    //

    _defaultSchemaKeys = Utils::Schema::schemaSettings->defaultSchemaKeys();
    // QMaps are sorted by key automatically
    QMap<QString, QString> defaultSchemaNameKeys;

    const QSettings& defaultSchemaSettings = Utils::Schema::schemaSettings->defaultSchemaSettings();

    // Gather the default schema names and keys in correctly sorted order
    Q_FOREACH (const QString& schemaKey, _defaultSchemaKeys) {
        QString name = defaultSchemaSettings.value(schemaKey + "/Name").toString();

        // Enforce the light theme to be on top
        if (schemaKey == QStringLiteral("EditorColorSchema-6033d61b-cb96-46d5-a3a8-20d5172017eb")) {
            name = QStringLiteral("  ") + name;
        }

        // Enforce the dark theme to be 2nd
        if (schemaKey == QStringLiteral("EditorColorSchema-cdbf28fc-1ddc-4d13-bb21-6a4043316a2f")) {
            name = QStringLiteral(" ") + name;
        }

        defaultSchemaNameKeys.insert(name, schemaKey);
    }

    QString currentSchemaKey = settingValue(QStringLiteral("Editor/CurrentSchemaKey"),
                                            !_defaultSchemaKeys.empty() ? _defaultSchemaKeys[0] : QString())
                                   .toString();
    int index = 0;
    int currentIndex = 0;

    Q_FOREACH (const QString& schemaKey, defaultSchemaNameKeys.values()) {
        const QString name = defaultSchemaSettings.value(schemaKey + "/Name").toString();
        _controls->colorSchemeComboBox->addItem(name.trimmed(), schemaKey);

        if (currentSchemaKey == schemaKey) {
            currentIndex = index;
        }

        index++;
    }

    //
    // Load the custom schemes
    //
    QStringList schemes = settingValue(QStringLiteral("Editor/ColorSchemes")).toStringList();
    Q_FOREACH (QString schemaKey, schemes) {
        QString name = settingValue(schemaKey + QStringLiteral("/Name")).toString();
        _controls->colorSchemeComboBox->addItem(name, schemaKey);

        if (currentSchemaKey == schemaKey) {
            currentIndex = index;
        }

        index++;
    }

    // Set the current color schema
    _controls->colorSchemeComboBox->setCurrentIndex(currentIndex);
}

FontColorWidget::~FontColorWidget() {
    delete _controls;
}

/**
 * Sets a new foreground color
 */
void FontColorWidget::on_foregroundColorButton_clicked() {
    int index = textSettingsIndex();
    QColor color = Utils::Schema::schemaSettings->getForegroundColor(index);
    QColor newColor = QColorDialog::getColor(color);

    if (!newColor.isValid() || newColor == color) {
        return;
    }

    color = newColor;

    _controls->foregroundColorButton->setStyleSheet(
        QStringLiteral("* {background: %1; border: none}").arg(color.name()));

    setSchemaValue(textSettingsKey(QStringLiteral("ForegroundColor")), color);

    // update the current or all text items, depending on the index
    updateTextItems(index);

    // Notify listeners that the schema has changed so they can apply it live
    Q_EMIT schemaChanged();
}
/**
 * Sets a new background color
 */
void FontColorWidget::on_backgroundColorButton_clicked() {
    int index = textSettingsIndex();
    QColor color = Utils::Schema::schemaSettings->getBackgroundColor(index);
    QColor newColor = QColorDialog::getColor(color);

    if (!newColor.isValid() || newColor == color) {
        return;
    }

    color = newColor;

    _controls->backgroundColorButton->setStyleSheet(
        QStringLiteral("* {background: %1; border: none}").arg(color.name()));

    setSchemaValue(textSettingsKey(QStringLiteral("BackgroundColor")), color);

    // update the current or all text items, depending on the index
    updateTextItems(index);

    // Notify listeners that the schema has changed so they can apply it live
    Q_EMIT schemaChanged();
}

/**
 * Updates the current or all text items, depending on the index
 *
 * @param index
 */
void FontColorWidget::updateTextItems(int index) {
    if (index < 0) {
        // update all text items
        updateAllTextItems();
    } else {
        // update the styling of the current text tree widget item
        updateTextItem();
    }
}

/**
 * Initializes the text tree widget items
 */
void FontColorWidget::initTextTreeWidgetItems() {
    addTextTreeWidgetItem(tr("Text preset"), Utils::Schema::TextPresetIndex);
    addTextTreeWidgetItem(tr("Emphasized text"), MarkdownHighlighter::Italic);
    addTextTreeWidgetItem(tr("Underlined text"), MarkdownHighlighter::StUnderline);
    addTextTreeWidgetItem(tr("Strong text"), MarkdownHighlighter::Bold);
    addTextTreeWidgetItem(tr("Link"), MarkdownHighlighter::Link);
    addTextTreeWidgetItem(tr("Link (internal)"), MarkdownHighlighter::LinkInternal);
    addTextTreeWidgetItem(tr("Wiki link"), MarkdownHighlighter::WikiLink);
    addTextTreeWidgetItem(tr("Wiki link (broken)"), MarkdownHighlighter::WikiLinkBroken);
    addTextTreeWidgetItem(tr("Image"), MarkdownHighlighter::Image);
    addTextTreeWidgetItem(tr("Code (block)"), MarkdownHighlighter::CodeBlock);
    addTextTreeWidgetItem(tr("Code (inline)"), MarkdownHighlighter::InlineCodeBlock);

    addTextTreeWidgetItem(tr("Code (keyword)"), MarkdownHighlighter::CodeKeyWord);
    addTextTreeWidgetItem(tr("Code (type)"), MarkdownHighlighter::CodeType);
    addTextTreeWidgetItem(tr("Code (comment)"), MarkdownHighlighter::CodeComment);
    addTextTreeWidgetItem(tr("Code (string)"), MarkdownHighlighter::CodeString);
    addTextTreeWidgetItem(tr("Code (built in)"), MarkdownHighlighter::CodeBuiltIn);
    addTextTreeWidgetItem(tr("Code (num literal)"), MarkdownHighlighter::CodeNumLiteral);
    addTextTreeWidgetItem(tr("Code (other)"), MarkdownHighlighter::CodeOther);

    addTextTreeWidgetItem(tr("List item"), MarkdownHighlighter::List);
    addTextTreeWidgetItem(tr("Checkbox unchecked"), MarkdownHighlighter::CheckBoxUnChecked);
    addTextTreeWidgetItem(tr("Checkbox checked"), MarkdownHighlighter::CheckBoxChecked);
    addTextTreeWidgetItem(tr("Header, level 1"), MarkdownHighlighter::H1);
    addTextTreeWidgetItem(tr("Header, level 2"), MarkdownHighlighter::H2);
    addTextTreeWidgetItem(tr("Header, level 3"), MarkdownHighlighter::H3);
    addTextTreeWidgetItem(tr("Header, level 4"), MarkdownHighlighter::H4);
    addTextTreeWidgetItem(tr("Header, level 5"), MarkdownHighlighter::H5);
    addTextTreeWidgetItem(tr("Header, level 6"), MarkdownHighlighter::H6);
    addTextTreeWidgetItem(tr("Horizontal rule"), MarkdownHighlighter::HorizontalRule);
    addTextTreeWidgetItem(tr("Block quote"), MarkdownHighlighter::BlockQuote);
    addTextTreeWidgetItem(tr("Table"), MarkdownHighlighter::Table);
    addTextTreeWidgetItem(tr("(HTML) Comment"), MarkdownHighlighter::Comment);
    addTextTreeWidgetItem(tr("Masked syntax",
                             "text that will highlighted in "
                             "a way that it's barely visible"),
                          MarkdownHighlighter::MaskedSyntax);
    addTextTreeWidgetItem(tr("Current line background color"), MarkdownHighlighter::CurrentLineBackgroundColor);
    addTextTreeWidgetItem(tr("Broken link"), MarkdownHighlighter::BrokenLink);
    addTextTreeWidgetItem(tr("Trailing space"), MarkdownHighlighter::TrailingSpace);

    // jump to the top of the list so that an item is selected
    auto* event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier);
    QApplication::postEvent(_controls->textTreeWidget, event);
}

void FontColorWidget::addTextTreeWidgetItem(const QString& text, int index) {
    auto* item = new QTreeWidgetItem();
    item->setText(0, text);
    item->setData(0, Qt::UserRole, index);

    if (index == Utils::Schema::TextPresetIndex) {
        item->setToolTip(0, tr("This item will be used to preset colors for the other items"));
    }

    // update the styling of the text tree widget item
    updateTextItem(item);

    _controls->textTreeWidget->addTopLevelItem(item);
}

/**
 * Shows or hides the wiki link highlighter state items in the text tree widget
 */
void FontColorWidget::setWikiLinkItemsVisible(bool visible) {
    const int count = _controls->textTreeWidget->topLevelItemCount();

    for (int i = 0; i < count; i++) {
        QTreeWidgetItem* item = _controls->textTreeWidget->topLevelItem(i);

        if (item == nullptr) {
            continue;
        }

        const int index = item->data(0, Qt::UserRole).toInt();

        if (index == MarkdownHighlighter::WikiLink || index == MarkdownHighlighter::WikiLinkBroken) {
            item->setHidden(!visible);
        }
    }
}

/**
 * Updates the schema edit frame for the currently selected text item
 */
void FontColorWidget::updateSchemeEditFrame() {
    QTreeWidgetItem* item = _controls->textTreeWidget->currentItem();

    if (item == nullptr) {
        // select a current item if none was selected
        _controls->textTreeWidget->setCurrentItem(_controls->textTreeWidget->topLevelItem(0));
        return;
    }

    int index = textSettingsIndex();

    // check if we are not viewing a default schema
    _controls->schemeEditFrame->setEnabled(!_currentSchemaIsDefault);

    bool enabled =
        Utils::Schema::schemaSettings->getSchemaValue(textSettingsKey(QStringLiteral("FontEnabled"))).toBool();
    updateFontCheckBox(enabled);

    enabled = Utils::Schema::schemaSettings->getSchemaValue(textSettingsKey(QStringLiteral("ForegroundColorEnabled")))
                  .toBool();
    updateForegroundColorCheckBox(enabled);

    QColor color = Utils::Schema::schemaSettings->getForegroundColor(index);
    _controls->foregroundColorButton->setStyleSheet(
        QStringLiteral("* {background: %1; border: none;}").arg(color.name()));

    enabled = Utils::Schema::schemaSettings->getSchemaValue(textSettingsKey(QStringLiteral("BackgroundColorEnabled")))
                  .toBool();
    updateBackgroundColorCheckBox(enabled);

    color = Utils::Schema::schemaSettings->getBackgroundColor(index);
    _controls->backgroundColorButton->setStyleSheet(
        QStringLiteral("* {background: %1; border: none;}").arg(color.name()));

    bool isCurrentLineBackgroundColorIndex = index == MarkdownHighlighter::HighlighterState::CurrentLineBackgroundColor;

    _controls->boldCheckBox->setVisible(index >= 0 && !isCurrentLineBackgroundColorIndex);
    _controls->italicCheckBox->setVisible(index >= 0 && !isCurrentLineBackgroundColorIndex);
    _controls->underlineCheckBox->setVisible(index >= 0 && !isCurrentLineBackgroundColorIndex);
    _controls->fontSizeAdaptionSpinBox->setVisible(index >= 0 && !isCurrentLineBackgroundColorIndex);
    _controls->fontCheckBox->setVisible(index >= 0 && !isCurrentLineBackgroundColorIndex);
    _controls->fontComboBox->setVisible(index >= 0 && !isCurrentLineBackgroundColorIndex);
    _controls->foregroundColorCheckBox->setVisible(!isCurrentLineBackgroundColorIndex);
    _controls->foregroundColorButton->setVisible(!isCurrentLineBackgroundColorIndex);
    _controls->label->setVisible(!isCurrentLineBackgroundColorIndex);
    _controls->fontSizeAdaptionSpinBox->setVisible(!isCurrentLineBackgroundColorIndex);

    if (index >= 0) {
        const QSignalBlocker blocker(_controls->boldCheckBox);
        Q_UNUSED(blocker)

        _controls->boldCheckBox->setChecked(
            Utils::Schema::schemaSettings->getSchemaValue(textSettingsKey(QStringLiteral("Bold"))).toBool());

        const QSignalBlocker blocker2(_controls->italicCheckBox);
        Q_UNUSED(blocker2)

        _controls->italicCheckBox->setChecked(
            Utils::Schema::schemaSettings->getSchemaValue(textSettingsKey(QStringLiteral("Italic"))).toBool());

        const QSignalBlocker blocker3(_controls->underlineCheckBox);
        Q_UNUSED(blocker3)

        _controls->underlineCheckBox->setChecked(
            Utils::Schema::schemaSettings->getSchemaValue(textSettingsKey(QStringLiteral("Underline"))).toBool());

        const QSignalBlocker blocker4(_controls->fontSizeAdaptionSpinBox);
        Q_UNUSED(blocker4)

        _controls->fontSizeAdaptionSpinBox->setValue(
            Utils::Schema::schemaSettings->getSchemaValue(textSettingsKey(QStringLiteral("FontSizeAdaption")), 100)
                .toInt());

        const QSignalBlocker blocker5(_controls->fontComboBox);
        Q_UNUSED(blocker5)

        auto font = Utils::Schema::schemaSettings
                        ->getSchemaValue(textSettingsKey(QStringLiteral("Font")),
                                         Utils::Schema::schemaSettings->getEditorTextFont())
                        .value<QFont>();

        _controls->fontComboBox->setCurrentFont(font);
    }
}

/**
 * Returns the text settings key for the currently selected text
 *
 * @param key
 * @param item
 * @return
 */
QString FontColorWidget::textSettingsKey(const QString& key, QTreeWidgetItem* item) {
    return Utils::Schema::textSettingsKey(key, textSettingsIndex(item));
}

/**
 * Returns the text settings index for the currently selected text
 *
 * @param item
 * @return
 */
int FontColorWidget::textSettingsIndex(QTreeWidgetItem* item) {
    if (item == nullptr) {
        item = _controls->textTreeWidget->currentItem();
    }

    return item == nullptr ? -1000 : item->data(0, Qt::UserRole).toInt();
}

/**
 * Sets a schema value
 *
 * @param schemaKey
 * @param key
 * @param value
 */
void FontColorWidget::setSchemaValue(const QString& key, const QVariant& value, QString schemaKey) {
    if (schemaKey.isEmpty()) {
        schemaKey = _currentSchemaKey;
    }

    setSettingValue(schemaKey + QStringLiteral("/") + key, value);
}

/**
 * Applies the schema at the given combo box index: updates internal state,
 * persists the key to settings, and refreshes the widget preview.
 * Does NOT emit schemaChanged(); callers decide whether to emit it.
 *
 * @param index
 */
void FontColorWidget::applySchemaByIndex(int index) {
    _currentSchemaKey = _controls->colorSchemeComboBox->itemData(index).toString();
    _currentSchemaIsDefault = _defaultSchemaKeys.contains(_currentSchemaKey);

    _controls->deleteSchemeButton->setEnabled(!_currentSchemaIsDefault);
    _controls->schemeEditFrame->setEnabled(!_currentSchemaIsDefault);

    setSettingValue(QStringLiteral("Editor/CurrentSchemaKey"), _currentSchemaKey);

    updateSchemeEditFrame();

    // Update all text items in the widget preview
    updateAllTextItems();
}

/**
 * Selects the current scheme
 *
 * @param index
 */
void FontColorWidget::on_colorSchemeComboBox_currentIndexChanged(int index) {
    applySchemaByIndex(index);

    // Notify listeners that the schema has changed so they can apply it live
    Q_EMIT schemaChanged();
}

/**
 * Updates all text items
 */
void FontColorWidget::updateAllTextItems() {
    // update all text items
    for (int i = 0; i < _controls->textTreeWidget->topLevelItemCount(); i++) {
        QTreeWidgetItem* item = _controls->textTreeWidget->topLevelItem(i);
        updateTextItem(item);
    }
}

bool FontColorWidget::selectFirstLightSchema() {
    if (_controls->colorSchemeComboBox->count() >= 1) {
        // Block signals to prevent the slot from being triggered via the
        // signal; we call applySchemaByIndex() directly below instead
        const QSignalBlocker blocker(_controls->colorSchemeComboBox);
        Q_UNUSED(blocker)
        _controls->colorSchemeComboBox->setCurrentIndex(0);

        // Apply the schema (write to settings, refresh preview) without
        // emitting schemaChanged(); the caller handles the live apply
        applySchemaByIndex(0);
        return true;
    }

    return false;
}

bool FontColorWidget::selectFirstDarkSchema() {
    if (_controls->colorSchemeComboBox->count() >= 2) {
        // Block signals to prevent the slot from being triggered via the
        // signal; we call applySchemaByIndex() directly below instead
        const QSignalBlocker blocker(_controls->colorSchemeComboBox);
        Q_UNUSED(blocker)
        _controls->colorSchemeComboBox->setCurrentIndex(1);

        // Apply the schema (write to settings, refresh preview) without
        // emitting schemaChanged(); the caller handles the live apply
        applySchemaByIndex(1);
        return true;
    }

    return false;
}

/**
 * Updates the styling of certain items when the current text tree widget
 * item changes
 *
 * @param current
 * @param previous
 */
void FontColorWidget::on_textTreeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    Q_UNUSED(current)
    Q_UNUSED(previous)

    // update the schema edit frame for the current item
    updateSchemeEditFrame();

    // update the styling of the current text tree widget item
    updateTextItem();
}

/**
 * Updates the styling of a text tree widget item
 * @param item
 */
void FontColorWidget::updateTextItem(QTreeWidgetItem* item) {
    if (item == nullptr) {
        item = _controls->textTreeWidget->currentItem();
    }

    if (item == nullptr) {
        return;
    }

    int index = textSettingsIndex(item);

    // set the foreground color
    QColor color = Utils::Schema::schemaSettings->getForegroundColor(index);
    QBrush brush = item->foreground(0);
    brush.setColor(color);
    brush.setStyle(Qt::BrushStyle::SolidPattern);
    item->setForeground(0, brush);

    // set the background color
    color = Utils::Schema::schemaSettings->getBackgroundColor(index);
    brush = item->background(0);
    brush.setColor(color);
    brush.setStyle(Qt::SolidPattern);
    item->setBackground(0, brush);

    QFont font = Utils::Schema::schemaSettings->getFont(index);
    font.setBold(Utils::Schema::schemaSettings->getSchemaValue(textSettingsKey("Bold", item)).toBool());
    font.setItalic(Utils::Schema::schemaSettings->getSchemaValue(textSettingsKey("Italic", item)).toBool());
    font.setUnderline(Utils::Schema::schemaSettings->getSchemaValue(textSettingsKey("Underline", item)).toBool());

    // adapt the font size
    Utils::Schema::schemaSettings->adaptFontSize(index, font);

    item->setFont(0, font);
}

/**
 * Copies a color schema
 */
void FontColorWidget::on_copySchemeButton_clicked() {
    // ask the user for a new schema name
    QString name =
        QInputDialog::getText(this, tr("Copy color schema"), tr("Color schema name"), QLineEdit::Normal,
                              _controls->colorSchemeComboBox->currentText() + " (" + tr("Copy", "as noun") + ")");

    if (name.isEmpty()) {
        return;
    }

    const QStringList& keys = Utils::Schema::schemaSettings->getSchemaKeys(_currentSchemaKey);
    QString uuid = Utils::Misc::createUuidString();
    _currentSchemaKey = "EditorColorSchema-" + uuid;

    // store the new color schema data
    Q_FOREACH (const QString& key, keys) {
        QVariant value = key == "Name" ? QVariant(name) : Utils::Schema::schemaSettings->getSchemaValue(key);
        setSchemaValue(key, value, _currentSchemaKey);
    }

    // add the new color schema to the color schemes list in the settings
    QStringList schemes = settingValue(QStringLiteral("Editor/ColorSchemes")).toStringList();
    schemes << _currentSchemaKey;
    setSettingValue(QStringLiteral("Editor/ColorSchemes"), schemes);
    setSettingValue(QStringLiteral("Editor/CurrentSchemaKey"), _currentSchemaKey);

    // select the last schema
    selectLastSchema();
}

/**
 * Selects the last schema
 */
void FontColorWidget::selectLastSchema() {
    // reload the schema selector
    initSchemaSelector();

#ifdef Q_OS_MAC
    // under macOS we have to use a workaround to select the newly created schema
    QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, Qt::Key_End, Qt::NoModifier);
    QApplication::postEvent(_controls->colorSchemeComboBox, event);
#else
    // set the index to the (new) last item
    _controls->colorSchemeComboBox->setCurrentIndex(_controls->colorSchemeComboBox->count() - 1);
#endif
}

/**
 * Takes the proper actions if the foreground enabler was toggled
 *
 * @param checked
 */
void FontColorWidget::on_foregroundColorCheckBox_toggled(bool checked) {
    updateForegroundColorCheckBox(checked, true);

    // update the current or all text items, depending on the index
    updateTextItems(textSettingsIndex());

    // update the scheme edit frame
    updateSchemeEditFrame();

    // Notify listeners that the schema has changed so they can apply it live
    Q_EMIT schemaChanged();
}

void FontColorWidget::updateForegroundColorCheckBox(bool checked, bool store) {
    const QSignalBlocker blocker(_controls->foregroundColorCheckBox);
    Q_UNUSED(blocker)

    _controls->foregroundColorCheckBox->setChecked(checked);
    _controls->foregroundColorButton->setEnabled(checked);

    // update the styling of the current text tree widget item
    updateTextItem();

    if (store && !_currentSchemaIsDefault) {
        setSchemaValue(textSettingsKey("ForegroundColorEnabled"), checked);
    }
}

/**
 * Takes the proper actions if the background enabler was toggled
 *
 * @param checked
 */
void FontColorWidget::on_backgroundColorCheckBox_toggled(bool checked) {
    updateBackgroundColorCheckBox(checked, true);

    // update the current or all text items, depending on the index
    updateTextItems(textSettingsIndex());

    // update the scheme edit frame
    updateSchemeEditFrame();

    // Notify listeners that the schema has changed so they can apply it live
    Q_EMIT schemaChanged();
}

void FontColorWidget::updateBackgroundColorCheckBox(bool checked, bool store) {
    const QSignalBlocker blocker(_controls->backgroundColorCheckBox);
    Q_UNUSED(blocker)

    _controls->backgroundColorCheckBox->setChecked(checked);
    _controls->backgroundColorButton->setEnabled(checked);

    // update the styling of the current text tree widget item
    updateTextItem();

    if (store && !_currentSchemaIsDefault) {
        setSchemaValue(textSettingsKey("BackgroundColorEnabled"), checked);
    }
}

/**
 * Removes the current schema
 */
void FontColorWidget::on_deleteSchemeButton_clicked() {
    if (_currentSchemaKey.isEmpty()) {
        return;
    }

    if (Utils::Gui::question(this, tr("Remove schema"), tr("Remove current schema? This cannot be undone!"),
                             "remove-color-schema") != QMessageBox::Yes) {
        return;
    }

    // Remove the group and all its keys.
    removeSettingValue(_currentSchemaKey);

    // remove the current schema from the list of schemas
    QStringList schemes = settingValue(QStringLiteral("Editor/ColorSchemes")).toStringList();
    schemes.removeAll(_currentSchemaKey);
    setSettingValue(QStringLiteral("Editor/ColorSchemes"), schemes);

    initSchemaSelector();
}

void FontColorWidget::storeCheckBoxState(const QString& name, bool checked) {
    if (!_currentSchemaIsDefault) {
        setSchemaValue(textSettingsKey(name), checked);
    }

    // update the styling of the current text tree widget item
    updateTextItem();
}

void FontColorWidget::on_boldCheckBox_toggled(bool checked) {
    storeCheckBoxState("Bold", checked);

    // Notify listeners that the schema has changed so they can apply it live
    Q_EMIT schemaChanged();
}

void FontColorWidget::on_italicCheckBox_toggled(bool checked) {
    storeCheckBoxState("Italic", checked);

    // Notify listeners that the schema has changed so they can apply it live
    Q_EMIT schemaChanged();
}

void FontColorWidget::on_underlineCheckBox_toggled(bool checked) {
    storeCheckBoxState("Underline", checked);

    // Notify listeners that the schema has changed so they can apply it live
    Q_EMIT schemaChanged();
}

/**
 * Exports the current color schema to a file
 */
void FontColorWidget::on_exportSchemeButton_clicked() {
    FileDialog dialog("SchemaExport");
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(tr("INI files") + " (*.ini)");
    dialog.setWindowTitle(tr("Export schema"));
    dialog.selectFile(_controls->colorSchemeComboBox->currentText() + ".ini");
    int ret = dialog.exec();

    if (ret == QDialog::Accepted) {
        QString fileName = dialog.selectedFile();

        if (!fileName.isEmpty()) {
            if (QFileInfo(fileName).suffix().isEmpty()) {
                fileName.append(".ini");
            }

            QSettings exportSettings(fileName, QSettings::IniFormat);

            // clear the settings in case the settings file already existed
            exportSettings.clear();

            // store the schema key
            exportSettings.setValue("Export/SchemaKey", _currentSchemaKey);

            exportSettings.beginGroup(_currentSchemaKey);
            const QStringList& keys = Utils::Schema::schemaSettings->getSchemaKeys(_currentSchemaKey);

            // store the color schema data to the export settings
            Q_FOREACH (const QString& key, keys) {
                QVariant value = Utils::Schema::schemaSettings->getSchemaValue(key);
                exportSettings.setValue(key, value);
            }

            Utils::Misc::openFolderSelect(fileName, QStringLiteral("show-exported-color-schema-in-file-manager"));
        }
    }
}

/**
 * Imports a schema from a file
 */
void FontColorWidget::on_importSchemeButton_clicked() {
    FileDialog dialog("SchemaImport");
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setNameFilter(tr("INI files") + " (*.ini)");
    dialog.setWindowTitle(tr("Import schema"));
    int ret = dialog.exec();

    if (ret == QDialog::Accepted) {
        QStringList fileNames = dialog.selectedFiles();
        if (fileNames.count() > 0) {
            Q_FOREACH (QString fileName, fileNames) {
                QSettings importSettings(fileName, QSettings::IniFormat);
                QString schemaKey = importSettings.value("Export/SchemaKey").toString();

                // create a new schema key for the import
                QString uuid = Utils::Misc::createUuidString();
                _currentSchemaKey = "EditorColorSchema-" + uuid;

                QStringList schemes = settingValue(QStringLiteral("Editor/ColorSchemes")).toStringList();
                schemes << _currentSchemaKey;
                setSettingValue(QStringLiteral("Editor/ColorSchemes"), schemes);
                setSettingValue(QStringLiteral("Editor/CurrentSchemaKey"), _currentSchemaKey);

                importSettings.beginGroup(schemaKey);
                QStringList keys = importSettings.allKeys();

                // store the color schema data to the settings
                Q_FOREACH (QString key, keys) {
                    QVariant value = importSettings.value(key);
                    setSettingValue(_currentSchemaKey + QStringLiteral("/") + key, value);
                }

                // select the last schema
                selectLastSchema();

                importSettings.endGroup();
            }
        }
    }
}

/**
 * Initializes the font selectors
 */
void FontColorWidget::initFontSelectors() {
    QTextEdit textEdit;
    QFont font = textEdit.font();
    QString fontString = settingValue(QStringLiteral("MainWindow/noteTextEdit.font")).toString();

    if (!fontString.isEmpty()) {
        // set the note text edit font
        font.fromString(fontString);
    } else {
        // store the default settings
        fontString = textEdit.font().toString();
        setSettingValue(QStringLiteral("MainWindow/noteTextEdit.font"), fontString);
    }

    _controls->fontFamilyComboBox->setCurrentFont(font);
    _controls->fontSizeSpinBox->setValue(font.pointSize());
}

/**
 * Stores the font size adaption value
 *
 * @param value
 */
void FontColorWidget::on_fontSizeAdaptionSpinBox_valueChanged(int value) {
    if (!_currentSchemaIsDefault) {
        setSchemaValue(textSettingsKey("FontSizeAdaption"), value);
    }

    // update the styling of the current text tree widget item
    updateTextItem();

    // Notify listeners that the schema has changed so they can apply it live
    Q_EMIT schemaChanged();
}

/**
 * Opens a new GitHub issue to share a schema
 */
void FontColorWidget::on_shareSchemaPushButton_clicked() {
    QDesktopServices::openUrl(QUrl("https://github.com/pbek/ArcNotes/issues/new/choose"));
}

/**
 * Declares that we need a restart
 */
void FontColorWidget::needRestart() {
    Utils::Misc::needRestart();
}

void FontColorWidget::on_fontCheckBox_toggled(bool checked) {
    updateFontCheckBox(checked, true);

    // update the current or all text items, depending on the index
    updateTextItems(textSettingsIndex());

    // update the scheme edit frame
    updateSchemeEditFrame();

    // Notify listeners that the schema has changed so they can apply it live
    Q_EMIT schemaChanged();
}

void FontColorWidget::updateFontCheckBox(bool checked, bool store) {
    const QSignalBlocker blocker(_controls->fontCheckBox);
    Q_UNUSED(blocker)

    _controls->fontCheckBox->setChecked(checked);
    _controls->fontComboBox->setEnabled(checked);

    // update the styling of the current text tree widget item
    updateTextItem();

    if (store && !_currentSchemaIsDefault) {
        setSchemaValue(textSettingsKey("FontEnabled"), checked);
    }
}

void FontColorWidget::on_fontComboBox_currentFontChanged(const QFont& f) {
    setSchemaValue(textSettingsKey("Font"), f);

    // update the styling of the current text tree widget item
    updateTextItem();

    // Notify listeners that the schema has changed so they can apply it live
    Q_EMIT schemaChanged();
}

QVariant FontColorWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void FontColorWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}

void FontColorWidget::removeSettingValue(const QString& key) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->removePersistentSetting(key);
    }
}
