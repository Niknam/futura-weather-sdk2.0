#include <pebble.h>

#include "weather_layer.h"
#include "network.h"
#include "config.h"

/* Keep a pointer to the current weather data as a global variable */
static WeatherData *weather_data = 0;

/* Global variables to keep track of the UI elements */
static Window *window = 0;
static TextLayer *date_layer = 0;
static TextLayer *time_layer = 0;
static WeatherLayer *weather_layer = 0;

static char date_text[] = "XXX 00";
static char time_text[] = "00:00";

/* Preload the fonts */
static GFont font_date;
static GFont font_time;

// the threshold under which we take acceleration to be zero
static const int still_mode_jitter_threshold = 200;
static time_t time_still_mode = 0;

static int i_dirty = 0;

static bool b_night_time = 0;

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

static AccelData history_g[26];
static AccelData min_a;
static AccelData max_a;
static int history_g_i = -1;

static void reset_min_max()
{
	memset(&min_a, 0, sizeof(min_a));
	memset(&max_a, 0, sizeof(max_a));
}

static void request_weather_and_alert()
{
	// update if we haven't updated for at least 3 minutes
	if (weather_data->updated < time(0) - 60*3) 
	{
		request_weather();
		set_dirty();

		// turn on the light if after sunset and before sunrise
		if(b_night_time)
		{
			light_enable_interaction();
		}

		// Vibe pattern: ms on/off/on:
		static const uint32_t const segments[] = { 50 };
		VibePattern pat = {
			.durations = segments,
			.num_segments = ARRAY_LENGTH(segments),
		};
		vibes_enqueue_custom_pattern(pat); 
		
		APP_LOG(APP_LOG_LEVEL_DEBUG, "updating weather");
	}
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
  int b_request_weather = 0;
  
  if (units_changed & MINUTE_UNIT) 
  {  
	b_request_weather = ((!weather_data->b_still_mode) && (tick_time->tm_min % 15) == 0);
  
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

	//APP_LOG(APP_LOG_LEVEL_DEBUG, "accel %i %i %i", (int)history_g[history_g_i].x, (int)history_g[history_g_i].y, (int)history_g[history_g_i].z);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "min_a %i %i %i", (int)min_a.x, (int)min_a.y, (int)min_a.z);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "max_a %i %i %i", (int)max_a.x, (int)max_a.y, (int)max_a.z);
	
	if(!weather_data->b_still_mode)
	{
		if(max_a.z < still_mode_jitter_threshold && min_a.z > -still_mode_jitter_threshold)
		{
			if(!weather_data->battery.is_charging)
			{
				weather_data->b_still_mode = 1;
				time_still_mode = time(0);
				APP_LOG(APP_LOG_LEVEL_DEBUG, "in still mode");
			}
		}
		else
		{
			reset_min_max();
		}
	}
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
		b_night_time = (weather_data->current_time < weather_data->sunrise || weather_data->current_time > weather_data->sunset);

		//weather_layer_set_icon(weather_layer, weather_icon_for_condition(weather_data->condition, b_night_time));
		weather_layer_set_icon(weather_layer, yahoo_weather_icon_for_condition(weather_data->condition));
    }
  }

  if(b_request_weather)
  {
	request_weather_and_alert();
  }
  else
  {
	done_dirty();
  }
}

static void handle_tap(AccelAxisType axis, int32_t direction)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "tap axis %i direction %i", (int)axis, (int)direction);
	request_weather_and_alert();
}

static void handle_accelerator(AccelData *samples, uint32_t num_samples)
{
	if(!num_samples)
	{
		return;
	}
	
	if(history_g_i < 0)
	{
		memset(&history_g, 0, sizeof(history_g));
		history_g_i = 0;
	}
	
	const int history_g_len = sizeof(history_g)/sizeof(history_g[0]);
	
	int init_min_max = 0;
	
	for(uint32_t i = 1; i < num_samples; i++)
	{
		// ignore if vibrating
		if(samples[i].did_vibrate)
		{
			return;
		}
		
		// previous sample of g
		AccelData *prevg = &history_g[history_g_i];

		// increment the index and when we get to the end wrap around to the start
		history_g_i++;
		if(history_g_i >= history_g_len)
		{
			history_g_i -= history_g_len;
		}
		
		// current value of g we are calculating now
		AccelData *g = &history_g[history_g_i];
		
		// g = g*0.9+a*0.1
		g->x = (prevg->x*9 + samples[i].x*1)/10;
		g->y = (prevg->y*9 + samples[i].y*1)/10;
		g->z = (prevg->z*9 + samples[i].z*1)/10;
		
		// now look at the oldest sample we have, which will be the next one in the ring buffer
		int oldest_g = history_g_i + 1;
		if(oldest_g >= history_g_len)
		{
			oldest_g -= history_g_len;
		}
		
		AccelData *oldg = &history_g[oldest_g];
		
		// now get the dot product of the two gravity direction vectors
		int dot_product = g->x*oldg->x + g->y*oldg->y + g->z*oldg->z;
		
		// if the vectors are perpendicular the dot product is zero, see how close we are to zero, e.g. if the watch had just rotated 90 degrees
		if((dot_product > -100000) && (dot_product < 100000))
		{
			// g in this direction means the watch is at about the angle when being looked at
			if((g->y < -600) && (g->z < -600))
			{
				request_weather_and_alert();
				APP_LOG(APP_LOG_LEVEL_DEBUG, "dot prod %i", dot_product);
			}
			else
			{
				APP_LOG(APP_LOG_LEVEL_DEBUG, "dot prod %i ignoring rotate because y=%i, z=%i ", dot_product, g->y, g->z);
			
			}
		}
		
		// now subtract g from the acceleration to see if the watch is actually accelerating
		samples[i].x = samples[i].x - g->x; 
		samples[i].y = samples[i].y - g->y; 
		samples[i].z = samples[i].z - g->z; 
		
		// initialise
		if(!min_a.z && !min_a.y && !min_a.x)
		{
			init_min_max = 1;
			min_a = samples[i];
			max_a = samples[i];
		}

		if(min_a.x > samples[i].x)
		{
			min_a.x = samples[i].x;
		}
		if(min_a.y > samples[i].y)
		{
			min_a.y = samples[i].y;
		}
		if(min_a.z > samples[i].z)
		{
			min_a.z = samples[i].z;
		}
		if(max_a.x < samples[i].x)
		{
			max_a.x = samples[i].x;
		}
		if(max_a.y < samples[i].y)
		{
			max_a.y = samples[i].y;
		}
		if(max_a.z < samples[i].z)
		{
			max_a.z = samples[i].z;
		}
	}
	
	if(init_min_max)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG, "initialise min/max a");
	}
	
	if(weather_data->b_still_mode)
	{
		if(max_a.z > still_mode_jitter_threshold || min_a.z < -still_mode_jitter_threshold)
		{
			// not in still mode any more
			weather_data->b_still_mode = 0;
			// need to be in still mode for this long before updating the display
			if(time(0) - time_still_mode > 60*1)
			{
				request_weather_and_alert();
			}
			
			APP_LOG(APP_LOG_LEVEL_DEBUG, "out of still mode");
		}
	}
	
	/*
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
		request_weather_and_alert();
		
		APP_LOG(APP_LOG_LEVEL_DEBUG, "%i %i %i %i %i\n", (int)sum_z, (int)sum_delta_z, (int)max_z, (int)max_delta, (int)num_samples);
	}
	*/
}

static void handle_battery_state(BatteryChargeState charge)
{
	weather_data->battery = charge;
	
	set_dirty();
}

//#define TIME_FRAME      (GRect(0, 2, 144, 168-6))
//#define DATE_FRAME      (GRect(1, 66, 144, 168-62))
#define TIME_FRAME      (GRect(0, -8, 144, 168))	// -8 gets the time right on the top pixel line
#define DATE_FRAME      (GRect(44, 66-10, 144-44, 168))

static void init(void) 
{
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  weather_data = malloc(sizeof(WeatherData));
  memset(weather_data, 0, sizeof(WeatherData));
  init_network(weather_data);
  
  reset_min_max();
  
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
  weather_layer = weather_layer_create(GRect(0, 50, 144, 118)); // screen res is 144x168
  layer_add_child(window_get_root_layer(window), weather_layer);

  // Update the screen right away
  time_t now = time(NULL);
  handle_tick(localtime(&now), SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT );
  
  set_dirty();
  
  uint32_t samples_per_update = 4;
  accel_data_service_subscribe(samples_per_update, handle_accelerator); 
  
  accel_tap_service_subscribe(handle_tap);	

	
	weather_data->battery = battery_state_service_peek();
	battery_state_service_subscribe(handle_battery_state);	
}

static void deinit(void) {
  window_destroy(window);
  tick_timer_service_unsubscribe();
  accel_data_service_unsubscribe();
  battery_state_service_unsubscribe();
  accel_tap_service_unsubscribe();	

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
