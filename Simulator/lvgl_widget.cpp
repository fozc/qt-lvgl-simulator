#include "lvgl_widget.h"

#include <QMouseEvent>
#include <QPainter>
#include <cstring>

LvglWidget::LvglWidget(int width, int height)
    : framebuffer(width, height, QImage::Format_RGB16)
    , drawBuffer(width * 40)
{
    setFixedSize(width, height);
    setMouseTracking(true);
    framebuffer.fill(Qt::white);
    refreshTimer.setTimerType(Qt::PreciseTimer);
    connect(&refreshTimer, &QTimer::timeout, this, &LvglWidget::processLvgl);
}

LvglWidget::~LvglWidget()
{
    refreshTimer.stop();
    if (input) lv_indev_delete(input);
    if (display) lv_display_delete(display);
}

void LvglWidget::initializeLvgl()
{
    display = lv_display_create(width(), height());
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_user_data(display, this);
    lv_display_set_flush_cb(display, flushPixels);
    lv_display_set_buffers(display, drawBuffer.data(), nullptr,
                           static_cast<uint32_t>(drawBuffer.size() * sizeof(uint16_t)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    input = lv_indev_create();
    lv_indev_set_type(input, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(input, display);
    lv_indev_set_user_data(input, this);
    lv_indev_set_read_cb(input, readPointer);
}

void LvglWidget::start()
{
    clock.start();
    previousTick = 0;
    refreshTimer.start(5);
}

void LvglWidget::processLvgl()
{
    const qint64 currentTick = clock.elapsed();
    lv_tick_inc(static_cast<uint32_t>(currentTick - previousTick));
    previousTick = currentTick;
    lv_timer_handler();
}

void LvglWidget::flushPixels(lv_display_t * display, const lv_area_t * area, uint8_t * pixels)
{
    auto * widget = static_cast<LvglWidget *>(lv_display_get_user_data(display));
    const int rowBytes = lv_area_get_width(area) * sizeof(uint16_t);
    const uint32_t sourceStride = lv_draw_buf_width_to_stride(lv_area_get_width(area), LV_COLOR_FORMAT_RGB565);

    // Both LVGL and QImage use native-endian RGB565; copy only the changed rows.
    for (int row = area->y1; row <= area->y2; ++row) {
        std::memcpy(widget->framebuffer.scanLine(row) + area->x1 * sizeof(uint16_t),
                    pixels + (row - area->y1) * sourceStride, rowBytes);
    }

    if (lv_display_flush_is_last(display)) widget->update();
    lv_display_flush_ready(display);
}

void LvglWidget::readPointer(lv_indev_t * input, lv_indev_data_t * data)
{
    auto * widget = static_cast<LvglWidget *>(lv_indev_get_user_data(input));
    data->point.x = widget->pointerPosition.x();
    data->point.y = widget->pointerPosition.y();
    data->state = widget->pointerPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->continue_reading = false;
}

void LvglWidget::paintEvent(QPaintEvent * event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.drawImage(0, 0, framebuffer);
}

void LvglWidget::mousePressEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton) {
        pointerPosition = event->pos();
        pointerPressed = true;
        event->accept();
    }
}

void LvglWidget::mouseMoveEvent(QMouseEvent * event)
{
    pointerPosition = event->pos();
    event->accept();
}

void LvglWidget::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton) {
        pointerPosition = event->pos();
        pointerPressed = false;
        event->accept();
    }
}