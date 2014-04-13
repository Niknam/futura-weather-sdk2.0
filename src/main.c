#include <pebble.h>

#include "weather_layer.h"
#include "network.h"
#include "config.h"

/* Keep a pointer to the current weather data as a global variable */
static WeatherData *weather_data;

/* Global variables to keep track of the UI elements */
static Window *window;
static TextLayer *date_layer;
static TextLayer *time_layer;
static WeatherLayer *weather_layer;

static char date_text[] = "XXX 00";
static char time_text[] = "00:00";

/* Preload the fonts */
GFont font_date;
GFont font_time;

static int i_dirty = 0;

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

void set_dirty()
{
	if(i_dirty < 0)
	{
		// update every second
		tick_timer_service_unsubscribe();
		tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
	}
	
	i_dirty = 10;
}

void done_dirty()
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "done dirty %i", i_dirty);
	if(i_dirty > 0)
	{
		i_dirty--;
	}
	else if(!i_dirty)
	{
		i_dirty = -1;
		tick_timer_service_unsubscribe();
		tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
	}
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
  int b_request_weather = 0;
  
  if (units_changed & MINUTE_UNIT) 
  {
	b_request_weather = ((tick_time->tm_min % 15) == 0);
  
    // Update the time - Fix to deal with 12 / 24 centering bug
    time_t currentTime = time(0);
    struct tm *currentLocalTime = localtime(&currentTime);

    // Manually format the time as 12 / 24 hour, as specified
    strftime(   time_text, 
                sizeof(time_text), 
                clock_is_24h_style() ? "%R" : "%I:%M", 
                currentLocalTime);

    // Drop the first char of time_text if needed
    if (!clock_is_24h_style() && (time_text[0] == '0')) {
      memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }

    text_layer_set_text(time_layer, time_text);
  }
  
  if (units_changed & DAY_UNIT) 
  {
    // Update the date - Without a leading 0 on the day of the month
    char day_text[4];
    strftime(day_text, sizeof(day_text), "%a", tick_time);
    snprintf(date_text, sizeof(date_text), "%s %i", day_text, tick_time->tm_mday);
    text_layer_set_text(date_layer, date_text);
  }

  // Update the bottom half of the screen: icon and temperature
  static int animation_step = 0;
  if (weather_data->updated == 0 && weather_data->error == WEATHER_E_OK)
  {
    // 'Animate' loading icon until the first successful weather request
    if (animation_step == 0) {
      weather_layer_set_icon(weather_layer, WEATHER_ICON_LOADING1);
    }
    else if (animation_step == 1) {
      weather_layer_set_icon(weather_layer, WEATHER_ICON_LOADING2);
    }
    else if (animation_step >= 2) {
      weather_layer_set_icon(weather_layer, WEATHER_ICON_LOADING3);
    }
    animation_step = (animation_step + 1) % 3;
  }
  else 
  {
  
	static int first_update = 1;
	if(first_update)
	{
		// turn the light on, it will turn off automatically
		light_enable_interaction();	
		first_update = 0;
	}
  
    // Update the weather icon and temperature
    if (weather_data->error) 
	{
      weather_layer_set_icon(weather_layer, WEATHER_ICON_PHONE_ERROR);
    }
    else 
	{
		// Show the temperature as 'stale' if it has not been updated in 30 minutes
		bool stale = (weather_data->updated < time(NULL) - 30*60);
    
		weather_layer_set_temperature(weather_layer, weather_data, stale);
	  
		// Day/night check
		bool night_time = (weather_data->current_time < weather_data->sunrise || weather_data->current_time > weather_data->sunset);

		weather_layer_set_icon(weather_layer, weather_icon_for_condition(weather_data->condition, night_time));
    }
  }

  if(b_request_weather)
  {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "15 minute weather update");

    request_weather();
	set_dirty();
  }
  else
  {
	done_dirty();
  }
}

static void handle_accelerator(AccelData *samples, uint32_t num_samples)
{
	if(num_samples < 2)
	{
		return;
	}
	
	int32_t sum_z = samples[0].z;
	int32_t sum_delta_z = 0;
	int32_t max_z = sum_z;
	int32_t max_delta = 0;
	for(uint32_t i = 1; i < num_samples; i++)
	{
		// ignore if vibrating
		if(samples[i].did_vibrate)
		{
			return;
		}
		
		//uint64_t time = samples[i].timestamp;
		
		int32_t delta = samples[i].z - samples[i-1].z;
		
		if(delta < max_delta)
		{
			max_delta = delta;
		}
		if(samples[i].z < max_z)
		{
			max_z = samples[i].z;
		}
		
		sum_delta_z += delta;
		sum_z += samples[i].z;
	}
	
	sum_z /= (int)num_samples;
	sum_delta_z /= (int)(num_samples-1);
	
	if(max_z < -2000)
	{
		// update if we haven't updated for at least 3 minutes
		if (weather_data->updated < time(NULL) - 60*3) 
		{
			request_weather();
			set_dirty();

			light_enable_interaction();
		
			// Vibe pattern: ms on/off/on:
			static const uint32_t const segments[] = { 50 };
			VibePattern pat = {
				.durations = segments,
				.num_segments = ARRAY_LENGTH(segments),
			};
			vibes_enqueue_custom_pattern(pat); 
			
			APP_LOG(APP_LOG_LEVEL_DEBUG, "movement, and need to update weather\n");
		}
		
		APP_LOG(APP_LOG_LEVEL_DEBUG, "%i %i %i %i %i\n", (int)sum_z, (int)sum_delta_z, (int)max_z, (int)max_delta, (int)num_samples);
	}
}

static void handle_battery_state(BatteryChargeState charge)
{
	set_dirty();
}

//#define TIME_FRAME      (GRect(0, 2, 144, 168-6))
//#define DATE_FRAME      (GRect(1, 66, 144, 168-62))
#define TIME_FRAME      (GRect(0, 2, 144, 168-6))
#define DATE_FRAME      (GRect(44, 66, 144-44, 168-62))

static void init(void) {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  weather_data = malloc(sizeof(WeatherData));
  init_network(weather_data);

  font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_18));
  font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_53));

  time_layer = text_layer_create(TIME_FRAME);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_font(time_layer, font_time);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));

  date_layer = text_layer_create(DATE_FRAME);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_font(date_layer, font_date);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));

  // Add weather layer
  //weather_layer = weather_layer_create(GRect(0, 90, 144, 80));
  weather_layer = weather_layer_create(GRect(0, 60, 144, 108)); // screen res is 144x168
  layer_add_child(window_get_root_layer(window), weather_layer);

  // Update the screen right away
  time_t now = time(NULL);
  handle_tick(localtime(&now), SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT );
  
  set_dirty();
  
  uint32_t samples_per_update = 4;
  accel_data_service_subscribe(samples_per_update, handle_accelerator); 
	

	battery_state_service_subscribe(handle_battery_state);	
}

static void deinit(void) {
  window_destroy(window);
  tick_timer_service_unsubscribe();
  accel_data_service_unsubscribe();
  battery_state_service_unsubscribe();

  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
  weather_layer_destroy(weather_layer);

  fonts_unload_custom_font(font_date);
  fonts_unload_custom_font(font_time);

  free(weather_data);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
