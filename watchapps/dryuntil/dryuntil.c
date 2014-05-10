/*
 * dryuntil - a watchapp that shows how long until the next rain shower
 * if that's within the next 2 hours.
 *
 * weather data is read from buienradar.nl, and may only work in .nl
 *
 * Copyright (c) 2014 Christiaan Ottow
 * Distributed under MIT license, see LICENSE
 */

#include <pebble.h>
#define STR_SIZ 20
#define MSG_SIZ 42
#define RAIN_THRESHOLD 30

static Window *window;
static TextLayer *loading_layer;    // not used yet
static TextLayer *city_layer;
static TextLayer *status_layer;
static TextLayer *rain_layer;

static char city[STR_SIZ];          // our locality (city name)
static char rain[STR_SIZ];          // rain predicted yes/no
static char start[6];               // rain start time
static int rain_d;

/*
 * ************************************
 * AppMessage handlers
 * ************************************
 */
enum {
    AKEY_CITY,
    AKEY_RAIN,
    AKEY_START
};

/**
 * Incoming message received from Phone JS
 * This indicates a weather update and is handled here
 */
void in_received_handler(DictionaryIterator *received, void *context) 
{
    Tuple *tuple = dict_read_first(received);
    while(tuple) {
        switch(tuple->key) {
            case AKEY_CITY:
                snprintf(city, 19, "in %s", tuple->value->cstring );
                break;
            case AKEY_RAIN:
                rain_d = (int)tuple->value->int32;
                break;
            case AKEY_START:
                strncpy(start, tuple->value->cstring, 5);
                break;
            default:
                APP_LOG(APP_LOG_LEVEL_DEBUG, "unknown key");
                break;
        }
        tuple = dict_read_next(received);
    }

    if(rain_d) {
        text_layer_set_text(status_layer, "Dry until");
        text_layer_set_text(rain_layer, start);
    } else {
        text_layer_set_text(status_layer, "It stays");
        text_layer_set_text(rain_layer, "Dry");
    }
    text_layer_set_text(city_layer, city);

}

/*
 * ************************************
 * button handlers
 * Unused now, but here for future use
 * ************************************
 */
static void select_click_handler(ClickRecognizerRef recognizer, void *context) 
{
//    text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) 
{
//    text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) 
{
//    text_layer_set_text(text_layer, "Down");
}

static void click_config_provider(void *context) 
{
    /*
       window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
       window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
       window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
       */
}

/*
 * ************************************
 * UI initialization
 * ************************************
 */
static void window_load(Window *window) 
{
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    loading_layer = text_layer_create((GRect) { .origin = { 0, 20 }, .size = { bounds.size.w, 40 } });
    text_layer_set_font(loading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text(loading_layer, "Loading...");
    text_layer_set_text_alignment(loading_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(loading_layer));

    // Layer for the status display
    status_layer = text_layer_create((GRect) { .origin = { 0, 10 }, .size = { bounds.size.w, 40 } });
    text_layer_set_font(status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text(status_layer, "Loading...");
    text_layer_set_text_alignment(status_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(status_layer));

    // Layer for the rain start time display
    rain_layer = text_layer_create((GRect) { .origin = { 0, 55 }, .size = { bounds.size.w, 40 } });
    text_layer_set_font(rain_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_text_alignment(rain_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(rain_layer));

    // Layer for the city display
    city_layer = text_layer_create((GRect) { .origin = { 0, 110 }, .size = { bounds.size.w, 40 } });
    text_layer_set_font(city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(city_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(city_layer));

}

static void window_unload(Window *window) 
{
    text_layer_destroy(status_layer);
    text_layer_destroy(rain_layer);
}

static void init(void) 
{
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
            });
    const bool animated = true;
    window_stack_push(window, animated);
}

static void deinit(void) 
{
    window_destroy(window);
}

int main(void) 
{
    init();

    // setup AppMessage channel
    app_message_register_inbox_received(in_received_handler);
    const uint32_t inbound_size = 64;
    const uint32_t outbound_size = 64;
    app_message_open(inbound_size, outbound_size);

    // main loop
    app_event_loop();
    deinit();
}

