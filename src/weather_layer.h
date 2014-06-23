#ifndef WEATHER_LAYER_H
#define WEATHER_LAYER_H

typedef enum {
  WEATHER_ICON_CLEAR_DAY = 0,
  WEATHER_ICON_CLEAR_NIGHT,
  WEATHER_ICON_RAIN,
  WEATHER_ICON_SNOW,
  WEATHER_ICON_SLEET,
  WEATHER_ICON_WIND,
  WEATHER_ICON_FOG,
  WEATHER_ICON_CLOUDY,
  WEATHER_ICON_PARTLY_CLOUDY_DAY,
  WEATHER_ICON_PARTLY_CLOUDY_NIGHT,
  WEATHER_ICON_THUNDER,
  WEATHER_ICON_RAIN_SNOW,
  WEATHER_ICON_RAIN_SLEET,
  WEATHER_ICON_SNOW_SLEET,
  WEATHER_ICON_COLD,
  WEATHER_ICON_HOT,
  WEATHER_ICON_DRIZZLE,
  WEATHER_ICON_NOT_AVAILABLE,
  WEATHER_ICON_PHONE_ERROR,
  WEATHER_ICON_CLOUD_ERROR,
  WEATHER_ICON_LOADING1,
  WEATHER_ICON_LOADING2,
  WEATHER_ICON_LOADING3,
  WEATHER_ICON_COUNT
} WeatherIcon;

typedef Layer WeatherLayer;

WeatherLayer *weather_layer_create(GRect frame);
void weather_layer_destroy(WeatherLayer* weather_layer);
void weather_layer_set_icon(WeatherLayer* weather_layer, WeatherIcon icon);
void weather_layer_set_temperature(WeatherLayer* weather_layer, int16_t temperature);
uint8_t weather_icon_for_condition(int condition, bool night_time);

#endif
