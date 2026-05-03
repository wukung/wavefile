#include "widget.h"
#include "ui_widget.h"
#include "pcmformatdialog.h"
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QKeyEvent>
#include <QFont>
#include <QPointF>
#include <QImage>
#include <QColor>
#include <vector>
#include <algorithm>
#include <cmath>
#ifdef WAVEFILE_USE_FFTW
#include <fftw3.h>
#endif

namespace {
QColor SpectrogramColorMap(double t)
{
    t = std::clamp(t, 0.0, 1.0);

    // Multi-stop "turbo-like" map:
    // dark navy -> blue -> cyan -> green -> yellow -> orange -> red.
    struct Stop {
        double pos;
        int r;
        int g;
        int b;
    };
    constexpr Stop stops[] = {
        {0.00,  10,   8,  30},
        {0.15,  25,  35, 110},
        {0.35,  20, 130, 220},
        {0.52,  15, 200, 150},
        {0.68, 180, 230,  40},
        {0.84, 250, 150,  35},
        {1.00, 235,  45,  35},
    };

    for (size_t i = 0; i + 1 < std::size(stops); ++i) {
        if (t >= stops[i].pos && t <= stops[i + 1].pos) {
            const double span = stops[i + 1].pos - stops[i].pos;
            const double u = (span > 0.0) ? (t - stops[i].pos) / span : 0.0;
            const int r = static_cast<int>(std::lround(stops[i].r + (stops[i + 1].r - stops[i].r) * u));
            const int g = static_cast<int>(std::lround(stops[i].g + (stops[i + 1].g - stops[i].g) * u));
            const int b = static_cast<int>(std::lround(stops[i].b + (stops[i + 1].b - stops[i].b) * u));
            return QColor(r, g, b);
        }
    }
    return QColor(stops[std::size(stops) - 1].r, stops[std::size(stops) - 1].g, stops[std::size(stops) - 1].b);
}
} // namespace

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    m_IsPanning(false),
    m_OverviewHeight(60),
    m_DetailViewMode(DetailViewMode::Waveform)
{
    ui->setupUi(this);

    m_DrawWave = false;
    m_SamplesPerPixel = 1.0;
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
        m_SamplesPerPixel = std::max(1.0 / 16.0, static_cast<double>(m_Wavefile.datanum) / (width() > 0 ? width() : 1));
        m_OffsetInSamples = 0;
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

    int W = this->width();
    if (W <= 0) return;

    double mouseX = event->position().x();
    double sampleAtMouse = mouseX * m_SamplesPerPixel + m_OffsetInSamples;

    double zoomFactor = (event->angleDelta().y() > 0) ? 0.9 : 1.1;
    double newSamplesPerPixel = m_SamplesPerPixel * zoomFactor;

    // Allow zooming past single-sample to show per-sample points clearly.
    double minSamplesPerPixel = 1.0 / 16.0;
    double maxSamplesPerPixel = std::max(1.0, static_cast<double>(m_Wavefile.datanum) / W);

    if (newSamplesPerPixel < minSamplesPerPixel) newSamplesPerPixel = minSamplesPerPixel;
    if (newSamplesPerPixel > maxSamplesPerPixel) newSamplesPerPixel = maxSamplesPerPixel;

    m_SamplesPerPixel = newSamplesPerPixel;
    m_OffsetInSamples = sampleAtMouse - mouseX * m_SamplesPerPixel;

    if (m_Wavefile.datanum > (W * m_SamplesPerPixel)) {
        m_OffsetInSamples = std::max(0.0, std::min(m_OffsetInSamples, (double)m_Wavefile.datanum - (W * m_SamplesPerPixel)));
    } else {
        m_OffsetInSamples = 0;
    }

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
    if (m_IsPanning) {
        QPoint delta = event->pos() - m_LastMousePos;
        m_LastMousePos = event->pos();

        m_OffsetInSamples -= delta.x() * m_SamplesPerPixel;

        int W = this->width();
        if (m_Wavefile.datanum > (W * m_SamplesPerPixel)) {
            m_OffsetInSamples = std::max(0.0, std::min(m_OffsetInSamples, (double)m_Wavefile.datanum - (W * m_SamplesPerPixel)));
        } else {
            m_OffsetInSamples = 0;
        }

        update();
    }
}

void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_IsPanning = false;
    }
}

void Widget::paintEvent(QPaintEvent *)
{
    int W = this->width();
    int H = this->height();

    if (W <= 0 || H <= 0) {
        return;
    }

    QPixmap pix(W, H);
    pix.fill(Qt::black);
    QPainter p(&pix);

    const int overviewH = m_OverviewHeight;
    const int detailH = H - overviewH;

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
        // Draw overview
        DrawOverview(p, W, overviewH);

        // Draw separator line
        p.setPen(QPen(QColor(0x00, 0x60, 0x40)));
        p.drawLine(0, overviewH, W, overviewH);

        // Draw detail view
        if (m_DetailViewMode == DetailViewMode::Spectrogram) {
            DrawSpectrogram(p, W, detailH, m_SamplesPerPixel, m_OffsetInSamples, overviewH);
        } else {
            DrawWaveform(p, W, detailH, m_SamplesPerPixel, m_OffsetInSamples, overviewH);
        }

        p.setPen(QColor(0xB0, 0xFF, 0xD0));
        QFont font = p.font();
        font.setPointSize(9);
        p.setFont(font);
        const QString zoomText = tr("SPP: %1").arg(m_SamplesPerPixel, 0, 'f', 2);
        const QString modeText = (m_DetailViewMode == DetailViewMode::Spectrogram)
            ? tr("Mode: Spectrogram (W: waveform, S: spectrogram)")
            : tr("Mode: Waveform (W: waveform, S: spectrogram)");
        p.drawText(QRect(8, overviewH + 8, W - 16, 20), Qt::AlignLeft | Qt::AlignTop, zoomText);
        p.drawText(QRect(8, overviewH + 26, W - 16, 20), Qt::AlignLeft | Qt::AlignTop, modeText);
    }

    QPainter painter(this);
    painter.drawPixmap(0, 0, pix);
}

void Widget::DrawSpectrogram(QPainter &p, int W, int H, double samplesPerPixel, double offsetInSamples, int yOffset)
{
    if (W <= 0 || H <= 0 || samplesPerPixel <= 0.0 || m_Wavefile.datanum <= 0) {
        return;
    }

    const int datanum = static_cast<int>(m_Wavefile.datanum);
    const int fftSize = 512;
    const int halfBins = fftSize / 2 + 1;
    constexpr double kPi = 3.14159265358979323846;

    QImage image(W, H, QImage::Format_RGB32);
    image.fill(QColor(0, 0, 0));

    std::vector<double> window(static_cast<size_t>(fftSize));
    for (int n = 0; n < fftSize; ++n) {
        window[static_cast<size_t>(n)] = 0.5 - 0.5 * std::cos((2.0 * kPi * n) / (fftSize - 1));
    }

#ifdef WAVEFILE_USE_FFTW
    std::vector<double> frame(static_cast<size_t>(fftSize), 0.0);
    fftw_complex *spectrum = reinterpret_cast<fftw_complex *>(
        fftw_malloc(sizeof(fftw_complex) * static_cast<size_t>(halfBins)));
    if (!spectrum) {
        return;
    }
    fftw_plan plan = fftw_plan_dft_r2c_1d(fftSize, frame.data(), spectrum, FFTW_ESTIMATE);
    if (!plan) {
        fftw_free(spectrum);
        return;
    }

    std::vector<double> magnitudes(static_cast<size_t>(halfBins), 0.0);
    for (int x = 0; x < W; ++x) {
        const double centerPos = offsetInSamples + x * samplesPerPixel;
        const int centerSample = static_cast<int>(std::lround(centerPos));

        for (int n = 0; n < fftSize; ++n) {
            const int idx = centerSample + n - fftSize / 2;
            double s = 0.0;
            if (idx >= 0 && idx < datanum) {
                s = static_cast<double>(m_Wavefile.Data[idx]) / 32768.0;
            }
            frame[static_cast<size_t>(n)] = s * window[static_cast<size_t>(n)];
        }

        fftw_execute(plan);

        for (int b = 0; b < halfBins; ++b) {
            const double re = spectrum[static_cast<size_t>(b)][0];
            const double im = spectrum[static_cast<size_t>(b)][1];
            magnitudes[static_cast<size_t>(b)] = std::sqrt(re * re + im * im) / fftSize;
        }

        for (int y = 0; y < H; ++y) {
            const double nyquistRatio = static_cast<double>(H - 1 - y) / std::max(1, H - 1);
            const int bin = std::clamp(static_cast<int>(nyquistRatio * (halfBins - 1)), 0, halfBins - 1);
            const double db = 20.0 * std::log10(magnitudes[static_cast<size_t>(bin)] + 1e-9);
            const double normalized = std::clamp((db + 90.0) / 90.0, 0.0, 1.0);

            image.setPixelColor(x, y, SpectrogramColorMap(normalized));
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(spectrum);
#else
    // Fallback path when FFTW is unavailable.
    std::vector<double> frame(static_cast<size_t>(fftSize), 0.0);
    for (int x = 0; x < W; ++x) {
        const double centerPos = offsetInSamples + x * samplesPerPixel;
        const int centerSample = static_cast<int>(std::lround(centerPos));
        for (int n = 0; n < fftSize; ++n) {
            const int idx = centerSample + n - fftSize / 2;
            double s = 0.0;
            if (idx >= 0 && idx < datanum) {
                s = static_cast<double>(m_Wavefile.Data[idx]) / 32768.0;
            }
            frame[static_cast<size_t>(n)] = s * window[static_cast<size_t>(n)];
        }
        for (int y = 0; y < H; ++y) {
            const double nyquistRatio = static_cast<double>(H - 1 - y) / std::max(1, H - 1);
            const int bin = std::clamp(static_cast<int>(nyquistRatio * (halfBins - 1)), 0, halfBins - 1);
            double real = 0.0;
            double imag = 0.0;
            for (int n = 0; n < fftSize; ++n) {
                const double phase = (2.0 * kPi * bin * n) / fftSize;
                const double sample = frame[static_cast<size_t>(n)];
                real += sample * std::cos(phase);
                imag -= sample * std::sin(phase);
            }
            const double magnitude = std::sqrt(real * real + imag * imag) / fftSize;
            const double db = 20.0 * std::log10(magnitude + 1e-9);
            const double normalized = std::clamp((db + 90.0) / 90.0, 0.0, 1.0);
            image.setPixelColor(x, y, SpectrogramColorMap(normalized));
        }
    }
#endif

    p.drawImage(0, yOffset, image);
}

void Widget::DrawOverview(QPainter &p, int W, int H)
{
    if (W <= 0 || H <= 0 || m_Wavefile.datanum == 0) return;

    const int datanum = static_cast<int>(m_Wavefile.datanum);
    const QColor outerColor(0x1A, 0x5A, 0x4C);
    auto scaleY = [&](int sample) -> int {
        return H - static_cast<int>(((static_cast<long long>(sample) + 32768LL) * H) / 65536LL);
    };

    double overviewSamplesPerPixel = (double)datanum / W;

    for (int x = 0; x < W; x++) {
        int startIdx = static_cast<int>(x * overviewSamplesPerPixel);
        int endIdx   = static_cast<int>((x + 1) * overviewSamplesPerPixel);
        endIdx = std::min(endIdx, datanum);
        if (startIdx >= datanum) break;

        short minVal = m_Wavefile.Data[startIdx];
        short maxVal = minVal;

        for (int s = startIdx; s < endIdx; s++) {
            short v = m_Wavefile.Data[s];
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
        }

        int yTop = scaleY(maxVal);
        int yBot = scaleY(minVal);
        if (yTop > yBot) std::swap(yTop, yBot);

        p.setPen(QPen(outerColor));
        p.drawLine(x, yTop, x, yBot);
    }

    // Draw viewport indicator
    const double viewportStart = std::max(0.0, m_OffsetInSamples);
    const double viewportWidth = std::min(static_cast<double>(datanum), W * m_SamplesPerPixel);

    if (viewportWidth > 0 && viewportWidth < datanum) {
        int viewX = static_cast<int>(std::lround((viewportStart / datanum) * W));
        int viewW = static_cast<int>(std::lround((viewportWidth / datanum) * W));
        viewX = std::clamp(viewX, 0, W - 1);
        viewW = std::max(2, viewW);
        if (viewX + viewW > W) viewW = W - viewX;

        p.setPen(Qt::NoPen);
        p.fillRect(viewX, 0, viewW, H, QBrush(QColor(0x4B, 0xF3, 0xA7, 30)));

        p.setPen(QPen(QColor(0x4B, 0xF3, 0xA7)));
        p.drawRect(viewX, 0, viewW, H - 1);
    }
}

void Widget::DrawWaveform(QPainter &p, int W, int H, double samplesPerPixel, double offsetInSamples, int yOffset)
{
    if (samplesPerPixel <= 0.0 || W <= 0 || H <= 0)
        return;

    const int datanum = static_cast<int>(m_Wavefile.datanum);

    // CoolEdit-style colours
    const QColor outerColor(0x27, 0x7A, 0x5C);
    const QColor rmsColor(0x4B, 0xF3, 0xA7);

    auto scaleY = [&](int sample) -> int {
        return yOffset + H - static_cast<int>(((static_cast<long long>(sample) + 32768LL) * H) / 65536LL);
    };

    const bool smoothMode = samplesPerPixel <= 8.0;
    if (smoothMode) {
        std::vector<QPointF> points;
        points.reserve(static_cast<size_t>(W));
        for (int x = 0; x < W; x++) {
            const double samplePos = offsetInSamples + (x * samplesPerPixel);
            if (samplePos < 0.0 || samplePos >= datanum) continue;

            const int idx0 = std::clamp(static_cast<int>(std::floor(samplePos)), 0, datanum - 1);
            const int idx1 = std::min(idx0 + 1, datanum - 1);
            const double frac = samplePos - idx0;
            const double v = (1.0 - frac) * m_Wavefile.Data[idx0] + frac * m_Wavefile.Data[idx1];
            int y = scaleY(static_cast<int>(std::lround(v)));
            points.emplace_back(x, y);
        }

        if (points.empty()) return;

        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(rmsColor, 1.6));

        QPainterPath smoothPath;
        smoothPath.moveTo(points[0]);
        if (points.size() == 1) {
            p.drawPoint(points[0]);
        } else {
            for (size_t i = 0; i + 1 < points.size(); ++i) {
                const QPointF &p0 = (i == 0) ? points[i] : points[i - 1];
                const QPointF &p1 = points[i];
                const QPointF &p2 = points[i + 1];
                const QPointF &p3 = (i + 2 < points.size()) ? points[i + 2] : points[i + 1];

                const QPointF c1 = p1 + (p2 - p0) / 6.0;
                const QPointF c2 = p2 - (p3 - p1) / 6.0;
                smoothPath.cubicTo(c1, c2, p2);
            }
            p.drawPath(smoothPath);
        }

        // At high zoom (many pixels per sample), show sample points with a smooth
        // curve through samples (ocenaudio/CoolEdit-like) instead of jagged
        // point-to-point poly-lines.
        const double pixelsPerSample = 1.0 / samplesPerPixel;
        if (pixelsPerSample >= 4.0) {
            const int firstSample = std::max(0, static_cast<int>(std::floor(offsetInSamples)));
            const int lastSample = std::min(datanum - 1, static_cast<int>(std::ceil(offsetInSamples + W * samplesPerPixel)));
            std::vector<QPointF> samplePoints;
            samplePoints.reserve(static_cast<size_t>(std::max(0, lastSample - firstSample + 1)));

            for (int s = firstSample; s <= lastSample; ++s) {
                const double x = (s - offsetInSamples) / samplesPerPixel;
                if (x < 0.0 || x > W) continue;
                const int y = scaleY(m_Wavefile.Data[s]);
                samplePoints.emplace_back(x, y);
            }

            if (!samplePoints.empty()) {
                p.setRenderHint(QPainter::Antialiasing, true);

                QPainterPath sampleSmoothPath;
                sampleSmoothPath.moveTo(samplePoints[0]);
                if (samplePoints.size() > 1) {
                    for (size_t i = 0; i + 1 < samplePoints.size(); ++i) {
                        const QPointF &p0 = (i == 0) ? samplePoints[i] : samplePoints[i - 1];
                        const QPointF &p1 = samplePoints[i];
                        const QPointF &p2 = samplePoints[i + 1];
                        const QPointF &p3 = (i + 2 < samplePoints.size()) ? samplePoints[i + 2] : samplePoints[i + 1];

                        const QPointF c1 = p1 + (p2 - p0) / 6.0;
                        const QPointF c2 = p2 - (p3 - p1) / 6.0;
                        sampleSmoothPath.cubicTo(c1, c2, p2);
                    }
                }

                p.setPen(QPen(QColor(0xA2, 0xFF, 0xD4), 1.2));
                p.drawPath(sampleSmoothPath);

                p.setPen(QPen(QColor(0xC8, 0xFF, 0xE8), 0.9));
                p.setBrush(QColor(0xD5, 0xFF, 0xEA));
                const double radius = (pixelsPerSample >= 10.0) ? 2.3 : 1.8;
                for (const QPointF &pt : samplePoints) {
                    p.drawEllipse(pt, radius, radius);
                }
            }
        }

        p.restore();
        return;
    }

    for (int x = 0; x < W; x++) {
        int startIdx = static_cast<int>(x * samplesPerPixel) + static_cast<int>(offsetInSamples);
        int endIdx   = static_cast<int>((x + 1) * samplesPerPixel) + static_cast<int>(offsetInSamples);
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

        int yTop = scaleY(maxVal);
        int yBot = scaleY(minVal);
        if (yTop > yBot) std::swap(yTop, yBot);

        p.setPen(QPen(outerColor));
        p.drawLine(x, yTop, x, yBot);

        int rmsPixels = static_cast<int>(rms * H / 65536.0);
        int yRmsTop   = std::max(yOffset + H / 2 - rmsPixels, yTop);
        int yRmsBot   = std::min(yOffset + H / 2 + rmsPixels, yBot);
        if (yRmsTop <= yRmsBot) {
            p.setPen(QPen(rmsColor));
            p.drawLine(x, yRmsTop, x, yRmsBot);
        }
    }
}
