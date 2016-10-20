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

// Underlying hardware.
WiFiUDP udp;
NewPing sonar(4, 5, 200);

// Credentials for this Access Point (AP).
const char* ssid = "tron-tunnel";
const char* password = "tq9Zjk23";
const int udpPort = 4210;

// Smoothing array for cleaning up noise in the ultrasonic.
#define SMOOTH_SIZE 10
int smooth[SMOOTH_SIZE];
int smoothSum;
int s;

void setup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.begin(9600);
  udp.begin(udpPort);

  smoothSum = 0;
  s = 0;
}

void loop() {
  udp.beginPacketMulticast(IPAddress(192,168,4,255), udpPort, WiFi.softAPIP());
  char position[255];

  int uS = sonar.ping();
  int cm = uS / US_ROUNDTRIP_CM;

  smoothSum = smoothSum - smooth[s];
  smooth[s] = cm;
  smoothSum = smoothSum + smooth[s];
  s = (s + 1) % SMOOTH_SIZE;

  float n = std::min(1.0, ((smoothSum/SMOOTH_SIZE) / 187.0));
  String(n).toCharArray(position, 255);

  udp.write(position);
  udp.endPacket();

  Serial.println(n);
  delay(50);
}