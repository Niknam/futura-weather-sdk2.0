#include <pebble.h>

#include "weather_layer.h"
#include "network.h"

#define TIME_FRAME      (GRect(0, 2, 144, 168-6))
#define DATE_FRAME      (GRect(1, 66, 144, 168-62))
#define LOADING_TIMEOUT 30000

/* Static variables to keep track of the UI elements */
static Window *s_window;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;
static WeatherLayer *s_weather_layer;

static char s_date_text[] = "XXX 00";
static char s_time_text[] = "00:00";

static bool s_weather_loaded = false;

static AppTimer *s_loading_timeout = NULL;

/* Preload the fonts */
static GFont s_font_date;
static GFont s_font_time;

static void step_loading_animation() {
  static int s_animation_step = 0;
  // 'Animate' loading icon until the first successful weather request
  if (s_animation_step == 0) {
    weather_layer_set_icon(s_weather_layer, WEATHER_ICON_LOADING1);
  }
  else if (s_animation_step == 1) {
    weather_layer_set_icon(s_weather_layer, WEATHER_ICON_LOADING2);
  }
  else if (s_animation_step >= 2) {
    weather_layer_set_icon(s_weather_layer, WEATHER_ICON_LOADING3);
  }
  s_animation_step = (s_animation_step + 1) % 3;
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
  if (units_changed & MINUTE_UNIT) {
    clock_copy_time_string(s_time_text, sizeof(s_time_text));
    // clock_copy_time_string may including a trailing space as it tries to fit in
    // the AM/PM in 24-hour time; truncate it if so.
    if (s_time_text[4] == ' ') {
      s_time_text[4] = '\0';
    }

    text_layer_set_text(s_time_layer, s_time_text);
  }
  if (units_changed & DAY_UNIT) {
    // Update the date - Without a leading 0 on the day of the month
    char day_text[4];
    strftime(day_text, sizeof(day_text), "%a", tick_time);
    snprintf(s_date_text, sizeof(s_date_text), "%s %i", day_text, tick_time->tm_mday);
    text_layer_set_text(s_date_layer, s_date_text);
  }

  // Run the animation if we haven't loaded the weather yet.
  if (!s_weather_loaded) {
    step_loading_animation();
  }

  // Refresh the weather info every 15 minutes
  if ((units_changed & MINUTE_UNIT) && (tick_time->tm_min % 15 == 0)) {
    request_weather();
  }
}

static void mark_weather_loaded(void) {
  if (!s_weather_loaded) {
    s_weather_loaded = true;
    // We don't need to run this every second any more.
    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  }
  if (s_loading_timeout != NULL) {
    app_timer_cancel(s_loading_timeout);
    s_loading_timeout = NULL;
  }
}

static void handle_weather_update(WeatherData* weather) {
  weather_layer_set_temperature(s_weather_layer, weather->temperature);

  const bool is_night = (weather->current_time < weather->sunrise || weather->current_time > weather->sunset);
  weather_layer_set_icon(s_weather_layer, weather_icon_for_condition(weather->condition, is_night));

  mark_weather_loaded();
}

static void handle_weather_error(WeatherError error) {
  // We apparently don't actually care what the error was at all.
  weather_layer_set_icon(s_weather_layer, WEATHER_ICON_PHONE_ERROR);

  mark_weather_loaded();
}

static void handle_loading_timeout(void* unused) {
  s_loading_timeout = NULL;
  if (!s_weather_loaded) {
    handle_weather_error(WEATHER_E_PHONE);
  }
}

static void init(void) {
  s_window = window_create();
  window_stack_push(s_window, true /* Animated */);
  window_set_background_color(s_window, GColorBlack);

  init_network();
  set_weather_update_handler(handle_weather_update);
  set_weather_error_handler(handle_weather_error);

  s_font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18));
  s_font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_53));

  s_time_layer = text_layer_create(TIME_FRAME);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, s_font_time);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_time_layer));

  s_date_layer = text_layer_create(DATE_FRAME);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, s_font_date);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_date_layer));

  // Add weather layer
  s_weather_layer = weather_layer_create(GRect(0, 90, 144, 80));
  layer_add_child(window_get_root_layer(s_window), s_weather_layer);

  // Update the screen right away
  time_t now = time(NULL);
  handle_tick(localtime(&now), SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT );
  // And then every second
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

  // We don't trigger the weather from here, because it is possible for the other end
  // to not yet be running when we finish startup. However, we do set a timeout so we
  // don't sit around animating forever. However, if Bluetooth is not connected, then
  // this cannot possibly succeed and we fail instantly.
  if(bluetooth_connection_service_peek()) {
    s_loading_timeout = app_timer_register(LOADING_TIMEOUT, handle_loading_timeout, NULL);
  } else {
    handle_weather_error(WEATHER_E_DISCONNECTED);
  }
}

static void deinit(void) {
  window_destroy(s_window);
  tick_timer_service_unsubscribe();

  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  weather_layer_destroy(s_weather_layer);

  fonts_unload_custom_font(s_font_date);
  fonts_unload_custom_font(s_font_time);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
