/*
 * Copyright (c) Clinton Freeman 2016
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
#include "TronTunnel.h"

extern "C" {
  #include "c_types.h"
  #include "ets_sys.h"
  #include "os_type.h"
  #include "osapi.h"
  #include "mem.h"
  #include "user_interface.h"
}

const char* ssid = "tron-tunnel";
const char* password = "tq9Zjk23";
Ultrasonic ultrasonic;

Ultrasonic initUltrasonic(int trigger, int echo) {
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);

  return (Ultrasonic) {trigger, echo, 0, {0.0, 0.0, 0.0, 0.0, 0.0}};
}

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
}

void sendPosition(IPAddress addr, float pos) {
  WiFiClient client;
  if (!client.connect(addr, 80)) {
    Serial.println("Unable to connect to client.");
    return;
  }

  client.print(String("GET ") + "/update?p=" + String(pos) + " HTTP/1.1\r\n"+
               "Host: " + addr.toString() + "\r\n" +
               "Connection: close\r\n\r\n");

  client.stop();
}

void loop() {
  float ud = getDistance(&ultrasonic);
  float n = std::min(1.0, (ud / 187.0));

  // Get the IP addresses of all the clients connected to the ACCESS point.
  station_info *stat_info = wifi_softap_get_station_info();
  while (stat_info != NULL) {
    struct ip_addr *ipaddr;

    ipaddr = &stat_info->ip;
    sendPosition(ipaddr->addr, n);

    stat_info = STAILQ_NEXT(stat_info, next);
  }

  delay(100);
}
