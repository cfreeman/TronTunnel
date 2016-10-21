/*
 * Copyright (c) Clinton Freeman & Kurt Schoenhoff 2016
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "ESP8266WiFi.h"
#include "ESP8266WiFiGeneric.h"
#include "ESP8266WifiAP.h"
#include "WiFiUDP.h"
#include "NewPing.h"

extern "C" {
  #include "c_types.h"
  #include "ets_sys.h"
  #include "os_type.h"
  #include "osapi.h"
  #include "mem.h"
  #include "user_interface.h"
}

// Credentials for this Access Point (AP).
const char* ssid = "tron-tunnel";
const char* password = "tq9Zjk23";
const int udpPort = 4210;
IPAddress multicast_ip_addr(239, 255, 255, 250);

IPAddress masterIP(192,168,0,10);
IPAddress gateway(192, 168, 1, 1); // set gateway to match your network
IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your



// Smoothing array for cleaning noise in ultrasonic measurements.
#define SMOOTH_SIZE 5
typedef struct {
  NewPing sonar;

  int smooth[SMOOTH_SIZE];
  int smoothSum;
  int s;
} Tripwire;

int smooth(Tripwire *sm, int v) {
  sm->smoothSum = sm->smoothSum - sm->smooth[sm->s];
  sm->smooth[sm->s] = v;
  sm->smoothSum = sm->smoothSum + sm->smooth[sm->s];
  sm->s = (sm->s + 1) % SMOOTH_SIZE;

  return (sm->smoothSum/SMOOTH_SIZE);
}

// Underlying hardware.
#define NUM_TRIP_WIRES 4
#define COOLDOWN 1000
#define TRIP_UPPER 120
#define TRIP_LOWER 60
WiFiUDP udp;
Tripwire trap[NUM_TRIP_WIRES] = {
  {NewPing(2, 15, 140), {0}, 0, 0},
  {NewPing(0, 13, 140), {0}, 0, 0},
  {NewPing(4, 12, 140), {0}, 0, 0},
  {NewPing(5, 14, 140), {0}, 0, 0}
};

// Sensor state.
float currentPos;
unsigned long lastTrip;

void setup() {
  delay(2000);  // Give the sensor / master ESP8266 a couple of seconds head start.
  Serial.begin(9600);

  // Added to prevent having to power cycle after upload.
  WiFi.disconnect();

  // Connect to the Access Point / sensor / master ESP8266.
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(masterIP, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connected!");

  udp.begin(udpPort);

  currentPos = 0.0;
  lastTrip = millis();
}

void loop() {
  //for (int i = (NUM_TRIP_WIRES - 1); i >= 0; i--) {
  for (int i = 0; i < NUM_TRIP_WIRES; i++) {
    int uS = trap[i].sonar.ping();
    int cm = smooth(&trap[i], (uS / US_ROUNDTRIP_CM));

    if (cm > TRIP_LOWER && cm < TRIP_UPPER) {
      currentPos = (i + 1) / (float) (NUM_TRIP_WIRES + 1);
      lastTrip = millis();
      //break;
    }

    // if (i == 3) {
    //   Serial.print("s");
    //   Serial.print(i);
    //   Serial.print("=");
    //   Serial.println(cm);
    // }

    delay(50);
  }

  // If we haven't detected any trips in a long while, drop
  // back down to zero.
  if ((millis() - lastTrip) > COOLDOWN) {
    currentPos = 0.0;
  }

  Serial.println(currentPos);

  udp.beginPacketMulticast(multicast_ip_addr, udpPort, WiFi.localIP());
  char buffer[255];
  String(currentPos).toCharArray(buffer, 255);
  udp.write(buffer);
  udp.endPacket();
  delay(50);
}
