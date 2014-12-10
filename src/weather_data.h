#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

// number of days of weather conditions we are dealing with
#define NUM_WEATHER_CONDITIONS 5

typedef enum {
  WEATHER_E_OK = 0,
  WEATHER_E_DISCONNECTED,
  WEATHER_E_PHONE,
  WEATHER_E_NETWORK
} WeatherError;

typedef struct {
  int temperature;
  int intemp;
  int outtemp;
  int rrate;
  int condition;
  int conditions[NUM_WEATHER_CONDITIONS];
  int sunrise;
  int sunset;
  int current_time;
  char place[12];
  BatteryChargeState battery;
  //int night_time;
  int b_still_mode;
  time_t updated;
  WeatherError error;
} WeatherData;

#endif
