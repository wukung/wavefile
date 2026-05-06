#ifndef WAVEFORMRENDERER_H
#define WAVEFORMRENDERER_H

class QPainter;
class WaveFile;
class ViewState;

namespace WaveformRenderer {
void drawOverview(QPainter &p, const WaveFile &waveFile, const ViewState &viewState, int w, int h);
void drawDetail(QPainter &p, const WaveFile &waveFile, const ViewState &viewState, int w, int h, int yOffset);
}

#endif // WAVEFORMRENDERER_H
