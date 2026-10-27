#include "app_climate.h"

#include <time.h>

#define INK 0x101715
#define PAPER 0xF1F5EF
#define MUTED 0x8EAAA1
#define GRID 0x2C3C36
#define LIME 0xDBF68A
#define HEAT 0xFF987C
#define WATER 0x63DCE5
#define SAMPLE_SECONDS 300U
#define HISTORY_SECONDS 86400U

typedef struct {
    lv_obj_t * screen;
    lv_obj_t * return_screen;
    lv_timer_t * timer;
    lv_obj_t * clock;
    lv_obj_t * seconds;
    lv_obj_t * date;
    lv_obj_t * time_rail;
    lv_obj_t * temperature;
    lv_obj_t * humidity;
    lv_obj_t * heat_status;
    lv_obj_t * water_status;
    lv_obj_t * fins[16];
    lv_obj_t * cells[32];
    lv_obj_t * chart;
    lv_chart_series_t * series;
    lv_chart_cursor_t * cursor;
    lv_obj_t * selected_time;
    lv_obj_t * selected_values;
    lv_obj_t * live;
    lv_obj_t * previous;
    lv_obj_t * next;
    lv_obj_t * ranges[3];
    lv_obj_t * modes[2];
    lv_obj_t * axis[2];
    lv_obj_t * ticks[3];
    lv_obj_t * stats[3];
    uint32_t latest;
    uint32_t window_end;
    uint32_t selected;
    uint16_t steps;
    bool moisture;
    bool following;
} climate_state_t;

static climate_state_t climate;
static uint32_t clock_epoch = 172800U;
static uint32_t clock_tick;
static int16_t clock_offset;
static bool clock_ready;

static uint32_t current_time(void)
{
    return clock_epoch + (uint32_t)(lv_tick_get() - clock_tick) / 1000U;
}

void app_climate_set_time(uint32_t unix_seconds, int16_t utc_offset_minutes)
{
    clock_epoch = unix_seconds;
    clock_tick = lv_tick_get();
    clock_offset = utc_offset_minutes;
    clock_ready = true;
}

static int32_t temperature_at(uint32_t timestamp)
{
    const int32_t daily = (int32_t)(timestamp / 60U % 1440U) * 360 / 1440;
    const int32_t ripple = (int32_t)(timestamp / 60U % 180U) * 360 / 180;
    return 230 + 34 * lv_trigo_sin((int16_t)daily) / LV_TRIGO_SIN_MAX
               + 7 * lv_trigo_sin((int16_t)ripple) / LV_TRIGO_SIN_MAX;
}

static int32_t humidity_at(uint32_t timestamp)
{
    const int32_t daily = (int32_t)(timestamp / 60U % 1440U) * 360 / 1440;
    const int32_t ripple = (int32_t)(timestamp / 60U % 240U) * 360 / 240;
    return 560 + 95 * lv_trigo_sin((int16_t)(daily + 100)) / LV_TRIGO_SIN_MAX
               + 22 * lv_trigo_sin((int16_t)ripple) / LV_TRIGO_SIN_MAX;
}

static lv_obj_t * block(lv_obj_t * parent, int x, int y, int width, int height, uint32_t color)
{
    lv_obj_t * object = lv_obj_create(parent);
    lv_obj_remove_style_all(object);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, height);
    lv_obj_set_style_bg_color(object, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(object, false);
    lv_obj_set_clickable(object, false);
    return object;
}

static lv_obj_t * text(lv_obj_t * parent, int x, int y, int width, const char * value,
                       const lv_font_t * font, uint32_t color, const char * name)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_CLIP);
    lv_label_set_text(label, value);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_user_data(label, (void *)name);
    return label;
}

static lv_obj_t * action(lv_obj_t * parent, int x, int y, int width, int height,
                         const char * caption, const char * name, lv_event_cb_t callback, void * data)
{
    lv_obj_t * button = lv_button_create(parent);
    lv_obj_remove_style_all(button);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, width, height);
    lv_obj_set_style_radius(button, 4, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(GRID), 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x42584C), LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(button, lv_color_hex(LIME), LV_STATE_CHECKED);
    lv_obj_set_style_text_color(button, lv_color_hex(PAPER), 0);
    lv_obj_set_style_text_color(button, lv_color_hex(INK), LV_STATE_CHECKED);
    lv_obj_set_style_text_font(button, &lv_font_montserrat_14, 0);
    lv_obj_set_style_opa(button, LV_OPA_30, LV_STATE_DISABLED);
    lv_obj_set_scrollable(button, false);
    lv_obj_set_user_data(button, (void *)name);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, data);
    lv_obj_t * label = lv_label_create(button);
    lv_label_set_text(label, caption);
    lv_obj_center(label);
    return button;
}

static void format_time(char * buffer, size_t size, uint32_t timestamp)
{
    const uint32_t local = (uint32_t)((int64_t)timestamp + (int32_t)clock_offset * 60);
    lv_snprintf(buffer, size, "%02u:%02u", (unsigned)(local / 3600U % 24U),
                (unsigned)(local / 60U % 60U));
}

static void update_selection(void)
{
    const uint32_t start = climate.window_end - climate.steps * SAMPLE_SECONDS;
    if (climate.selected < start) climate.selected = start;
    if (climate.selected > climate.window_end) climate.selected = climate.window_end;
    const uint32_t index = (climate.selected - start) / SAMPLE_SECONDS;
    climate.selected = start + index * SAMPLE_SECONDS;
    lv_chart_set_cursor_point(climate.chart, climate.cursor, climate.series, index);

    char selected[8];
    format_time(selected, sizeof(selected), climate.selected);
    const int64_t offset = (int32_t)clock_offset * 60;
    const bool today = ((int64_t)climate.selected + offset) / 86400
                      == ((int64_t)current_time() + offset) / 86400;
    lv_label_set_text_fmt(climate.selected_time, "%s / %s", today ? "TODAY" : "YESTERDAY", selected);
    const int32_t heat = temperature_at(climate.selected);
    const int32_t water = humidity_at(climate.selected);
    lv_label_set_text_fmt(climate.selected_values, "%d.%d C  /  %d.%d %%",
                          (int)(heat / 10), (int)(heat % 10), (int)(water / 10), (int)(water % 10));
    lv_obj_set_state(climate.live, LV_STATE_CHECKED, climate.following);
}

static void render_history(void)
{
    const uint32_t span = climate.steps * SAMPLE_SECONDS;
    const uint32_t earliest = climate.latest > HISTORY_SECONDS ? climate.latest - HISTORY_SECONDS : 0;
    if (climate.window_end > climate.latest) climate.window_end = climate.latest;
    if (climate.window_end < earliest + span) climate.window_end = earliest + span;
    const uint32_t start = climate.window_end - span;
    if (climate.following) climate.selected = climate.window_end;
    const uint32_t color = climate.moisture ? WATER : HEAT;
    lv_chart_set_point_count(climate.chart, climate.steps + 1U);
    lv_chart_set_series_color(climate.chart, climate.series, lv_color_hex(color));
    lv_chart_set_axis_range(climate.chart, LV_CHART_AXIS_PRIMARY_Y,
                            climate.moisture ? 300 : 180, climate.moisture ? 800 : 280);
    int32_t minimum = INT32_MAX;
    int32_t maximum = INT32_MIN;
    int32_t total = 0;
    for (uint32_t index = 0; index <= climate.steps; ++index) {
        const uint32_t timestamp = start + index * SAMPLE_SECONDS;
        const int32_t value = climate.moisture ? humidity_at(timestamp) : temperature_at(timestamp);
        lv_chart_set_series_value_by_id(climate.chart, climate.series, index, value);
        if (value < minimum) minimum = value;
        if (value > maximum) maximum = value;
        total += value;
    }
    lv_chart_refresh(climate.chart);
    const int32_t values[] = {minimum, total / (climate.steps + 1), maximum};
    for (int index = 0; index < 3; ++index) {
        lv_label_set_text_fmt(climate.stats[index], "%d.%d %s", (int)(values[index] / 10),
                              (int)(values[index] % 10), climate.moisture ? "%" : "C");
        lv_obj_set_style_text_color(climate.stats[index], lv_color_hex(color), 0);
        char tick[8];
        format_time(tick, sizeof(tick), start + span * (uint32_t)index / 2U);
        lv_label_set_text(climate.ticks[index], tick);
    }
    lv_label_set_text(climate.axis[0], climate.moisture ? "80" : "28");
    lv_label_set_text(climate.axis[1], climate.moisture ? "30" : "18");
    lv_obj_set_state(climate.previous, LV_STATE_DISABLED, start <= earliest);
    lv_obj_set_state(climate.next, LV_STATE_DISABLED, climate.window_end >= climate.latest);
    const uint16_t ranges[] = {12, 72, 288};
    for (int index = 0; index < 3; ++index)
        lv_obj_set_state(climate.ranges[index], LV_STATE_CHECKED, climate.steps == ranges[index]);
    lv_obj_set_state(climate.modes[0], LV_STATE_CHECKED, !climate.moisture);
    lv_obj_set_state(climate.modes[1], LV_STATE_CHECKED, climate.moisture);
    update_selection();
}

static void inspect_chart(lv_event_t * event)
{
    const lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING) return;
    lv_indev_t * input = lv_indev_active();
    if (!input) return;
    lv_point_t pointer;
    lv_indev_get_point(input, &pointer);
    lv_area_t area;
    lv_obj_get_coords(climate.chart, &area);
    int32_t position = pointer.x - area.x1;
    const int32_t width = lv_area_get_width(&area) - 1;
    if (position < 0) position = 0;
    if (position > width) position = width;
    const uint32_t index = (uint32_t)((position * climate.steps + width / 2) / width);
    climate.selected = climate.window_end - (climate.steps - index) * SAMPLE_SECONDS;
    climate.following = false;
    update_selection();
}

static void change_range(lv_event_t * event)
{
    climate.steps = (uint16_t)(intptr_t)lv_event_get_user_data(event);
    render_history();
}

static void change_mode(lv_event_t * event)
{
    climate.moisture = (intptr_t)lv_event_get_user_data(event) != 0;
    render_history();
}

static void pan_history(lv_event_t * event)
{
    const int32_t direction = (int32_t)(intptr_t)lv_event_get_user_data(event);
    const int32_t shift = direction * (int32_t)(climate.steps * SAMPLE_SECONDS / 2U);
    climate.window_end = (uint32_t)((int64_t)climate.window_end + shift);
    climate.following = false;
    render_history();
}

static void return_live(lv_event_t * event)
{
    LV_UNUSED(event);
    climate.following = true;
    climate.window_end = climate.latest;
    render_history();
}

static void return_controls(lv_event_t * event)
{
    LV_UNUSED(event);
    lv_timer_pause(climate.timer);
    lv_screen_load_anim(climate.return_screen, LV_SCREEN_LOAD_ANIM_MOVE_RIGHT, 220, 0, true);
}

static void screen_deleted(lv_event_t * event)
{
    LV_UNUSED(event);
    if (climate.timer) lv_timer_delete(climate.timer);
    climate.timer = NULL;
    climate.screen = NULL;
}

static void refresh_live(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    const uint32_t now = current_time();
    const uint32_t local = (uint32_t)((int64_t)now + (int32_t)clock_offset * 60);
    char clock_text[8];
    format_time(clock_text, sizeof(clock_text), now);
    lv_label_set_text(climate.clock, clock_ready ? clock_text : "--:--");
    lv_label_set_text_fmt(climate.seconds, "%02u", (unsigned)(local % 60U));
    lv_bar_set_value(climate.time_rail, (int32_t)(local % 60U * 1000U
                     + (uint32_t)(lv_tick_get() - clock_tick) % 1000U), LV_ANIM_OFF);
    const time_t local_date = (time_t)local;
    const struct tm * date = gmtime(&local_date);
    if (clock_ready && date) {
        static const char * days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        static const char * months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                       "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
        lv_label_set_text_fmt(climate.date, "%s / %02d %s", days[date->tm_wday], date->tm_mday,
                              months[date->tm_mon]);
    }
    const int32_t heat = temperature_at(now);
    const int32_t water = humidity_at(now);
    lv_label_set_text_fmt(climate.temperature, "%d.%d", (int)(heat / 10), (int)(heat % 10));
    lv_label_set_text_fmt(climate.humidity, "%d", (int)(water / 10));
    lv_label_set_text(climate.heat_status, heat < 200 ? "COOL / DEMO" : heat > 260 ? "WARM / DEMO" : "BALANCED / DEMO");
    lv_label_set_text(climate.water_status, water < 400 ? "DRY / DEMO" : water > 650 ? "HUMID / DEMO" : "BALANCED / DEMO");
    for (int index = 0; index < 16; ++index) {
        const int32_t wave = lv_trigo_sin((int16_t)((lv_tick_get() / 35U + index * 11U) % 360U));
        const int32_t height = 10 + index * 2 + wave * 5 / LV_TRIGO_SIN_MAX;
        lv_obj_set_height(climate.fins[index], height);
        lv_obj_set_y(climate.fins[index], 292 - height);
        lv_obj_set_style_bg_opa(climate.fins[index], (lv_opa_t)(100 + index * 10), 0);
    }
    const int filled = water * 32 / 1000;
    for (int index = 0; index < 32; ++index) {
        const uint32_t color = index < filled ? WATER : GRID;
        lv_obj_set_style_bg_color(climate.cells[index], lv_color_hex(color), 0);
        lv_obj_set_style_bg_opa(climate.cells[index], index < filled && index == (int)(lv_tick_get() / 250U % 32U)
                               ? LV_OPA_50 : LV_OPA_COVER, 0);
    }
    const uint32_t latest = now / SAMPLE_SECONDS * SAMPLE_SECONDS;
    if (latest != climate.latest) {
        climate.latest = latest;
        if (climate.following) climate.window_end = latest;
        render_history();
    }
}

void app_climate_show(lv_obj_t * return_screen)
{
    if (climate.screen) return;
    climate = (climate_state_t){0};
    climate.return_screen = return_screen;
    climate.steps = 72;
    climate.following = true;
    climate.latest = current_time() / SAMPLE_SECONDS * SAMPLE_SECONDS;
    climate.window_end = climate.latest;
    climate.screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(climate.screen);
    lv_obj_set_style_bg_color(climate.screen, lv_color_hex(INK), 0);
    lv_obj_set_style_bg_opa(climate.screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(climate.screen, &lv_font_montserrat_14, 0);
    lv_obj_set_scrollable(climate.screen, false);
    lv_obj_add_event_cb(climate.screen, screen_deleted, LV_EVENT_DELETE, NULL);
    lv_obj_t * screen = climate.screen;

    text(screen, 24, 18, 180, "ATMOS / 02", &lv_font_montserrat_14, LIME, NULL);
    lv_obj_t * heading = text(screen, 204, 18, 252, "CLIMATE OBSERVATORY", &lv_font_montserrat_14, MUTED, NULL);
    lv_obj_set_style_text_align(heading, LV_TEXT_ALIGN_RIGHT, 0);
    climate.clock = text(screen, 20, 53, 246, "--:--", &lv_font_montserrat_48, PAPER, "climate.clock");
    text(screen, 278, 48, 126, "LOCAL TIME", &lv_font_montserrat_14, MUTED, NULL);
    climate.seconds = text(screen, 410, 46, 46, "00", &lv_font_montserrat_20, LIME, "climate.seconds");
    climate.date = text(screen, 278, 80, 178, "TIME UNAVAILABLE", &lv_font_montserrat_14, PAPER, NULL);
    text(screen, 278, 103, 178, "DEMO SIGNAL / 24H", &lv_font_montserrat_14, MUTED, NULL);
    climate.time_rail = lv_bar_create(screen);
    lv_obj_remove_style_all(climate.time_rail);
    lv_obj_set_pos(climate.time_rail, 24, 132);
    lv_obj_set_size(climate.time_rail, 432, 3);
    lv_obj_set_style_bg_color(climate.time_rail, lv_color_hex(GRID), 0);
    lv_obj_set_style_bg_opa(climate.time_rail, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(climate.time_rail, lv_color_hex(LIME), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(climate.time_rail, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_bar_set_range(climate.time_rail, 0, 60000);

    text(screen, 24, 160, 190, "01 / TEMPERATURE", &lv_font_montserrat_14, MUTED, NULL);
    text(screen, 264, 160, 192, "02 / HUMIDITY", &lv_font_montserrat_14, MUTED, NULL);
    climate.temperature = text(screen, 22, 183, 145, "--.-", &lv_font_montserrat_40, HEAT, "climate.temperature");
    text(screen, 160, 200, 48, "\xC2\xB0" "C", &lv_font_montserrat_20, HEAT, NULL);
    climate.humidity = text(screen, 262, 183, 110, "--", &lv_font_montserrat_40, WATER, "climate.humidity");
    text(screen, 365, 200, 48, "%", &lv_font_montserrat_20, WATER, NULL);
    block(screen, 239, 163, 1, 148, GRID);
    for (int index = 0; index < 16; ++index)
        climate.fins[index] = block(screen, 24 + index * 12, 250, 7, 42, HEAT);
    for (int index = 0; index < 32; ++index)
        climate.cells[index] = block(screen, 264 + index % 8 * 24, 243 + index / 8 * 13, 19, 9, GRID);
    climate.heat_status = text(screen, 24, 305, 200, "BALANCED / DEMO", &lv_font_montserrat_14, MUTED, NULL);
    climate.water_status = text(screen, 264, 305, 192, "BALANCED / DEMO", &lv_font_montserrat_14, MUTED, NULL);
    block(screen, 24, 332, 432, 1, GRID);

    text(screen, 24, 346, 180, "Signal history", &lv_font_montserrat_20, PAPER, NULL);
    climate.previous = action(screen, 236, 342, 32, 30, LV_SYMBOL_LEFT, "climate.previous", pan_history, (void *)(intptr_t)-1);
    climate.next = action(screen, 274, 342, 32, 30, LV_SYMBOL_RIGHT, "climate.next", pan_history, (void *)(intptr_t)1);
    const char * captions[] = {"1H", "6H", "24H"};
    const char * names[] = {"climate.1h", "climate.6h", "climate.24h"};
    const int ranges[] = {12, 72, 288};
    for (int index = 0; index < 3; ++index)
        climate.ranges[index] = action(screen, 324 + index * 46, 342, 40, 30, captions[index], names[index],
                                        change_range, (void *)(intptr_t)ranges[index]);
    climate.selected_time = text(screen, 24, 386, 186, "", &lv_font_montserrat_14, LIME, "climate.selection");
    climate.live = action(screen, 206, 381, 48, 26, "LIVE", "climate.live", return_live, NULL);
    climate.selected_values = text(screen, 266, 386, 190, "", &lv_font_montserrat_14, PAPER, "climate.values");
    lv_obj_set_style_text_align(climate.selected_values, LV_TEXT_ALIGN_RIGHT, 0);

    climate.chart = lv_chart_create(screen);
    lv_obj_remove_style_all(climate.chart);
    lv_obj_set_pos(climate.chart, 60, 420);
    lv_obj_set_size(climate.chart, 396, 100);
    lv_obj_set_user_data(climate.chart, (void *)"climate.chart");
    lv_obj_set_clickable(climate.chart, true);
    lv_obj_set_press_lock(climate.chart, true);
    lv_obj_set_scrollable(climate.chart, false);
    lv_chart_set_type(climate.chart, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(climate.chart, 3, 5);
    lv_obj_set_style_line_color(climate.chart, lv_color_hex(GRID), LV_PART_MAIN);
    lv_obj_set_style_line_width(climate.chart, 1, LV_PART_MAIN);
    lv_obj_set_style_line_width(climate.chart, 3, LV_PART_ITEMS);
    lv_obj_set_style_size(climate.chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(climate.chart, 1, LV_PART_CURSOR);
    lv_obj_set_style_line_dash_width(climate.chart, 3, LV_PART_CURSOR);
    lv_obj_set_style_line_dash_gap(climate.chart, 3, LV_PART_CURSOR);
    lv_obj_set_style_size(climate.chart, 8, 8, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(climate.chart, lv_color_hex(LIME), LV_PART_CURSOR);
    lv_obj_set_style_bg_opa(climate.chart, LV_OPA_COVER, LV_PART_CURSOR);
    climate.series = lv_chart_add_series(climate.chart, lv_color_hex(HEAT), LV_CHART_AXIS_PRIMARY_Y);
    climate.cursor = lv_chart_add_cursor(climate.chart, lv_color_hex(LIME), LV_DIR_VER);
    lv_obj_add_event_cb(climate.chart, inspect_chart, LV_EVENT_ALL, NULL);
    climate.axis[0] = text(screen, 24, 414, 30, "28", &lv_font_montserrat_14, MUTED, NULL);
    climate.axis[1] = text(screen, 24, 505, 30, "18", &lv_font_montserrat_14, MUTED, NULL);
    for (int index = 0; index < 3; ++index) {
        climate.ticks[index] = text(screen, 60 + index * 166, 528, 64, "", &lv_font_montserrat_14, MUTED, NULL);
        lv_obj_set_style_text_align(climate.ticks[index], index == 2 ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
    }
    const char * stat_names[] = {"MINIMUM", "AVERAGE", "MAXIMUM"};
    for (int index = 0; index < 3; ++index) {
        text(screen, 24 + index * 148, 555, 132, stat_names[index], &lv_font_montserrat_14, MUTED, NULL);
        climate.stats[index] = text(screen, 24 + index * 148, 576, 132, "", &lv_font_montserrat_20, HEAT, NULL);
    }
    action(screen, 24, 606, 120, 30, LV_SYMBOL_LEFT " Controls", "climate.back", return_controls, NULL);
    climate.modes[0] = action(screen, 158, 606, 144, 30, "TEMPERATURE", "climate.mode.temperature", change_mode, NULL);
    climate.modes[1] = action(screen, 312, 606, 144, 30, "HUMIDITY", "climate.mode.humidity", change_mode, (void *)(intptr_t)1);
    render_history();
    refresh_live(NULL);
    climate.timer = lv_timer_create(refresh_live, 250, NULL);
    lv_screen_load_anim(screen, LV_SCREEN_LOAD_ANIM_MOVE_LEFT, 220, 0, false);
}