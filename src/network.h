
#ifndef NETWORK_H
#define NETWORK_H
#include <pebble.h>
#include "weather_data.h"

#define KEY_TEMPERATURE 0
#define KEY_CONDITION 1
#define KEY_SUNRISE 2
#define KEY_SUNSET 3
#define KEY_CURRENT_TIME 4
#define KEY_ERROR 5
#define KEY_INTEMP 6
#define KEY_OUTTEMP 7
#define KEY_PLACE 8
#define KEY_CCNOW 9
#define KEY_CC0 10
#define KEY_CC1 11
#define KEY_CC2 12
#define KEY_CC3 13
#define KEY_CC4 14
#define KEY_REQUEST_UPDATE 42

void init_network(WeatherData *weather_data);
void close_network();

void request_weather();
#endif
