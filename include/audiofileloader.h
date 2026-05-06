#ifndef AUDIOFILELOADER_H
#define AUDIOFILELOADER_H

#include <QString>

class QWidget;
class WaveFile;

namespace AudioFileLoader {
bool openAudioFile(QWidget *parent, WaveFile &waveFile, QString &filenameOut);
}

#endif // AUDIOFILELOADER_H
