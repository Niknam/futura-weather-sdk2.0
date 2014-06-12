#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITION 1
#define KEY_SUNRISE 2
#define KEY_SUNSET 3
#define KEY_CURRENT_TIME 4
#define KEY_ERROR 5
#define KEY_REQUEST_UPDATE 42
typedef enum {
  WEATHER_E_OK = 0,
  WEATHER_E_DISCONNECTED,
  WEATHER_E_PHONE,
  WEATHER_E_NETWORK
} WeatherError;

typedef struct {
  int temperature;
  int condition;
  int sunrise;
  int sunset;
  int current_time;
  time_t updated;
} WeatherData;

typedef void (*WeatherUpdateHandler)(WeatherData*);
typedef void (*WeatherErrorHandler)(WeatherError);

void init_network(void);
void close_network(void);

void request_weather(void);

void set_weather_update_handler(WeatherUpdateHandler handler);
void set_weather_error_handler(WeatherErrorHandler handler);
