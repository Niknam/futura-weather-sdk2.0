#include <pebble.h>
#include "weather_layer.h"

static uint8_t WEATHER_ICONS[] = {
  RESOURCE_ID_ICON_CLEAR_DAY,
  RESOURCE_ID_ICON_CLEAR_NIGHT,
  RESOURCE_ID_ICON_RAIN,
  RESOURCE_ID_ICON_SNOW,
  RESOURCE_ID_ICON_SLEET,
  RESOURCE_ID_ICON_WIND,
  RESOURCE_ID_ICON_FOG,
  RESOURCE_ID_ICON_CLOUDY,
  RESOURCE_ID_ICON_PARTLY_CLOUDY_DAY,
  RESOURCE_ID_ICON_PARTLY_CLOUDY_NIGHT,
  RESOURCE_ID_ICON_THUNDER,
  RESOURCE_ID_ICON_RAIN_SNOW,
  RESOURCE_ID_ICON_RAIN_SLEET,
  RESOURCE_ID_ICON_SNOW_SLEET,
  RESOURCE_ID_ICON_COLD,
  RESOURCE_ID_ICON_HOT,
  RESOURCE_ID_ICON_DRIZZLE,
  RESOURCE_ID_ICON_NOT_AVAILABLE,
  RESOURCE_ID_ICON_PHONE_ERROR,
  RESOURCE_ID_ICON_CLOUD_ERROR,
  RESOURCE_ID_ICON_LOADING1,
  RESOURCE_ID_ICON_LOADING2,
  RESOURCE_ID_ICON_LOADING3,
};

// Keep pointers to the two fonts we use.
static GFont large_font, small_font;

WeatherLayer *weather_layer_create(GRect frame)
{
  // Create a new layer with some extra space to save our custom Layer infos
  WeatherLayer *weather_layer = layer_create_with_data(frame, sizeof(WeatherLayerData));
  WeatherLayerData *wld = layer_get_data(weather_layer);

  large_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_40));
  small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_35));

  // Add background layer
  wld->temp_layer_background = text_layer_create(GRect(0, 10, 144, 68));
  text_layer_set_background_color(wld->temp_layer_background, GColorWhite);
  layer_add_child(weather_layer, text_layer_get_layer(wld->temp_layer_background));

  // Add temperature layer
  wld->temp_layer = text_layer_create(GRect(70, 19, 72, 80));
  text_layer_set_background_color(wld->temp_layer, GColorClear);
  text_layer_set_text_alignment(wld->temp_layer, GTextAlignmentCenter);
  text_layer_set_font(wld->temp_layer, large_font);
  layer_add_child(weather_layer, text_layer_get_layer(wld->temp_layer));

  // Add bitmap layer
  wld->icon_layer = bitmap_layer_create(GRect(9, 13, 60, 60));
  layer_add_child(weather_layer, bitmap_layer_get_layer(wld->icon_layer));

  wld->icon = NULL;

  return weather_layer;
}

void weather_layer_set_icon(WeatherLayer* weather_layer, WeatherIcon icon) {
  WeatherLayerData *wld = layer_get_data(weather_layer);

  GBitmap *new_icon =  gbitmap_create_with_resource(WEATHER_ICONS[icon]);
  // Display the new bitmap
  bitmap_layer_set_bitmap(wld->icon_layer, new_icon);

  // Destroy the ex-current icon if it existed
  if (wld->icon != NULL) {
    // A cast is needed here to get rid of the const-ness
    gbitmap_destroy(wld->icon);
  }
  wld->icon = new_icon;
}

void weather_layer_set_temperature(WeatherLayer* weather_layer, int16_t t, bool is_stale) {
  WeatherLayerData *wld = layer_get_data(weather_layer);

  snprintf(wld->temp_str, sizeof(wld->temp_str), "%i%s", t, is_stale ? " " : "°");

  // Temperature between -9° -> 9° or 20° -> 99°
  if ((t >= -9 && t <= 9) || (t >= 20 && t < 100)) {
    text_layer_set_font(wld->temp_layer, large_font);
    text_layer_set_text_alignment(wld->temp_layer, GTextAlignmentCenter);

	// Is the temperature below zero?
	if (wld->temp_str[0] == '-') {
	  memmove(
          wld->temp_str + 1 + 1,
          wld->temp_str + 1,
          5 - (1 + 1)
      );
	  memcpy(&wld->temp_str[1], " ", 1);
	}
  }
  // Temperature between 10° -> 19°
  else if (t >= 10 && t < 20) {
    text_layer_set_font(wld->temp_layer, large_font);
    text_layer_set_text_alignment(wld->temp_layer, GTextAlignmentLeft);
  }
  // Temperature above 99° or below -9°
  else {
    text_layer_set_font(wld->temp_layer, small_font);
    text_layer_set_text_alignment(wld->temp_layer, GTextAlignmentCenter);
  }
  text_layer_set_text(wld->temp_layer, wld->temp_str);
}

void weather_layer_destroy(WeatherLayer* weather_layer) {
  WeatherLayerData *wld = layer_get_data(weather_layer);

  text_layer_destroy(wld->temp_layer);
  text_layer_destroy(wld->temp_layer_background);
  bitmap_layer_destroy(wld->icon_layer);

  // Destroy the previous bitmap if we have one
  if (wld->icon != NULL) {
    gbitmap_destroy(wld->icon);
  }
  layer_destroy(weather_layer);

  fonts_unload_custom_font(large_font);
  fonts_unload_custom_font(small_font);
}

/*
 * Converts an API Weather Condition into one of our icons.
 * Refer to: http://bugs.openweathermap.org/projects/api/wiki/Weather_Condition_Codes
 */
uint8_t weather_icon_for_condition(int c, bool night_time) {
  // Thunderstorm
  if (c < 300) {
    return WEATHER_ICON_THUNDER;
  }
  // Drizzle
  else if (c < 500) {
    return WEATHER_ICON_DRIZZLE;
  }
  // Rain / Freezing rain / Shower rain
  else if (c < 600) {
    return WEATHER_ICON_RAIN;
  }
  // Snow
  else if (c < 700) {
    return WEATHER_ICON_SNOW;
  }
  // Fog / Mist / Haze / etc.
  else if (c < 771) {
    return WEATHER_ICON_FOG;
  }
  // Tornado / Squalls
  else if (c < 800) {
    return WEATHER_ICON_WIND;
  }
  // Sky is clear
  else if (c == 800) {
    if (night_time)
      return WEATHER_ICON_CLEAR_NIGHT;
    else
      return WEATHER_ICON_CLEAR_DAY;
  }
  // few/scattered/broken clouds
  else if (c < 804) {
    if (night_time)
      return WEATHER_ICON_PARTLY_CLOUDY_NIGHT;
    else
      return WEATHER_ICON_PARTLY_CLOUDY_DAY;
  }
  // overcast clouds
  else if (c == 804) {
    return WEATHER_ICON_CLOUDY;
  }
  // Extreme
  else if ((c >= 900 && c < 903) || (c > 904 && c < 1000)) {
    return WEATHER_ICON_WIND;
  }
  // Cold
  else if (c == 903) {
      return WEATHER_ICON_COLD;
  }
  // Hot
  else if (c == 904) {
      return WEATHER_ICON_HOT;
  }
  else {
    // Weather condition not available
    return WEATHER_ICON_NOT_AVAILABLE;
  }
}
