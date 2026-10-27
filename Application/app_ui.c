#include "app_ui.h"
#include "app_climate.h"

static lv_obj_t * value_label;
static lv_obj_t * value_slider;
static int32_t previous_value;

static void update_value(void)
{
    const int32_t value = lv_slider_get_value(value_slider);
    lv_label_set_text_fmt(value_label, "%d", (int)value);
    if (value != previous_value) {
        LV_LOG_USER("Slider value: %d", (int)value);
        previous_value = value;
    }
}

static void slider_changed(lv_event_t * event)
{
    LV_UNUSED(event);
    update_value();
}

static void increase_clicked(lv_event_t * event)
{
    LV_UNUSED(event);
    lv_slider_set_value(value_slider, lv_slider_get_value(value_slider) + 1, LV_ANIM_OFF);
    update_value();
}

static void decrease_clicked(lv_event_t * event)
{
    LV_UNUSED(event);
    lv_slider_set_value(value_slider, lv_slider_get_value(value_slider) - 1, LV_ANIM_OFF);
    update_value();
}

static void reset_clicked(lv_event_t * event)
{
    LV_UNUSED(event);
    lv_slider_set_value(value_slider, 0, LV_ANIM_OFF);
    update_value();
}

static void climate_clicked(lv_event_t * event)
{
    app_climate_show(lv_event_get_user_data(event));
}

void app_ui_create(lv_obj_t * parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0xF3F5F4), 0);
    lv_obj_set_style_text_color(parent, lv_color_hex(0x202724), 0);
    lv_obj_set_style_pad_all(parent, 32, 0);
    lv_obj_set_style_pad_row(parent, 28, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * title = lv_label_create(parent);
    lv_label_set_text(title, "Control Panel");

    lv_obj_t * caption = lv_label_create(parent);
    lv_label_set_text(caption, "Setting Value");

    value_label = lv_label_create(parent);
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(value_label, lv_color_hex(0x087F5B), 0);
    lv_obj_set_width(value_label, 100);
    lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_CENTER, 0);

    value_slider = lv_slider_create(parent);
    lv_obj_set_width(value_slider, lv_pct(85));
    lv_slider_set_range(value_slider, 0, 100);
    lv_slider_set_value(value_slider, 50, LV_ANIM_OFF);
    previous_value = lv_slider_get_value(value_slider);
    lv_obj_set_style_bg_color(value_slider, lv_color_hex(0x087F5B), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(value_slider, lv_color_hex(0x087F5B), LV_PART_KNOB);
    lv_obj_add_event_cb(value_slider, slider_changed, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t * buttons = lv_obj_create(parent);
    lv_obj_remove_style_all(buttons);
    lv_obj_set_size(buttons, lv_pct(100), 52);
    lv_obj_set_flex_flow(buttons, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttons, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * decrease = lv_button_create(buttons);
    lv_obj_set_size(decrease, 64, 48);
    lv_obj_add_event_cb(decrease, decrease_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t * decrease_label = lv_label_create(decrease);
    lv_label_set_text(decrease_label, LV_SYMBOL_MINUS);
    lv_obj_center(decrease_label);

    lv_obj_t * reset = lv_button_create(buttons);
    lv_obj_set_size(reset, 120, 48);
    lv_obj_add_event_cb(reset, reset_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t * reset_label = lv_label_create(reset);
    lv_label_set_text(reset_label, "Reset");
    lv_obj_center(reset_label);

    lv_obj_t * increase = lv_button_create(buttons);
    lv_obj_set_size(increase, 64, 48);
    lv_obj_add_event_cb(increase, increase_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t * increase_label = lv_label_create(increase);
    lv_label_set_text(increase_label, LV_SYMBOL_PLUS);
    lv_obj_center(increase_label);

    lv_obj_t * climate = lv_button_create(parent);
    lv_obj_set_size(climate, 180, 44);
    lv_obj_set_user_data(climate, (void *)"climate.open");
    lv_obj_add_event_cb(climate, climate_clicked, LV_EVENT_CLICKED, parent);
    lv_obj_t * climate_label = lv_label_create(climate);
    lv_label_set_text(climate_label, "Climate " LV_SYMBOL_RIGHT);
    lv_obj_center(climate_label);

    update_value();
}