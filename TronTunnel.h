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
#ifndef _TRON_TUNNEL_ACH_
#define _TRON_TUNNEL_ACH_

#define SMOOTH_SIZE 5

typedef struct {
  int t;						// The trigger pin of the ultrasonic sensor.
  int e;						// The echo pin of the ultrasonic sensor.
  int s;						// The index of the smoothing array we are currently updating.
  float smooth[SMOOTH_SIZE];	// An array of readings used for averaging the sensor output.
} Ultrasonic;

// initUltrasconic initalises an ultrasonic sensor with the supplied trigger and echo pin numbers.
Ultrasonic initUltrasonic(int trigger, int echo);

// getDistance returns the distance in centimetres of an objects detected by the ultrasonic sensor 'u'.
// The maximum value getDistance will return is 200.
float getDistance(Ultrasonic *u);

#endif