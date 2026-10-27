#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QString>
#include <QTimer>
#include "lvgl_widget.h"
#include "app_ui.h"
#include "app_climate.h"

static void print_lvgl_log(lv_log_level_t level, const char * message)
{
    Q_UNUSED(level);
    qInfo().noquote() << QString::fromUtf8(message).trimmed();
}

static void sync_clock()
{
    const QDateTime now = QDateTime::currentDateTime();
    app_climate_set_time(static_cast<uint32_t>(now.toSecsSinceEpoch()),
                         static_cast<int16_t>(now.offsetFromUtc() / 60));
}

int main(int argc, char * argv[])
{
    QApplication application(argc, argv);

    lv_init();
    lv_log_register_print_cb(print_lvgl_log);
    sync_clock();
    QTimer systemClock;
    QObject::connect(&systemClock, &QTimer::timeout, &application, sync_clock);
    systemClock.start(1000);

    LvglWidget window(480, 640);
    window.setWindowTitle(QString("Qt LVGL Simulator - LVGL %1.%2.%3")
                          .arg(LVGL_VERSION_MAJOR).arg(LVGL_VERSION_MINOR).arg(LVGL_VERSION_PATCH));
    window.initializeLvgl();

    app_ui_create(lv_screen_active());

    window.start();
    window.show();
    return application.exec();
}