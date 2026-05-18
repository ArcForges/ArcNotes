#include "imagedialog.h"

#include <utils/misc.h>
#include <viewmodels/viewmodellocator.h>
#include <widgets/graphicsview.h>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFrame>
#include <QGraphicsScene>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QRubberBand>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QTemporaryFile>
#include <QToolButton>
#include <QUrl>

#include "filedialog.h"

ImageDialog::ImageDialog(QWidget* parent) : MasterDialog(parent) {
    buildUi();
    afterSetupUI();
    _fileEdit->setFocus();
    _previewFrame->setVisible(false);
    _toolFrame->hide();

    if (SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel()) {
        _disableCopyingCheckBox->setChecked(
            viewModel->persistentSetting(QStringLiteral("ImageDialog/disableCopying")).toBool());
    }

    QClipboard* clipboard = QApplication::clipboard();
    QPixmap pixmap = clipboard->pixmap();

    if (!pixmap.isNull()) {
        _fileEdit->setDisabled(true);
        _disableCopyingCheckBox->setChecked(false);
        _disableCopyingCheckBox->setDisabled(true);
        setPixmap(pixmap, true);
    } else {
        const QString text = clipboard->text().trimmed();
        const QUrl url(text);

        if (url.isValid()) {
            _fileEdit->setText(text);
        }
    }

    connect(_graphicsView, &GraphicsView::scrolledContentsBy, this, &ImageDialog::scrolledGraphicsViewContentsBy);
    connect(_graphicsView, &GraphicsView::resizedBy, this, &ImageDialog::resizedGraphicsViewBy);
}

ImageDialog::~ImageDialog() {
    if (SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel()) {
        viewModel->setPersistentSetting(QStringLiteral("ImageDialog/disableCopying"),
                                        _disableCopyingCheckBox->isChecked());
    }

    delete _imageFile;
    delete _tempFile;
}

void ImageDialog::buildUi() {
    setWindowTitle(tr("Insert image"));
    resize(806, 524);

    auto* layout = new QGridLayout(this);
    _fileEdit = new QLineEdit(this);
    _fileEdit->setStatusTip(tr("Image filename or URL"));
    _fileEdit->setPlaceholderText(tr("Path to file or URL"));
    _fileEdit->setClearButtonEnabled(true);
    auto* openButton = new QToolButton(this);
    openButton->setToolTip(tr("Select image file"));
    openButton->setText(QStringLiteral("..."));
    _titleEdit = new QLineEdit(this);
    _titleEdit->setStatusTip(tr("Title of the image link"));
    _titleEdit->setPlaceholderText(tr("Title"));
    _titleEdit->setClearButtonEnabled(true);
    _disableCopyingCheckBox = new QCheckBox(tr("Don't copy image to media folder"), this);
    _disableCopyingCheckBox->setToolTip(
        tr("The path or url will be inserted directly, paths to files will be made relative to the "
           "current note"));

    layout->addWidget(_fileEdit, 0, 0, 1, 2);
    layout->addWidget(openButton, 0, 2);
    layout->addWidget(_titleEdit, 1, 0, 1, 3);
    layout->addWidget(_disableCopyingCheckBox, 2, 0, 1, 3);

    _previewFrame = new QFrame(this);
    _previewFrame->setFrameShape(QFrame::NoFrame);
    auto* previewLayout = new QGridLayout(_previewFrame);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    _scaleFrame = new QFrame(_previewFrame);
    _scaleFrame->setFrameShape(QFrame::NoFrame);
    auto* scaleLayout = new QHBoxLayout(_scaleFrame);
    scaleLayout->setContentsMargins(0, 0, 0, 0);
    auto* scaleLabel = new QLabel(tr("Scaling width:"), _scaleFrame);
    _widthSpinBox = new QSpinBox(_scaleFrame);
    _widthSpinBox->setSuffix(QStringLiteral("px"));
    _widthSpinBox->setRange(1, 100000);
    _widthScaleHorizontalSlider = new QSlider(Qt::Horizontal, _scaleFrame);
    _widthScaleHorizontalSlider->setRange(1, 20);
    _widthScaleHorizontalSlider->setPageStep(1);
    _widthScaleHorizontalSlider->setSliderPosition(10);
    _widthScaleLabel = new QLabel(QStringLiteral("1x"), _scaleFrame);
    scaleLayout->addWidget(scaleLabel);
    scaleLayout->addWidget(_widthSpinBox);
    scaleLayout->addWidget(_widthScaleHorizontalSlider, 1);
    scaleLayout->addWidget(_widthScaleLabel);
    _graphicsView = new GraphicsView(_previewFrame);
    _graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    previewLayout->addWidget(_scaleFrame, 2, 0, 1, 3);
    previewLayout->addWidget(_graphicsView, 3, 0, 1, 3);
    layout->addWidget(_previewFrame, 3, 0, 1, 3);

    _toolFrame = new QFrame(this);
    _toolFrame->setFrameShape(QFrame::NoFrame);
    auto* toolLayout = new QHBoxLayout(_toolFrame);
    toolLayout->setContentsMargins(0, 0, 0, 0);
    auto* cropButton = new QPushButton(tr("&Crop"), _toolFrame);
    cropButton->setToolTip(tr("Crop image"));
    cropButton->setIcon(QIcon::fromTheme(QStringLiteral("transform-crop"),
                                         QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/transform-crop.svg"))));
    toolLayout->addWidget(cropButton);
    toolLayout->addStretch();
    layout->addWidget(_toolFrame, 4, 0, 1, 3);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->setRowStretch(5, 1);
    layout->addWidget(buttonBox, 6, 0, 1, 3);

    connect(openButton, &QToolButton::clicked, this, &ImageDialog::on_openButton_clicked);
    connect(_fileEdit, &QLineEdit::textChanged, this, &ImageDialog::on_fileEdit_textChanged);
    connect(_disableCopyingCheckBox, &QCheckBox::toggled, this, &ImageDialog::on_disableCopyingCheckBox_toggled);
    connect(_widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ImageDialog::on_widthSpinBox_valueChanged);
    connect(_widthScaleHorizontalSlider, &QSlider::valueChanged, this,
            &ImageDialog::on_widthScaleHorizontalSlider_valueChanged);
    connect(_graphicsView, &QGraphicsView::rubberBandChanged, this, &ImageDialog::on_graphicsView_rubberBandChanged);
    connect(cropButton, &QPushButton::clicked, this, &ImageDialog::on_cropButton_clicked);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        on_buttonBox_accepted();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

bool ImageDialog::isDisableCopying() {
    return _disableCopyingCheckBox->isChecked();
}

QFile* ImageDialog::getImageFile() {
    return _imageFile;
}

QString ImageDialog::getFilePathOrUrl() {
    return _fileEdit->text().trimmed();
}

QString ImageDialog::getImageTitle() {
    return _titleEdit->text();
}

void ImageDialog::on_openButton_clicked() {
    QStringList nameFilters;
    nameFilters << tr("Image files") + QStringLiteral(
                                           " (*.jpg *.jpeg *.png *.gif *.svg *.bmp *.pbm *.pgm "
                                           "*.ppm *.xbm *.xpm *.webp)")
                << tr("Any files") + QStringLiteral(" (*)");

    FileDialog dialog(QStringLiteral("InsertImage"));
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setNameFilters(nameFilters);
    dialog.setWindowTitle(tr("Select image to insert"));

    if (dialog.exec() == QDialog::Accepted) {
        const QString filePath = dialog.selectedFile();

        if (!filePath.isEmpty()) {
            _disableCopyingCheckBox->setEnabled(true);
            _fileEdit->setEnabled(true);
            _fileEdit->setText(filePath.trimmed());
        }
    }
}

void ImageDialog::setPixmap(const QPixmap& pixmap, bool updateBase) {
    if (pixmap.isNull()) {
        return;
    }

    _pixmap = pixmap;
    auto* scene = new QGraphicsScene(this);
    scene->addPixmap(pixmap);
    _graphicsView->setScene(scene);

    if (updateBase) {
        _basePixmap = pixmap;

        const QSignalBlocker blocker(_widthSpinBox);
        Q_UNUSED(blocker)
        const QSignalBlocker blocker2(_widthScaleHorizontalSlider);
        Q_UNUSED(blocker2)

        _widthSpinBox->setValue(pixmap.width());
        _widthScaleHorizontalSlider->setValue(10);
        updateWidthScaleLabelValue();
    }

    _previewFrame->setVisible(true);
}

void ImageDialog::on_buttonBox_accepted() {
    delete _imageFile;
    _imageFile = nullptr;
    delete _tempFile;
    _tempFile = nullptr;

    if (_fileEdit->text().trimmed().isEmpty() || _imageWasCropped || _imageWasDownloaded ||
        (!_basePixmap.isNull() && _widthSpinBox->value() != _basePixmap.width())) {
        _tempFile =
            new QTemporaryFile(QDir::tempPath() + QDir::separator() + QStringLiteral("arcnotes-media-XXXXXX.png"));

        if (_tempFile->open()) {
            _pixmap.save(_tempFile->fileName(), "PNG");
            _imageFile = new QFile(_tempFile->fileName());
        }
    } else {
        _imageFile = new QFile(_fileEdit->text().trimmed());
    }
}

void ImageDialog::on_widthSpinBox_valueChanged(int arg1) {
    if (_basePixmap.isNull() || _basePixmap.width() == 0) {
        return;
    }

    const double factor = static_cast<double>(arg1) / _basePixmap.width();

    const QSignalBlocker blocker(_widthScaleHorizontalSlider);
    Q_UNUSED(blocker)
    _widthScaleHorizontalSlider->setValue(static_cast<int>(factor * 10));
    updateWidthScaleLabelValue();

    QPixmap pixmap = _basePixmap.scaledToWidth(arg1, Qt::SmoothTransformation);
    setPixmap(pixmap);
}

void ImageDialog::updateWidthScaleLabelValue() const {
    const double factor = static_cast<double>(_widthScaleHorizontalSlider->value()) / 10;
    _widthScaleLabel->setText(QString::number(factor) + QStringLiteral("x"));
}

void ImageDialog::on_widthScaleHorizontalSlider_valueChanged(int value) {
    if (_basePixmap.isNull()) {
        return;
    }

    updateWidthScaleLabelValue();
    const int width = static_cast<int>(_basePixmap.width() * value / 10);
    _widthSpinBox->setValue(width);
}

void ImageDialog::on_fileEdit_textChanged(const QString& arg1) {
    QString pathOrUrl = arg1.trimmed();

    if (pathOrUrl.isEmpty()) {
        _previewFrame->setVisible(false);
        _pixmap = QPixmap();
        _basePixmap = QPixmap();
        return;
    }

    const QUrl url(pathOrUrl);

    if (!url.isValid()) {
        return;
    }

    if (url.scheme().startsWith(QLatin1String("http"))) {
        const QByteArray data = Utils::Misc::downloadUrl(url);

        if (data.isEmpty()) {
            return;
        }

        QPixmap pixmap;
        pixmap.loadFromData(data);

        if (pixmap.isNull()) {
            return;
        }

        setPixmap(pixmap, true);
        _imageWasDownloaded = true;
        return;
    }

    _imageWasDownloaded = false;

    if (url.scheme() == QLatin1String("file")) {
        pathOrUrl = url.toLocalFile();
    }

    QFile file(pathOrUrl);
    if (file.size() == 0) {
        return;
    }

    setPixmap(QPixmap(pathOrUrl), true);
}

void ImageDialog::on_disableCopyingCheckBox_toggled(bool checked) {
    _scaleFrame->setDisabled(checked);
    _graphicsView->setDragMode(checked ? QGraphicsView::NoDrag : QGraphicsView::RubberBandDrag);

    if (checked) {
        _widthScaleHorizontalSlider->setValue(10);
    }
}

void ImageDialog::on_graphicsView_rubberBandChanged(QRect viewportRect, QPointF fromScenePoint, QPointF toScenePoint) {
    if (viewportRect.isEmpty()) {
        _rubberBand = new QRubberBand(QRubberBand::Rectangle, _graphicsView);

        QMargins margin = _graphicsView->contentsMargins();
        _lastRubberBandViewportRect.adjust(margin.left(), margin.top(), margin.left(), margin.top());

        _rubberBand->setGeometry(_lastRubberBandViewportRect);
        _rubberBand->show();
    } else {
        if (_rubberBand != nullptr && _rubberBand->isVisible()) {
            _rubberBand->close();
        }

        QPoint fromScenePointI = fromScenePoint.toPoint();
        QPoint toScenePointI = toScenePoint.toPoint();

        if (fromScenePointI.x() < 0) {
            fromScenePointI.setX(0);
        }
        if (fromScenePointI.y() < 0) {
            fromScenePointI.setY(0);
        }
        if (toScenePointI.x() < 0) {
            toScenePointI.setX(0);
        }
        if (toScenePointI.y() < 0) {
            toScenePointI.setY(0);
        }

        if (fromScenePointI.x() > toScenePointI.x() && fromScenePointI.y() > toScenePointI.y()) {
            _rubberBandSceneRect = QRect(toScenePointI, fromScenePointI);
        } else {
            _rubberBandSceneRect = QRect(fromScenePointI, toScenePointI);
        }

        _lastRubberBandViewportRect = viewportRect;
        _toolFrame->show();
    }
}

void ImageDialog::on_cropButton_clicked() {
    if (!_rubberBandSceneRect.isEmpty()) {
        QPixmap cropped = _pixmap.copy(_rubberBandSceneRect);
        setPixmap(cropped, true);

        _rubberBandSceneRect = QRect(0, 0, 0, 0);
        if (_rubberBand != nullptr) {
            _rubberBand->close();
        }
        _imageWasCropped = true;
        _toolFrame->hide();
    }
}

void ImageDialog::scrolledGraphicsViewContentsBy(int dx, int dy) {
    if (_rubberBand == nullptr || _rubberBand->isHidden()) {
        return;
    }

    _rubberBand->move(_rubberBand->x() + dx, _rubberBand->y() + dy);
}

void ImageDialog::resizedGraphicsViewBy(int dw, int dh) {
    if (_rubberBand == nullptr || _rubberBand->isHidden()) {
        return;
    }

    Q_UNUSED(dw)
    Q_UNUSED(dh)
    _rubberBand->close();
}
