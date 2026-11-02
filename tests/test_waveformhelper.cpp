#include <QtTest/QtTest>

#include "waveformrenderer.h"

class TestWaveformHelper : public QObject
{
    Q_OBJECT

private slots:
    void basicMapping();
    void fractionalOffsetSummedThenCast();
    void endClampedAtDatanum();
    void emptyData();
};

void TestWaveformHelper::basicMapping()
{
    // samplesPerPixel=10, offset=0, datanum=1000: column 0 -> [0,10), column 1 -> [10,20).
    const auto r0 = WaveformRenderer::columnRange(0, 10.0, 0.0, 1000);
    QCOMPARE(r0.first, 0);
    QCOMPARE(r0.second, 10);

    const auto r1 = WaveformRenderer::columnRange(1, 10.0, 0.0, 1000);
    QCOMPARE(r1.first, 10);
    QCOMPARE(r1.second, 20);
}

void TestWaveformHelper::fractionalOffsetSummedThenCast()
{
    // Fractional offset must be summed in double first, then cast:
    // int(x*spp + offset), NOT int(x*spp) + int(offset).
    // x=3, spp=10.0, offset=5.7 -> start=int(35.7)=35, end=int(45.7)=45.
    // The old truncating-separately form would give start=30+5=35 here
    // but differs in general; verify a case where it diverges:
    // x=1, spp=2.5, offset=0.9 -> summed: start=int(3.4)=3, end=int(5.9)=5.
    // Separate truncation: int(2.5)+int(0.9)=2+0=2 (wrong).
    const auto r = WaveformRenderer::columnRange(1, 2.5, 0.9, 100);
    QCOMPARE(r.first, static_cast<int>(1 * 2.5 + 0.9));
    QCOMPARE(r.second, static_cast<int>(2 * 2.5 + 0.9));
    QCOMPARE(r.first, 3);
    QCOMPARE(r.second, 5);
}

void TestWaveformHelper::endClampedAtDatanum()
{
    // Near the end of data, endIdx must clamp to datanum.
    // datanum=100, spp=10, offset=0: column 9 -> [90,100), column 10 -> start=100.
    const auto r9 = WaveformRenderer::columnRange(9, 10.0, 0.0, 100);
    QCOMPARE(r9.first, 90);
    QCOMPARE(r9.second, 100);

    const auto r10 = WaveformRenderer::columnRange(10, 10.0, 0.0, 100);
    QCOMPARE(r10.first, 100);
    QCOMPARE(r10.second, 100); // clamped: min(110, 100)

    // Offset pushing the window past the end clamps as well.
    const auto rOff = WaveformRenderer::columnRange(0, 10.0, 95.0, 100);
    QCOMPARE(rOff.first, 95);
    QCOMPARE(rOff.second, 100);
}

void TestWaveformHelper::emptyData()
{
    // datanum=0: both indices collapse to 0 via the clamp.
    const auto r = WaveformRenderer::columnRange(0, 10.0, 0.0, 0);
    QCOMPARE(r.first, 0);
    QCOMPARE(r.second, 0);
}

QTEST_MAIN(TestWaveformHelper)
#include "test_waveformhelper.moc"
