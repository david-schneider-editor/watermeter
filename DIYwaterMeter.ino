//
// Arduino Sketch for DIY water-usage monitor (10/2015)
//
//Uses the (retired) Sparkfun HMC6352 digital compass module
//Hook up power (5 v okay) and ground. Attach SDL (analog 5 on UNO) and SCA (analog 4 on UNO)
//
// Will determine frequency from raw X magnetometer values using autocorrelation

#include <Wire.h>
int HMC6352Address = 0x42;
// This is calculated in the setup() function
int slaveAddress;
byte magData[2];
int sampleData[400];

int len=400;
long sum, sum_old, sum0, max;

int thresh = 0;
boolean flow, seekPeak;
float freq_per = 0;
byte pd_state = 0;

int i, k, magValue, period;
unsigned long start, cycles;
float dampedMag1, dampedMag2, signal, lastSignal, freq;
float alpha = 0.01; // damping factor to tune
float beta = 0.3;  // damping factor to tune

void setup()
{
  // Shift the device's documented slave address (0x42) 1 bit right
  // This compensates for how the TWI library only wants the
  // 7 most significant bits (with the high bit padded with 0)
  slaveAddress = HMC6352Address >> 1;   // This results in 0x21 as the address to pass to TWI

  Serial.begin(1200);
  Wire.begin();

  // Put the HMC6352 in raw-output mode
  Wire.beginTransmission(slaveAddress);
  Wire.write("G");              // write-to-RAM command
  Wire.write(0x4E);             // address of output-mode register
  Wire.write(0x03);             // set to output (bias-corrected) X values only
  Wire.endTransmission();
  delay(1);                     // needs 70 uS, but can't hurt to wait a full mS
  
  //Warm things up (so that damped version equilibrates)
   for (i=0; i < 2000; i++){
    magValue = getMag();
    dampedMag1 = (alpha * magValue) + ((1.0 - alpha) * dampedMag1);
    dampedMag2 = (beta * magValue) + ((1.0 - beta) * dampedMag2);
    
  //  Serial output only for debugging or adjusting placement of sensor
  //  Serial.print (magValue);
  //  Serial.print ("  ");
  //  Serial.print (dampedMag1);
  //  Serial.print ("  ");
  //  Serial.print (dampedMag2);
  //  Serial.print ("  ");
  //  Serial.println (dampedMag2 - dampedMag1);
  }
  // Serial.println ("Warmed up");
}

void loop(){
  //Init things
  seekPeak = false;
  max = 0;
  flow = false;
    
  //Take some measurements, subtract heavily damped version from lightly damped version to remove long-term drift, and gather into an array
  for (i=0; i < len; i++){
    magValue = getMag();
    dampedMag1 = (alpha * magValue) + ((1.0 - alpha) * dampedMag1);  //Heavily damped
    dampedMag2 = (beta * magValue) + ((1.0 - beta) * dampedMag2);    //Just a little damping
    sampleData[i] = dampedMag2 - dampedMag1;
  //  sampleData[i] = magValue - dampedMag1;
  }
    
  //Perform autocorrelation on captured values
  for(i=0; i < len; i++)
  {
    sum_old = sum;
    sum = 0;
    for(k=0; k < len-i; k++) sum += (sampleData[k])*(sampleData[k+i]);
    
    if (i==0){
      sum0 = sum;
      if (sum0 > 500){
        flow = true;
      }
    }
    
    if (sum < 0) {
      seekPeak = true;          //Must be well past the peak at zero, so we can seek the highest peak
    }
    //Find peak value (but not the one at zero offset)
    if ((seekPeak) && (sum > max)) {
      max = sum;
      period = i;
    }
 //   Serial.print (max);
 //   Serial.print ("  ");
 //   Serial.println (sum);
    
    /*
    // Peak Detect State Machine
    if (i > 10 && pd_state == 2 && (sum-sum_old) <=0) 
    {
      period = i;
      pd_state = 3;
      stale = false;
    }
    if (pd_state == 1 && (sum > thresh) && (sum-sum_old) > 0) pd_state = 2;
    if (i==0) {
      thresh = sum * 0.5;
      pd_state = 1;
    }
    */
  }
  
  
  if (flow) {
      Serial.println (1000/period);
  } else {
    Serial.println ("0");
  }
}


int getMag() {
  int theMagValue, index;
  // Send a "A" command to the HMC6352
  // This requests the current heading data
  Wire.beginTransmission(slaveAddress);
  Wire.write("A");              // The "Get Data" command
  Wire.endTransmission();
  delay(7);                     // The HMC6352 needs at least a 6+ mS delay after this command.
  Wire.requestFrom(slaveAddress, 2);        // Request the 2 byte result (MSB comes first)
  index = 0;
  while(Wire.available() && index < 2)
  { 
    magData[index] = Wire.read();
    index++;
  }
  theMagValue = magData[0]*256 + magData[1];  // Put the MSB and LSB together
  return theMagValue;
}

