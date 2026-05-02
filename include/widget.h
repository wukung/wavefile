#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QWheelEvent>
#include <QMouseEvent>

#include "WaveFile.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void openFile();

private:
    Ui::Widget *ui;

    void DrawWaveform(QPainter &p, int W, int H, double samplesPerPixel, double offsetInSamples, int yOffset);

    // Necessary variables for wave form
    WaveFile m_Wavefile;
    bool m_DrawWave;
    QString m_Filename;
    double m_SamplesPerPixel;
    double m_OffsetInSamples;

    QPoint m_LastMousePos;
    bool m_IsPanning;
    int m_OverviewHeight;

    void DrawOverview(QPainter &p, int W, int H);
};

#endif // WIDGET_H
