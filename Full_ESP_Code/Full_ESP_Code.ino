#include <ArduinoJson.h>

#define RXp2 16
#define TXp2 17

String sval;

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);
  Serial.println("Start:");
}

void loop() {  

  //// Serial Read /////

  if (Serial2.available()){
    while (Serial2.available()>0) {
      sval = Serial2.readStringUntil('\n');
    }
    sval.trim();
    Serial.print("Got:");
    Serial.println(sval);

    ///// JSON Read /////

    StaticJsonDocument<200> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, sval);

    // Test if parsing succeeds.
    if (error) {
      //Serial.print(F("deserializeJson() failed: "));
      //Serial.println(error.f_str());
      return;
    }

    /// "{\"temp\":" + String(temperature) + ",\"hr\":" + String(heartRate) + ",\"spo2\":" + String(spo2) + ",\"pno\":113,\"am1\":"+alm1+",\"am2\":"+alm2+",\"am3\":"+alm3+"}"

    String tem = doc["temp"];
    String hr = doc["hr"];
    String spo2 = doc["spo2"];
    String pno = doc["pno"];
    String am1 = doc["am1"];
    String am2 = doc["am2"];
    String am3 = doc["am3"];

    ///////////////////////

    // Print values.
    Serial.println("Values:");
    Serial.println(tem);
    Serial.println(hr);
    Serial.println(spo2);
    Serial.println(pno);
    Serial.println(am1);
    Serial.println(am2);
    Serial.println(am3);

  }
  delay(1000);

}
