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

WeatherLayer *weather_layer_create(GRect frame)
{
  // Create a new layer with some extra space to save our custom Layer infos
  WeatherLayer *weather_layer = layer_create_with_data(frame, sizeof(WeatherLayerData));
  WeatherLayerData *wld = layer_get_data(weather_layer);

  // Add background layer
  wld->temp_layer_background = text_layer_create(GRect(0, 10, 144, 68));
  text_layer_set_background_color(wld->temp_layer_background, GColorBlack);
  layer_add_child(weather_layer, text_layer_get_layer(wld->temp_layer_background));

  // Add temperature layer
  wld->temp_layer = text_layer_create(GRect(70, 19, 72, 80));
  text_layer_set_background_color(wld->temp_layer, GColorClear);
  text_layer_set_text_alignment(wld->temp_layer, GTextAlignmentCenter);
  text_layer_set_font(wld->temp_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_40)));
  layer_add_child(weather_layer, text_layer_get_layer(wld->temp_layer));

  // Add bitmap layer
  wld->icon_layer = bitmap_layer_create(GRect(9, 13, 60, 60));
  layer_add_child(weather_layer, bitmap_layer_get_layer(wld->icon_layer));

  return weather_layer;
}

void weather_layer_set_icon(WeatherLayer* weather_layer, WeatherIcon icon) {
  WeatherLayerData *wld = layer_get_data(weather_layer);

  // Save the pointer to the current bitmap
  const GBitmap *bitmap = bitmap_layer_get_bitmap(wld->icon_layer);

  // Display the new bitmap
  bitmap_layer_set_bitmap(wld->icon_layer, gbitmap_create_with_resource(WEATHER_ICONS[icon]));

  // Destroy the ex-current bitmap if it existed
  if (bitmap != NULL) {
    // A cast is needed here to get rid of the const-ness
    gbitmap_destroy((GBitmap*)bitmap);
  }
}

void weather_layer_set_temperature(WeatherLayer* weather_layer, int16_t t, bool is_stale) {
  WeatherLayerData *wld = layer_get_data(weather_layer);

  snprintf(wld->temp_str, sizeof(wld->temp_str), "%i %s", t, is_stale ? " " : "°");
  text_layer_set_text_alignment(wld->temp_layer, GTextAlignmentCenter);
  text_layer_set_text(wld->temp_layer, wld->temp_str);

  // Temperature between -9° -> 9° or 20° -> 99°
  if ((t >= -9 && t <= 9) || t >= 20) {
    text_layer_set_font(wld->temp_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_40)));
    text_layer_set_text_alignment(wld->temp_layer, GTextAlignmentCenter);
  }
  // Temperature between 10° -> 19°
  else if (t >= 10) {
    text_layer_set_font(wld->temp_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_40)));
    text_layer_set_text_alignment(wld->temp_layer, GTextAlignmentLeft);
  }
  // Temperature above 99° or below -9°
  else {
    text_layer_set_font(wld->temp_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_35)));
    text_layer_set_text_alignment(wld->temp_layer, GTextAlignmentCenter);
  }
}

void weather_layer_destroy(WeatherLayer* weather_layer) {
  WeatherLayerData *wld = layer_get_data(weather_layer);

  text_layer_destroy(wld->temp_layer);
  text_layer_destroy(wld->temp_layer_background);

  const GBitmap *bitmap = bitmap_layer_get_bitmap(wld->icon_layer);
  bitmap_layer_destroy(wld->icon_layer);

  // Destroy the previous bitmap if we have one
  if (bitmap != NULL) {
    gbitmap_destroy((GBitmap*)bitmap);
  }
  layer_destroy(weather_layer);
}
