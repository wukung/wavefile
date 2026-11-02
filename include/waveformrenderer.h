#ifndef WAVEFORMRENDERER_H
#define WAVEFORMRENDERER_H

#include <utility>

class QPainter;
class WaveFile;
class ViewState;

namespace WaveformRenderer {
void drawOverview(QPainter &p, const WaveFile &waveFile, const ViewState &viewState, int w, int h);
void drawDetail(QPainter &p, const WaveFile &waveFile, const ViewState &viewState, int w, int h, int yOffset);
std::pair<int, int> columnRange(int x, double samplesPerPixel, double offsetInSamples, int datanum);
}

#endif // WAVEFORMRENDERER_H
