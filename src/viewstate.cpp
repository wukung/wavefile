#include "viewstate.h"

#include <algorithm>

namespace {
constexpr double kMinSamplesPerPixel = 1.0 / 16.0;
}

ViewState::ViewState()
    : m_SamplesPerPixel(1.0),
      m_OffsetInSamples(0.0)
{
}

void ViewState::resetForData(int dataCount, int widgetWidth)
{
    const int safeWidth = std::max(1, widgetWidth);
    m_SamplesPerPixel = std::max(kMinSamplesPerPixel, static_cast<double>(dataCount) / safeWidth);
    m_OffsetInSamples = 0.0;
}

void ViewState::zoomAt(double mouseX, int widgetWidth, int dataCount, double zoomFactor)
{
    if (widgetWidth <= 0 || dataCount <= 0 || zoomFactor <= 0.0) return;

    const double sampleAtMouse = mouseX * m_SamplesPerPixel + m_OffsetInSamples;
    double newSamplesPerPixel = m_SamplesPerPixel * zoomFactor;
    const double maxSamplesPerPixel = std::max(1.0, static_cast<double>(dataCount) / widgetWidth);

    newSamplesPerPixel = std::clamp(newSamplesPerPixel, kMinSamplesPerPixel, maxSamplesPerPixel);

    m_SamplesPerPixel = newSamplesPerPixel;
    m_OffsetInSamples = sampleAtMouse - mouseX * m_SamplesPerPixel;
    clampOffset(widgetWidth, dataCount);
}

void ViewState::panPixels(int deltaX, int widgetWidth, int dataCount)
{
    if (widgetWidth <= 0 || dataCount <= 0) return;
    m_OffsetInSamples -= deltaX * m_SamplesPerPixel;
    clampOffset(widgetWidth, dataCount);
}

double ViewState::viewportSampleWidth(int widgetWidth) const
{
    return widgetWidth > 0 ? widgetWidth * m_SamplesPerPixel : 0.0;
}

void ViewState::clampOffset(int widgetWidth, int dataCount)
{
    if (widgetWidth <= 0 || dataCount <= 0) {
        m_OffsetInSamples = 0.0;
        return;
    }

    const double visibleSamples = widgetWidth * m_SamplesPerPixel;
    if (dataCount > visibleSamples) {
        m_OffsetInSamples = std::clamp(m_OffsetInSamples, 0.0, static_cast<double>(dataCount) - visibleSamples);
    } else {
        m_OffsetInSamples = 0.0;
    }
}
