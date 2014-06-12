#include <pebble.h>
#include "network.h"

static WeatherUpdateHandler s_weather_update_handler = NULL;
static WeatherErrorHandler s_weather_error_handler = NULL;

static void appmsg_in_received(DictionaryIterator *received, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In received.");

  WeatherData weather;

  Tuple *temperature_tuple = dict_find(received, KEY_TEMPERATURE);
  Tuple *condition_tuple = dict_find(received, KEY_CONDITION);
  Tuple *sunrise_tuple = dict_find(received, KEY_SUNRISE);
  Tuple *sunset_tuple = dict_find(received, KEY_SUNSET);
  Tuple *current_time_tuple = dict_find(received, KEY_CURRENT_TIME);
  Tuple *error_tuple = dict_find(received, KEY_ERROR);

  if (temperature_tuple && condition_tuple) {
    weather.temperature = temperature_tuple->value->int32;
    weather.condition = condition_tuple->value->int32;
    weather.sunrise = sunrise_tuple->value->int32;
    weather.sunset = sunset_tuple->value->int32;
    weather.current_time = current_time_tuple->value->int32;
    weather.updated = time(NULL);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got temperature %i and condition %i", weather.temperature, weather.condition);

    if(s_weather_update_handler) {
      s_weather_update_handler(&weather);
    }
  }
  else if (error_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got error %s", error_tuple->value->cstring);
    if(s_weather_error_handler) {
      s_weather_error_handler(WEATHER_E_NETWORK);
    }
  }
  else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got message with unknown keys... temperature=%p condition=%p error=%p",
      temperature_tuple, condition_tuple, error_tuple);
    if(s_weather_error_handler) {
      s_weather_error_handler(WEATHER_E_PHONE);
    }
  }
}

static void appmsg_in_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: %i", reason);
  // Request a new update...
  request_weather();
}

static void appmsg_out_sent(DictionaryIterator *sent, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Out sent.");
}

static void appmsg_out_failed(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Out failed: %i", reason);

  WeatherError error;

  switch (reason) {
    case APP_MSG_NOT_CONNECTED:
      error = WEATHER_E_DISCONNECTED;
      request_weather();
      break;
    case APP_MSG_SEND_REJECTED:
    case APP_MSG_SEND_TIMEOUT:
      error = WEATHER_E_PHONE;
      request_weather();
      break;
    default:
      error = WEATHER_E_PHONE;
      request_weather();
      break;
  }

  if(s_weather_error_handler) {
    s_weather_error_handler(error);
  }
}

void init_network(void)
{
  app_message_register_inbox_received(appmsg_in_received);
  app_message_register_inbox_dropped(appmsg_in_dropped);
  app_message_register_outbox_sent(appmsg_out_sent);
  app_message_register_outbox_failed(appmsg_out_failed);
  app_message_open(124, 256);
}

void close_network(void)
{
  app_message_deregister_callbacks();
}

void request_weather(void)
{
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  dict_write_uint8(iter, KEY_REQUEST_UPDATE, 42);

  app_message_outbox_send();
}

void set_weather_update_handler(WeatherUpdateHandler handler) {
  s_weather_update_handler = handler;
}

void set_weather_error_handler(WeatherErrorHandler handler) {
  s_weather_error_handler = handler;
}
