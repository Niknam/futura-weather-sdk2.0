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
  wld->temperature_layer = text_layer_create(GRect(0, 36-4, 142, 80));  // y+h=86, looks like this should be 68? anyway this is a right hand rectangle of the weather area
  text_layer_set_background_color(wld->temperature_layer, GColorClear);
  text_layer_set_text_alignment(wld->temperature_layer, GTextAlignmentCenter);
  text_layer_set_font(wld->temperature_layer, small_font);
  layer_add_child(weather_layer, text_layer_get_layer(wld->temperature_layer));

  wld->update_time_layer = text_layer_create(GRect(60, 54-4, 144-60, 80));  // y+h=86, looks like this should be 68? anyway this is a right hand rectangle of the weather area
  text_layer_set_background_color(wld->update_time_layer, GColorClear);
  text_layer_set_text_alignment(wld->update_time_layer, GTextAlignmentRight);
  text_layer_set_font(wld->update_time_layer, small_font);
  layer_add_child(weather_layer, text_layer_get_layer(wld->update_time_layer));

  // Add bitmap layer
  //wld->icon_layer = bitmap_layer_create(GRect(9, 13, 60, 60));
	for(int i = 0; i < NUM_WEATHER_CONDITIONS+1; i++)
	{
		if(i == 0)
		{
			wld->icon_layer[0] = bitmap_layer_create(GRect(9, 3, 30, 30));
		}
		else
		{
			wld->icon_layer[i] = bitmap_layer_create(GRect(((i-1)*frame.size.w)/(NUM_WEATHER_CONDITIONS), 90, 30, 30));
		}
		
		layer_add_child(weather_layer, bitmap_layer_get_layer(wld->icon_layer[i]));
		wld->icon[i] = NULL;
	}


  return weather_layer;
}

void weather_layer_set_icon(WeatherLayer* weather_layer, WeatherIcon icon) {
  WeatherLayerData *wld = layer_get_data(weather_layer);

  GBitmap *new_icon =  gbitmap_create_with_resource(WEATHER_ICONS[icon]);
  // Display the new bitmap
  bitmap_layer_set_bitmap(wld->icon_layer[0], new_icon);

  // Destroy the ex-current icon if it existed
  if (wld->icon[0] != NULL) {
    // A cast is needed here to get rid of the const-ness
    gbitmap_destroy(wld->icon[0]);
  }
  wld->icon[0] = new_icon;
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
		
	if(wld->last_weather_data.rrate != w->rrate)
	{
		changed = 1;
		wld->trend_weather_data.rrate = wld->last_weather_data.rrate;
	}
	const char* s_trend_rrate = (w->rrate > wld->trend_weather_data.rrate) ? "+" : ".";

	wld->last_weather_data = *w;
  
	// values are multiplied by 10 in order to give more trending data, add 5 to round up
	int16_t t = (w->temperature+5)/10;
	int16_t in = (w->intemp+5)/10;
	int16_t out = (w->outtemp+5)/10;
	int16_t rrate = w->rrate; // rainfall * 10 so the min is 3 for our measurement
  
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
	
	char* stale_text = (is_stale)? ":(" : "";

	char* s_trend_charging = (is_charging) ? "+" : " ";
	
	snprintf(wld->output_str, sizeof(wld->output_str), "%i%s%i%s%i%s\n%i%s%i\n%s %s", 
		in, s_trend_intemp, out, s_trend_outtemp, t, s_trend_temperature,
		percent, s_trend_charging, rrate,//&time_text[time_index], 
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

	for(int i = 1; i < NUM_WEATHER_CONDITIONS+1; i++)
	{
		WeatherIcon icon = yahoo_weather_icon_for_condition(w->conditions[i-1]);
		GBitmap *new_icon =  gbitmap_create_with_resource(WEATHER_ICONS[icon]);
		
		// Display the new bitmap
		bitmap_layer_set_bitmap(wld->icon_layer[i], new_icon);

		// Destroy the ex-current icon if it existed
		if (wld->icon[i] != NULL) 
		{
			gbitmap_destroy(wld->icon[i]);
		}
		wld->icon[i] = new_icon;
	}

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

  // Destroy the previous bitmap if we have one
	for(int i = 0; i < NUM_WEATHER_CONDITIONS+1; i++)
	{
		bitmap_layer_destroy(wld->icon_layer[i]);
		if (wld->icon[i] != NULL) 
		{
			gbitmap_destroy(wld->icon[i]);
		}
	}
  layer_destroy(weather_layer);

  fonts_unload_custom_font(large_font);
  fonts_unload_custom_font(small_font);
}


/*
 * Converts a yahoo weather code to one of our icons.
 * Refer to: https://developer.yahoo.com/weather/
 * images http://l.yimg.com/a/i/us/we/52/34.gif
 */
 
uint8_t yahoo_weather_icon_for_condition(int c) 
{
	switch(c)
	{
		// 19 means dust, 21 haze, 22 smoky
		
		case 32: case 34:
			return WEATHER_ICON_CLEAR_DAY;
		case 31: case 33:
			return WEATHER_ICON_CLEAR_NIGHT;
		case 10: case 11: case 12: case 35: case 40:
			return WEATHER_ICON_RAIN;
		case 13: case 14: case 15: case 16: case 17: case 41: case 46:
			return WEATHER_ICON_SNOW;
		case 18:
			return WEATHER_ICON_SLEET;
		case 23: case 24:
			return WEATHER_ICON_WIND;
		case 20:
			return WEATHER_ICON_FOG;
		case 26: case 44:
			return WEATHER_ICON_CLOUDY;
		case 28: case 30:
			return WEATHER_ICON_PARTLY_CLOUDY_DAY;
		case 27: case 29:
			return WEATHER_ICON_PARTLY_CLOUDY_NIGHT;
		case 1: case 2: case 3:	case 4: case 37: case 38: case 39: case 45: case 47:
			return WEATHER_ICON_THUNDER;
		case 5: case 42: case 43:
			return WEATHER_ICON_RAIN_SNOW;
		case 6:
			return WEATHER_ICON_RAIN_SLEET;
		case 7:
			return WEATHER_ICON_SNOW_SLEET;
		case 25:
			return WEATHER_ICON_COLD;
		case 36:
			return WEATHER_ICON_HOT;
		case 8: case 9:
			return WEATHER_ICON_DRIZZLE;
		
	}

	return WEATHER_ICON_NOT_AVAILABLE;
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
