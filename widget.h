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
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    void paintEvent(QPaintEvent *);

private:
    Ui::Widget *ui;

    void Draw16Bit(QPainter &p, int W, int H);
    void Draw8Bit(QPainter &p, int W, int H);

    // Necessary variables for wave form
    WaveFile m_Wavefile;
    bool m_DrawWave;
    QString m_Filename;
    double m_SamplesPerPixel;
    double m_ZoomFactor;
    int m_OffsetInSamples;
};

#endif // WIDGET_H
