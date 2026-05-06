#include "spectrogramrenderer.h"

#include "WaveFile.h"
#include "viewstate.h"

#include <QColor>
#include <QImage>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <vector>

#ifdef WAVEFILE_USE_FFTW
#include <fftw3.h>
#endif

namespace {

QColor spectrogramColorMap(double t)
{
    t = std::clamp(t, 0.0, 1.0);
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

namespace SpectrogramRenderer {

void drawDetail(QPainter &p, const WaveFile &waveFile, const ViewState &viewState, int w, int h, int yOffset)
{
    const double samplesPerPixel = viewState.samplesPerPixel();
    const double offsetInSamples = viewState.offsetInSamples();
    if (w <= 0 || h <= 0 || samplesPerPixel <= 0.0 || waveFile.datanum <= 0) return;

    const int datanum = static_cast<int>(waveFile.datanum);
    const int fftSize = 512;
    const int halfBins = fftSize / 2 + 1;
    constexpr double kPi = 3.14159265358979323846;

    QImage image(w, h, QImage::Format_RGB32);
    image.fill(QColor(0, 0, 0));

    std::vector<double> window(static_cast<size_t>(fftSize));
    for (int n = 0; n < fftSize; ++n) {
        window[static_cast<size_t>(n)] = 0.5 - 0.5 * std::cos((2.0 * kPi * n) / (fftSize - 1));
    }

#ifdef WAVEFILE_USE_FFTW
    std::vector<double> frame(static_cast<size_t>(fftSize), 0.0);
    fftw_complex *spectrum = reinterpret_cast<fftw_complex *>(
        fftw_malloc(sizeof(fftw_complex) * static_cast<size_t>(halfBins)));
    if (!spectrum) return;
    fftw_plan plan = fftw_plan_dft_r2c_1d(fftSize, frame.data(), spectrum, FFTW_ESTIMATE);
    if (!plan) {
        fftw_free(spectrum);
        return;
    }

    std::vector<double> magnitudes(static_cast<size_t>(halfBins), 0.0);
    for (int x = 0; x < w; ++x) {
        const double centerPos = offsetInSamples + x * samplesPerPixel;
        const int centerSample = static_cast<int>(std::lround(centerPos));
        for (int n = 0; n < fftSize; ++n) {
            const int idx = centerSample + n - fftSize / 2;
            double s = 0.0;
            if (idx >= 0 && idx < datanum) s = static_cast<double>(waveFile.Data[idx]) / 32768.0;
            frame[static_cast<size_t>(n)] = s * window[static_cast<size_t>(n)];
        }

        fftw_execute(plan);
        for (int b = 0; b < halfBins; ++b) {
            const double re = spectrum[static_cast<size_t>(b)][0];
            const double im = spectrum[static_cast<size_t>(b)][1];
            magnitudes[static_cast<size_t>(b)] = std::sqrt(re * re + im * im) / fftSize;
        }

        for (int y = 0; y < h; ++y) {
            const double nyquistRatio = static_cast<double>(h - 1 - y) / std::max(1, h - 1);
            const int bin = std::clamp(static_cast<int>(nyquistRatio * (halfBins - 1)), 0, halfBins - 1);
            const double db = 20.0 * std::log10(magnitudes[static_cast<size_t>(bin)] + 1e-9);
            const double normalized = std::clamp((db + 90.0) / 90.0, 0.0, 1.0);
            image.setPixelColor(x, y, spectrogramColorMap(normalized));
        }
    }
    fftw_destroy_plan(plan);
    fftw_free(spectrum);
#else
    std::vector<double> frame(static_cast<size_t>(fftSize), 0.0);
    for (int x = 0; x < w; ++x) {
        const double centerPos = offsetInSamples + x * samplesPerPixel;
        const int centerSample = static_cast<int>(std::lround(centerPos));
        for (int n = 0; n < fftSize; ++n) {
            const int idx = centerSample + n - fftSize / 2;
            double s = 0.0;
            if (idx >= 0 && idx < datanum) s = static_cast<double>(waveFile.Data[idx]) / 32768.0;
            frame[static_cast<size_t>(n)] = s * window[static_cast<size_t>(n)];
        }
        for (int y = 0; y < h; ++y) {
            const double nyquistRatio = static_cast<double>(h - 1 - y) / std::max(1, h - 1);
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
            image.setPixelColor(x, y, spectrogramColorMap(normalized));
        }
    }
#endif

    p.drawImage(0, yOffset, image);
}

} // namespace SpectrogramRenderer
