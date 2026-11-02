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

    if (waveFile.WavRead(filename.toStdString())) {
        filenameOut = filename;
        return true;
    }

    PcmFormatDialog dlg(parent);
    if (dlg.exec() == QDialog::Accepted) {
        if (waveFile.RawRead(filename.toStdString(), dlg.format())) {
            filenameOut = filename;
            return true;
        }
    }
    return false;
}

} // namespace AudioFileLoader
