#include <pebble.h>
#define KEY_TEMP 0
#define KEY_ICON 1

static Window *s_main_window;
static Layer *s_dial_layer, *s_hands_layer, *s_temp_circle, *s_battery_circle, *s_health_circle;
static TextLayer *s_temp_layer, *s_health_layer, *s_day_text_layer, *s_date_text_layer;
static GBitmap *s_weather_bitmap, *s_health_bitmap, *s_bluetooth_bitmap, *s_charging_bitmap, *s_bluetooth_bitmap;
static BitmapLayer *s_weather_bitmap_layer, *s_health_bitmap_layer, *s_bluetooth_bitmap_layer, *s_charging_bitmap_layer, *s_bluetooth_bitmap_layer;
static GPath *s_minute_arrow, *s_hour_arrow, *s_minute_filler, *s_hour_filler;
static int buf=8, battery_percent, step_goal=100;
static GFont s_font;
static char icon_layer_buf[32];
static double step_count;
static char *char_current_steps;
static bool charging;

//////////////////////
// draw minute hand //
//////////////////////
static const GPathInfo MINUTE_HAND_POINTS = {
  5, (GPoint []) {
    {5, 16},
    {-5, 16},
    {-4, -64},
    {0, -70},
    {4, -64}
  }
};
static const GPathInfo MINUTE_HAND_FILLER = {
  4, (GPoint []) {
    {2, -16},
    {-2, -16},
    {-2, -60},
    {2, -60}
  }
};

////////////////////
// draw hour hand //
////////////////////
static const GPathInfo HOUR_HAND_POINTS = {
  5, (GPoint []) {
    {5, 16},
    {-5, 16},
    {-4, -48}, 
    {0, -54},
    {4, -48}
  }
};
static const GPathInfo HOUR_HAND_FILLER = {
  4, (GPoint []) {
    {2, -16},
    {-2, -16},
    {-2, -44},
    {2, -44}
  }
};

//////////////////////
// hide clock hands //
//////////////////////
static void hide_hands() {
  layer_set_hidden(s_hands_layer, true); 
}

//////////////////////
// show clock hands //
//////////////////////
static void show_hands() {
  layer_set_hidden(s_hands_layer, false);
}

/////////////////////////////////////////////////
// select click                                //
// hides hands for 5 seconds, then shows again //
/////////////////////////////////////////////////
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  hide_hands();
  app_timer_register(5000, show_hands, NULL);
}

///////////////////
// assign clicks //
///////////////////
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

/////////////////////////
// draws dial on watch //
/////////////////////////
static void dial_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds); 
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, (bounds.size.w+buf)/2);
  
  // set number of tickmarks
  int tick_marks_number = 60;

  // tick mark lengths
  int tick_length_end = (bounds.size.w+buf)/2; 
  int tick_length_start;
  
  // set colors
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 6);
  
  for(int i=0; i<tick_marks_number; i++) {
    // if number is divisible by 5, make large mark
    if(i%5==0) {
      graphics_context_set_stroke_width(ctx, 4);
      tick_length_start = tick_length_end-8;
    } else {
      graphics_context_set_stroke_width(ctx, 1);
      tick_length_start = tick_length_end-4;
    }

    int angle = TRIG_MAX_ANGLE * i/tick_marks_number;

    GPoint tick_mark_start = {
      .x = (int)(sin_lookup(angle) * (int)tick_length_start / TRIG_MAX_RATIO) + center.x,
      .y = (int)(-cos_lookup(angle) * (int)tick_length_start / TRIG_MAX_RATIO) + center.y,
    };
    
    GPoint tick_mark_end = {
      .x = (int)(sin_lookup(angle) * (int)tick_length_end / TRIG_MAX_RATIO) + center.x,
      .y = (int)(-cos_lookup(angle) * (int)tick_length_end / TRIG_MAX_RATIO) + center.y,
    };      
    
    graphics_draw_line(ctx, tick_mark_end, tick_mark_start);  
  } // end of loop 
  
  // draw box for day and date on right of watch
  GRect temp_rect = GRect(87, 77, 43, 13);
  graphics_draw_round_rect(ctx, temp_rect, 3);
  // dividing line in date round rectange
  GPoint start_temp_line = GPoint(114, 77);
  GPoint end_temp_line = GPoint(114, 88);
  graphics_draw_line(ctx, start_temp_line, end_temp_line);    
}

////////////////////////
// update temperature //
////////////////////////
static void temp_update_proc(Layer *layer, GContext *ctx) {
  GPoint center = GPoint(144/2, 46);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, center, 36/2);
}

///////////////////////////
// update battery status //
///////////////////////////
static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = GRect(16, 66, 36, 36);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, 2, DEG_TO_TRIGANGLE(360-(battery_percent*3.6)), DEG_TO_TRIGANGLE(360));
  
  // draw vertical battery
  graphics_draw_round_rect(ctx, GRect(31, 77, 7, 14), 1);
  int batt = battery_percent/10;
  graphics_fill_rect(ctx, GRect(33, 89-batt, 3, batt), 1, GCornerNone);
  graphics_fill_rect(ctx, GRect(33, 76, 3, 1), 0, GCornerNone);  
  
  // set visibility of charging icon
  layer_set_hidden(bitmap_layer_get_layer(s_charging_bitmap_layer), !charging);
}

//////////////////////////
// update health status //
//////////////////////////
static void health_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = GRect(54, 104, 36, 36);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, 2, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE((step_count/step_goal)*360));
}

/////////////////////////////////
// draw hands and update ticks //
/////////////////////////////////
static void ticks_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds); 
    
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  // set stroke to 1
  graphics_context_set_stroke_width(ctx, 1);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  
  // draw minute hand
  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  // draw hour hand
  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);
  
  // switch color for fillers
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);  
   
  // draw minute filler
  gpath_rotate_to(s_minute_filler, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_filler);
  gpath_draw_outline(ctx, s_minute_filler);
  
  // draw hour filler
  gpath_rotate_to(s_hour_filler, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_filler);
  gpath_draw_outline(ctx, s_hour_filler);

  // switch colors for center circle
  graphics_context_set_fill_color(ctx, GColorWhite);
  
  // circle overlay
  graphics_fill_circle(ctx, center, 7);
  
  // outline circle overlay
  graphics_draw_circle(ctx, center, 7);
  
  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 1);
}

//////////////////////
// load main window //
//////////////////////
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // set background color
  window_set_background_color(s_main_window, GColorBlack);
  
  // register button clicks
  window_set_click_config_provider(window, click_config_provider);
  
  s_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);

  // create canvas layer for dial
  s_dial_layer = layer_create(bounds);
  layer_set_update_proc(s_dial_layer, dial_update_proc);
  layer_add_child(window_layer, s_dial_layer);  
  
  // create temp circle
  s_temp_circle = layer_create(bounds);
  layer_set_update_proc(s_temp_circle, temp_update_proc);
  layer_add_child(s_dial_layer, s_temp_circle);
  
  // create temp text
  s_temp_layer = text_layer_create(GRect(60, 28, 24, 16));
  text_layer_set_background_color(s_temp_layer, GColorClear);
  text_layer_set_text_alignment(s_temp_layer, GTextAlignmentCenter);
  text_layer_set_font(s_temp_layer, s_font);
  text_layer_set_text(s_temp_layer, "100");
  layer_add_child(s_dial_layer, text_layer_get_layer(s_temp_layer));
  
  // create battery layer
  s_battery_circle = layer_create(bounds);
  layer_set_update_proc(s_battery_circle, battery_update_proc);
  layer_add_child(s_dial_layer, s_battery_circle);
  
  // charging icon
  s_charging_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LIGHTENING_BLACK_ICON);
  s_charging_bitmap_layer = bitmap_layer_create(GRect(36, 76, 14, 14));
  bitmap_layer_set_compositing_mode(s_charging_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_charging_bitmap_layer, s_charging_bitmap); 
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_charging_bitmap_layer));    
  
  // bluetooth disconnected icon
  s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_DISCONNECTED_BLACK_ICON);
  s_bluetooth_bitmap_layer = bitmap_layer_create(GRect(18, 76, 14, 14));
  bitmap_layer_set_compositing_mode(s_bluetooth_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_bluetooth_bitmap_layer, s_bluetooth_bitmap); 
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_bluetooth_bitmap_layer));       
  
  // create health layer text
  s_health_layer = text_layer_create(GRect(54, 108, 36, 16));
  text_layer_set_background_color(s_health_layer, GColorClear);
  text_layer_set_text_alignment(s_health_layer, GTextAlignmentCenter);
  text_layer_set_font(s_health_layer, s_font);
  layer_add_child(s_dial_layer, text_layer_get_layer(s_health_layer));  
  
  // create health layer circle
  s_health_circle = layer_create(bounds);
  layer_set_update_proc(s_health_circle, health_update_proc);
  layer_add_child(s_dial_layer, s_health_circle);
    
  // create shoe icon
  s_health_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SHOE_BLACK_ICON);
  s_health_bitmap_layer = bitmap_layer_create(GRect(60, 123, 24, 16));
  bitmap_layer_set_compositing_mode(s_health_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_health_bitmap_layer, s_health_bitmap); 
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_health_bitmap_layer));
  
  // Day Text
  s_day_text_layer = text_layer_create(GRect(88, 74, 26, 14));
  text_layer_set_background_color(s_day_text_layer, GColorClear);
  text_layer_set_text_alignment(s_day_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_day_text_layer, s_font);
  layer_add_child(s_dial_layer, text_layer_get_layer(s_day_text_layer));
  
  // Date text
  s_date_text_layer = text_layer_create(GRect(113, 74, 16, 14));
  text_layer_set_background_color(s_date_text_layer, GColorClear);
  text_layer_set_text_alignment(s_date_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_text_layer, s_font);
  layer_add_child(s_dial_layer, text_layer_get_layer(s_date_text_layer));  
    
  // create canvas layer for hands
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, ticks_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

///////////////////////
// update clock time //
///////////////////////
static void update_time() {
  // get a tm strucutre
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // write date to buffer
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%d", tick_time);
  
  // write day to buffer
  static char day_buffer[16];
  strftime(day_buffer, sizeof(day_buffer), "%a", tick_time);
  
  // display this time on the text layer
  text_layer_set_text(s_date_text_layer, date_buffer);
  text_layer_set_text(s_day_text_layer, day_buffer);  
}

//////////////////
// handle ticks //
//////////////////
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_hands_layer);
  update_time();
}

/////////////////////////////////////
// registers battery update events //
/////////////////////////////////////
static void battery_handler(BatteryChargeState charge_state) {
  battery_percent = charge_state.charge_percent;
  if(charge_state.is_charging || charge_state.is_plugged) {
    charging = true;
  } else {
    charging = false;
  }
  // force update to circle
  layer_mark_dirty(s_battery_circle);
}

/////////////////////////////
// manage bluetooth status //
/////////////////////////////
static void bluetooth_callback(bool connected) {
  layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_bitmap_layer), connected);
  if(!connected) {  
    vibes_double_pulse();
  } 
}

// registers health update events
static void health_handler(HealthEventType event, void *context) {
  if(event==HealthEventMovementUpdate) {
    step_count = (double)health_service_sum_today(HealthMetricStepCount);
    
    // write to char_current_steps variable
    static char health_buf[16];
    snprintf(health_buf, sizeof(health_buf), "%d", (int)step_count);
    char_current_steps = health_buf;
    text_layer_set_text(s_health_layer, char_current_steps);
    
    // force update to circle
    layer_mark_dirty(s_health_circle);
    
    APP_LOG(APP_LOG_LEVEL_INFO, "health_handler completed");
  }
}

///////////////////
// unload window //
///////////////////
static void main_window_unload(Window *window) {
  layer_destroy(s_dial_layer);
  layer_destroy(s_hands_layer);
  layer_destroy(s_temp_circle);
  layer_destroy(s_battery_circle);
  layer_destroy(s_health_circle);
  text_layer_destroy(s_temp_layer);
  text_layer_destroy(s_health_layer);
  text_layer_destroy(s_day_text_layer);
  text_layer_destroy(s_date_text_layer);
  gbitmap_destroy(s_weather_bitmap);
  gbitmap_destroy(s_health_bitmap);
  gbitmap_destroy(s_bluetooth_bitmap);
  gbitmap_destroy(s_charging_bitmap);
  gbitmap_destroy(s_bluetooth_bitmap);
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);
  gpath_destroy(s_minute_filler);
  gpath_destroy(s_hour_filler);
}

//////////////////////////////////////
// display appropriate weather icon //
//////////////////////////////////////
static void load_icons() {
  // populate icon variable
    if(strcmp(icon_layer_buf, "clear-day")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_DAY_BLACK_ICON);  
    } else if(strcmp(icon_layer_buf, "clear-night")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_NIGHT_BLACK_ICON);
    }else if(strcmp(icon_layer_buf, "rain")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RAIN_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "snow")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SNOW_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "sleet")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SLEET_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "wind")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WIND_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "fog")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FOG_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "cloudy")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLOUDY_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "partly-cloudy-day")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_DAY_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "partly-cloudy-night")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_NIGHT_BLACK_ICON);
    }
  // populate weather icon
  s_weather_bitmap_layer = bitmap_layer_create(GRect(60, 46, 24, 16));
  bitmap_layer_set_compositing_mode(s_weather_bitmap_layer, GCompOpSet);  
  bitmap_layer_set_bitmap(s_weather_bitmap_layer, s_weather_bitmap); 
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_weather_bitmap_layer)); 
}

///////////////////
// weather calls //
///////////////////
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temp_buf[8];
  static char icon_buf[32];
  static char temp_layer_buf[32];

  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMP);
  Tuple *icon_tuple = dict_find(iterator, KEY_ICON);  

  // If all data is available, use it
  if(temp_tuple && icon_tuple) {
    
    // temp
    snprintf(temp_buf, sizeof(temp_buf), "%d", (int)temp_tuple->value->int32);
    snprintf(temp_layer_buf, sizeof(temp_layer_buf), "%s", temp_buf);
    text_layer_set_text(s_temp_layer, temp_layer_buf);

    // icon
    snprintf(icon_buf, sizeof(icon_buf), "%s", icon_tuple->value->cstring);
    snprintf(icon_layer_buf, sizeof(icon_layer_buf), "%s", icon_buf);    
  }  
  
  load_icons();
  APP_LOG(APP_LOG_LEVEL_INFO, "inbox_received_callback");
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

////////////////////
// initialize app //
////////////////////
static void init() {
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // show window on the watch with animated=true
  window_stack_push(s_main_window, true);
  
  // subscribe to time events
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Make sure the time is displayed from the start
  update_time();
  
  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_minute_filler = gpath_create(&MINUTE_HAND_FILLER);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);
  s_hour_filler = gpath_create(&HOUR_HAND_FILLER);
  
  // move hands to proper locations
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_minute_filler, center);
  gpath_move_to(s_hour_arrow, center);    
  gpath_move_to(s_hour_filler, center);   
    
  // subscribe to health events 
  health_service_events_subscribe(health_handler, NULL); 
  // force initial update
  health_handler(HealthEventMovementUpdate, NULL);   
    
  // register with Battery State Service
  battery_state_service_subscribe(battery_handler);
  // force initial update
  battery_handler(battery_state_service_peek());      
  
  // register with bluetooth state service
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
  bluetooth_callback(connection_service_peek_pebble_app_connection());  
  
  // Register weather callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);  
  
  // Open AppMessage for weather callbacks
  const int inbox_size = 64;
  const int outbox_size = 64;
  app_message_open(inbox_size, outbox_size);  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Clock show_clock_window");  
}

///////////////////////
// de-initialize app //
///////////////////////
static void deinit() {
  window_destroy(s_main_window);
}

/////////////
// run app //
/////////////
int main(void) {
  init();
  app_event_loop();
  deinit();
}