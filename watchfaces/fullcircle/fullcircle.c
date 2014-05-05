#include <pebble.h>

/*
 * Watchface that shows the time as a progressing circle
 * with the hour in text in the center
 *
 * Idea taken from one of the watchfaces of "letsmuv" 
 * (http://letsmuv.com)
 *
 * Copyright (c) 2014 Christiaan Ottow <chris@6core.net>
 * Distriuted under MIT license, see license.txt
 */

static Window *window;

// center
static GPoint center;

// magic numbers
static uint16_t clock_radius = 64;
static uint16_t hour_radius = 30;

// layers, ordering as on screen
static Layer *dial_layer;       // the white clock dial
static Layer *minute_layer;     // the minute marker
static TextLayer *hour_layer;   // the hour text
static Layer *deco_layer;       // decorations

static GPath *minute_path;      // mask for minutes
static const GPathInfo PATH_INFO = {
    .num_points = 4,
    .points = (GPoint []) { 
      {0,0}, 
      {0,-1*(168/2)}, 
      {(144/2),-1*(168/2)}, 
      {(144/2),0} 
  }
};

static char hour_display[3];
static unsigned short minutes = 0;

/*
 * set the global minutes variable
 */
static void set_time() {
  time_t now = time(NULL);
  struct tm *ltime = localtime(&now);
  minutes = ltime->tm_min;
}
/**
 * Set the bounds of the minute layer
 */
static void set_minute_bounds(unsigned short int m) {
  // re-set the frame: if minutes > 45, the rotated black quadrant
  // becomes a problem because it moves over 0. easiest way around this
  // is to set the bounds of the layer; since clipping is enbaled the
  // quadrant will not move over 0
  if(m > 30) {
    layer_set_frame(minute_layer, GRect(0,0,(144/2),168));
  } else {
    layer_set_frame(minute_layer, GRect(0,0,144,168));
  }
}

/**
 * Handler for minute tick
 * Updates the text layer if needed and triggers updating
 * of the minute layer
 */
static void do_update_time(struct tm *current_time) {

  // update the hour layer
  snprintf(hour_display,3,"%d",(int)current_time->tm_hour);
  text_layer_set_text(hour_layer, hour_display);

  // register the minutes
  minutes = current_time->tm_min;

  // re-set the bounds. we shouldn't be doing this from
  // .update_proc, change doesn't take effect until next render.
  // but as we're doing this far in advance, it will be ok by 
  // the time it's needed
  set_minute_bounds(minutes);

  // mark the minute layer as dirty so it will be redrawn
  layer_mark_dirty(minute_layer);
  layer_mark_dirty(deco_layer);
}

/**
 * Redraw the dial layer
 * draws the white clock dial
 */
static void dial_layer_draw(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  // draw the white minute circle
  graphics_fill_circle(ctx, center, clock_radius);   
  
  // draw the black center circle for hour display
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, hour_radius);
}

/**
 * Redraw handler for the minute layer
 * Draws the minute dial
 */
static void minute_layer_draw(Layer *layer, GContext *ctx) {

  // rotate the black quadrant that we use to mask the white circle
  gpath_rotate_to(minute_path, (TRIG_MAX_ANGLE * minutes / 60));
  gpath_move_to(minute_path, GPoint(144/2,168/2));
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, minute_path);

  // draw the other overlays
  if(minutes < 45){
    // hide the last quadrant (45-60)
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0,0,144/2,168/2), 0, 0);
  }

  if(minutes < 30) {
    // hide third quadrant (30-45)
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0,168/2,144/2,168/2), 0, 0);
  }

  if(minutes <= 15) {
    // hide second quadrant (15-30)
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(144/2,168/2,144/2,168/2), 0, 0);
  }

}

/**
 * Update the decorations layer
 */
static void deco_layer_draw(Layer *layer, GContext *ctx) {
//    graphics_context_set_fill_color(ctx, GColorBlack);
//    graphics_fill_circle(ctx, center, 42);
//    graphics_context_set_fill_color(ctx, GColorWhite);
//    graphics_fill_circle(ctx, center, 33);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, center, 26);
}

/**
 * Gets called at the new minute, passes the buck
 */
static void handle_minute_tick(struct tm *time_tick, TimeUnits units_changed) {
  do_update_time(time_tick);
}

/**
 * Create the layers
 */
static void window_load(Window *window) {

  // get the current time
  set_time();

  // globals
  window_set_background_color(window, GColorBlack);
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  center = grect_center_point(&bounds);

  // create the dial layer
  dial_layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(dial_layer, dial_layer_draw);
  layer_add_child(window_layer, dial_layer);

  // create the minutes drawing layer
  minute_path = gpath_create(&PATH_INFO);
  minute_layer = layer_create(GRect(0,0,144,168));
  layer_set_clips(minute_layer, true);
  layer_set_update_proc(minute_layer, minute_layer_draw);
  layer_add_child(window_layer, minute_layer);

  // decorations layer
  deco_layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(deco_layer, deco_layer_draw);
  layer_add_child(window_layer, deco_layer);
  
  // create the hour display layer
  int r = 21;
  hour_layer = text_layer_create(GRect((144/2)-r, (168/2)-r, 2*r, 2*r));
  text_layer_set_text_alignment(hour_layer, GTextAlignmentCenter);
  text_layer_set_text_color(hour_layer, GColorWhite);
  text_layer_set_background_color(hour_layer, GColorClear);
  text_layer_set_font(hour_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  layer_add_child(window_layer, text_layer_get_layer(hour_layer));

}

/**
 * tear down the layers
 */
static void window_unload(Window *window) {
  text_layer_destroy(hour_layer);
  layer_destroy(minute_layer);
  layer_destroy(dial_layer);
}

/**
 * create layers, subscribe to time ticks
 */
static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  // subscribe to minute tick for clock update
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
