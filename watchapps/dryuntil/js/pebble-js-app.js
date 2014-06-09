var city = "";
var rain = -1;
var rain_starts = "";

/* send the weather update to pebble if all parameters are received */
function send_message()
{
    if(city == "" || rain == -1 ) {
        console.log("not returning yet, need both rain and city (city="+city+", rain="+rain+")");
        return;
    }

    console.log("Sending city "+city+" and rain "+rain);
    Pebble.sendAppMessage( {
        "city" : city,
        "rain" : rain,
        "start" : rain_starts
    });
}

/* fetch weather for given location */
function fetchWeather(lat, lon)
{
    var response;
    var city_req = new XMLHttpRequest();

    // get city for pretty display
    var city_url = "http://maps.googleapis.com/maps/api/geocode/json?latlng="+lat+","+lon+"&sensor=true";
    console.log("Requesting URL "+city_url);
    city_req.open('GET', city_url, true);
    city_req.onload = function(e) {
        if (city_req.readyState == 4 && city_req.status == 200) {
            response = JSON.parse(city_req.responseText);
            if (response) {
                city = response.results[0].address_components[2].short_name;
                send_message();
            }
        }
    }
    city_req.send(null);

    var rain_req = new XMLHttpRequest();

    // get raininess for location
    rain_url = "http://gps.buienradar.nl/getrr.php?" + "lat=" + lat + "&lon=" + lon; 
    console.log("Requesting URL "+rain_url);
    rain_req.open('GET', rain_url, true);
    rain_req.onload = function(e) {
        if (rain_req.readyState == 4 && rain_req.status == 200) {
            rows = rain_req.responseText.split("\n");
            rain = 0;   // assume no rain
            for(i=0;i<rows.length;i++) // loop over next hour
            {
                [rs,time] = rows[i].split("|");
                r = parseInt(rs);
                if( r > 0 )
                    rain = 1
                if(rain_starts == "" && r > 0)
                {
                    rain_starts = time;
                }
            }
            send_message();
        }
    }
    rain_req.send(null);
}

function locationSuccess(pos)
{
    // we have a loc, get weather
    var coordinates = pos.coords;
    fetchWeather(coordinates.latitude, coordinates.longitude);
    console.log("Got location, lat="+coordinates.latitude+" lon="+coordinates.longitude);
}

function locationError(err)
{
    // error retrieving loc, let watch know
    console.log("Unable to get location, error code "+err.code);
    Pebble.sendAppMessage( {
        "city": "Unavailable",
        "rain": 2
    });
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 

Pebble.addEventListener("ready",
    function(e) {
        l = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
    }

);
