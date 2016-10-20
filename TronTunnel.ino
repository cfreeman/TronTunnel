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
NewPing s1(4, 5, 200);
NewPing s2(2, 0, 200);

// Credentials for this Access Point (AP).
const char* ssid = "tron-tunnel";
const char* password = "tq9Zjk23";
const int udpPort = 4210;

const int s2Handover = 60;
const float nHandover = 1.2;

// Calibration.
// typedef struct {
//   float upperN;
//   float lowerN;
//   int uppperCM;
//   int lowerCM;
// } calibration;

// s1C = {0.25, }


// Smoothing array for cleaning noise in ultrasonic measurements.
#define SMOOTH_SIZE 5
typedef struct {
  int smooth[SMOOTH_SIZE];
  int smoothSum;
  int s;
} smoother;
smoother sm1, sm2;

int smooth(smoother *sm, int v) {
  sm->smoothSum = sm->smoothSum - sm->smooth[sm->s];
  sm->smooth[sm->s] = v;
  sm->smoothSum = sm->smoothSum + sm->smooth[sm->s];
  sm->s = (sm->s + 1) % SMOOTH_SIZE;

  return (sm->smoothSum/SMOOTH_SIZE);
}

void setup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.begin(9600);
  udp.begin(udpPort);

  sm1 = {{0}, 0, 0};
  sm2 = {{0}, 0, 0};
}

void loop() {
  int uS = s1.ping();
  int s1CM = smooth(&sm1, (uS / US_ROUNDTRIP_CM));

  Serial.print("S1=");
  Serial.println(s1CM);

  delay(50);
  uS = s2.ping();
  int s2CM = smooth(&sm2, (uS / US_ROUNDTRIP_CM));

  Serial.print("S2=");
  Serial.println(s2CM);

  float n = 0.0;
  // if (s1CM < 140) {
  //   //60   0.3 0.15
  //   //140  1.2 0.6
  //   n = ((s1CM / 140.0) * 0.6);
  // } else {
  //   //50 1.2 0.6
  //   //130 2.0 1.0

  //   n =

  // }

  Serial.print("N=");
  Serial.println(n);

  // TODO: BLEND measurements between the two sensors.

  udp.beginPacketMulticast(IPAddress(192,168,4,255), udpPort, WiFi.softAPIP());
  char position[255];
  String(n).toCharArray(position, 255);
  udp.write(position);
  udp.endPacket();
}