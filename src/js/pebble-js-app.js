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
    window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
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
  var req = new XMLHttpRequest();
  req.open('GET', "http://api.openweathermap.org/data/2.1/find/city?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        console.log(req.responseText);
        response = JSON.parse(req.responseText);
        var temperature, icon, city;
        if (response && response.list && response.list.length > 0) {
          var weatherResult = response.list[0];
          temperature = Math.round(weatherResult.main.temp - 273.15);
          condition = weatherResult.weather[0].id;

          console.log("Temperature: " + temperature + " Condition: " + condition);
          Pebble.sendAppMessage({
            "condition": condition,
            "temperature": temperature
          });
          updateInProgress = false;
        }
      } else {
        console.log("Error");
        updateInProgress = false;
        Pebble.sendAppMessage({ "error": "HTTP Error" });
      }
    }
  }
  req.send(null);
}



