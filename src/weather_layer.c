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

WeatherLayer *weather_layer_create(GRect frame)	// 0, 60, 144, 108
{
  // Create a new layer with some extra space to save our custom Layer infos
  WeatherLayer *weather_layer = layer_create_with_data(frame, sizeof(WeatherLayerData));
  WeatherLayerData *wld = layer_get_data(weather_layer);
  
  // initialise
  memset(wld, 0, sizeof(wld));
  
  large_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_40));
  //small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_35));
  small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18));


  // Add background layer - used purely to provide a solid background color - note this leaves a 10 line gap from the frame rect and this rect
  wld->temperature_layer_background = text_layer_create(GRect(0, 10, 144, 68)); // 0, 10, 144, 68
  text_layer_set_background_color(wld->temperature_layer_background, GColorBlack);
  //layer_add_child(weather_layer, text_layer_get_layer(wld->temperature_layer_background));

  // Add temperature layer
  //wld->temperature_layer = text_layer_create(GRect(70, 6, 72, 80));
  wld->temperature_layer = text_layer_create(GRect(0, 36, 142, 80));  // y+h=86, looks like this should be 68? anyway this is a right hand rectangle of the weather area
  text_layer_set_background_color(wld->temperature_layer, GColorClear);
  text_layer_set_text_alignment(wld->temperature_layer, GTextAlignmentCenter);
  text_layer_set_font(wld->temperature_layer, small_font);
  layer_add_child(weather_layer, text_layer_get_layer(wld->temperature_layer));

  wld->update_time_layer = text_layer_create(GRect(60, 54, 144-60, 80));  // y+h=86, looks like this should be 68? anyway this is a right hand rectangle of the weather area
  text_layer_set_background_color(wld->update_time_layer, GColorClear);
  text_layer_set_text_alignment(wld->update_time_layer, GTextAlignmentRight);
  text_layer_set_font(wld->update_time_layer, small_font);
  layer_add_child(weather_layer, text_layer_get_layer(wld->update_time_layer));

  // Add bitmap layer
  //wld->icon_layer = bitmap_layer_create(GRect(9, 13, 60, 60));
  wld->icon_layer = bitmap_layer_create(GRect(9, 3, 30, 30));
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

void weather_layer_set_temperature(WeatherLayer* weather_layer, WeatherData* w, bool is_stale) 
{
	WeatherLayerData *wld = layer_get_data(weather_layer);

	int changed = 0;
	
	if(wld->last_weather_data.temperature != w->temperature)
	{
		changed = 1;
		wld->trend_weather_data.temperature = wld->last_weather_data.temperature;
	}
	const char* s_trend_temperature = (w->temperature > wld->trend_weather_data.temperature) ? "+" : ".";

	if(wld->last_weather_data.intemp != w->intemp)
	{
		changed = 1;
		wld->trend_weather_data.intemp = wld->last_weather_data.intemp;
	}
	const char* s_trend_intemp = (w->intemp > wld->trend_weather_data.intemp) ? "+" : ".";

	if(wld->last_weather_data.outtemp != w->outtemp)
	{
		changed = 1;
		wld->trend_weather_data.outtemp = wld->last_weather_data.outtemp;
	}
	const char* s_trend_outtemp = (w->outtemp > wld->trend_weather_data.outtemp) ? "+" : ".";
		
	wld->last_weather_data = *w;
  
	// values are multiplied by 10 in order to give more trending data, add 5 to round up
	int16_t t = (w->temperature+5)/10;
	int16_t in = (w->intemp+5)/10;
	int16_t out = (w->outtemp+5)/10;
  
	uint8_t percent = w->battery.charge_percent;
	bool is_charging = w->battery.is_charging;

    struct tm *updated_time = localtime(&w->updated);

	int time_index = 0;
	char time_text[10];
    strftime(   time_text, 
                sizeof(time_text), 
                clock_is_24h_style() ? "%R" : "%I:%M", 
                updated_time);
  
    // Drop the first char of time_text if needed
    if (!clock_is_24h_style() && (time_text[0] == '0')) 
	{
		time_index++;
    }
	
	char* stale_text = (is_stale)? "old" : "";

	char* s_trend_charging = (is_charging) ? "+" : " ";
	
	snprintf(wld->output_str, sizeof(wld->output_str), "%i%s%i%s%i%s\n%i%s\n%s %s", 
		in, s_trend_intemp, out, s_trend_outtemp, t, s_trend_temperature,
		percent, s_trend_charging, //&time_text[time_index], 
		w->place, stale_text);

      //APP_LOG(APP_LOG_LEVEL_DEBUG, "weather layer place %s", &place[0]);

  
	text_layer_set_text_color(wld->temperature_layer, GColorWhite);
	text_layer_set_font(wld->temperature_layer, small_font);
	text_layer_set_text_alignment(wld->temperature_layer, GTextAlignmentLeft);
	text_layer_set_text(wld->temperature_layer, wld->output_str);

	static char s_update_time[10];
	snprintf(s_update_time, sizeof(s_update_time), "%s", time_text);
	text_layer_set_text_color(wld->update_time_layer, GColorWhite);
	text_layer_set_font(wld->update_time_layer, small_font);
	text_layer_set_text_alignment(wld->update_time_layer, GTextAlignmentRight);
	text_layer_set_text(wld->update_time_layer, s_update_time);

	// if there was a change alert the user. If we're in still mode nobody is looking so save the battery instead, unless we are charging.
	if(changed && ((!w->b_still_mode) || is_charging))
	{
		// Vibe pattern: ms on/off/on:
		static const uint32_t const segments[] = { 50, 100, 40 };
		VibePattern pat = {
			.durations = segments,
			.num_segments = ARRAY_LENGTH(segments),
		};
		vibes_enqueue_custom_pattern(pat);  
		
		light_enable_interaction();
	}
}

void weather_layer_destroy(WeatherLayer* weather_layer) {
  WeatherLayerData *wld = layer_get_data(weather_layer);

  text_layer_destroy(wld->temperature_layer);
  text_layer_destroy(wld->temperature_layer_background);
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
