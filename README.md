# nodemcu-weatherman
Short ESP8266 Arduino app to grab weather data from OpenWeatherMap and display it to the SSD1306.

## This is taken and modified from the example project: https://randomnerdtutorials.com/esp8266-nodemcu-http-get-open-weather-map-thingspeak-arduino/

## Description:
The program grabs the weather information on the desired area (using HTTP GET) from OpenWeatherMap.org in the form of JSON.

A brief information from the JSON is printed onto the SSD1306 after taking in the data.

This app deep sleeps every 20 seconds and then cycle the displays of the weather conditions in the form of an icon, then the temperature and the humidity.

For every 10 minutes the app grabs the data from the OpenWeatherMap.

## Issues:
Unfortunately, connecting the D0 directly to the reset does not help in rebooting the module after going into deep sleep.

This problem is mitigated by using a separate small microcontroller (like PIC16F). The simple method is as follows:
- When the falling edge on D0 is present, the microcontroller detects it by its interrupt.
- Then it disables the interrupt and issues two short pulses to the Nodemcu's reset pin via another pin.
- After the two pulses, delay for another 5 seconds for the Nodemcu module to reboot.
- Interrupt enabled, and then microcontroller goes to sleep mode until the next falling edge from NodeMCU's D0 pin.

The sample microcontroller is a PIC16F1503 and using a MikroC PIC compiler (can be used in trial version) to build it.

## Schematic for the Reset Pulse Generator microcontroller connection to Nodemcu:
Note: Does not include the SSD1306 connections!
![alt tag](https://github.com/nyh-workshop/nodemcu-weatherman/blob/main/schematic.png)

## Requirements:
- NodeMCU ESP-12
- An 8-bit microcontroller to assist in resetting the module after going into deep sleep. Sample code provided - "resetPulseGenForESP8266".

## Extra notes:
1. Please get your own OpenWeatherMap account - take the free one first. 
2. Do not repeatedly restart the app if you are connected to your free account - there is a limit to how many times you can retrieve the information per hour!
3. If you have to test your app, use the OpenWeatherMap sample first!
