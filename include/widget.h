#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QWheelEvent>
#include <QMouseEvent>

#include "WaveFile.h"
#include "viewstate.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void openFile();

private:
    enum class DetailViewMode {
        Waveform,
        Spectrogram
    };

    Ui::Widget *ui;

    // Necessary variables for wave form
    WaveFile m_Wavefile;
    bool m_DrawWave;
    QString m_Filename;
    ViewState m_ViewState;

    QPoint m_LastMousePos;
    bool m_IsPanning;
    int m_OverviewHeight;
    DetailViewMode m_DetailViewMode;
};

#endif // WIDGET_H
