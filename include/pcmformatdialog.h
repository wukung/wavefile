#ifndef PCMFORMATDIALOG_H
#define PCMFORMATDIALOG_H

#include <QDialog>
#include "WaveFile.h"

class QComboBox;
class QLabel;
class QDialogButtonBox;

class PcmFormatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PcmFormatDialog(QWidget *parent = nullptr);

    WaveFile::RawPcmFormat format() const;

private:
    QComboBox *m_sampleRateBox;
    QComboBox *m_bitDepthBox;
    QComboBox *m_channelsBox;
    QComboBox *m_endianBox;
};

#endif // PCMFORMATDIALOG_H
