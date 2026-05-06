#ifndef VIEWSTATE_H
#define VIEWSTATE_H

class ViewState
{
public:
    ViewState();

    void resetForData(int dataCount, int widgetWidth);
    void zoomAt(double mouseX, int widgetWidth, int dataCount, double zoomFactor);
    void panPixels(int deltaX, int widgetWidth, int dataCount);

    double samplesPerPixel() const { return m_SamplesPerPixel; }
    double offsetInSamples() const { return m_OffsetInSamples; }
    double viewportSampleWidth(int widgetWidth) const;

private:
    void clampOffset(int widgetWidth, int dataCount);

    double m_SamplesPerPixel;
    double m_OffsetInSamples;
};

#endif // VIEWSTATE_H
