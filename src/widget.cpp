#include "widget.h"

#include "audiofileloader.h"
#include "spectrogramrenderer.h"
#include "ui_widget.h"
#include "waveformrenderer.h"

#include <QFont>
#include <QKeyEvent>
#include <QPainter>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    m_DrawWave(false),
    m_IsPanning(false),
    m_OverviewHeight(60),
    m_DetailViewMode(DetailViewMode::Waveform)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::openFile()
{
    m_DrawWave = AudioFileLoader::openAudioFile(this, m_Wavefile, m_Filename);
    if (m_DrawWave) {
        m_Wavefile.WavInfo();
        m_ViewState.resetForData(static_cast<int>(m_Wavefile.datanum), width());
    }
    update();
}

void Widget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_O && event->modifiers() == Qt::ControlModifier) {
        openFile();
    } else if (event->key() == Qt::Key_S && event->modifiers() == Qt::NoModifier) {
        m_DetailViewMode = DetailViewMode::Spectrogram;
        update();
    } else if (event->key() == Qt::Key_W && event->modifiers() == Qt::NoModifier) {
        m_DetailViewMode = DetailViewMode::Waveform;
        update();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void Widget::wheelEvent(QWheelEvent *event)
{
    if (!m_DrawWave) {
        QWidget::wheelEvent(event);
        return;
    }

    const int w = width();
    if (w <= 0) return;

    const double zoomFactor = (event->angleDelta().y() > 0) ? 0.9 : 1.1;
    m_ViewState.zoomAt(event->position().x(), w, static_cast<int>(m_Wavefile.datanum), zoomFactor);
    update();
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_IsPanning = true;
        m_LastMousePos = event->pos();
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_IsPanning || !m_DrawWave) return;
    const QPoint delta = event->pos() - m_LastMousePos;
    m_LastMousePos = event->pos();
    m_ViewState.panPixels(delta.x(), width(), static_cast<int>(m_Wavefile.datanum));
    update();
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_IsPanning = false;
    }
}

void Widget::paintEvent(QPaintEvent *)
{
    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0) return;

    QPixmap pix(w, h);
    pix.fill(Qt::black);
    QPainter p(&pix);

    const int overviewH = m_OverviewHeight;
    const int detailH = h - overviewH;

    if (!m_DrawWave) {
        const QString msg = m_Filename.isEmpty()
            ? tr("Press Ctrl+O to open a WAV file")
            : tr("Cannot read wave file: ") + m_Filename;
        p.drawText(pix.rect(), Qt::AlignCenter, msg);
        QPainter painter(this);
        painter.drawPixmap(0, 0, pix);
        return;
    }

    if (m_Wavefile.datanum > 0) {
        WaveformRenderer::drawOverview(p, m_Wavefile, m_ViewState, w, overviewH);
        p.setPen(QPen(QColor(0x00, 0x60, 0x40)));
        p.drawLine(0, overviewH, w, overviewH);

        if (m_DetailViewMode == DetailViewMode::Spectrogram) {
            SpectrogramRenderer::drawDetail(p, m_Wavefile, m_ViewState, w, detailH, overviewH);
        } else {
            WaveformRenderer::drawDetail(p, m_Wavefile, m_ViewState, w, detailH, overviewH);
        }

        p.setPen(QColor(0xB0, 0xFF, 0xD0));
        QFont font = p.font();
        font.setPointSize(9);
        p.setFont(font);
        const QString zoomText = tr("SPP: %1").arg(m_ViewState.samplesPerPixel(), 0, 'f', 2);
        const QString modeText = (m_DetailViewMode == DetailViewMode::Spectrogram)
            ? tr("Mode: Spectrogram (W: waveform, S: spectrogram)")
            : tr("Mode: Waveform (W: waveform, S: spectrogram)");
        p.drawText(QRect(8, overviewH + 8, w - 16, 20), Qt::AlignLeft | Qt::AlignTop, zoomText);
        p.drawText(QRect(8, overviewH + 26, w - 16, 20), Qt::AlignLeft | Qt::AlignTop, modeText);
    }

    QPainter painter(this);
    painter.drawPixmap(0, 0, pix);
}
