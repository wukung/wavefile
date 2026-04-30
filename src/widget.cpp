#include "widget.h"
#include "ui_widget.h"
#include "pcmformatdialog.h"
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QKeyEvent>
#include <algorithm>
#include <cmath>

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

    if (m_Wavefile.datanum > 0) {
        DrawWaveform(p, W, H);
    }

    // Draw center line on top of waveform
    p.setPen(QPen(QColor(0x00, 0x60, 0x40)));
    p.drawLine(0, H/2, W, H/2);

    QPainter painter(this);
    painter.drawPixmap(0, 0, pix);
}

void Widget::DrawWaveform(QPainter &p, int W, int H)
{
    if (m_SamplesPerPixel <= 0.0)
        return;

    const int datanum = static_cast<int>(m_Wavefile.datanum);

    // CoolEdit-style colours
    // Outer envelope (min/max range): medium green
    const QColor outerColor(0x27, 0x7A, 0x5C);
    // Inner RMS band: bright teal
    const QColor rmsColor(0x4B, 0xF3, 0xA7);

    // Helper: map a signed 16-bit sample to screen Y (0 = top, H = bottom)
    auto scaleY = [&](int sample) -> int {
        return H - static_cast<int>(((static_cast<long long>(sample) + 32768LL) * H) / 65536LL);
    };

    for (int x = 0; x < W; x++) {
        int startIdx = static_cast<int>(x * m_SamplesPerPixel) + m_OffsetInSamples;
        int endIdx   = static_cast<int>((x + 1) * m_SamplesPerPixel) + m_OffsetInSamples;
        if (endIdx <= startIdx) endIdx = startIdx + 1;
        endIdx = std::min(endIdx, datanum);
        if (startIdx >= datanum) break;

        short minVal = m_Wavefile.Data[startIdx];
        short maxVal = minVal;
        double sumSq = 0.0;

        for (int s = startIdx; s < endIdx; s++) {
            short v = m_Wavefile.Data[s];
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
            sumSq += static_cast<double>(v) * v;
        }

        int count = endIdx - startIdx;
        double rms = (count > 0) ? std::sqrt(sumSq / count) : 0.0;

        // Screen coordinates for outer envelope
        int yTop = scaleY(maxVal); // max sample → highest point (smallest Y)
        int yBot = scaleY(minVal); // min sample → lowest point (largest Y)
        if (yTop > yBot) std::swap(yTop, yBot);

        // Draw outer envelope
        p.setPen(QPen(outerColor));
        p.drawLine(x, yTop, x, yBot);

        // Draw inner RMS band (clamped to the envelope)
        int rmsPixels = static_cast<int>(rms * H / 65536.0);
        int yRmsTop   = std::max(H / 2 - rmsPixels, yTop);
        int yRmsBot   = std::min(H / 2 + rmsPixels, yBot);
        if (yRmsTop <= yRmsBot) {
            p.setPen(QPen(rmsColor));
            p.drawLine(x, yRmsTop, x, yRmsBot);
        }
    }
}
