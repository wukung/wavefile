#ifndef SPECTROGRAMRENDERER_H
#define SPECTROGRAMRENDERER_H

class QPainter;
class WaveFile;
class ViewState;

namespace SpectrogramRenderer {
void drawDetail(QPainter &p, const WaveFile &waveFile, const ViewState &viewState, int w, int h, int yOffset);
}

#endif // SPECTROGRAMRENDERER_H
