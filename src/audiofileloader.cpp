#include "audiofileloader.h"

#include "WaveFile.h"
#include "pcmformatdialog.h"

#include <QDialog>
#include <QFileDialog>
#include <QWidget>

namespace AudioFileLoader {

bool openAudioFile(QWidget *parent, WaveFile &waveFile, QString &filenameOut)
{
    const QString filename = QFileDialog::getOpenFileName(
        parent,
        QObject::tr("Open Audio File"),
        QString(),
        QObject::tr("WAV Files (*.wav);;Raw PCM Files (*.raw *.pcm *.bin);;All Files (*)"));

    if (filename.isEmpty()) return false;

    filenameOut = filename;
    if (waveFile.WavRead(filenameOut.toStdString())) return true;

    PcmFormatDialog dlg(parent);
    if (dlg.exec() == QDialog::Accepted) {
        return waveFile.RawRead(filenameOut.toStdString(), dlg.format());
    }
    return false;
}

} // namespace AudioFileLoader
