#ifndef LVGL_WIDGET_H
#define LVGL_WIDGET_H

#include <QElapsedTimer>
#include <QImage>
#include <QPoint>
#include <QTimer>
#include <QWidget>
#include <vector>
#include "lvgl.h"

class LvglWidget : public QWidget
{
public:
    LvglWidget(int width, int height);
    ~LvglWidget() override;

    void initializeLvgl();
    void start();

protected:
    void paintEvent(QPaintEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;

private:
    static void flushPixels(lv_display_t * display, const lv_area_t * area, uint8_t * pixels);
    static void readPointer(lv_indev_t * input, lv_indev_data_t * data);
    void processLvgl();

    QImage framebuffer;
    std::vector<uint16_t> drawBuffer;
    QTimer refreshTimer;
    QElapsedTimer clock;
    qint64 previousTick = 0;
    QPoint pointerPosition;
    bool pointerPressed = false;
    lv_display_t * display = nullptr;
    lv_indev_t * input = nullptr;
};

#endif