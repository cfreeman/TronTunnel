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
#include "TronTunnel.h"

extern "C" {
  #include "c_types.h"
  #include "ets_sys.h"
  #include "os_type.h"
  #include "osapi.h"
  #include "mem.h"
  #include "user_interface.h"
}

// Underlying hardware.
WiFiUDP udp;
Ultrasonic ultrasonic;

// Credentials for this Access Point (AP).
const char* ssid = "tron-tunnel";
const char* password = "tq9Zjk23";
const int udpPort = 4210;

// initUltrasonic configures an ultrasonic sensor on the supplied trigger
// and echo pins.
Ultrasonic initUltrasonic(int trigger, int echo) {
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);

  return (Ultrasonic) {trigger, echo, 0, {0.0, 0.0, 0.0, 0.0, 0.0}};
}

// getDistance returns the distance in centimetres from any detected
// obstacles from the supplied ultrasonic sensor.
float getDistance(Ultrasonic *u) {
  // Pulse the trigger pin and wait for an echo.
  digitalWrite(u->t, HIGH);
  delayMicroseconds(10);
  digitalWrite(u->t, LOW);

  long duration = pulseIn(u->e, HIGH);
  u->smooth[u->s] = _min(200.0, (duration/2) / 29.412);
  u->s = (u->s + 1) % SMOOTH_SIZE;

  float res = 0.0;
  for (int i = 0; i < SMOOTH_SIZE; i++) {
    res = res + u->smooth[i];
  }

  return (res / (float)SMOOTH_SIZE);
}

void setup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.begin(9600);
  ultrasonic = initUltrasonic(4, 5);

  udp.begin(udpPort);
}

void loop() {
  float ud = getDistance(&ultrasonic);
  float n = std::min(1.0, (ud / 187.0));

  udp.beginPacketMulticast(IPAddress(192,168,4,255), udpPort, WiFi.softAPIP());
  char position[255];
  String(n).toCharArray(position, 255);
  udp.write(position);
  udp.endPacket();

  delay(25);
}
