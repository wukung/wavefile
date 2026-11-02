#include <QtTest/QtTest>

#include "viewstate.h"

class TestViewState : public QObject
{
    Q_OBJECT

private slots:
    void resetForData_fitsDataToWidth();
    void resetForData_emptyDataIsSafe();
    void resetForData_zeroWidthIsSafe();
    void zoomAt_keepsSampleUnderMouseStationary();
    void zoomAt_clampsAtMinSamplesPerPixel();
    void zoomAt_clampsAtFitAllMax();
    void panPixels_movesOffsetByDeltaTimesSpp();
    void panPixels_clampsAtBothEnds();
    void viewportSampleWidth_equalsWidthTimesSpp();
};

void TestViewState::resetForData_fitsDataToWidth()
{
    ViewState v;
    v.resetForData(1000, 500);
    QCOMPARE(v.samplesPerPixel(), 2.0);
    QCOMPARE(v.offsetInSamples(), 0.0);
}

void TestViewState::resetForData_emptyDataIsSafe()
{
    ViewState v;
    v.resetForData(0, 500); // must not crash; offset zeroed
    QCOMPARE(v.offsetInSamples(), 0.0);
    QVERIFY(v.samplesPerPixel() > 0.0);
}

void TestViewState::resetForData_zeroWidthIsSafe()
{
    ViewState v;
    v.resetForData(100, 0); // must not crash (divide-by-zero guard); offset zeroed
    QCOMPARE(v.offsetInSamples(), 0.0);
    QVERIFY(v.samplesPerPixel() > 0.0);
}

void TestViewState::zoomAt_keepsSampleUnderMouseStationary()
{
    ViewState v;
    v.resetForData(1000, 500); // SPP = 2.0, offset = 0
    const double mouseX = 100.0;
    const double sampleBefore = mouseX * v.samplesPerPixel() + v.offsetInSamples();
    QCOMPARE(sampleBefore, 200.0);

    v.zoomAt(mouseX, 500, 1000, 0.5); // zoom in: SPP 2.0 -> 1.0
    QCOMPARE(v.samplesPerPixel(), 1.0);
    const double sampleAfter = mouseX * v.samplesPerPixel() + v.offsetInSamples();
    QCOMPARE(sampleAfter, sampleBefore);
    QCOMPARE(v.offsetInSamples(), 100.0);
}

void TestViewState::zoomAt_clampsAtMinSamplesPerPixel()
{
    ViewState v;
    v.resetForData(1000, 500); // SPP = 2.0
    v.zoomAt(250.0, 500, 1000, 1e-6); // far past maximum zoom-in
    QCOMPARE(v.samplesPerPixel(), 1.0 / 16.0);
}

void TestViewState::zoomAt_clampsAtFitAllMax()
{
    ViewState v;
    v.resetForData(1000, 500); // SPP = 2.0 == fit-all max (1000/500)
    v.zoomAt(100.0, 500, 1000, 0.5); // zoom in to SPP = 1.0 first
    QCOMPARE(v.samplesPerPixel(), 1.0);
    v.zoomAt(100.0, 500, 1000, 100.0); // far past maximum zoom-out
    QCOMPARE(v.samplesPerPixel(), 2.0); // clamped to dataCount/width
}

void TestViewState::panPixels_movesOffsetByDeltaTimesSpp()
{
    ViewState v;
    v.resetForData(1000, 100); // SPP = 10.0, whole file visible
    v.zoomAt(0.0, 100, 1000, 0.5); // SPP = 5.0, offset stays 0, room to pan
    QCOMPARE(v.samplesPerPixel(), 5.0);
    QCOMPARE(v.offsetInSamples(), 0.0);

    v.panPixels(-20, 100, 1000); // drag left: content moves right, offset grows
    QCOMPARE(v.offsetInSamples(), 100.0); // -(-20) * 5.0

    v.panPixels(10, 100, 1000); // drag right: offset shrinks back
    QCOMPARE(v.offsetInSamples(), 50.0); // 100 - 10 * 5.0
}

void TestViewState::panPixels_clampsAtBothEnds()
{
    ViewState v;
    v.resetForData(1000, 100); // SPP = 10.0
    v.zoomAt(0.0, 100, 1000, 0.5); // SPP = 5.0, visible = 500, max offset = 500

    v.panPixels(10000, 100, 1000); // far past the left edge
    QCOMPARE(v.offsetInSamples(), 0.0); // no negative offset

    v.panPixels(-10000, 100, 1000); // far past the right edge
    QCOMPARE(v.offsetInSamples(), 500.0); // no overrun: dataCount - visible
}

void TestViewState::viewportSampleWidth_equalsWidthTimesSpp()
{
    ViewState v;
    v.resetForData(1000, 500); // SPP = 2.0
    QCOMPARE(v.viewportSampleWidth(500), 500 * v.samplesPerPixel());
    QCOMPARE(v.viewportSampleWidth(500), 1000.0);
    QCOMPARE(v.viewportSampleWidth(0), 0.0);
}

QTEST_MAIN(TestViewState)
#include "test_viewstate.moc"
