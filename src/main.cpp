#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <U8g2Graphing.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include "MAX30105.h"
#include "heartRate.h"

// const char *ssid = "ETUDIANT_Plus";
// const char *password = "EnetcomEtud";

// IPAddress local_IP(192, 168, 1, 251); // 192.168.1.251/update
// IPAddress gateway(192, 168, 1, 1);
// IPAddress subnet(255, 255, 255, 0);
// IPAddress primaryDNS(8, 8, 8, 8);
// IPAddress secondaryDNS(8, 8, 4, 4);
// AsyncWebServer server(80);

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
U8g2Graphing graph(&u8g2);

MAX30105 particleSensor;

// float result = 0;
// float _ndx = 0;

const byte RATE_SIZE = 4; // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];    // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; // Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;
String statusBar = "";

// void initWiFi();

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");

  // if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  // {
  //   Serial.println("STA Failed to configure");
  // }

  // initWiFi();

  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  //           { request->send(200, "text/plain", "Hi! I am ESP32."); });

  // AsyncElegantOTA.begin(&server); // Start ElegantOTA
  // server.begin();

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) // Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1)
      ;
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();                    // Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); // Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0);  // Turn off Green LED

  u8g2.begin();
  graph.clearData();

  u8g2.firstPage();
  do
  {
  } while (u8g2.nextPage());

  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_helvB14_te);
    u8g2.setCursor(26, 14 + 12);
    u8g2.print("Medical");
    u8g2.setCursor(48, 36 + 12);
    u8g2.print("Pro");
    delay(64);
  } while (u8g2.nextPage());

  u8g2.setFont(u8g2_font_tom_thumb_4x6_tf);
  u8g2.setFontMode(1);
  u8g2.setDrawColor(2);

  // There are 2 method to initiate the graph, .beginInt and .begin
  //.beginInt is used for integer data and lower RAM allocation.
  //.begin is used for floating point data but higher RAM allocation.
  // Using .begin on Uno will cause glitch and crash as it'll be over the RAM limit.
  // The arguments for the function is (from x, from y, to x, to y) in pixels.
  graph.beginInt(3, 19, 123, 59);

  // First value determines the X axis visibility (true = visible, false = hidden)
  // Second value determines the sytle of how the graph is displayed (false = line, true = dotted)
  graph.displaySet(false, false);
  graph.pointerSet(true, (int)beatsPerMinute);
  graph.rangeSet(true, 0, 160);
}

void loop()
{
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    // We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the array
      rateSpot %= RATE_SIZE;                    // Wrap variable

      // Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);
  if (irValue < 50000)
    Serial.print(" No finger?");
  Serial.println();

  if (irValue < 50000)
  {
    statusBar = "BPM          AVG 0";
    graph.inputValue(0);
  }
  else
  {
    statusBar = "BPM          AVG " + String(beatAvg);
    graph.inputValue(beatsPerMinute);
  }

  u8g2.firstPage();
  do
  {
    u8g2.setCursor(4, 12);
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.print(statusBar);

    // Display the graph
    graph.displayGraph();
  } while (u8g2.nextPage());
}

// void initWiFi()
// {
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(ssid, password);
//   unsigned long timeout = millis();

//   Serial.print("Connecting");
//   while (WiFi.status() != WL_CONNECTED)
//   {
//     delay(500);
//     Serial.print(".");
//     if (millis() - timeout > 10000)
//       goto this_place;
//   }
//   Serial.println("");
//   Serial.print("Connected to WiFi network with IP Address: ");
//   Serial.println(WiFi.localIP());
//   return;
// this_place:
//   Serial.print("Not connected");
//   Serial.println("\n");
// }