#include <pebble.h>
#include "network.h"

/*
http://gary.ath.cx/gary.php 
looks like this:
<?php
//error_reporting(E_ALL);
//ini_set("display_errors", 1);

include "cumuluswebtags.php";

$lat = $_REQUEST['lat'];
$lon = $_REQUEST['lon'];

// limit to 2 decimal places as this gives better results from google, 6 dec places is the resolution of yahoo api's using an app id
$lat = round($lat, 6);
$lon = round($lon, 6);

$jsonurl = "http://api.openweathermap.org/data/2.5/weather?lat=$lat&lon=$lon&cnt=1";
$json = file_get_contents($jsonurl);
$vars = json_decode($json, true);

$jsonurl = "http://maps.googleapis.com/maps/api/geocode/json?latlng=$lat,$lon&sensor=false";
$json = file_get_contents($jsonurl);
$vars_map = json_decode($json, true);
$place = $vars_map['results'][0]['address_components'][2]['short_name'];

$yql_query = 'select * from geo.placefinder where text="'.$lat.','.$lon.'" and gflags="R"';
$yql_query_url = 'http://query.yahooapis.com/v1/public/yql' . "?q=" . urlencode($yql_query) . "&format=json";
$json = file_get_contents($yql_query_url);
$vars_map = json_decode($json, true);
$woeid = $vars_map['query']['results']['Result']['woeid'];

$yql_query = "select * from weather.forecast where woeid = $woeid and u = 'c' | truncate(count=5)";
$yql_query_url = 'http://query.yahooapis.com/v1/public/yql' . "?q=" . urlencode($yql_query) . "&format=json";
$json = file_get_contents($yql_query_url);
$vars_map = json_decode($json, true);
$place = $vars_map['query']['results']['channel']['location']['city'];
$ytemp = $vars_map['query']['results']['channel']['item']['condition']['temp'];
$ccnow = $vars_map['query']['results']['channel']['item']['condition']['code'];
$cc0 = $vars_map['query']['results']['channel']['item']['forecast'][0]['code'];
$cc1 = $vars_map['query']['results']['channel']['item']['forecast'][1]['code'];
$cc2 = $vars_map['query']['results']['channel']['item']['forecast'][2]['code'];
$cc3 = $vars_map['query']['results']['channel']['item']['forecast'][3]['code'];
$cc4 = $vars_map['query']['results']['channel']['item']['forecast'][4]['code'];

// add on garys home data, and the yahoo weather data
$gary = array(
	'intemp' => $RCintemp, 
	'temp' => $RCtemp, 
	'ytemp' => (int)$ytemp, 
	'rrate' => $RCrrate,
	'place' => $place, 
	'ccnow' => (int)$ccnow, 
	'cc0' => (int)$cc0,
	'cc1' => (int)$cc1,
	'cc2' => (int)$cc2,
	'cc3' => (int)$cc3,
	'cc4' => (int)$cc4,
	);
	
$vars['gary'] = $gary;

header("Cache-Control: no-cache, must-revalidate"); // HTTP/1.1
header("Expires: Sat, 26 Jul 1997 05:00:00 GMT"); // Date in the past
header('Content-type: application/json');
echo json_encode($vars);
?>
*/

static void appmsg_in_received(DictionaryIterator *received, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In received.");

  WeatherData *weather = (WeatherData*) context;

  Tuple *temperature_tuple = dict_find(received, KEY_TEMPERATURE);
  Tuple *intemp_tuple = dict_find(received, KEY_INTEMP);
  Tuple *outtemp_tuple = dict_find(received, KEY_OUTTEMP);
  Tuple *place_tuple = dict_find(received, KEY_PLACE);
  
  Tuple *condition_tuple = dict_find(received, KEY_CONDITION);
  Tuple *sunrise_tuple = dict_find(received, KEY_SUNRISE);
  Tuple *sunset_tuple = dict_find(received, KEY_SUNSET);
  Tuple *current_time_tuple = dict_find(received, KEY_CURRENT_TIME);
  Tuple *ccnow_tuple = dict_find(received, KEY_CCNOW);
  Tuple *cc0_tuple = dict_find(received, KEY_CC0);
  Tuple *cc1_tuple = dict_find(received, KEY_CC1);
  Tuple *cc2_tuple = dict_find(received, KEY_CC2);
  Tuple *cc3_tuple = dict_find(received, KEY_CC3);
  Tuple *cc4_tuple = dict_find(received, KEY_CC4);
  Tuple *rrate_tuple = dict_find(received, KEY_RRATE);

  Tuple *error_tuple = dict_find(received, KEY_ERROR);

  if (temperature_tuple && ccnow_tuple) {
    weather->temperature = temperature_tuple->value->int32;
    weather->intemp = intemp_tuple->value->int32;
    weather->outtemp = outtemp_tuple->value->int32;
    weather->condition = ccnow_tuple->value->int32;
    weather->sunrise = sunrise_tuple->value->int32;
    weather->sunset = sunset_tuple->value->int32;
    weather->current_time = current_time_tuple->value->int32;
    weather->conditions[0] = cc0_tuple->value->int32;
    weather->conditions[1] = cc1_tuple->value->int32;
    weather->conditions[2] = cc2_tuple->value->int32;
    weather->conditions[3] = cc3_tuple->value->int32;
    weather->conditions[4] = cc4_tuple->value->int32;
    weather->rrate = rrate_tuple->value->int32;
	strncpy(&weather->place[0], place_tuple->value->cstring, sizeof(weather->place)/sizeof(weather->place[0])-1);
    weather->error = WEATHER_E_OK;
    weather->updated = time(NULL);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got temperature %i and condition %i cc0 %i place %s condition=%p error=%p", 
		weather->temperature, weather->condition, weather->conditions[0], &weather->place[0],
		condition_tuple, error_tuple
		);
  }
  else if (error_tuple) {
    weather->error = WEATHER_E_NETWORK;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got error %s", error_tuple->value->cstring);
  }
  else {
    weather->error = WEATHER_E_PHONE;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got message with unknown keys... temperature=%p condition=%p error=%p",
      temperature_tuple, ccnow_tuple, error_tuple);
  }
}

static void appmsg_in_dropped(AppMessageResult reason, void *context) 
{
	switch(reason)
	{
		case APP_MSG_BUFFER_OVERFLOW:
		{
			APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: overflow");
			break;
		}
		
		case APP_MSG_BUSY:
		{
			APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: busy");
			break;
		}
		
		default:
		{
			APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: %i", reason);
			// Request a new update...
			request_weather();
		}
	}
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
  app_message_open(256, 256);

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
