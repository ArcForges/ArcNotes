#pragma once

#include <QFile>
#include <QPixmap>
#include <QPointF>
#include <QRect>

#include "masterdialog.h"

class QCheckBox;
class QFrame;
class QGraphicsScene;
class QLineEdit;
class QRubberBand;
class QSlider;
class QSpinBox;
class QTemporaryFile;
class GraphicsView;
class QLabel;

class ImageDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit ImageDialog(QWidget* parent = nullptr);
    ~ImageDialog() override;

    QFile* getImageFile();
    QString getImageTitle();
    bool isDisableCopying();
    QString getFilePathOrUrl();

private slots:
    void on_openButton_clicked();
    void on_buttonBox_accepted();
    void on_widthSpinBox_valueChanged(int arg1);
    void on_widthScaleHorizontalSlider_valueChanged(int value);
    void on_fileEdit_textChanged(const QString& arg1);
    void on_disableCopyingCheckBox_toggled(bool checked);
    void on_graphicsView_rubberBandChanged(QRect viewportRect, QPointF fromScenePoint, QPointF toScenePoint);
    void on_cropButton_clicked();
    void scrolledGraphicsViewContentsBy(int dx, int dy);
    void resizedGraphicsViewBy(int dw, int dh);

private:
    QPixmap _basePixmap;
    QPixmap _pixmap;
    QFile* _imageFile = nullptr;
    QTemporaryFile* _tempFile = nullptr;
    QRubberBand* _rubberBand = nullptr;
    bool _imageWasCropped = false;
    bool _imageWasDownloaded = false;
    QRect _rubberBandSceneRect;
    QRect _lastRubberBandViewportRect;
    QLineEdit* _fileEdit = nullptr;
    QLineEdit* _titleEdit = nullptr;
    QCheckBox* _disableCopyingCheckBox = nullptr;
    QFrame* _previewFrame = nullptr;
    QFrame* _scaleFrame = nullptr;
    QFrame* _toolFrame = nullptr;
    GraphicsView* _graphicsView = nullptr;
    QSpinBox* _widthSpinBox = nullptr;
    QSlider* _widthScaleHorizontalSlider = nullptr;
    QLabel* _widthScaleLabel = nullptr;

    void buildUi();
    void setPixmap(const QPixmap& pixmap, bool updateBase = false);
    void updateWidthScaleLabelValue() const;
};
