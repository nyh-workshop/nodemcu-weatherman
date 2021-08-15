#include <Arduino.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// Reference: https://randomnerdtutorials.com/esp8266-nodemcu-http-get-open-weather-map-thingspeak-arduino/

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Include your SSID and Password:
const char* ssid = "";
const char* password = "";

// Your Domain name with URL path or IP address with path
String openWeatherMapApiKey = "";

// Insert your city name and country code:
String cityName = "";
String countryCode = "";
String openWeatherAPIcall = "http://api.openweathermap.org/data/2.5/weather?q=" + cityName + "&appid=" + openWeatherMapApiKey + "&units=metric";

String jsonBuffer;

struct rtcData {
  uint8_t isPoweredOn;
  uint8_t screenState;
  uint8_t minuteCount;
  uint8_t otherFlags;
};

struct resultJSON {
  uint32_t checksum;
  int32_t tempr;
  char weatherIconName[8];
  int32_t humidity;
};


String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

inline void drawGlyphScreen(uint32_t glyph) {
  u8g2.clearBuffer();  
  u8g2.drawGlyph(32,64, glyph);
  u8g2.sendBuffer();
}

void drawWeatherIcon(String& weatherIcon) {
  // https://openweathermap.org/weather-conditions
  weatherIcon.replace("\"", "");
  Serial.println(weatherIcon.c_str());
  
  enum WeatherGlyph {
    Cloudy = 64,
    Overcast,
    ClearNight,
    Rain,
    Star,
    ClearDay
  };

  if(weatherIcon == "01d") {
    //draw day and clear sky
    drawGlyphScreen(ClearDay);
    return;
  }
  else if (weatherIcon == "02d" || weatherIcon == "03d" || weatherIcon == "04d" ||
           weatherIcon == "02n" || weatherIcon == "03n" || weatherIcon == "04n") {
    // cloudy
    drawGlyphScreen(Cloudy);
    return;  
  }
  else if (weatherIcon == "09d" || weatherIcon == "10d" || weatherIcon == "11d" ||
           weatherIcon == "09n" || weatherIcon == "10n" || weatherIcon == "11n")
  {
    // rainy and/or thunderstorm
    drawGlyphScreen(Rain);
    return;
  }
  else if (weatherIcon == "01n") {
    // night and clear sky
    drawGlyphScreen(ClearNight);
    return;
  }
  else {
    // undefined.
    Serial.println("undefined weather icon code!");
    return;
  }
}

String getOpenWeatherHTTP() {
  String jsonBuffer = httpGETRequest(openWeatherAPIcall.c_str());
  return jsonBuffer;
}

void drawWeather(char* weatherString) {    
  u8g2.setFont(u8g2_font_open_iconic_weather_8x_t);
  String weatherIcon = String(weatherString);
  //Serial.println(weatherIcon.c_str());
  drawWeatherIcon(weatherIcon);
}

void drawTempr(uint32_t temprCelcius) {
  u8g2.setFont(u8g2_font_logisoso54_tf);
  String temprString = String(temprCelcius) + "\xb0";
  //Serial.println("Temperature");
  Serial.println(temprString.c_str());
  u8g2.clearBuffer();
  u8g2.drawStr(0, 63, temprString.c_str());
  u8g2.drawGlyph(90, 44, 'c');
  u8g2.sendBuffer();
}

void drawHumidity(uint32_t humidity) {
  u8g2.setFont(u8g2_font_logisoso54_tf);
  String humidityString = String(humidity);
  //Serial.println("Humidity");
  Serial.println(humidityString.c_str());
  humidityString.replace("\"","");
  humidityString = humidityString + "%";
  u8g2.clearBuffer();
  u8g2.drawStr(0, 63, humidityString.c_str());
  u8g2.sendBuffer();
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Connected!");
}

uint32_t generateChecksum(uint32_t sum) {
  return sum ^ 0xFFFFFFFF;  
}

void putResultsIntoRtcMem(struct resultJSON rj, String& jsonBuffer) {
  
    JSONVar temperatureJSON = JSON.parse(jsonBuffer);
    rj.tempr = int32_t(temperatureJSON["main"]["temp"]);
    strcpy(rj.weatherIconName, JSON.stringify(temperatureJSON["weather"][0]["icon"]).c_str());
    rj.humidity = int32_t(temperatureJSON["main"]["humidity"]);

    uint32_t sum = rj.tempr + rj.humidity + (uint32_t)rj.weatherIconName[0] + (uint32_t)rj.weatherIconName[1];
    rj.checksum = generateChecksum(sum);
    
    ESP.rtcUserMemoryWrite(4, (uint32_t*)&rj, sizeof(resultJSON));
}  


void drawPanelAndSleep() {

  rtcData rcdt;
  resultJSON rsltjs;
  
  enum showScreenStates { DrawWeather = 0, DrawTempr, DrawHumidity, NotAvailable };
  static uint8_t screenState = DrawWeather;
  uint32_t minuteCount = 0;
  const uint32_t tenMinutes = (60 * 10)/20;

  ESP.rtcUserMemoryRead(0, (uint32_t*)&rcdt, sizeof(rcdt));
  ESP.rtcUserMemoryRead(4, (uint32_t*)&rsltjs, sizeof(rsltjs));

  screenState = rcdt.screenState;
  minuteCount = rcdt.minuteCount;

  if(minuteCount > tenMinutes) {
    Serial.println("Getting weather info now! :D");
    minuteCount = 0;
    connectWiFi();
    jsonBuffer = getOpenWeatherHTTP();
    putResultsIntoRtcMem(rsltjs, jsonBuffer);
  }    
  else
    minuteCount++;

  if(screenState == NotAvailable)
    screenState = DrawWeather;

  Serial.println(screenState,DEC);  

  switch(screenState) {
    case DrawWeather:
      drawWeather(rsltjs.weatherIconName);
      screenState++;
      break;
    case DrawTempr:
      drawTempr(rsltjs.tempr);
      screenState++;
      break;
    case DrawHumidity:
      drawHumidity(rsltjs.humidity);
      screenState++;
      break;
    default:
      screenState = DrawWeather;    
      break;
  }

  rcdt.screenState = screenState;
  rcdt.minuteCount = minuteCount;

  ESP.rtcUserMemoryWrite(0, (uint32_t*)&rcdt, sizeof(rcdt));

  ESP.deepSleep(20e6);  
}

void setup() {
  //pinMode(16, OUTPUT);

  //digitalWrite(16, HIGH);

  rtcData rcdt;
  resultJSON rsltjs;
  
  u8g2.begin();

  u8g2.clearBuffer();
  
  Serial.begin(115200);

  ESP.rtcUserMemoryRead(0, (uint32_t*)&rcdt, sizeof(rcdt));
  ESP.rtcUserMemoryRead(4, (uint32_t*)&rsltjs, sizeof(rsltjs));

  uint32_t checksum = (rsltjs.humidity + rsltjs.tempr + (uint32_t)rsltjs.weatherIconName[0] + (uint32_t)rsltjs.weatherIconName[1] + rsltjs.checksum) ^ 0xffffffff;

  Serial.print("\nChecksum: ");
  Serial.println(checksum, HEX); 

  // if data obtained before the sleep doesn't tally, reconnect and get data!
  if(checksum) {
    connectWiFi();
    jsonBuffer = getOpenWeatherHTTP();
    rcdt.screenState = 0;
    rcdt.minuteCount = 0;

    putResultsIntoRtcMem(rsltjs, jsonBuffer);
  }

  ESP.rtcUserMemoryWrite(0, (uint32_t*)&rcdt, sizeof(rcdt));
  
  drawPanelAndSleep();
}


void loop() {
  // put your main code here, to run repeatedly:

}
