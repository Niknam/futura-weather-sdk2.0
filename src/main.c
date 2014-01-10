#include <pebble.h>

#include "weather_layer.h"
#include "link_monitor.h"
#include "config.h"

#define TIME_FRAME      (GRect(0, 2, 144, 168-6))
#define DATE_FRAME      (GRect(1, 66, 144, 168-62))

// Received variables
#define WEATHER_KEY_ICON 1
#define WEATHER_KEY_TEMPERATURE 2

/* Global variables to keep track of the UI elements */
static Window *window;
static TextLayer *date_layer;
static TextLayer *time_layer;
static WeatherLayer *weather_layer;

/* Preload the fonts */
GFont font_date;
GFont font_time;

static int initial_minute;
static bool has_failed = false;
static bool has_timed_out = false;
static bool phone_disconnected = false;

//Counters
static int fail_count = 0;
static int http_request_build_count = 0;
static int http_request_send_count = 0;
static int load_count = 0;

//Weather Stuff
static int our_latitude, our_longitude;
static int16_t last_valid_temperature;
static bool located = false;
static bool initial_request = true;
static bool has_temperature = false;
static bool is_stale = false;

// void request_weather();

// void failed(int32_t cookie, int http_status, void* context) {

//  /* This is triggered by invalid responses from the bridge on the phone,
//   * due to e.g. HTTP errors or phone disconnections.
//   */
//  if(HTTP_INVALID_BRIDGE_RESPONSE) {

//    // Phone can still be reached (assume HTTP error)
//    if (http_time_request() == HTTP_OK) {

//      /* Display 'cloud error' icon if:
//       * - HTTP error is noticed upon inital load
//       * - watch has recovered from a phone disconnection, but has no internet connection
//       */
//      if (!has_temperature || phone_disconnected) {
//        weather_layer_set_icon(&weather_layer, WEATHER_ICON_CLOUD_ERROR);
//        phone_disconnected = false;
//      }

//      /* Assume that weather information is stale after 1 hour without a successful
//       * weather request. Indicate this by removing the degree symbol.
//       */
//      else if (has_timed_out || has_temperature ||
//           http_request_build_count > 0 || http_request_send_count > 0) {
//        if (fail_count >= 60) {
//          is_stale = true;
//          weather_layer_set_temperature(&weather_layer, last_valid_temperature, is_stale);
//        }
//      }
//    }
//    else {

//      /* Redundant check to make sure that HTTP time-outs are not triggering the
//       * 'phone error' icon. If appearing to be timed out after 1 hour without a
//       * successful weather request, indicate stale weather information by removing
//       * the degree symbol.
//       */
//      if (fail_count >= 60 && has_timed_out && !phone_disconnected) {
//        is_stale = true;
//        weather_layer_set_temperature(&weather_layer, last_valid_temperature, is_stale);
//      }

//       Display 'phone error' icon if:
//       * - watch appears to be disconnected from phone
//       * - watch appears to be disconnected from bridge app on phone
//       *
//       * In case this is triggered by a HTTP time-out, subsequently send a
//       * weather request for a possible quick re-connection.

//      else {
//        if (http_status==1008) {
//          phone_disconnected = true;
//          weather_layer_set_icon(&weather_layer, WEATHER_ICON_PHONE_ERROR);
//          link_monitor_handle_failure(http_status);

//          http_location_request();
//          request_weather();
//        }
//      }
//    }
//  }

//  // Remove temperature text 30 minutes after a phone/bridge app disconnection
//  if (fail_count >= 30 && phone_disconnected) {
//    text_layer_set_text(&weather_layer.temp_layer, " ");
//    has_temperature = false;
//  }

//  // Indicate failure and activate fail counter (see handle_tick() function)
//  has_failed = true;

//  // Re-request the location and subsequently weather on next minute tick
//  located = false;
// }

// void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context) {

//  is_stale = false;

//  if(cookie != WEATHER_HTTP_COOKIE) return;
//  Tuple* icon_tuple = dict_find(received, WEATHER_KEY_ICON);
//  if(icon_tuple) {
//    int icon = icon_tuple->value->int8;
//    if(icon >= 0 && icon < 18 && icon != 17) {
//      weather_layer_set_icon(&weather_layer, icon);
//    }
//  }
//  Tuple* temperature_tuple = dict_find(received, WEATHER_KEY_TEMPERATURE);
//  if(temperature_tuple) {
//    last_valid_temperature = temperature_tuple->value->int16;
//    weather_layer_set_temperature(&weather_layer, temperature_tuple->value->int16, is_stale);
//    has_temperature = true;
//  }

//  link_monitor_handle_success();
//  has_failed = false;
//  phone_disconnected = false;
// }

// void location(float latitude, float longitude, float altitude, float accuracy, void* context) {
//  // Fix the floats
//  our_latitude = latitude * 1000000;
//  our_longitude = longitude * 1000000;
//  located = true;
//  request_weather();
// }

// void reconnect(void* context) {
//  located = false;
//  request_weather();
// }

// void request_weather();

static void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
  // 'Animate' loading icon until the first successful weather request
  if (!has_temperature && !has_failed)
  {
    if (load_count == 4) {
      weather_layer_set_icon(weather_layer, WEATHER_ICON_LOADING1);
    }
    if (load_count == 5) {
      weather_layer_set_icon(weather_layer, WEATHER_ICON_LOADING2);
    }
    if (load_count == 6) {
      weather_layer_set_icon(weather_layer, WEATHER_ICON_LOADING3);
      load_count = 3;
    }
    load_count++;
  }

  // Subsequently update time and date once every minute
  if (units_changed & MINUTE_UNIT)
  {
    // Need to be static because pointers to them are stored in the text layers
      static char date_text[] = "XXX 00";
      static char hour_text[] = "00";
      static char minute_text[] = ":00";

    if (units_changed & DAY_UNIT)
      {
        strftime(date_text, sizeof(date_text), "%a %d", tick_time);

      // Triggered if day of month < 10
      if (date_text[4] == '0')
      {
          // Hack to get rid of the leading zero of the day of month
              memmove(&date_text[4], &date_text[5], sizeof(date_text) - 1);
      }

          text_layer_set_text(date_layer, date_text);
      }

      if (clock_is_24h_style())
      {
          strftime(hour_text, sizeof(hour_text), "%H", tick_time);
      if (hour_text[0] == '0')
          {
              // Hack to get rid of the leading zero of the hour
              memmove(&hour_text[0], &hour_text[1], sizeof(hour_text) - 1);
          }
      }
      else
      {
          strftime(hour_text, sizeof(hour_text), "%I", tick_time);
          if (hour_text[0] == '0')
          {
              // Hack to get rid of the leading zero of the hour
              memmove(&hour_text[0], &hour_text[1], sizeof(hour_text) - 1);
          }
      }

      strftime(minute_text, sizeof(minute_text), ":%M", tick_time);
      text_layer_set_text(time_layer, hour_text/*, minute_text*/);

    // Start a counter upon an error and increase by 1 each minute
    // Helps determine when to display error feedback and to stop trying to recover from errors
    if (has_failed)
    {
      fail_count++;
    }
    else
    {
      fail_count = 0;
    }

    // Request updated weather every 15 minutes
    // Stop requesting after 3 hours without any successful connection
    if(initial_request || !has_temperature || ((tick_time->tm_min % 15) == initial_minute && fail_count <= 180))
    {
//      http_location_request();
      initial_request = false;
    }
    else
    {
      // Ping the phone once every minute to validate the connection
      // Stop pinging after 3 hours without any successful connection
      if (fail_count <= 180)
      {
        // Upon an error, also make a weather request with each ping
        if (fail_count > 0)
        {
//          http_location_request();
        }

//        link_monitor_ping();
      }

      /****** The code snippet below is used for the "no vibration" version
      // Ping the phone every 5 minutes
      if (!(tick_time->tm_min % 5) && fail_count == 0)
      {
        link_monitor_ping();
      }
      // Upon an error, ping the phone every minute
      // Stop pinging after 3 hours without any successful connection
      else if (fail_count > 0 && fail_count <= 180)
      {
        http_location_request();
        link_monitor_ping();
      }
      ******/
    }
  }
}

static void init(void) {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  //window_set_fullscreen(window, true);
  /*window_set_window_handlers(window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload
  });*/

  const int inbound_size = 124;
  const int outbound_size = 256;
  app_message_open(inbound_size, outbound_size);

  font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18));
  font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_53));

  time_layer = text_layer_create(TIME_FRAME);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_font(time_layer, font_time);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));

  date_layer = text_layer_create(DATE_FRAME);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_font(date_layer, font_date);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));

  // Add weather layer
  weather_layer = weather_layer_create(GRect(0, 90, 144, 80));
  layer_add_child(window_get_root_layer(window), weather_layer);

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void deinit(void) {
  window_destroy(window);
  tick_timer_service_unsubscribe();

  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
  weather_layer_destroy(weather_layer);

  fonts_unload_custom_font(font_date);
  fonts_unload_custom_font(font_time);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

void request_weather() {

}
