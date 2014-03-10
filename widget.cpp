#include "widget.h"
#include "ui_widget.h"
#include <QPainter>
#include <qmessagebox.h>
#include <QTextCodec>

#define MIN(x, y)   ((x) < (y) ? (x) : (y))
#define MAX(x, y)   ((x) > (y) ? (x) : (y))

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    // Initiate member varialbes
    m_SamplesPerPixel = 0.0;
    m_OffsetInSamples = 0;
    m_Filename = "";
}

Widget::~Widget()
{
    delete ui;
}

void Widget::paintEvent(QPaintEvent *e)
{
    m_Filename = "/Users/acheng/Downloads/cat_1.wav";

    int W = this->width();
    int H = this->height();

    QPixmap pix(W, H);
    pix.fill(Qt::black);
    QPainter p(&pix);

    QPen pen;
    pen.setColor(QColor(00, 35, 00));
    p.setPen(pen);

    // Draw background
    int i;
    double x, y;
    int col_num = 30;
    int row_num = 13;

    // Draw vertical lines
    for (i=0; i<col_num; i++) {
        x = i * W / col_num;
        p.drawLine((int)x, 0, (int)x, H);
    }

    // Draw herizontal lines
    for (i=0; i<row_num; i++) {
        y = i * H / row_num;
        p.drawLine(0, (int)y, W, (int)y);
    }

    pen.setColor(QColor(0x4B, 0xF3, 0xA7));
    p.setPen(pen);

    m_Wavefile.WavRead(m_Filename.toStdString());
    m_Wavefile.WavInfo();
    m_DrawWave = true;

    if (m_DrawWave) {
        if (m_SamplesPerPixel == 0.0) {
            // Calculate mapping of pixel and sampling
            m_SamplesPerPixel = (m_Wavefile.datanum / W);
        }
        p.drawLine(0, H/2, W, H/2);

        if (m_Wavefile.bitpersample == 16) {
            Draw16Bit(p, W, H);
        }
        else if (m_Wavefile.bitpersample == 8) {
            Draw8Bit(p, W, H);
        }
    }

    QPainter painter(this);
    painter.drawPixmap(0, 0, pix);
}

void Widget::Draw16Bit(QPainter &p, int W, int H)
{
    int prevX = 0;
    int prevY = 0;
    int i = 0;

    // index is how far to offset into the data array
    int index = m_OffsetInSamples;
    int maxSampleToShow = (int) ((m_SamplesPerPixel * W) + m_OffsetInSamples);

    maxSampleToShow = MIN(maxSampleToShow, m_Wavefile.datanum);

    while (index < maxSampleToShow) {
        short maxValue = -32767;
        short minValue = 32767;

        // find the max & min peaks for this pixel
        for (int x=0; x<m_SamplesPerPixel; x++) {
            maxValue = MAX(maxValue, m_Wavefile.Data[x+index]);
            minValue = MIN(minValue, m_Wavefile.Data[x+index]);
        }

        // scales based on height of windows
        int scaledMinVal = (int)(((minValue + 32768)*H)/65536);
        int scaledMaxVal = (int)(((maxValue + 32768)*H)/65536);

        scaledMinVal = H - scaledMinVal;
        scaledMaxVal = H - scaledMaxVal;

        // if samples per pixel is small or less than zero,
        // we are out of zoom range, so don't display anything
        if (m_SamplesPerPixel > 0.0000000001) {
            if (scaledMinVal == scaledMaxVal) {
                if (prevY != 0) {
                    p.drawLine(prevX, prevY, i, scaledMaxVal);
                }
            }
            else {
                p.drawLine(i, scaledMinVal, i, scaledMaxVal);
            }
        }
        else {
            return;
        }
        prevX = i;
        prevY = scaledMaxVal;

        i++;
        index = (int)(i * m_SamplesPerPixel) + m_OffsetInSamples;
    }
}

void Widget::Draw8Bit(QPainter &p, int W, int H)
{
    int prevX = 0;
    int prevY = 0;
    int i = 0;

    // index is how far to offset into the data array
    int index = m_OffsetInSamples;
    int maxSampleToShow = (int) ((m_SamplesPerPixel * W) + m_OffsetInSamples);

    maxSampleToShow = MIN(maxSampleToShow, m_Wavefile.datanum);

    while (index < maxSampleToShow) {
        short maxVal = 0;
        short minVal = 255;

        for (int x=0; x<m_SamplesPerPixel; x++) {
            short low = (short) (m_Wavefile.Data[x+index] & 0x00ff);
            short high = (short) (m_Wavefile.Data[x+index] >> 8 & 0x00ff);
            maxVal = MAX(maxVal, low);
            maxVal = MAX(maxVal, high);
            minVal = MIN(minVal, low);
            minVal = MIN(minVal, high);
        }

        int scaledMinVal = H - (int)(((minVal) * H) / 256);
        int scaledMaxVal = H - (int)(((maxVal) * H) / 256);

        if (m_SamplesPerPixel > 0.0000000001) {
            if (scaledMaxVal == scaledMinVal) {
                if (prevY != 0) {
                    p.drawLine(prevX, prevY, i, scaledMaxVal);
                }
            }
            else {
                p.drawLine(i, scaledMinVal, i, scaledMaxVal);
            }
        }
        else {
            return;
        }
        prevX = i;
        prevY = scaledMaxVal;

        i++;
        index = (int)(i * m_SamplesPerPixel) + m_OffsetInSamples;
    }
}
