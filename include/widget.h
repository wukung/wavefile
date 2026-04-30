#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

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

private slots:
    void openFile();

private:
    Ui::Widget *ui;

    void DrawWaveform(QPainter &p, int W, int H);

    // Necessary variables for wave form
    WaveFile m_Wavefile;
    bool m_DrawWave;
    QString m_Filename;
    double m_SamplesPerPixel;
    double m_ZoomFactor;
    int m_OffsetInSamples;
};

#endif // WIDGET_H
