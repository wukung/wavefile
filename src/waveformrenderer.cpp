#include "waveformrenderer.h"

#include "WaveFile.h"
#include "viewstate.h"

#include <QPainter>
#include <QPainterPath>
#include <QPointF>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace WaveformRenderer {

std::pair<int, int> columnRange(int x, double samplesPerPixel, double offsetInSamples, int datanum)
{
    int startIdx = static_cast<int>(x * samplesPerPixel + offsetInSamples);
    int endIdx = static_cast<int>((x + 1) * samplesPerPixel + offsetInSamples);
    if (endIdx <= startIdx) endIdx = startIdx + 1;
    endIdx = std::min(endIdx, datanum);
    return {startIdx, endIdx};
}

void drawOverview(QPainter &p, const WaveFile &waveFile, const ViewState &viewState, int w, int h)
{
    if (w <= 0 || h <= 0 || waveFile.datanum == 0) return;

    const int datanum = static_cast<int>(waveFile.datanum);
    const QColor outerColor(0x1A, 0x5A, 0x4C);
    auto scaleY = [&](int sample) -> int {
        return h - static_cast<int>(((static_cast<long long>(sample) + 32768LL) * h) / 65536LL);
    };

    const double overviewSamplesPerPixel = static_cast<double>(datanum) / w;
    for (int x = 0; x < w; x++) {
        int startIdx = static_cast<int>(x * overviewSamplesPerPixel);
        int endIdx   = static_cast<int>((x + 1) * overviewSamplesPerPixel);
        endIdx = std::min(endIdx, datanum);
        if (startIdx >= datanum) break;

        short minVal = waveFile.Data[startIdx];
        short maxVal = minVal;
        for (int s = startIdx; s < endIdx; s++) {
            short v = waveFile.Data[s];
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
        }

        int yTop = scaleY(maxVal);
        int yBot = scaleY(minVal);
        if (yTop > yBot) std::swap(yTop, yBot);

        p.setPen(QPen(outerColor));
        p.drawLine(x, yTop, x, yBot);
    }

    const double viewportStart = std::max(0.0, viewState.offsetInSamples());
    const double viewportWidth = std::min(static_cast<double>(datanum), viewState.viewportSampleWidth(w));
    if (viewportWidth > 0 && viewportWidth < datanum) {
        int viewX = static_cast<int>(std::lround((viewportStart / datanum) * w));
        int viewW = static_cast<int>(std::lround((viewportWidth / datanum) * w));
        viewX = std::clamp(viewX, 0, w - 1);
        viewW = std::max(2, viewW);
        if (viewX + viewW > w) viewW = w - viewX;

        p.setPen(Qt::NoPen);
        p.fillRect(viewX, 0, viewW, h, QBrush(QColor(0x4B, 0xF3, 0xA7, 30)));

        p.setPen(QPen(QColor(0x4B, 0xF3, 0xA7)));
        p.drawRect(viewX, 0, viewW, h - 1);
    }
}

void drawDetail(QPainter &p, const WaveFile &waveFile, const ViewState &viewState, int w, int h, int yOffset)
{
    const double samplesPerPixel = viewState.samplesPerPixel();
    const double offsetInSamples = viewState.offsetInSamples();
    if (samplesPerPixel <= 0.0 || w <= 0 || h <= 0 || waveFile.datanum <= 0) return;

    const int datanum = static_cast<int>(waveFile.datanum);
    const QColor outerColor(0x27, 0x7A, 0x5C);
    const QColor rmsColor(0x4B, 0xF3, 0xA7);

    auto scaleY = [&](int sample) -> int {
        return yOffset + h - static_cast<int>(((static_cast<long long>(sample) + 32768LL) * h) / 65536LL);
    };

    if (samplesPerPixel <= 8.0) {
        const double pixelsPerSample = 1.0 / samplesPerPixel;
        const int firstSample = std::max(0, static_cast<int>(std::floor(offsetInSamples)));
        const int lastSample = std::min(datanum - 1, static_cast<int>(std::ceil(offsetInSamples + w * samplesPerPixel)));
        std::vector<QPointF> samplePoints;
        if (lastSample >= firstSample)
            samplePoints.reserve(static_cast<size_t>(lastSample - firstSample + 1));

        for (int s = firstSample; s <= lastSample; ++s) {
            const double x = (s - offsetInSamples) / samplesPerPixel;
            if (x < 0.0 || x > w) continue;
            samplePoints.emplace_back(x, scaleY(waveFile.Data[s]));
        }
        if (samplePoints.empty()) return;

        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);

        QPainterPath tracePath;
        tracePath.moveTo(samplePoints[0]);
        if (samplePoints.size() == 1) {
            p.setPen(QPen(rmsColor, 1.6));
            p.drawPoint(samplePoints[0]);
        } else {
            for (size_t i = 1; i < samplePoints.size(); ++i)
                tracePath.lineTo(samplePoints[i]);
            p.setPen(QPen(rmsColor, 1.6));
            p.drawPath(tracePath);
        }

        if (pixelsPerSample >= 4.0) {
            p.setPen(QPen(QColor(0xC8, 0xFF, 0xE8), 0.9));
            p.setBrush(QColor(0xD5, 0xFF, 0xEA));
            const double radius = (pixelsPerSample >= 10.0) ? 2.3 : 1.8;
            for (const QPointF &pt : samplePoints) p.drawEllipse(pt, radius, radius);
        }
        p.restore();
        return;
    }

    for (int x = 0; x < w; x++) {
        const auto [startIdx, endIdx] = columnRange(x, samplesPerPixel, offsetInSamples, datanum);
        if (startIdx >= datanum) break;

        short minVal = waveFile.Data[startIdx];
        short maxVal = minVal;
        double sumSq = 0.0;
        for (int s = startIdx; s < endIdx; s++) {
            short v = waveFile.Data[s];
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
            sumSq += static_cast<double>(v) * v;
        }

        const int count = endIdx - startIdx;
        const double rms = (count > 0) ? std::sqrt(sumSq / count) : 0.0;

        int yTop = scaleY(maxVal);
        int yBot = scaleY(minVal);
        if (yTop > yBot) std::swap(yTop, yBot);

        p.setPen(QPen(outerColor));
        p.drawLine(x, yTop, x, yBot);

        const int rmsPixels = static_cast<int>(rms * h / 65536.0);
        const int yRmsTop = std::max(yOffset + h / 2 - rmsPixels, yTop);
        const int yRmsBot = std::min(yOffset + h / 2 + rmsPixels, yBot);
        if (yRmsTop <= yRmsBot) {
            p.setPen(QPen(rmsColor));
            p.drawLine(x, yRmsTop, x, yRmsBot);
        }
    }
}

} // namespace WaveformRenderer
