#include "pcmformatdialog.h"
#include <QComboBox>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>

using Encoding = WaveFile::RawPcmFormat::Encoding;

PcmFormatDialog::PcmFormatDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Raw PCM Format"));

    m_sampleRateBox = new QComboBox(this);
    for (int rate : {8000, 11025, 16000, 22050, 44100, 48000, 96000})
        m_sampleRateBox->addItem(QString::number(rate) + " Hz", rate);
    m_sampleRateBox->setCurrentIndex(0); // 8000 Hz as default

    m_bitDepthBox = new QComboBox(this);
    m_bitDepthBox->addItem("8-bit",  8);
    m_bitDepthBox->addItem("16-bit", 16);
    m_bitDepthBox->setCurrentIndex(1); // 16-bit as default

    m_channelsBox = new QComboBox(this);
    m_channelsBox->addItem(tr("Mono"),   1);
    m_channelsBox->addItem(tr("Stereo"), 2);

    m_endianBox = new QComboBox(this);
    m_endianBox->addItem(tr("Little-endian"), true);
    m_endianBox->addItem(tr("Big-endian"),    false);

    m_encodingBox = new QComboBox(this);
    m_encodingBox->addItem(tr("Linear PCM"), static_cast<int>(Encoding::Linear));
    m_encodingBox->addItem(tr("A-law (G.711)"),  static_cast<int>(Encoding::ALaw));
    m_encodingBox->addItem(tr("μ-law (G.711)"),  static_cast<int>(Encoding::ULaw));
    // A/μ-law only apply to 8-bit; disable when 16-bit is selected
    m_encodingBox->setEnabled(m_bitDepthBox->currentIndex() == 0);

    connect(m_bitDepthBox, &QComboBox::currentIndexChanged,
            this, &PcmFormatDialog::onBitDepthChanged);

    auto *formLayout = new QFormLayout;
    formLayout->addRow(tr("Sample Rate:"),  m_sampleRateBox);
    formLayout->addRow(tr("Bit Depth:"),    m_bitDepthBox);
    formLayout->addRow(tr("Encoding:"),     m_encodingBox);
    formLayout->addRow(tr("Channels:"),     m_channelsBox);
    formLayout->addRow(tr("Byte Order:"),   m_endianBox);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttons);
    setLayout(mainLayout);
}

void PcmFormatDialog::onBitDepthChanged(int index)
{
    bool is8bit = (m_bitDepthBox->itemData(index).toInt() == 8);
    m_encodingBox->setEnabled(is8bit);
    if (!is8bit)
        m_encodingBox->setCurrentIndex(0); // reset to Linear
}

WaveFile::RawPcmFormat PcmFormatDialog::format() const
{
    WaveFile::RawPcmFormat fmt;
    fmt.sampleRate    = m_sampleRateBox->currentData().toUInt();
    fmt.bitsPerSample = static_cast<uint16_t>(m_bitDepthBox->currentData().toInt());
    fmt.channels      = static_cast<uint16_t>(m_channelsBox->currentData().toInt());
    fmt.littleEndian  = m_endianBox->currentData().toBool();
    fmt.encoding      = static_cast<Encoding>(m_encodingBox->currentData().toInt());
    return fmt;
}
