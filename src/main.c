#include <pebble.h>

#include "weather_layer.h"
#include "network.h"
#include "config.h"

#define TIME_FRAME      (GRect(0, 2, 144, 168-6))
#define DATE_FRAME      (GRect(1, 66, 144, 168-62))

/* Static variables to keep track of the UI elements */
static Window *s_window;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;
static WeatherLayer *s_weather_layer;

static char s_date_text[] = "XXX 00";
static char s_time_text[] = "00:00";

static time_t s_last_weather_update = 0;
static bool s_weather_loaded = false;

/* Preload the fonts */
static GFont s_font_date;
static GFont s_font_time;

static void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Tick handler.");
  if (units_changed & MINUTE_UNIT) {
    clock_copy_time_string(s_time_text, sizeof(s_time_text));
    // Despite Matt's assurances otherwise, the above will actually attempt to include
    // an AM/PM string even if it will not fit. Truncate a trailing space if necessary.
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
  static int s_animation_step = 0;
  if (!s_weather_loaded)
  {
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
  } else {
    // Show the temperature as 'stale' if it has not been updated in 30 minutes
    bool stale = (s_last_weather_update > time(NULL) + 1800);
    if (stale) {
      weather_layer_mark_stale(s_weather_layer);
    }
  }

  // Refresh the weather info every 15 minutes
  if (units_changed & MINUTE_UNIT && (tick_time->tm_min % 15) == 0) {
    request_weather();
  }
}

static void handle_weather_update(WeatherData* weather) {
  s_last_weather_update = time(NULL);
  weather_layer_set_temperature(s_weather_layer, weather->temperature);

  const bool is_night = (weather->current_time < weather->sunrise || weather->current_time > weather->sunset);
  weather_layer_set_icon(s_weather_layer, weather_icon_for_condition(weather->condition, is_night));
  
  if(!s_weather_loaded) {
    s_weather_loaded = true;
    // We don't need to run this every second any more.
    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  }
}

static void init(void) {
  s_window = window_create();
  window_stack_push(s_window, true /* Animated */);
  window_set_background_color(s_window, GColorBlack);

  init_network();
  set_weather_update_handler(handle_weather_update);

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
