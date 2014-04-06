Pebble.addEventListener("ready", function(e) {
    console.log("Starting ...");
    updateWeather();
});

Pebble.addEventListener("appmessage", function(e) {
    console.log("Got a message - Starting weather request...");
    updateWeather();
});

var updateInProgress = false;

function updateWeather() {
    if (!updateInProgress) {
        updateInProgress = true;
        var locationOptions = { "timeout": 15000, "maximumAge": 60000 };
        navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
    }
    else {
        console.log("Not starting a new request. Another one is in progress...");
    }
}

function locationSuccess(pos) {
    var coordinates = pos.coords;
    console.log("Got coordinates: " + JSON.stringify(coordinates));
    fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
    console.warn('Location error (' + err.code + '): ' + err.message);
    Pebble.sendAppMessage({ "error": "Loc unavailable" });
    updateInProgress = false;
}

function fetchWeather(latitude, longitude) {
    var response;
    var intemp  = 0;
    var outtemp = 0;

    var req = new XMLHttpRequest();
    req.open('GET', "http://gary.ath.cx/gary.php?" +
        "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
            if(req.status == 200) {
                console.log(req.responseText);
                response = JSON.parse(req.responseText);
                var temperature, icon, city, sunrise, sunset, condition;
                var current_time = Date.now() / 1000;
                if (response) {
                    var tempResult = response.main.temp;
                    if (response.sys.country === "US") {
                        // Convert temperature to Fahrenheit if user is within the US
                        temperature = Math.round(((tempResult - 273.15) * 1.8) + 32);
                    }
                    else {
                        // Otherwise, convert temperature to Celsius
                        temperature = Math.round(tempResult - 273.15);
                    }		 
                    condition = response.weather[0].id;
                    sunrise = response.sys.sunrise;
                    sunset = response.sys.sunset;
                    intemp = Math.round(response.gary.intemp);
                    outtemp = Math.round(response.gary.temp);

                    var place = response.gary.place;

		    
                    console.log("place:" + place + " in:" + intemp + " Temperature: " + temperature + " Condition: " + condition + " Sunrise: " + sunrise +
                              " Sunset: " + sunset + " Now: " + Date.now() / 1000);
                              
                    Pebble.sendAppMessage({
                        "condition": condition,
                        "temperature": temperature,
						"intemp": intemp,
						"outtemp": outtemp,
                        "sunrise": sunrise,
                        "sunset": sunset,
                        "current_time": current_time,
						"place": place
                    });
                    updateInProgress = false;
                }
            } else {
                console.log("Error");
                updateInProgress = false;
                Pebble.sendAppMessage({ "error": "HTTP Error" });
            }
        }
    };
    req.send(null);

/*
  var req2 = new XMLHttpRequest();
  req2.open('GET', "http://gary.ath.cx/gary.php?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
  req2.onload = function(e) 
  {
    console.log("IN THE REQ2 LOAD");
    if (req2.readyState == 4) 
    {
      if(req2.status == 200) 
      {
        console.log(req2.responseText);
        response = JSON.parse(req2.responseText);
       
        if (response)
        {
          
                var current_time = Date.now() / 1000;
		var condition = 742;
          intemp = Math.round(response.intemp);
	  outtemp = Math.round(response.temp);
                    Pebble.sendAppMessage({
                        "condition": condition,
                        "temperature": outtemp,
			"intemp": intemp,
			"outtemp": outtemp,
                        "sunrise": 1396899167,
                        "sunset": 1396944761,
                        "current_time": current_time
                    });
                    updateInProgress = false;
        }

      } 
      else 
      {
        console.log("Error");
      }
    }
  }
  req2.send(null);
*/

/*
    var req = new XMLHttpRequest();
    req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" +
        "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
            if(req.status == 200) {
                console.log(req.responseText);
                response = JSON.parse(req.responseText);
                var temperature, icon, city, sunrise, sunset, condition;
                var current_time = Date.now() / 1000;
                if (response) {
                    var tempResult = response.main.temp;
                    if (response.sys.country === "US") {
                        // Convert temperature to Fahrenheit if user is within the US
                        temperature = Math.round(((tempResult - 273.15) * 1.8) + 32);
                    }
                    else {
                        // Otherwise, convert temperature to Celsius
                        temperature = Math.round(tempResult - 273.15);
                    }		 
                    condition = response.weather[0].id;
                    sunrise = response.sys.sunrise;
                    sunset = response.sys.sunset;


		    
                    console.log("Temperature: " + temperature + " Condition: " + condition + " Sunrise: " + sunrise +
                              " Sunset: " + sunset + " Now: " + Date.now() / 1000);
                              
                    Pebble.sendAppMessage({
                        "condition": condition,
                        "temperature": temperature,
			"intemp": intemp,
			"outtemp": outtemp,
                        "sunrise": sunrise,
                        "sunset": sunset,
                        "current_time": current_time
                    });
                    updateInProgress = false;
                }
            } else {
                console.log("Error");
     e           updateInProgress = false;
                Pebble.sendAppMessage({ "error": "HTTP Error" });
            }
        }
    };
    req.send(null);
*/
}
