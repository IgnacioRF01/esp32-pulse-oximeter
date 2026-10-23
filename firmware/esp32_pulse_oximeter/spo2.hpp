/*
 * spo2.hpp — SpO2 estimation from MAX30102 red/IR FIFO samples
 *
 * Adapted from coniferconifer/ESP32_MAX30102_simple-SpO2_plotter
 * https://github.com/coniferconifer/ESP32_MAX30102_simple-SpO2_plotter
 * Original work licensed under the Apache License, Version 2.0.
 *
 * Modifications by Ignacio Romero Fragoso: wrapped the SpO2 computation in
 * an oxi() function returning the filtered estimate, so it can be called from
 * the main sketch alongside heart-rate detection and the OLED interface.
 */


double avered2    = 0;
double aveir2     = 0;
double sumirrms  = 0;
double sumredrms = 0;
int    i2         = 0;
int    Num2       = 100;  // calculate SpO2 by this sampling interval
int    Temperature;
int    temp;
float  ESpO2;            // initial value of estimated SpO2
double FSpO2     = 0.7;  // filter factor for estimated SpO2
double frate2     = 0.95; // low pass filter for IR/red LED value to eliminate AC component
#define TIMETOBOOT2 1  // wait for this time(msec) to output SpO2
#define SCALE      88.0  // adjust to display heart beat and SpO2 in the same scale
#define SAMPLING2   100 //25 //5     // if you want to see heart beat more precisely, set SAMPLING to 1
#define FINGER_ON2  30000 // if red signal is lower than this, it indicates your finger is not on the sensor
#define USEFIFO2

float oxi ()
{

  uint32_t ir, red, green;
  double fred, fir;
  double SpO2 = 0; //raw SpO2 before low pass filtered

#ifdef USEFIFO2
  particleSensor.check(); //Check the sensor, read up to 3 samples

  while (particleSensor.available()) {//do we have new data
#ifdef MAX30105
   red = particleSensor.getFIFORed(); //Sparkfun's MAX30105
   ir  = particleSensor.getFIFOIR();  //Sparkfun's MAX30105
#else
   red = particleSensor.getFIFOIR();  //why getFOFOIR output Red data by MAX30102 on MH-ET LIVE breakout board
   ir  = particleSensor.getFIFORed(); //why getFIFORed output IR data by MAX30102 on MH-ET LIVE breakout board
#endif

    i2++;
    fred = (double)red;
    fir  = (double)ir;
    avered2 = avered2 * frate2 + (double)red * (1.0 - frate2); //average red level by low pass filter
    aveir2 = aveir2 * frate2 + (double)ir * (1.0 - frate2); //average IR level by low pass filter
    sumredrms += (fred - avered2) * (fred - avered2); //square sum of alternate component of red level
    sumirrms += (fir - aveir2) * (fir - aveir2);//square sum of alternate component of IR level
    if ((i2 % SAMPLING2) == 0) {//slow down graph plotting speed for arduino Serial plotter by thin out
      if ( millis() > TIMETOBOOT2) {
        float ir_forGraph = (2.0 * fir - aveir2) / aveir2 * SCALE;
        float red_forGraph = (2.0 * fred - avered2) / avered2 * SCALE;
        //truncation for Serial plotter's autoscaling
        if ( ir_forGraph > 100.0) ir_forGraph = 100.0;
        if ( ir_forGraph < 80.0) ir_forGraph = 80.0;
        if ( red_forGraph > 100.0 ) red_forGraph = 100.0;
        if ( red_forGraph < 80.0 ) red_forGraph = 80.0;
        // Print out red and IR sensor reading to serial interface for monitoring...

        float temperature = particleSensor.readTemperatureF();



      }
    }
    if ((i2 % Num2) == 0) {
      double R = (sqrt(sumredrms) / avered2) / (sqrt(sumirrms) / aveir2);
      // Serial.println(R);
      SpO2 = -23.3 * (R - 0.4) + 100; //http://ww1.microchip.com/downloads/jp/AppNotes/00001525B_JP.pdf -- I don't see this directly in the App Note... look here https://github.com/espressif/arduino-esp32/issues/4561
      ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2;//low pass filter
      //  Serial.print(SpO2);Serial.print(",");Serial.println(ESpO2);
      sumredrms = 0.0; sumirrms = 0.0; i2 = 0;
      break;
    }
    particleSensor.nextSample(); //We're finished with this sample so move to next sample
   // Serial.println(SpO2);
  }
#endif
return ESpO2;
}
