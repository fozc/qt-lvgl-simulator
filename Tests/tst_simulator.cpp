#include <QtTest>
#include "lvgl_widget.h"
#include "app_ui.h"
#include "app_climate.h"

static QStringList log_messages;
static QStringList warning_messages;

static void capture_log(lv_log_level_t level, const char * message)
{
    if (level == LV_LOG_LEVEL_USER) log_messages.append(QString::fromUtf8(message));
    if (level == LV_LOG_LEVEL_WARN) warning_messages.append(QString::fromUtf8(message));
}

class SimulatorTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        lv_init();
        lv_log_register_print_cb(capture_log);
    }

    void renderAndInteract()
    {
        LvglWidget window(480, 640);
        window.initializeLvgl();
        app_ui_create(lv_screen_active());
        window.start();
        window.show();
        QTest::qWait(100);

        lv_obj_t * screen = lv_screen_active();
        lv_obj_t * valueLabel = lv_obj_get_child(screen, 2);
        lv_obj_t * slider = lv_obj_get_child(screen, 3);
        lv_obj_t * buttons = lv_obj_get_child(screen, 4);
        lv_obj_t * decrease = lv_obj_get_child(buttons, 0);
        lv_obj_t * reset = lv_obj_get_child(buttons, 1);
        lv_obj_t * increase = lv_obj_get_child(buttons, 2);

        QVERIFY(lv_obj_check_type(slider, &lv_slider_class));
        QCOMPARE(lv_slider_get_value(slider), 50);
        QCOMPARE(QString::fromUtf8(lv_label_get_text(valueLabel)), QString("50"));
        QVERIFY(log_messages.isEmpty());

        const QImage frame = window.grab().toImage();
        QCOMPARE(frame.size(), QSize(480, 640));
        const QColor background = frame.pixelColor(5, 5);
        QVERIFY(qAbs(background.red() - 243) <= 8);
        QVERIFY(qAbs(background.green() - 245) <= 4);
        QVERIFY(qAbs(background.blue() - 244) <= 8);
        lv_area_t sliderArea;
        lv_obj_get_coords(slider, &sliderArea);
        const int centerY = (sliderArea.y1 + sliderArea.y2) / 2;
        const QColor sliderColor = frame.pixelColor(sliderArea.x1 + lv_area_get_width(&sliderArea) / 4, centerY);
        QVERIFY(sliderColor.green() > sliderColor.red() + 30);
        QVERIFY(frame.save(QCoreApplication::applicationDirPath() + "/simulator.png"));

        click(window, increase);
        QCOMPARE(lv_slider_get_value(slider), 51);
        QCOMPARE(QString::fromUtf8(lv_label_get_text(valueLabel)), QString("51"));
        QCOMPARE(log_messages.size(), 1);
        QVERIFY(log_messages.last().contains("Slider value: 51"));
        click(window, decrease);
        QCOMPARE(lv_slider_get_value(slider), 50);
        QVERIFY(log_messages.last().contains("Slider value: 50"));
        click(window, reset);
        QCOMPARE(lv_slider_get_value(slider), 0);
        QCOMPARE(QString::fromUtf8(lv_label_get_text(valueLabel)), QString("0"));
        QVERIFY(log_messages.last().contains("Slider value: 0"));
        const int resetLogCount = log_messages.size();
        click(window, reset);
        QCOMPARE(log_messages.size(), resetLogCount);

        lv_slider_set_value(slider, 100, LV_ANIM_OFF);
        click(window, increase);
        QCOMPARE(lv_slider_get_value(slider), 100);
        lv_slider_set_value(slider, 0, LV_ANIM_OFF);
        click(window, decrease);
        QCOMPARE(lv_slider_get_value(slider), 0);

        const QPoint start(sliderArea.x1, centerY);
        const QPoint finish(sliderArea.x1 + lv_area_get_width(&sliderArea) * 9 / 10, centerY);
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, start);
        QTest::qWait(60);
        QTest::mouseMove(&window, finish);
        QTest::qWait(60);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, finish);
        QTest::qWait(60);
        QVERIFY(lv_slider_get_value(slider) >= 80);
        QCOMPARE(QString::fromUtf8(lv_label_get_text(valueLabel)), QString::number(lv_slider_get_value(slider)));
        QVERIFY(log_messages.last().contains(QString("Slider value: %1").arg(lv_slider_get_value(slider))));
    }

    void climateHistory()
    {
        const uint32_t epoch = static_cast<uint32_t>(QDateTime(QDate(2026, 10, 20), QTime(14, 32, 10), Qt::UTC).toSecsSinceEpoch());
        app_climate_set_time(epoch, 0);
        LvglWidget window(480, 640);
        window.initializeLvgl();
        app_ui_create(lv_screen_active());
        window.start();
        window.show();
        QTest::qWait(80);
        lv_obj_t * controls = lv_screen_active();
        lv_obj_t * open = find(controls, "climate.open");
        QVERIFY(open);
        click(window, open);
        QTest::qWait(300);
        lv_obj_t * screen = lv_screen_active();
        QVERIFY(screen != controls);
        lv_obj_t * chart = find(screen, "climate.chart");
        lv_obj_t * selected = find(screen, "climate.selection");
        lv_obj_t * clock = find(screen, "climate.clock");
        lv_obj_t * live = find(screen, "climate.live");
        lv_obj_t * next = find(screen, "climate.next");
        lv_obj_t * previous = find(screen, "climate.previous");
        QVERIFY(chart && selected && clock && live && next && previous);
        QCOMPARE(lv_chart_get_type(chart), LV_CHART_TYPE_LINE);
        QVERIFY(warning_messages.filter("LV_USE_VECTOR_GRAPHIC").isEmpty());
        QCOMPARE(QString::fromUtf8(lv_label_get_text(clock)), QString("14:32"));
        QCOMPARE(lv_chart_get_point_count(chart), 73U);
        QVERIFY(lv_obj_has_state(live, LV_STATE_CHECKED));
        QVERIFY(lv_obj_has_state(next, LV_STATE_DISABLED));
        QVERIFY(!lv_obj_has_state(previous, LV_STATE_DISABLED));
        QVERIFY(textFits(screen));
        const QImage frame = window.grab().toImage();
        QVERIFY(frame.pixelColor(4, 4).lightness() < 35);
        QVERIFY(frame.pixelColor(266, 245).green() > 150);
        QVERIFY(frame.save(QCoreApplication::applicationDirPath() + "/climate.png"));
        QTest::qWait(350);
        QVERIFY(frame.copy(24, 240, 192, 53) != window.grab().toImage().copy(24, 240, 192, 53));

        lv_area_t area;
        lv_obj_get_coords(chart, &area);
        const QPoint start(area.x1 + lv_area_get_width(&area) / 5, (area.y1 + area.y2) / 2);
        const QPoint finish(area.x1 + lv_area_get_width(&area) * 3 / 4, start.y());
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, start);
        QTest::qWait(80);
        const QString first = QString::fromUtf8(lv_label_get_text(selected));
        QMouseEvent movement(QEvent::MouseMove, QPointF(finish), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&window, &movement);
        QTest::qWait(80);
        const QString inspected = QString::fromUtf8(lv_label_get_text(selected));
        QVERIFY(first != inspected);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, finish);
        QTest::qWait(80);
        QVERIFY(!lv_obj_has_state(live, LV_STATE_CHECKED));
        app_climate_set_time(epoch + 600, 0);
        QTest::qWait(350);
        QCOMPARE(QString::fromUtf8(lv_label_get_text(selected)), inspected);
        QVERIFY(window.grab().save(QCoreApplication::applicationDirPath() + "/climate-history.png"));

        click(window, find(screen, "climate.mode.humidity"));
        QCOMPARE(QString::fromUtf8(lv_label_get_text(selected)), inspected);
        QVERIFY(lv_obj_has_state(find(screen, "climate.mode.humidity"), LV_STATE_CHECKED));
        QVERIFY(window.grab().save(QCoreApplication::applicationDirPath() + "/climate-humidity.png"));
        click(window, find(screen, "climate.24h"));
        QCOMPARE(lv_chart_get_point_count(chart), 289U);
        QVERIFY(lv_obj_has_state(previous, LV_STATE_DISABLED));
        QVERIFY(lv_obj_has_state(next, LV_STATE_DISABLED));
        click(window, find(screen, "climate.1h"));
        QCOMPARE(lv_chart_get_point_count(chart), 13U);
        click(window, live);
        const QString latest = QString::fromUtf8(lv_label_get_text(selected));
        click(window, previous);
        QVERIFY(QString::fromUtf8(lv_label_get_text(selected)) != latest);
        QVERIFY(!lv_obj_has_state(next, LV_STATE_DISABLED));
        click(window, next);
        QVERIFY(lv_obj_has_state(next, LV_STATE_DISABLED));
        click(window, live);
        QCOMPARE(QString::fromUtf8(lv_label_get_text(selected)), latest);
        QVERIFY(lv_obj_has_state(live, LV_STATE_CHECKED));
        click(window, find(screen, "climate.6h"));
        QCOMPARE(lv_chart_get_point_count(chart), 73U);

        const uint32_t midnight = static_cast<uint32_t>(QDateTime(QDate(2026, 10, 21), QTime(0, 2), Qt::UTC).toSecsSinceEpoch());
        app_climate_set_time(midnight, 0);
        QTest::qWait(350);
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(area.x1 + 2, start.y()));
        QTest::qWait(60);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(area.x1 + 2, start.y()));
        QTest::qWait(60);
        QVERIFY(QString::fromUtf8(lv_label_get_text(selected)).startsWith("YESTERDAY /"));
        QVERIFY(textFits(screen));
        click(window, find(screen, "climate.back"));
        QTest::qWait(300);
        QCOMPARE(lv_screen_active(), controls);
        QVERIFY(!lv_obj_is_valid(screen));
        click(window, open);
        QTest::qWait(300);
        QVERIFY(find(lv_screen_active(), "climate.chart"));
        click(window, find(lv_screen_active(), "climate.back"));
        QTest::qWait(300);
        QCOMPARE(lv_screen_active(), controls);
    }

    void cleanupTestCase()
    {
        lv_deinit();
    }

private:
    lv_obj_t * find(lv_obj_t * root, const char * name)
    {
        const char * role = static_cast<const char *>(lv_obj_get_user_data(root));
        if (role && qstrcmp(role, name) == 0) return root;
        for (uint32_t index = 0; index < lv_obj_get_child_count(root); ++index) {
            lv_obj_t * match = find(lv_obj_get_child(root, index), name);
            if (match) return match;
        }
        return nullptr;
    }

    bool textFits(lv_obj_t * root)
    {
        if (lv_obj_check_type(root, &lv_label_class)) {
            lv_point_t size;
            lv_text_get_size(&size, lv_label_get_text(root), lv_obj_get_style_text_font(root, LV_PART_MAIN),
                             0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
            if (size.x > lv_obj_get_width(root)) {
                qWarning() << "Clipped label:" << lv_label_get_text(root) << size.x << lv_obj_get_width(root);
                return false;
            }
        }
        for (uint32_t index = 0; index < lv_obj_get_child_count(root); ++index)
            if (!textFits(lv_obj_get_child(root, index))) return false;
        return true;
    }

    void click(LvglWidget & window, lv_obj_t * button)
    {
        lv_area_t area;
        lv_obj_get_coords(button, &area);
        const QPoint center((area.x1 + area.x2) / 2, (area.y1 + area.y2) / 2);
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, center);
        QTest::qWait(60);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, center);
        QTest::qWait(60);
    }
};

QTEST_MAIN(SimulatorTest)
#include "tst_simulator.moc"