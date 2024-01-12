#include <LiquidCrystal_I2C.h> // For LCD

#include <Wire.h> // For MAX
#include "MAX30105.h" // For MAX
#include "spo2_algorithm.h" // For MAX

/////////// For Buzzer ////////

const int buzzerPin = 7;

/////////// For LCD ///////////////

LiquidCrystal_I2C lcd(0x27, 16, 2); // For LCD

/////////// For MAX /////////////

MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[100]; //infrared LED sensor data
uint16_t redBuffer[100];  //red LED sensor data
#else
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
#endif

int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

byte pulseLED = 11; //Must be on PWM pin
byte readLED = 13; //Blinks with each data read

//////////// For Alarms ////////////////////

bool alm1 = false;
bool alm2 = false;
bool alm3 = false;

///////////////////////////////////////

void setup() {

  //// Serial Begin ////

  Serial1.begin(9600); // Serial Begin
  Serial.begin(9600); // Serial Begin

  //// LCD Set //////

  lcd.init();
  lcd.clear();
  lcd.backlight();      // Make sure backlight is on

  //// MAX ////////

  pinMode(pulseLED, OUTPUT);
  pinMode(readLED, OUTPUT);

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }

  //Serial.println(F("Attach sensor to finger with rubber band. Press any key to start conversion"));
  //while (Serial.available() == 0) ; //wait until user presses a key
  //Serial.read();

  byte ledBrightness = 60; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings

  particleSensor.enableDIETEMPRDY(); //Enable the temp ready interrupt. This is required.

  /////// Buzzer ////////

  pinMode(buzzerPin, OUTPUT);

}

void loop() {

  Serial.println("Program is starting... Please keep finger on sensor");

  ///// LCD - Start MSG /////

  lcd.clear();
  lcd.setCursor(0, 0);  // Column, Row
  lcd.print("Please keep");
  lcd.setCursor(0, 1);  // Column, Row
  lcd.print("finger on sensor");

  ///// For MAX /////////////

  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

  //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample

    Serial.print(F("red="));
    Serial.print(redBuffer[i], DEC);
    Serial.print(F(", ir="));
    Serial.println(irBuffer[i], DEC);
  }

  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  while (1)
  {
    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

      digitalWrite(readLED, !digitalRead(readLED)); //Blink onboard LED with every data read

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample
    }

    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    /////////////// Temp Read MAX ////////////////////

    float temperature = particleSensor.readTemperature();

    //// ### Display and Send Data ######

    if (heartRate < -1 || spo2 < -1) {
      ////////////// LCD Display //////////

      lcd.clear();
      lcd.setCursor(0, 0);  // Column, Row
      lcd.print("No finger? wait.");
      lcd.setCursor(0, 1);  // Column, Row
      lcd.print("Keep finger...");

      Serial.println(" No finger?");
    }
    else {
      ////////////// LCD Display //////////

      lcd.clear();
      lcd.setCursor(0, 0);  // Column, Row
      lcd.print("T:");
      lcd.setCursor(2, 0);  // Column, Row
      lcd.print(temperature);
      lcd.setCursor(8, 0);  // Column, Row
      lcd.print("HR:");
      lcd.setCursor(11, 0);  // Column, Row
      lcd.print(heartRate);
      lcd.setCursor(0, 1);  // Column, Row
      lcd.print("SpO2:");
      lcd.setCursor(5, 1);  // Column, Row
      lcd.print(spo2);

      /////////////////////////////

      if(heartRate > 150){
        alm1 = true;
        tone(buzzerPin, 500, 500);
      }else{
        alm1 = false;
      }

      //////////////// Data Send ///////

      String json = "{\"temp\":" + String(temperature) + ",\"hr\":" + String(heartRate) + ",\"spo2\":" + String(spo2) + ",\"pno\":113,\"am1\":"+alm1+",\"am2\":"+alm2+",\"am3\":"+alm3+"}";

      Serial1.println(json);
      Serial.print("Sent: ");
      Serial.println(json);

    }

    ////////////////////////////

    delay(100);
  }

  delay(500);
}
