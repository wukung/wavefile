#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QByteArray>

#include "WaveFile.h"

namespace {

void appendLe16(QByteArray &b, uint16_t v)
{
    b.append(static_cast<char>(v & 0xff));
    b.append(static_cast<char>((v >> 8) & 0xff));
}

void appendLe32(QByteArray &b, uint32_t v)
{
    b.append(static_cast<char>(v & 0xff));
    b.append(static_cast<char>((v >> 8) & 0xff));
    b.append(static_cast<char>((v >> 16) & 0xff));
    b.append(static_cast<char>((v >> 24) & 0xff));
}

/** Minimal WAV: one `fmt ` (16-byte body) followed by `data`. `formatTag` 1=PCM, 6=A-law, 7=μ-law (matches `WaveFile`). */
QByteArray makeWav(uint16_t formatTag, uint16_t channels, uint16_t bitsPerSample,
                   uint32_t sampleRate, const QByteArray &pcmPayload,
                   uint32_t declaredDataSize = 0)
{
    Q_ASSERT(bitsPerSample == 8 || bitsPerSample == 16);
    const uint16_t blockAlign = static_cast<uint16_t>((bitsPerSample / 8) * channels);
    const uint32_t byteRate = sampleRate * blockAlign;

    const uint32_t dataSize = declaredDataSize ? declaredDataSize : static_cast<uint32_t>(pcmPayload.size());

    QByteArray b;
    b.append("RIFF", 4);
    const int riffSizePos = b.size();
    appendLe32(b, 0); // patch below
    b.append("WAVE", 4);
    b.append("fmt ", 4);
    appendLe32(b, 16);
    appendLe16(b, formatTag);
    appendLe16(b, channels);
    appendLe32(b, sampleRate);
    appendLe32(b, byteRate);
    appendLe16(b, blockAlign);
    appendLe16(b, bitsPerSample);
    Q_ASSERT(b.size() == 36);

    b.append("data", 4);
    appendLe32(b, dataSize);
    b.append(pcmPayload);
    if (static_cast<uint32_t>(pcmPayload.size()) < dataSize) {
        // Truncated-on-disk: declared `data` chunk is longer than the bytes we append.
        // Intentionally do not pad — mirrors a short file with an optimistic chunk size.
    } else if (static_cast<uint32_t>(pcmPayload.size()) > dataSize) {
        b.chop(static_cast<int>(pcmPayload.size()) - static_cast<int>(dataSize));
    }

    const uint32_t riffChunkSize = static_cast<uint32_t>(b.size() - 8);
    b[riffSizePos + 0] = static_cast<char>(riffChunkSize & 0xff);
    b[riffSizePos + 1] = static_cast<char>((riffChunkSize >> 8) & 0xff);
    b[riffSizePos + 2] = static_cast<char>((riffChunkSize >> 16) & 0xff);
    b[riffSizePos + 3] = static_cast<char>((riffChunkSize >> 24) & 0xff);

    return b;
}

QByteArray makePcmWav(uint16_t channels, uint16_t bitsPerSample, uint32_t sampleRate,
                      const QByteArray &pcmPayload, uint32_t declaredDataSize = 0)
{
    return makeWav(1, channels, bitsPerSample, sampleRate, pcmPayload, declaredDataSize);
}

QByteArray int16Le(int16_t v)
{
    QByteArray x;
    appendLe16(x, static_cast<uint16_t>(v));
    return x;
}

void writeTemp(const QTemporaryDir &dir, const QString &name, const QByteArray &data, QString *outPath)
{
    *outPath = dir.path() + QLatin1Char('/') + name;
    QFile f(*outPath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    QCOMPARE(f.write(data), data.size());
    f.close();
}

} // namespace

class TestWaveFile : public QObject
{
    Q_OBJECT

private slots:
    void wavMono16_fourSamples();
    void wavStereo16_firstChannelOnly();
    void wavTruncated_declaredLargerThanFile();
    void wavRejectZeroBlockAlign();
    void wavRejectMultiChannelWithMonoSizedBlockAlign();
    void wavQuadChannel16_skipsOtherChannels();
    void wavMono8_linearTwoSamples();
    void wavUlaw8_oneSample();
    void rawPcmMono16_twoSamples();
    void rawPcmStereo16_littleEndian();
    void rawPcmStereo16_bigEndian();
    void rawPcmMono8_alawOneSample();
    void rawPcmMono8_ulawOneSample();
    void rawPcmMono16_rejectIncompleteFirstSample();
};

void TestWaveFile::wavMono16_fourSamples()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QByteArray payload;
    payload += int16Le(1000);
    payload += int16Le(-2000);
    payload += int16Le(3000);
    payload += int16Le(-4000);

    QString path;
    writeTemp(dir, QStringLiteral("m.wav"), makePcmWav(1, 16, 44100, payload), &path);

    WaveFile wf;
    QVERIFY(wf.WavRead(path.toStdString()));
    QCOMPARE(static_cast<int>(wf.datanum), 4);
    QCOMPARE(static_cast<int>(wf.totalsample), 4);
    QCOMPARE(static_cast<int>(wf.bitpersample), 16);
    QVERIFY(wf.Data != nullptr);
    QCOMPARE(wf.Data[0], static_cast<short>(1000));
    QCOMPARE(wf.Data[1], static_cast<short>(-2000));
    QCOMPARE(wf.Data[2], static_cast<short>(3000));
    QCOMPARE(wf.Data[3], static_cast<short>(-4000));
    QCOMPARE(static_cast<int>(wf.datalength), 8); // 4 frames × 2-byte align
}

void TestWaveFile::wavStereo16_firstChannelOnly()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QByteArray payload;
    payload += int16Le(111);  // L0
    payload += int16Le(999);  // R0
    payload += int16Le(-222); // L1
    payload += int16Le(888);  // R1

    QString path;
    writeTemp(dir, QStringLiteral("s.wav"), makePcmWav(2, 16, 48000, payload), &path);

    WaveFile wf;
    QVERIFY(wf.WavRead(path.toStdString()));
    QCOMPARE(static_cast<int>(wf.datanum), 2);
    QCOMPARE(wf.Data[0], static_cast<short>(111));
    QCOMPARE(wf.Data[1], static_cast<short>(-222));
}

void TestWaveFile::wavTruncated_declaredLargerThanFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QByteArray payload;
    payload += int16Le(10);
    payload += int16Le(20); // only 2 samples in file

    // Declare data chunk size 8 bytes (4 mono frames) but file ends after 4 payload bytes
    const QByteArray wav = makePcmWav(1, 16, 8000, payload, /*declaredDataSize*/ 8);
    QString path;
    writeTemp(dir, QStringLiteral("cut.wav"), wav, &path);

    WaveFile wf;
    QVERIFY(wf.WavRead(path.toStdString()));
    QCOMPARE(static_cast<int>(wf.datanum), 2);
    QCOMPARE(static_cast<int>(wf.datalength), 4); // 2 frames × 2-byte block align
    QCOMPARE(wf.Data[0], static_cast<short>(10));
    QCOMPARE(wf.Data[1], static_cast<short>(20));
}

void TestWaveFile::wavRejectZeroBlockAlign()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QByteArray b;
    b.append("RIFF", 4);
    appendLe32(b, 40);
    b.append("WAVE", 4);
    b.append("fmt ", 4);
    appendLe32(b, 16);
    appendLe16(b, 1);
    appendLe16(b, 1);
    appendLe32(b, 8000);
    appendLe32(b, 0);
    appendLe16(b, 0); // block align = 0 -> invalid
    appendLe16(b, 16);
    b.append("data", 4);
    appendLe32(b, 0);

    QString path;
    writeTemp(dir, QStringLiteral("bad.wav"), b, &path);

    WaveFile wf;
    QVERIFY(!wf.WavRead(path.toStdString()));
    QCOMPARE(static_cast<int>(wf.datanum), 0);
    QVERIFY(wf.Data == nullptr);
}

void TestWaveFile::wavRejectMultiChannelWithMonoSizedBlockAlign()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QByteArray b;
    b.append("RIFF", 4);
    appendLe32(b, 40);
    b.append("WAVE", 4);
    b.append("fmt ", 4);
    appendLe32(b, 16);
    appendLe16(b, 1);
    appendLe16(b, 2); // stereo
    appendLe32(b, 8000);
    appendLe32(b, 16000);
    appendLe16(b, 2); // block align = one channel only — inconsistent with stereo 16-bit
    appendLe16(b, 16);
    b.append("data", 4);
    appendLe32(b, 0);

    QString path;
    writeTemp(dir, QStringLiteral("badalign.wav"), b, &path);

    WaveFile wf;
    QVERIFY(!wf.WavRead(path.toStdString()));
    QCOMPARE(static_cast<int>(wf.datanum), 0);
}

void TestWaveFile::wavQuadChannel16_skipsOtherChannels()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // 4 ch × 16-bit = 8 bytes per frame; two frames
    QByteArray payload;
    for (int frame = 0; frame < 2; ++frame) {
        const int16_t ch0 = static_cast<int16_t>(100 * (frame + 1));
        for (int ch = 0; ch < 4; ++ch) {
            const int16_t v = (ch == 0) ? ch0 : static_cast<int16_t>(ch * 1000 + frame);
            payload += int16Le(v);
        }
    }

    QString path;
    writeTemp(dir, QStringLiteral("4ch.wav"), makePcmWav(4, 16, 44100, payload), &path);

    WaveFile wf;
    QVERIFY(wf.WavRead(path.toStdString()));
    QCOMPARE(static_cast<int>(wf.datanum), 2);
    QCOMPARE(wf.Data[0], static_cast<short>(100));
    QCOMPARE(wf.Data[1], static_cast<short>(200));
}

void TestWaveFile::rawPcmMono16_twoSamples()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QByteArray raw;
    raw += int16Le(42);
    raw += int16Le(-43);

    QString path;
    writeTemp(dir, QStringLiteral("x.raw"), raw, &path);

    WaveFile::RawPcmFormat fmt;
    fmt.sampleRate = 16000;
    fmt.bitsPerSample = 16;
    fmt.channels = 1;
    fmt.littleEndian = true;
    fmt.encoding = WaveFile::RawPcmFormat::Encoding::Linear;

    WaveFile wf;
    QVERIFY(wf.RawRead(path.toStdString(), fmt));
    QCOMPARE(static_cast<int>(wf.datanum), 2);
    QCOMPARE(wf.Data[0], static_cast<short>(42));
    QCOMPARE(wf.Data[1], static_cast<short>(-43));
    QCOMPARE(static_cast<int>(wf.datalength), 4); // 2 frames × 2 bytes
}

void TestWaveFile::wavMono8_linearTwoSamples()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QByteArray payload;
    payload.append(static_cast<char>(128)); // silence → 0 after decode
    payload.append(static_cast<char>(129)); // (129-128)*256 = 256

    QString path;
    writeTemp(dir, QStringLiteral("m8.wav"), makePcmWav(1, 8, 11025, payload), &path);

    WaveFile wf;
    QVERIFY(wf.WavRead(path.toStdString()));
    QCOMPARE(static_cast<int>(wf.datanum), 2);
    QCOMPARE(wf.Data[0], static_cast<short>(0));
    QCOMPARE(wf.Data[1], static_cast<short>(256));
}

void TestWaveFile::wavUlaw8_oneSample()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const uint8_t enc = 0x7E;
    QByteArray payload;
    payload.append(static_cast<char>(enc));

    QString path;
    writeTemp(dir, QStringLiteral("u.wav"), makeWav(7, 1, 8, 8000, payload), &path);

    WaveFile wf;
    QVERIFY(wf.WavRead(path.toStdString()));
    QCOMPARE(static_cast<int>(wf.datanum), 1);
    QCOMPARE(wf.Data[0], WaveFile::ulaw2linear(enc));
}

void TestWaveFile::rawPcmStereo16_littleEndian()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QByteArray raw;
    raw += int16Le(100);  // L0
    raw += int16Le(200);  // R0
    raw += int16Le(-300); // L1
    raw += int16Le(-400); // R1

    QString path;
    writeTemp(dir, QStringLiteral("st.raw"), raw, &path);

    WaveFile::RawPcmFormat fmt;
    fmt.sampleRate = 48000;
    fmt.bitsPerSample = 16;
    fmt.channels = 2;
    fmt.littleEndian = true;
    fmt.encoding = WaveFile::RawPcmFormat::Encoding::Linear;

    WaveFile wf;
    QVERIFY(wf.RawRead(path.toStdString(), fmt));
    QCOMPARE(static_cast<int>(wf.datanum), 2);
    QCOMPARE(wf.Data[0], static_cast<short>(100));
    QCOMPARE(wf.Data[1], static_cast<short>(-300));
    QCOMPARE(static_cast<int>(wf.datalength), 8);
}

void TestWaveFile::rawPcmStereo16_bigEndian()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // Stereo: only the first channel is decoded into Data[]. Two frames: L0=500, L1=-300 (16-bit BE per channel).
    QByteArray raw;
    raw.append(static_cast<char>(0x01));
    raw.append(static_cast<char>(0xF4)); // L0 = 500
    raw.append(static_cast<char>(0xFE));
    raw.append(static_cast<char>(0x0C)); // R0 = -500 (skipped for waveform buffer)
    raw.append(static_cast<char>(0xFE));
    raw.append(static_cast<char>(0xD4)); // L1 = -300
    raw.append(static_cast<char>(0x01));
    raw.append(static_cast<char>(0x90)); // R1 = 400 (skipped)

    QString path;
    writeTemp(dir, QStringLiteral("be.raw"), raw, &path);

    WaveFile::RawPcmFormat fmt;
    fmt.sampleRate = 44100;
    fmt.bitsPerSample = 16;
    fmt.channels = 2;
    fmt.littleEndian = false;
    fmt.encoding = WaveFile::RawPcmFormat::Encoding::Linear;

    WaveFile wf;
    QVERIFY(wf.RawRead(path.toStdString(), fmt));
    QCOMPARE(static_cast<int>(wf.datanum), 2);
    QCOMPARE(wf.Data[0], static_cast<short>(500));
    QCOMPARE(wf.Data[1], static_cast<short>(-300));
}

void TestWaveFile::rawPcmMono8_alawOneSample()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const uint8_t enc = 0x6A;
    QByteArray raw;
    raw.append(static_cast<char>(enc));

    QString path;
    writeTemp(dir, QStringLiteral("a.raw"), raw, &path);

    WaveFile::RawPcmFormat fmt;
    fmt.sampleRate = 8000;
    fmt.bitsPerSample = 8;
    fmt.channels = 1;
    fmt.littleEndian = true;
    fmt.encoding = WaveFile::RawPcmFormat::Encoding::ALaw;

    WaveFile wf;
    QVERIFY(wf.RawRead(path.toStdString(), fmt));
    QCOMPARE(static_cast<int>(wf.datanum), 1);
    QCOMPARE(wf.Data[0], WaveFile::alaw2linear(enc));
    QCOMPARE(static_cast<int>(wf.datalength), 1);
}

void TestWaveFile::rawPcmMono8_ulawOneSample()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const uint8_t enc = 0x5C;
    QByteArray raw;
    raw.append(static_cast<char>(enc));

    QString path;
    writeTemp(dir, QStringLiteral("u.raw"), raw, &path);

    WaveFile::RawPcmFormat fmt;
    fmt.sampleRate = 8000;
    fmt.bitsPerSample = 8;
    fmt.channels = 1;
    fmt.littleEndian = true;
    fmt.encoding = WaveFile::RawPcmFormat::Encoding::ULaw;

    WaveFile wf;
    QVERIFY(wf.RawRead(path.toStdString(), fmt));
    QCOMPARE(static_cast<int>(wf.datanum), 1);
    QCOMPARE(wf.Data[0], WaveFile::ulaw2linear(enc));
    QCOMPARE(static_cast<int>(wf.datalength), 1);
}

void TestWaveFile::rawPcmMono16_rejectIncompleteFirstSample()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QByteArray raw;
    raw.append(static_cast<char>(0x00)); // only one byte: cannot complete first int16

    QString path;
    writeTemp(dir, QStringLiteral("short.raw"), raw, &path);

    WaveFile::RawPcmFormat fmt;
    fmt.sampleRate = 8000;
    fmt.bitsPerSample = 16;
    fmt.channels = 1;
    fmt.littleEndian = true;
    fmt.encoding = WaveFile::RawPcmFormat::Encoding::Linear;

    WaveFile wf;
    QVERIFY(!wf.RawRead(path.toStdString(), fmt));
    QCOMPARE(static_cast<int>(wf.datanum), 0);
    QVERIFY(wf.Data == nullptr);
}

QTEST_MAIN(TestWaveFile)
#include "test_wavefile.moc"
