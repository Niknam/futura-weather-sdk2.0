#include <pebble.h>
#include "network.h"

static void appmsg_in_received(DictionaryIterator *received, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In received.");

  WeatherData *weather = (WeatherData*) context;

  Tuple *temperature_tuple = dict_find(received, KEY_TEMPERATURE);
  Tuple *condition_tuple = dict_find(received, KEY_CONDITION);
  Tuple *sunrise_tuple = dict_find(received, KEY_SUNRISE);
  Tuple *sunset_tuple = dict_find(received, KEY_SUNSET);
  Tuple *current_time_tuple = dict_find(received, KEY_CURRENT_TIME);
  Tuple *error_tuple = dict_find(received, KEY_ERROR);

  if (temperature_tuple && condition_tuple) {
    weather->temperature = temperature_tuple->value->int32;
    weather->condition = condition_tuple->value->int32;
    weather->sunrise = sunrise_tuple->value->int32;
    weather->sunset = sunset_tuple->value->int32;
    weather->current_time = current_time_tuple->value->int32;
    weather->error = WEATHER_E_OK;
    weather->updated = time(NULL);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got temperature %i and condition %i", weather->temperature, weather->condition);
  }
  else if (error_tuple) {
    weather->error = WEATHER_E_NETWORK;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got error %s", error_tuple->value->cstring);
  }
  else {
    weather->error = WEATHER_E_PHONE;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got message with unknown keys... temperature=%p condition=%p error=%p",
      temperature_tuple, condition_tuple, error_tuple);
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
  WeatherData *weather = (WeatherData*) context;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Out failed: %i", reason);

  switch (reason) {
    case APP_MSG_NOT_CONNECTED:
      weather->error = WEATHER_E_DISCONNECTED;
      request_weather();
      break;
    case APP_MSG_SEND_REJECTED:
    case APP_MSG_SEND_TIMEOUT:
      weather->error = WEATHER_E_PHONE;
      request_weather();
      break;
    default:
      weather->error = WEATHER_E_PHONE;
      request_weather();
      break;
  }
}

void init_network(WeatherData *weather_data)
{
  app_message_register_inbox_received(appmsg_in_received);
  app_message_register_inbox_dropped(appmsg_in_dropped);
  app_message_register_outbox_sent(appmsg_out_sent);
  app_message_register_outbox_failed(appmsg_out_failed);
  app_message_set_context(weather_data);
  app_message_open(124, 256);

  weather_data->error = WEATHER_E_OK;
  weather_data->updated = 0;

}

void close_network()
{
  app_message_deregister_callbacks();
}

void request_weather()
{
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  dict_write_uint8(iter, KEY_REQUEST_UPDATE, 42);

  app_message_outbox_send();
}
