#include "widget.h"
#include "ui_widget.h"
#include "pcmformatdialog.h"
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QKeyEvent>
#include <algorithm>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    m_DrawWave = false;
    m_ZoomFactor = 1.0;
    m_SamplesPerPixel = 0.0;
    m_OffsetInSamples = 0;
}

Widget::~Widget()
{
    delete ui;
}

void Widget::openFile()
{
    QString filename = QFileDialog::getOpenFileName(
        this, tr("Open Audio File"), QString(),
        tr("WAV Files (*.wav);;Raw PCM Files (*.raw *.pcm *.bin);;All Files (*)"));
    if (filename.isEmpty())
        return;
    m_Filename = filename;
    m_DrawWave = m_Wavefile.WavRead(m_Filename.toStdString());
    if (!m_DrawWave) {
        // WAV parse failed — ask user for raw PCM format
        PcmFormatDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            m_DrawWave = m_Wavefile.RawRead(m_Filename.toStdString(), dlg.format());
        }
    }
    if (m_DrawWave) {
        m_Wavefile.WavInfo();
    }
    update();
}

void Widget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_O && event->modifiers() == Qt::ControlModifier) {
        openFile();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void Widget::paintEvent(QPaintEvent *e)
{
    int W = this->width();
    int H = this->height();

    if (W <= 0 || H <= 0) {
        return;
    }

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

    // Draw horizontal lines
    for (i=0; i<row_num; i++) {
        y = i * H / row_num;
        p.drawLine(0, (int)y, W, (int)y);
    }

    pen.setColor(QColor(0x4B, 0xF3, 0xA7));
    p.setPen(pen);

    if (!m_DrawWave) {
        const QString msg = m_Filename.isEmpty()
            ? tr("Press Ctrl+O to open a WAV file")
            : tr("Cannot read wave file: ") + m_Filename;
        p.drawText(pix.rect(), Qt::AlignCenter, msg);
        QPainter painter(this);
        painter.drawPixmap(0, 0, pix);
        return;
    }

    // Dynamically calculate mapping of pixel and sampling to perfectly fill the window
    m_SamplesPerPixel = (double)m_Wavefile.datanum / W;

    p.drawLine(0, H/2, W, H/2);

    if (m_Wavefile.bitpersample == 16) {
        Draw16Bit(p, W, H);
    }
    else if (m_Wavefile.bitpersample == 8) {
        Draw8Bit(p, W, H);
    }

    QPainter painter(this);
    painter.drawPixmap(0, 0, pix);
}

void Widget::Draw16Bit(QPainter &p, int W, int H)
{
    int i = 0;

    // index is how far to offset into the data array
    int index = m_OffsetInSamples;
    int maxSampleToShow = (int) ((m_SamplesPerPixel * W) + m_OffsetInSamples);

    maxSampleToShow = std::min(maxSampleToShow, (int)m_Wavefile.datanum);

    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    bool first = true;

    while (index < maxSampleToShow && i < W) {
        // Retrieve the sampled value
        short sampleVal = m_Wavefile.Data[index];

        // scales based on height of windows
        int scaledVal = (int)(((sampleVal + 32768) * H) / 65536);
        scaledVal = H - scaledVal;

        if (m_SamplesPerPixel > 0.0000000001) {
            if (first) {
                path.moveTo(i, scaledVal);
                first = false;
            } else {
                path.lineTo(i, scaledVal);
            }
        }
        else {
            return;
        }

        i++;
        index = (int)(i * m_SamplesPerPixel) + m_OffsetInSamples;
    }
    p.drawPath(path);
}

void Widget::Draw8Bit(QPainter &p, int W, int H)
{
    int i = 0;

    // index is how far to offset into the data array
    int index = m_OffsetInSamples;
    int maxSampleToShow = (int) ((m_SamplesPerPixel * W) + m_OffsetInSamples);

    maxSampleToShow = std::min(maxSampleToShow, (int)m_Wavefile.datanum);

    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    bool first = true;

    while (index < maxSampleToShow && i < W) {
        short low = (short)(m_Wavefile.Data[index] & 0x00ff);
        short high = (short)((m_Wavefile.Data[index] >> 8) & 0x00ff);
        // Take the max or just the low byte for 8-bit mono
        short sampleVal = std::max(low, high);

        int scaledVal = H - (int)((sampleVal * H) / 256);

        if (m_SamplesPerPixel > 0.0000000001) {
            if (first) {
                path.moveTo(i, scaledVal);
                first = false;
            } else {
                path.lineTo(i, scaledVal);
            }
        }
        else {
            return;
        }

        i++;
        index = (int)(i * m_SamplesPerPixel) + m_OffsetInSamples;
    }
    p.drawPath(path);
}
