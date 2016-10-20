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
#define NUM_TRIP_WIRES 2
#define COOLDOWN 4000
#define TRIP_UPPER 60
#define TRIP_LOWER 30
WiFiUDP udp;
Tripwire trap[NUM_TRIP_WIRES] = {
  {NewPing(4, 5, 200), {0}, 0, 0},
  {NewPing(2, 0, 200), {0}, 0, 0}
};

// Sensor state.
float currentPos;
unsigned long lastTrip;

void setup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.begin(9600);
  udp.begin(udpPort);

  currentPos = 0.0;
  lastTrip = millis();
}

void loop() {
  for (int i = 0; i < NUM_TRIP_WIRES; i++) {
    int uS = trap[i].sonar.ping();
    int cm = smooth(&trap[i], (uS / US_ROUNDTRIP_CM));

    if (cm > TRIP_LOWER && cm < TRIP_UPPER) {
      currentPos = (i + 1) / (float) (NUM_TRIP_WIRES + 1);
      lastTrip = millis();
    }

    delay(33);
  }

  // If we haven't detected any trips in a long while, drop
  // back down to zero.
  if ((millis() - lastTrip) > COOLDOWN) {
    currentPos = 0.0;
  }

  udp.beginPacketMulticast(IPAddress(192,168,4,255), udpPort, WiFi.softAPIP());
  char buffer[255];
  String(currentPos).toCharArray(buffer, 255);
  udp.write(buffer);
  udp.endPacket();
}